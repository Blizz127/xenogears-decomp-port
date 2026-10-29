/* Native turn-record reset, actor speed and party-flag repair. Every packed
 * pointer and original caller frame stays on the battle bus; game calls use
 * the same native-first/refusal boundary as the surrounding loader. */
#define BATTLE_TURN_RESET_ADDRESS 0x80078508u
#define BATTLE_TURN_SPEED_ADDRESS 0x80098af8u
#define BATTLE_TURN_FLAGS_ADDRESS 0x80094d24u

#define TS_R8(a) battle_entry_read(entry,(a),1)
#define TS_R16(a) battle_entry_read(entry,(a),2)
#define TS_R32(a) battle_entry_read(entry,(a),4)
#define TS_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define TS_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define TS_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define TS_CALL0(target,site) battle_entry_call(entry,(target),(site),0,NULL)
#define TS_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define TS_S0 (entry->cpu->gpr[16])
#define TS_S1 (entry->cpu->gpr[17])
#define TS_S2 (entry->cpu->gpr[18])
#define TS_S3 (entry->cpu->gpr[19])
#define TS_S4 (entry->cpu->gpr[20])
#define TS_S5 (entry->cpu->gpr[21])
#define TS_V0 (entry->cpu->gpr[2])
#define TS_V1 (entry->cpu->gpr[3])

static void battle_turn_reset_run(BattleEntryCall *entry)
{
    uint32_t output=entry->cpu->gpr[4],sp=entry->cpu->gpr[29]-0x30u;
    entry->cpu->gpr[29]=sp;
    for (unsigned r=0;r<6;r++) TS_W32(sp+0x10u+r*4u,entry->cpu->gpr[16+r]);
    TS_W32(sp+0x28u,entry->cpu->gpr[31]);
    TS_S0=0; TS_S5=0xff; TS_S4=output;
    TS_S3=0x800d2e1cu; TS_S1=0x800d2df0u; TS_S2=0x800d2e06u;
    do {
        if (TS_R8(0x800d2dccu+TS_S0)!=0) {
            TS_CALL(BATTLE_TURN_SPEED_ADDRESS,0x80078560u,TS_S0,0);
            TS_W16(TS_S2,TS_V0); TS_W16(TS_S1,TS_V0);
        } else {
            TS_W16(TS_S2,TS_S5); TS_W16(TS_S1,TS_S5);
        }
        TS_W8(0x800d2de4u+TS_S0,0); TS_W16(TS_S3,0); TS_W8(TS_S4,0);
        TS_S4++; TS_S3+=2; TS_S1+=2; TS_S0++; TS_S2+=2;
    } while ((int32_t)TS_S0<11);
    TS_V0=0;
    entry->cpu->gpr[31]=TS_R32(sp+0x28u);
    for (unsigned r=0;r<6;r++) entry->cpu->gpr[16+r]=TS_R32(sp+0x10u+r*4u);
    entry->cpu->gpr[29]=sp+0x30u;
}

static void battle_turn_speed_run(BattleEntryCall *entry)
{
    uint32_t slot=entry->cpu->gpr[4]&0xffu,sp=entry->cpu->gpr[29]-0x18u;
    entry->cpu->gpr[29]=sp;
    uint32_t base=TS_R32(0x800c34b0u);
    TS_W32(sp+0x14u,entry->cpu->gpr[31]); TS_W32(sp+0x10u,TS_S0);
    uint32_t record=base+slot*0x170u;
    TS_W32(0x800d2d6cu,record+0xa4u); TS_W32(0x800c3e00u,record);
    if (slot<3) {
        uint32_t gear=TS_R8(record+0x15au)&0x80u;
        uint32_t table=base+(gear ? 0x2228u+slot*0x690u : 0x1058u+slot*0x5f0u)
                       +TS_R8(base+0x5fc2u)*0x28u;
        TS_W32(0x800c3dfcu,table);
        int32_t stat=(int32_t)TS_R8(record+(gear ? 0x13cu : 0x5au));
        int32_t penalty=(int8_t)TS_R8(table+0x27u);
        TS_S0=stat>penalty ? (uint32_t)(stat-penalty)*9u : 9u;
    } else TS_S0=TS_R8(record+0x5au)*9u;
    if ((TS_S0&0xffffu)>0xa5u) TS_S0=0xa0;
    TS_S0=0xa5u-TS_S0;
    TS_CALL0(0x8003fa38u,0x80098c24u);
    /* The original signed remainder truncates toward zero. Keep S0 as a
     * full word through the following call, including negative results. */
    int32_t spread=(int32_t)TS_V0%8-4;
    TS_S0-=(uint32_t)spread;
    TS_CALL0(BATTLE_TURN_FLAGS_ADDRESS,0x80098c4cu);
    TS_V0=TS_S0&0xffu;
    entry->cpu->gpr[31]=TS_R32(sp+0x14u); TS_S0=TS_R32(sp+0x10u);
    entry->cpu->gpr[29]=sp+0x18u;
}

static void battle_turn_flags_run(BattleEntryCall *entry)
{
    uint32_t first=0,second=0,base=TS_R32(0x800c34b0u);
    for (uint32_t i=0;i<3;i++) if (TS_R16(base+i*0x170u+0x7cu)&0xc000u) first++;
    base=TS_R32(0x800c34b0u);
    for (uint32_t i=0;i<3;i++) if (TS_R16(base+i*0x170u+0x7cu)&2u) second++;
    uint32_t last_flags=0;
    if ((first==1 && second==2) || (first==2 && second==1)) {
        base=TS_R32(0x800c34b0u);
        for (uint32_t i=0;i<3;i++) {
            uint32_t record=base+i*0x170u;
            last_flags=TS_R16(record+0x7cu);
            if (last_flags&2u) {
                TS_W16(record+0x7cu,last_flags&0xfffdu);
                TS_W16(record+0x7au,TS_R16(0x800c3aa4u+i*2u));
            }
        }
    }
    /* These leaf returns differ between the original two repair branches. */
    if (first==2 && second==1) { TS_V0=3; TS_V1=last_flags; }
    else { TS_V0=first==2 ? 1u : 2u; TS_V1=second; }
}

#undef TS_R8
#undef TS_R16
#undef TS_R32
#undef TS_W8
#undef TS_W16
#undef TS_W32
#undef TS_CALL0
#undef TS_CALL
#undef TS_S0
#undef TS_S1
#undef TS_S2
#undef TS_S3
#undef TS_S4
#undef TS_S5
#undef TS_V0
#undef TS_V1

static int battle_turn_speed_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct { uint32_t address,bytes; const char *identity; void (*run)(BattleEntryCall *); } bodies[]={
        {BATTLE_TURN_RESET_ADDRESS,204,"9fbd733c884e8570fb67a5412e46b995742fb7a3f4a6759ac3deb8786d413e97",battle_turn_reset_run},
        {BATTLE_TURN_SPEED_ADDRESS,372,"5ebb37bc38bf501c619fb46173513b551b7527b3bf42b4e72803247dba76257d",battle_turn_speed_run},
        {BATTLE_TURN_FLAGS_ADDRESS,448,"d11faa18aa8c324f98153d6d576e1aacd634e81577bfc4eb4bf40a016f51730d",battle_turn_flags_run}
    };
    unsigned selected;
    for (selected=0;selected<3;selected++) if (bodies[selected].address==target) break;
    if (selected==3 || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TURN_SPEED");
    if (control && strcmp(control,"0")==0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target),bodies[selected].bytes,digest);
    const char *expected=bodies[selected].identity;
    for (unsigned i=0;i<sizeof(digest);i++)
        if (file1_hex_digit(digest[i]>>4)!=expected[i*2] ||
            file1_hex_digit(digest[i]&15u)!=expected[i*2+1]) return 0;
    void (*volatile run)(BattleEntryCall *)=bodies[selected].run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry); cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
