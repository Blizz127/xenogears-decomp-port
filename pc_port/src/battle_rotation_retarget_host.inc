/* Complete rotation retargeter and packed 20-byte-slot allocator. Guest
 * words, live source reads, stack aliases and committed fault prefixes are
 * retained. No original CPU instructions are fetched or decoded here. */
#define RR_R(a) battle_entry_read(entry,(a),4)
#define RR_H(a) battle_entry_read(entry,(a),2)
#define RR_S(a) ((uint32_t)(int32_t)(int16_t)RR_H(a))
#define RR_B(a) battle_entry_read(entry,(a),1)
#define RR_W(a,v) battle_entry_write(entry,(a),4,(v))
#define RR_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define RR_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define RR_V0 (entry->cpu->gpr[2])
#define RR_V1 (entry->cpu->gpr[3])
#define RR_A0 (entry->cpu->gpr[4])
#define RR_A1 (entry->cpu->gpr[5])
#define RR_A2 (entry->cpu->gpr[6])
#define RR_A3 (entry->cpu->gpr[7])
#define RR_S0 (entry->cpu->gpr[16])
#define RR_S1 (entry->cpu->gpr[17])
#define RR_S2 (entry->cpu->gpr[18])
#define RR_S3 (entry->cpu->gpr[19])
#define RR_S4 (entry->cpu->gpr[20])
#define RR_SP (entry->cpu->gpr[29])
#define RR_RA (entry->cpu->gpr[31])

static void battle_rotation_pool_take_run(BattleEntryCall *entry)
{
    /* The first load of each adjacent pair must remain uncommitted if
     * the second read faults. Neither pair can be replaced by a word read. */
    uint32_t p=RR_H(RR_A0+4u),q=RR_H(RR_A0+6u),keep;
    RR_V1=p;RR_V0=q;RR_V0=RR_V1<RR_V0;
    keep=RR_V0;RR_V0=RR_V1<<2;
    if(!keep) {RR_V0=0;return;}

    RR_V0+=RR_V1;RR_V1=RR_R(RR_A0);RR_V0<<=2;
    RR_A3=RR_V0+RR_V1;RR_V0=RR_B(RR_A3);
    keep=RR_V0;RR_V0=0;
    if(keep)return;

    p=RR_H(RR_A0+4u);q=RR_H(RR_A0+6u);
    RR_V0=p;RR_V0++;RR_A1=q;
    RR_SH(RR_A0+4u,RR_V0);
    RR_V1=RR_H(RR_A0+4u);RR_V0=RR_V1<RR_A1;
    keep=RR_V0;RR_V0=RR_A3;
    if(!keep)return;

    /* A3 stays the returned candidate. The scan only advances the header
     * to its next free slot; the caller marks the candidate occupied. */
    RR_A2=RR_R(RR_A0);RR_V0=RR_V1<<2;
    for(;;) {
        RR_V0+=RR_V1;RR_V0<<=2;RR_V0+=RR_A2;
        RR_V0=RR_B(RR_V0);
        keep=RR_V0;RR_V0=RR_A3;
        if(!keep)return;
        RR_V0=RR_H(RR_A0+4u);RR_V0++;
        RR_SH(RR_A0+4u,RR_V0);
        RR_V1=RR_H(RR_A0+4u);RR_V0=RR_V1<RR_A1;
        keep=RR_V0;RR_V0=RR_V1<<2;
        if(!keep) {RR_V0=RR_A3;return;}
    }
}

static void battle_rotation_retarget_call(BattleEntryCall *entry)
{
    RR_RA=0x800adfb4u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x800a2330u);
    int rc=PcPortMipsRun(&callee,0x800a2330u,0x800adfb4u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-rotation-retarget] callee 0x800a2330 from 0x800adfac: %s\n",callee.error);
        battle_entry_fail(entry,"rotation-retarget allocator stopped",0x800a2330u);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending rotation-retarget allocator return",0x800a2330u);
}

static void battle_rotation_retarget_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    RR_SP-=0x28u;
    RR_W(RR_SP+0x18u,RR_S2);
    p=RR_R(RR_SP+0x38u);
    /* S2 is still pending while the following saved-S0 store executes. */
    RR_W(RR_SP+0x10u,RR_S0);RR_S2=p;RR_S0=RR_A1;
    RR_W(RR_SP+0x20u,RR_S4);RR_S4=RR_A2;
    RR_W(RR_SP+0x14u,RR_S1);RR_S1=RR_A3;
    RR_W(RR_SP+0x1cu,RR_S3);
    p=RR_R(RR_SP+0x3cu);RR_V0=(uint32_t)((int32_t)RR_S4<2);RR_S3=p;
    RR_W(RR_SP+0x24u,RR_RA);
    if(RR_V0) {
        RR_V0=1u;
        RR_SH(RR_S0+0x54u,RR_S1);
        RR_SH(RR_S0+0x56u,RR_S2);
        RR_SH(RR_S0+0x58u,RR_S3);
        RR_SB(RR_S0+5u,RR_V0);
        goto restore;
    }

    /* Full-word targets are compared to sign-extended node angles.
     * Preserve the short circuit, including untouched later components. */
    RR_V0=RR_S(RR_S0+0x54u);
    if(RR_V0==RR_S1) {
        RR_V0=RR_S(RR_S0+0x56u);
        if(RR_V0==RR_S2) {
            RR_V0=RR_S(RR_S0+0x58u);
            if(RR_V0==RR_S3)goto restore;
        }
    }
    RR_V0=RR_R(RR_S0+0x70u);RR_V1=RR_V0;
    if(!RR_V0) {
        battle_rotation_retarget_call(entry);
        RR_V1=RR_V0;
    }
    RR_V0=1u;
    if(!RR_V1)goto restore;

    RR_SB(RR_V1,RR_V0);RR_V0=3u;
    RR_SB(RR_V1+2u,RR_V0);RR_V0=0xfeu;
    RR_SB(RR_V1+1u,0);RR_SB(RR_V1+3u,RR_V0);
    /* Handle writes can alias the node: read each start value in order,
     * then read the angles again while computing each wrapped delta. */
    RR_V0=RR_H(RR_S0+0x54u);RR_SH(RR_V1+4u,RR_V0);
    RR_V0=RR_H(RR_S0+0x56u);RR_SH(RR_V1+6u,RR_V0);
    RR_V0=RR_H(RR_S0+0x58u);RR_SH(RR_V1+8u,RR_V0);
    RR_V0=RR_S(RR_S0+0x54u);RR_V0=RR_S1-RR_V0;RR_S1=RR_V0&0xfffu;
    RR_V0=(uint32_t)((int32_t)RR_S1<0x800);
    if(!RR_V0)RR_S1-=0x1000u;
    RR_SH(RR_V1+0xau,RR_S1);
    RR_V0=RR_S(RR_S0+0x56u);RR_V0=RR_S2-RR_V0;RR_S2=RR_V0&0xfffu;
    RR_V0=(uint32_t)((int32_t)RR_S2<0x800);
    if(!RR_V0)RR_S2-=0x1000u;
    RR_SH(RR_V1+0xcu,RR_S2);
    RR_V0=RR_S(RR_S0+0x58u);RR_V0=RR_S3-RR_V0;RR_S3=RR_V0&0xfffu;
    RR_V0=(uint32_t)((int32_t)RR_S3<0x800);
    if(!RR_V0)RR_S3-=0x1000u;
    RR_SH(RR_V1+0xeu,RR_S3);
    RR_SH(RR_V1+0x10u,0);RR_SH(RR_V1+0x12u,RR_S4);
    RR_W(RR_S0+0x70u,RR_V1);
restore:
    /* Each saved register commits only after the following load succeeds.
     * These live reads also retain any handle/node aliases into the frame. */
    p=RR_R(RR_SP+0x24u);q=RR_R(RR_SP+0x20u);RR_RA=p;
    p=RR_R(RR_SP+0x1cu);RR_S4=q;
    q=RR_R(RR_SP+0x18u);RR_S3=p;
    p=RR_R(RR_SP+0x14u);RR_S2=q;
    q=RR_R(RR_SP+0x10u);RR_S1=p;
    RR_SP+=0x28u;RR_S0=q;
}

static int battle_rotation_retarget_try_host(BattleMipsRuntime *runtime,
                                               PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x800adf1cu,380u,"92efa36721970c4ecc56ef5e288d8a14739b40e6024daa7b38411ab4d2ac6af4"},
        {0x800a2330u,184u,"77e70c10a2a207af117001db0a033bae64fa9bb7cf51128e5b82c824e241bbde"}
    };
    unsigned index=0;while(index<2u&&bodies[index].target!=target)index++;
    if(index==2u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ROTATION_RETARGET");
    if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800adf1cu)battle_rotation_retarget_run(&entry);
    else battle_rotation_pool_take_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef RR_R
#undef RR_H
#undef RR_S
#undef RR_B
#undef RR_W
#undef RR_SH
#undef RR_SB
#undef RR_V0
#undef RR_V1
#undef RR_A0
#undef RR_A1
#undef RR_A2
#undef RR_A3
#undef RR_S0
#undef RR_S1
#undef RR_S2
#undef RR_S3
#undef RR_S4
#undef RR_SP
#undef RR_RA
