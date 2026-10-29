/* Complete actor motion-slot resolver and status helper. Guest words, live
 * registers and ordered data accesses are retained; no instruction decoder. */
#define MS_R(a) battle_entry_read(entry,(a),4)
#define MS_H(a) battle_entry_read(entry,(a),2)
#define MS_B(a) battle_entry_read(entry,(a),1)
#define MS_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MS_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MS_AT (entry->cpu->gpr[1])
#define MS_V0 (entry->cpu->gpr[2])
#define MS_V1 (entry->cpu->gpr[3])
#define MS_A0 (entry->cpu->gpr[4])
#define MS_A1 (entry->cpu->gpr[5])
#define MS_A2 (entry->cpu->gpr[6])
#define MS_A3 (entry->cpu->gpr[7])
#define MS_T0 (entry->cpu->gpr[8])
#define MS_S0 (entry->cpu->gpr[16])
#define MS_S1 (entry->cpu->gpr[17])
#define MS_SP (entry->cpu->gpr[29])
#define MS_RA (entry->cpu->gpr[31])

static void battle_motion_slot_call(BattleEntryCall *entry)
{
    MS_RA=0x800af558u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x8009c050u);
    int rc=PcPortMipsRun(&callee,0x8009c050u,0x800af558u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-motion-slot] callee 0x8009c050 from 0x800af550: %s\n",callee.error);
        battle_entry_fail(entry,"motion-slot status callee stopped",0x8009c050u);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending motion-slot status return",0x8009c050u);
}

static void battle_motion_slot_run(BattleEntryCall *entry)
{
    uint32_t bit4,keep,low,p,q,value=0;
    MS_SP-=0x20u;
    MS_W(MS_SP+0x10u,MS_S0);MS_S0=MS_A0;
    MS_W(MS_SP+0x14u,MS_S1);MS_S1=MS_A2;
    MS_V1=MS_A1&0xffu;MS_V0=MS_V1<0xfeu;
    MS_W(MS_SP+0x18u,MS_RA);
    MS_W(MS_S1,0);
    if(MS_V0)goto table;
    MS_V0=0xffu;
    if(MS_V1!=MS_V0)goto cached;
    MS_A0=MS_B(MS_S0+0x20u);
    battle_motion_slot_call(entry);
    MS_A0=MS_V0;MS_V0=MS_A0&4u;
    bit4=MS_V0;MS_V0=MS_A0&2u;
    if(bit4) {
        MS_V0=MS_H(MS_S0+0x4au);MS_V0&=0x100u;
        keep=MS_V0;MS_V0=0x1bu;
        if(keep)goto store;
        MS_V0=MS_A0&2u;
    }
    if(MS_V0) {
        MS_V0=MS_H(MS_S0+0x4au);MS_V0&=0x80u;
        if(MS_V0) {
            MS_V0=MS_B(MS_S0+0x20u);
            MS_V1=MS_V0<<1;MS_V1+=MS_V0;MS_V1<<=3;MS_V1-=MS_V0;MS_V1<<=4;
            MS_AT=0x800d0000u;MS_AT+=MS_V1;
            MS_V0=MS_H(MS_AT-0x32e2u);MS_V0&=1u;
            keep=MS_V0;MS_V0=MS_A0&1u;
            if(keep)goto high_bit;
            MS_V0=6u;goto store;
        }
    }
    MS_V0=MS_H(MS_S0+0x4au);MS_V0&=0x400u;
    keep=MS_V0;MS_V0=MS_A0&1u;
    if(keep)goto high_bit;
    MS_V0=1u;
store:
    MS_SB(MS_S0+0x2au,MS_V0);MS_V0=MS_A0&1u;
high_bit:
    if(MS_V0) {
        MS_V0=MS_B(MS_S0+0x2au);MS_V0|=0x80u;
        MS_SB(MS_S0+0x2au,MS_V0);
    }
cached:
    MS_V0=MS_B(MS_S0+0x2au);MS_A1=MS_V0&0x7fu;MS_V0&=0x80u;
    MS_W(MS_S1,MS_V0);
table:
    MS_A1&=0xffu;MS_V0=MS_A1<0x40u;
    low=MS_V0;MS_V0=MS_A1<<2;
    if(low) {
        MS_V1=MS_R(MS_S0+0x14u);MS_V0+=MS_V1;
        value=MS_R(MS_V0+4u);
    } else {
        MS_V1=MS_R(MS_S0+0x18u);MS_V0+=MS_V1;
        MS_V0=MS_R(MS_V0-0xfcu);
    }
    /* The low-table result is pending at the first epilogue read. The high
     * path's jump delay already committed it. Keep that fault distinction. */
    p=MS_R(MS_SP+0x18u);if(low)MS_V0=value;
    q=MS_R(MS_SP+0x14u);MS_RA=p;
    p=MS_R(MS_SP+0x10u);MS_S1=q;
    MS_SP+=0x20u;MS_S0=p;
}

static void battle_motion_status_run(BattleEntryCall *entry)
{
    MS_A0&=0xffu;
    MS_V1=MS_A0<<1;MS_V1+=MS_A0;MS_V1<<=3;MS_V1-=MS_A0;MS_V1<<=4;
    MS_A0=0x800ccce8u;MS_A1=MS_A0+0xa4u;MS_A1+=MS_V1;
    MS_A3=MS_V1+MS_A0;MS_A0+=0x148u;MS_A0+=MS_V1;
    uint32_t p=MS_H(MS_A1+0x7cu),q=MS_R(MS_A1+0x60u);
    MS_V0=p&0x400u;MS_V1=q;MS_T0=MS_V0!=0;
    MS_V0=MS_R(MS_A1+0x64u);MS_V0>>=3;
    MS_V1=MS_V1<MS_V0;MS_A2=MS_T0;
    if(MS_V1) {
        MS_V0=MS_H(MS_A3+0x36u);MS_V0&=1u;
        if(!MS_V0)MS_A2=MS_T0|2u;
    }
    p=MS_B(MS_A0);MS_V0=4u;MS_V1=p;
    if(MS_V1==MS_V0)MS_A2|=4u;
    MS_V0=MS_A2&0xffu;
}

static int battle_motion_slot_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x800af518u,352u,"2d18d17de427734c635ed1df8c02be008bd5ede93dbc86da75893bd5a0552a1f"},
        {0x8009c050u,144u,"3cc54037f55580525109fb48ce6c5316c4b9a1be8ffe2c14fc62761b4ca25f6a"}
    };
    unsigned index=0;while(index<2u&&bodies[index].target!=target)index++;
    if(index==2u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MOTION_SLOT");
    if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800af518u)battle_motion_slot_run(&entry);
    else battle_motion_status_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MS_R
#undef MS_H
#undef MS_B
#undef MS_W
#undef MS_SB
#undef MS_AT
#undef MS_V0
#undef MS_V1
#undef MS_A0
#undef MS_A1
#undef MS_A2
#undef MS_A3
#undef MS_T0
#undef MS_S0
#undef MS_S1
#undef MS_SP
#undef MS_RA
