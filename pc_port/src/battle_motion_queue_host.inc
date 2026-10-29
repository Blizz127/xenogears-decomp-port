/* Complete motion queue/restart body 800AA934. Pointer slots retain their
 * canonical word representation; the motion updater is a separate callee. */
#define MQ_R(a) battle_entry_read(entry,(a),4)
#define MQ_B(a) battle_entry_read(entry,(a),1)
#define MQ_H(a) battle_entry_read(entry,(a),2)
#define MQ_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MQ_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MQ_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MQ_V0 (entry->cpu->gpr[2])
#define MQ_V1 (entry->cpu->gpr[3])
#define MQ_A0 (entry->cpu->gpr[4])
#define MQ_A1 (entry->cpu->gpr[5])
#define MQ_A2 (entry->cpu->gpr[6])
#define MQ_A3 (entry->cpu->gpr[7])
#define MQ_SP (entry->cpu->gpr[29])
#define MQ_RA (entry->cpu->gpr[31])

static void battle_motion_queue_update(BattleEntryCall *entry)
{
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x800aad54u);
    int rc=PcPortMipsRun(&callee,0x800aad54u,0x800aaa10u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-motion-queue] update from 0x800aaa08: %s\n",callee.error);
        battle_entry_fail(entry,"motion updater stopped",0x800aad54u);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending motion updater return",0x800aad54u);
}

static void battle_motion_queue_run(BattleEntryCall *entry)
{
    MQ_SP-=0x20u;MQ_W(MQ_SP+0x18u,MQ_RA);
    if(MQ_A0&&MQ_A1) {
        MQ_V0=MQ_B(MQ_A0+0x2bu);
        int queued=MQ_V0!=0;MQ_V0=MQ_V0<5u;
        if(queued) {
            if(MQ_V0) {MQ_V0=MQ_B(MQ_A0+0x2bu);MQ_V0++;MQ_SB(MQ_A0+0x2bu,MQ_V0);}
            /* The first LBU has not committed if the second read fails. */
            uint32_t length=MQ_B(MQ_A0+0x2bu),id=MQ_B(MQ_A1+0x20u);
            MQ_V0=MQ_A0+length;MQ_V1=id;MQ_SB(MQ_V0+0x2au,MQ_V1);
            MQ_V0=MQ_B(MQ_A0+0x2bu);MQ_V0+=MQ_A0;MQ_SB(MQ_V0+0x2eu,MQ_A3);
        } else {
            MQ_V0=(int32_t)MQ_A3<0x50;MQ_V1=MQ_A3<<2;
            if(MQ_V0) {
                MQ_V0=MQ_R(MQ_A1+8u);MQ_V1+=MQ_V0;MQ_V0=MQ_R(MQ_V1);
                MQ_W(MQ_A0+0x10u,MQ_V0);
            } else {
                MQ_V0=MQ_R(MQ_A1+0xcu);
                uint32_t table=MQ_R(MQ_V0+4u);MQ_V0=MQ_A3<<2;MQ_V1=table;
                MQ_V0+=MQ_V1;MQ_V0=MQ_R(MQ_V0-0x138u);MQ_W(MQ_A0+0x10u,MQ_V0);
            }
            MQ_A1=MQ_A2;MQ_A2=UINT32_MAX;MQ_V0=0x800c0000u;
            uint32_t now=MQ_H(0x800c3e30u);MQ_A3=1;MQ_V0=now;
            MQ_SH(MQ_A0+0x42u,0);MQ_SH(MQ_A0+0x40u,0);
            MQ_W(MQ_A0+0x50u,0);MQ_W(MQ_A0+0x54u,0);MQ_W(MQ_A0+0x4cu,0);
            MQ_SB(MQ_A0+0x23u,0);MQ_SH(MQ_A0+0x10au,MQ_V0);
            MQ_RA=0x800aaa10u;MQ_W(MQ_SP+0x10u,0);battle_motion_queue_update(entry);
        }
    }
    uint32_t ra=MQ_R(MQ_SP+0x18u);MQ_SP+=0x20u;MQ_RA=ra;
}

static int battle_motion_queue_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800aa934u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MOTION_QUEUE");
    if(control&&strcmp(control,"0")==0)return 0;
    static const char expected[]="5e3abe8f581f1199f35b02f61b60ba6681c7c152a2475df8d2a4e632dfeb4053";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),236,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_motion_queue_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MQ_R
#undef MQ_B
#undef MQ_H
#undef MQ_W
#undef MQ_SB
#undef MQ_SH
#undef MQ_V0
#undef MQ_V1
#undef MQ_A0
#undef MQ_A1
#undef MQ_A2
#undef MQ_A3
#undef MQ_SP
#undef MQ_RA
