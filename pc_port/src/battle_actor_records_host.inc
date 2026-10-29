/* Per-actor 0x70-byte record initialization, complete retail 800AA6E0 body.
 * Canonical addresses remain 32-bit tokens. This code performs no CPU fetch
 * or decode; the heap service is an independently checked native boundary. */
#define AR_V0 (entry->cpu->gpr[2])
#define AR_V1 (entry->cpu->gpr[3])
#define AR_A0 (entry->cpu->gpr[4])
#define AR_A1 (entry->cpu->gpr[5])
#define AR_A2 (entry->cpu->gpr[6])
#define AR_S0 (entry->cpu->gpr[16])
#define AR_SP (entry->cpu->gpr[29])
#define AR_RA (entry->cpu->gpr[31])

static void battle_actor_records_allocate(BattleEntryCall *entry)
{
    AR_RA=0x800aa710u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x80031bdcu);
    int rc=PcPortMipsRun(&callee,0x80031bdcu,0x800aa710u,battle_step_limit());
    /* Completed callee effects also remain visible when it refuses. */
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;
    entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED)
        battle_entry_fail(entry,"actor-record allocator stopped",0x80031bdcu);
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending actor-record allocator return",0x80031bdcu);
}

static void battle_actor_records_run(BattleEntryCall *entry)
{
    AR_SP-=0x20u;
    uint32_t sp=AR_SP;
    battle_entry_write(entry,sp+0x18u,4,AR_S0);
    AR_S0=AR_A0;
    battle_entry_write(entry,sp+0x1cu,4,AR_RA);
    AR_V0=battle_entry_read(entry,AR_S0+0x10cu,1);
    AR_A0=AR_V0<<3;
    if(AR_V0){
        AR_A0=(AR_A0-AR_V0)<<4;
        AR_A1=0;
        battle_actor_records_allocate(entry);
        AR_A0=0;
        AR_V1=battle_entry_read(entry,AR_S0+0x10cu,1);
        AR_A1=AR_V0;
        if((int32_t)AR_V1>0){
            AR_A2=UINT32_MAX;
            AR_V1=AR_A1;
            do{
                battle_entry_write(entry,AR_V1,2,AR_A2);
                battle_entry_write(entry,AR_V1+8u,4,0);
                AR_V0=battle_entry_read(entry,AR_S0+0x10cu,1);
                AR_A0++;
                AR_V0=(int32_t)AR_A0<(int32_t)AR_V0;
                AR_V1+=0x70u;
            }while(AR_V0);
        }
        battle_entry_write(entry,AR_S0+0x110u,4,AR_A1);
    }
    /* The RA load has not committed if the following S0 read faults. */
    uint32_t return_address=battle_entry_read(entry,sp+0x1cu,4);
    uint32_t saved_s0=battle_entry_read(entry,sp+0x18u,4);
    AR_RA=return_address;
    AR_SP=sp+0x20u;
    AR_S0=saved_s0;
}

static int battle_actor_records_try_host(BattleMipsRuntime *runtime,
                                          PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800aa6e0u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ACTOR_RECORDS");
    if(control&&strcmp(control,"0")==0)return 0;
    static const char expected[]=
        "0764da3a5f31586321a67a62de035066a3c1694ce28e81ecec92308e09dc580d";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),128,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||
           file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_actor_records_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

#undef AR_V0
#undef AR_V1
#undef AR_A0
#undef AR_A1
#undef AR_A2
#undef AR_S0
#undef AR_SP
#undef AR_RA
