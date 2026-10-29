/* Startup timer registration and deferred resource callback. Structured host
 * bodies; original CPU code is inspected only for identity, never decoded. */
#define TT_R(a) battle_entry_read(entry,(a),4)
#define TT_W(a,v) battle_entry_write(entry,(a),4,(v))
#define TT_V0 (entry->cpu->gpr[2])
#define TT_A0 (entry->cpu->gpr[4])
#define TT_A1 (entry->cpu->gpr[5])
#define TT_T0 (entry->cpu->gpr[8])
#define TT_S0 (entry->cpu->gpr[16])
#define TT_S1 (entry->cpu->gpr[17])
#define TT_SP (entry->cpu->gpr[29])
#define TT_RA (entry->cpu->gpr[31])
#define TT_CALL(t,s) battle_timer_task_call(entry,(t),(s))

static void battle_timer_task_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    TT_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-timer-task] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"timer-task callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending timer-task callee return",target);
}

static void battle_timer_task_restore(BattleEntryCall *entry)
{
    uint32_t ra=TT_R(TT_SP+0x18u),s1=TT_R(TT_SP+0x14u);TT_RA=ra;
    uint32_t s0=TT_R(TT_SP+0x10u);TT_S1=s1;TT_SP+=0x20u;TT_S0=s0;
}
static void battle_timer_task_register(BattleEntryCall *entry)
{
    TT_SP-=0x20u;TT_W(TT_SP+0x14u,TT_S1);TT_S1=TT_A0;TT_A0=0;TT_A1=0x78u;
    TT_W(TT_SP+0x18u,TT_RA);TT_RA=0x801e70b8u;TT_W(TT_SP+0x10u,TT_S0);TT_CALL(0x8001cd08u,0x801e70b0u);
    TT_S0=TT_V0;TT_A0=TT_S0;TT_A1=0x801e6fecu;TT_CALL(0x8001cd6cu,0x801e70c4u);
    TT_W(TT_S0+0x20u,TT_S1);battle_timer_task_restore(entry);
}
static void battle_timer_task_callback(BattleEntryCall *entry)
{
    TT_SP-=0x20u;TT_W(TT_SP+0x14u,TT_S1);TT_S1=TT_A0;TT_W(TT_SP+0x18u,TT_RA);
    TT_RA=0x801e7004u;TT_W(TT_SP+0x10u,TT_S0);TT_CALL(0x800286ccu,0x801e6ffcu);
    int busy=TT_V0!=0;TT_A0=0x4000u;
    if(!busy) {
        TT_A1=1;TT_CALL(0x80031bdcu,0x801e700cu);
        TT_S0=TT_V0;TT_V0=TT_S0+0x3ffcu;TT_T0=TT_V0;TT_W(TT_T0,TT_SP);TT_T0-=4u;TT_SP=TT_T0;
        TT_A0=TT_R(TT_S1+0x20u);TT_CALL(0x801e6314u,0x801e7030u);
        TT_A0=TT_R(TT_S1+0x20u);TT_CALL(0x801e6710u,0x801e703cu);
        TT_SP+=4u;uint32_t saved_sp=TT_R(TT_SP);TT_RA=0x801e7054u;TT_SP=saved_sp;
        TT_A0=TT_S0;TT_CALL(0x800320e8u,0x801e704cu);
        TT_A0=0;TT_CALL(0x800445d0u,0x801e7054u);
        TT_A0=TT_R(TT_S1+0x20u);TT_CALL(0x800320e8u,0x801e7060u);
        TT_A0=TT_S1;TT_A1=0x801e6f00u;TT_CALL(0x8001cd6cu,0x801e7070u);
        TT_A0=TT_S1+0x30u;TT_CALL(0x801e693cu,0x801e7078u);
    }
    battle_timer_task_restore(entry);
}

static int battle_timer_task_identity(uint32_t address,unsigned bytes,const char *sha)
{
    const uint8_t *code=PSX_ADDR(address);
    if(address==0x8001cd6cu) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(address);
        if(!image||!image->bytes||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[i*2]||file1_hex_digit(digest[i]&15u)!=sha[i*2+1])return 0;
    return 1;
}
static int battle_timer_task_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if((target!=0x801e7098u&&target!=0x801e6fecu&&target!=0x8001cd6cu)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TIMER_TASK");if(control&&strcmp(control,"0")==0)return 0;
    if(target==0x8001cd6cu) {
        if(!battle_timer_task_identity(target,8u,"6b5d5f1a0f2b7c721b6d3106c6530c0974e5b76ba7599b0e84edc41c452f5385"))return 0;
    } else {
        if(!battle_timer_task_identity(target,target==0x801e7098u?80u:172u,target==0x801e7098u?
           "18c2c25d94c59373835b679e30c03e3021b7bdcea6e14a283f6e11517b526a47":"84d648cd9b80bb74a86685973695a6f3a0e4925d3ab96754b118b07cef443167")||
           !battle_timer_task_identity(0x801e4000u,32u,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140")||
           !battle_timer_task_identity(0x801e5840u,228u,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"))return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x801e7098u)battle_timer_task_register(&entry);
    else if(target==0x801e6fecu)battle_timer_task_callback(&entry);
    else battle_entry_write(&entry,cpu->gpr[4]+8u,4,cpu->gpr[5]);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef TT_R
#undef TT_W
#undef TT_V0
#undef TT_A0
#undef TT_A1
#undef TT_T0
#undef TT_S0
#undef TT_S1
#undef TT_SP
#undef TT_RA
#undef TT_CALL
