/* Complete animation-track starter and release-all service. Packed pointer
 * fields remain guest words. Live reads, saved-frame aliases and committed
 * load-delay fault prefixes retain their original order; these bodies never
 * fetch or decode original CPU instructions. */
#define AS_R(a) battle_entry_read(entry,(a),4)
#define AS_H(a) battle_entry_read(entry,(a),2)
#define AS_B(a) battle_entry_read(entry,(a),1)
#define AS_W(a,v) battle_entry_write(entry,(a),4,(v))
#define AS_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define AS_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define AS_V0 (entry->cpu->gpr[2])
#define AS_V1 (entry->cpu->gpr[3])
#define AS_A0 (entry->cpu->gpr[4])
#define AS_A1 (entry->cpu->gpr[5])
#define AS_A2 (entry->cpu->gpr[6])
#define AS_A3 (entry->cpu->gpr[7])
#define AS_T0 (entry->cpu->gpr[8])
#define AS_S0 (entry->cpu->gpr[16])
#define AS_S1 (entry->cpu->gpr[17])
#define AS_S2 (entry->cpu->gpr[18])
#define AS_S3 (entry->cpu->gpr[19])
#define AS_S4 (entry->cpu->gpr[20])
#define AS_S5 (entry->cpu->gpr[21])
#define AS_S6 (entry->cpu->gpr[22])
#define AS_S7 (entry->cpu->gpr[23])
#define AS_SP (entry->cpu->gpr[29])
#define AS_FP (entry->cpu->gpr[30])
#define AS_RA (entry->cpu->gpr[31])

static void battle_animation_start_call(BattleEntryCall *entry,uint32_t target,
                                        uint32_t site)
{
    AS_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-animation-start] callee 0x%08x from 0x%08x: %s\n",
                target,site,callee.error);
        battle_entry_fail(entry,"animation-start callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending animation-start callee return",target);
}

/* Both channels have the same ordered writes, but retain separate keys,
 * modes, node slots and original call sites. No pointer or key is cached
 * across a child or a potentially aliasing write. */
static void battle_animation_start_channel(BattleEntryCall *entry,
                                           unsigned translation)
{
    const uint32_t slot_delta=translation?0u:0xfffffffcu;
    AS_V0=AS_H(AS_S1);AS_T0=0xffffu;
    if(!translation)AS_S5=AS_S1+4u;
    if(AS_V0!=AS_T0) {
        AS_V0=AS_R(AS_S0+slot_delta);AS_V1=AS_V0;
        if(AS_V0) {
            AS_V0=AS_B(AS_V1+3u);
            if(AS_V0==AS_S7)return;
        } else {
            AS_A0=AS_S4;
            battle_animation_start_call(entry,0x800a2330u,
                                         translation?0x800a2630u:0x800a2570u);
            AS_V1=AS_V0;
        }
        AS_V0=1u;
        if(!AS_V1)return;
        AS_SB(AS_V1,AS_V0);AS_SB(AS_V1+1u,AS_S6);
        /* Mode is still pending if the following fifth-argument read
         * faults. That argument byte is read anew for each used handle. */
        uint32_t p=AS_B(AS_S5+translation),q=AS_B(AS_SP+0x58u);
        AS_V0=p;AS_T0=q;
        AS_SB(AS_V1+3u,AS_T0);AS_SB(AS_V1+2u,AS_V0);
        p=AS_H(AS_S1);
        /* The key reload commits only after this store succeeds. */
        AS_SH(AS_V1+0x10u,0);AS_V0=p;
        AS_SH(AS_V1+0x12u,AS_FP);
        AS_V0=AS_S3+AS_V0;
        AS_W(AS_V1+4u,AS_V0);AS_W(AS_V1+8u,AS_V0);
        AS_W(AS_S0+slot_delta,AS_V1);
    } else {
        AS_A1=AS_R(AS_S0+slot_delta);
        /* Node zero retains even an unprotected sentinel-key handle.
         * Its byte 3 must not be read on that path. */
        if(!AS_A1||!AS_S2)return;
        AS_V0=AS_B(AS_A1+3u);
        if(AS_V0==AS_S7)return;
        AS_A0=AS_S4;
        battle_animation_start_call(entry,0x800a23e8u,
                                     translation?0x800a26a8u:0x800a25e8u);
        AS_W(AS_S0+slot_delta,0);
    }
}

static void battle_animation_start_run(BattleEntryCall *entry)
{
    uint32_t p,q,keep;
    AS_SP-=0x48u;
    AS_W(AS_SP+0x30u,AS_S4);AS_S4=AS_A0;
    AS_W(AS_SP+0x20u,AS_S0);AS_S0=AS_A1;
    AS_W(AS_SP+0x24u,AS_S1);AS_S1=AS_A2;
    AS_W(AS_SP+0x44u,AS_RA);AS_W(AS_SP+0x40u,AS_FP);
    AS_W(AS_SP+0x3cu,AS_S7);AS_W(AS_SP+0x38u,AS_S6);
    AS_W(AS_SP+0x34u,AS_S5);AS_W(AS_SP+0x2cu,AS_S3);
    AS_W(AS_SP+0x28u,AS_S2);
    AS_V0=AS_H(AS_S1+6u);AS_S6=AS_A3;
    if(AS_V0) {
        battle_animation_start_call(entry,0x800a2accu,0x800a247cu);
        AS_A0=AS_S0;AS_A1=AS_S1;
        battle_animation_start_call(entry,0x800a1b50u,0x800a2488u);
        AS_V0=1u;
        goto restore;
    }

    /* The first header load remains uncommitted if the node count faults.
     * nRot+1 stays a full word, including 65536 for nRot == 65535. */
    p=AS_H(AS_S1+0xcu);q=AS_H(AS_S0+0xau);
    AS_A1=p;AS_T0=q;AS_A0=AS_A1&0xffffu;AS_V1=AS_A0+1u;
    AS_V0=(uint32_t)((int32_t)AS_V1<(int32_t)AS_T0);
    AS_SH(AS_SP+0x10u,AS_T0);
    AS_A2=AS_H(AS_S1+0xeu);AS_S6&=1u;
    if(AS_V0) {AS_A1++;AS_SH(AS_SP+0x10u,AS_A1);}
    AS_FP=AS_H(AS_S1+2u);AS_V0=AS_V1<<1;
    if(!AS_S6)AS_FP--;
    AS_A1=AS_H(AS_S1+4u);AS_S1+=0x18u;
    AS_V0+=AS_V1;AS_V0<<=1;AS_S3=AS_S1+AS_V0;
    AS_V0=AS_A1&1u;keep=AS_V0;AS_V0=AS_A1&2u;
    if(!keep) {
        AS_V0=AS_A0<<1;AS_V0+=AS_A0;AS_V0<<=1;AS_S3+=AS_V0;
        AS_V0=AS_A1&2u;
    }
    AS_S2=0;
    if(!AS_V0) {
        AS_V0=AS_A2<<1;AS_V0+=AS_A2;AS_V0<<=1;AS_S3+=AS_V0;
    }
    AS_V0=AS_H(AS_SP+0x10u);keep=(uint32_t)((int32_t)AS_V0>0);
    AS_V0=0;
    if(!keep)goto restore;
    AS_S7=0xffu;AS_S0+=0x74u;
    do {
        battle_animation_start_channel(entry,0);
        AS_S1+=2u;
        battle_animation_start_channel(entry,1);
        AS_S1+=4u;
        /* Children and handle stores may alias this live frame count. */
        AS_V0=AS_H(AS_SP+0x10u);AS_S2++;
        AS_V0=(uint32_t)((int32_t)AS_S2<(int32_t)AS_V0);
        AS_S0+=0x7cu;
    } while(AS_V0);
    AS_V0=0;
restore:
    p=AS_R(AS_SP+0x44u);q=AS_R(AS_SP+0x40u);AS_RA=p;
    p=AS_R(AS_SP+0x3cu);AS_FP=q;
    q=AS_R(AS_SP+0x38u);AS_S7=p;
    p=AS_R(AS_SP+0x34u);AS_S6=q;
    q=AS_R(AS_SP+0x30u);AS_S5=p;
    p=AS_R(AS_SP+0x2cu);AS_S4=q;
    q=AS_R(AS_SP+0x28u);AS_S3=p;
    p=AS_R(AS_SP+0x24u);AS_S2=q;
    q=AS_R(AS_SP+0x20u);AS_S1=p;
    AS_SP+=0x48u;AS_S0=q;
}

static void battle_animation_release_all_run(BattleEntryCall *entry)
{
    uint32_t p,q,keep;
    AS_SP-=0x30u;
    AS_W(AS_SP+0x20u,AS_S2);AS_S2=AS_A0;
    AS_W(AS_SP+0x2cu,AS_RA);AS_W(AS_SP+0x28u,AS_S4);
    AS_W(AS_SP+0x24u,AS_S3);AS_W(AS_SP+0x1cu,AS_S1);
    AS_W(AS_SP+0x18u,AS_S0);
    AS_S3=AS_H(AS_A1+0xau);AS_S1=0;
    if(AS_S3) {
        AS_S4=0xffu;AS_S0=AS_A1+0x78u;
        do {
            AS_A1=AS_R(AS_S0-8u);
            if(AS_A1) {
                AS_V0=AS_B(AS_A1+3u);
                if(AS_V0!=AS_S4) {
                    AS_A0=AS_S2;
                    battle_animation_start_call(entry,0x800a23e8u,0x800a2b24u);
                    AS_W(AS_S0-8u,0);
                }
            }
            AS_A1=AS_R(AS_S0-4u);
            if(AS_A1) {
                AS_V0=AS_B(AS_A1+3u);
                if(AS_V0!=AS_S4) {
                    AS_A0=AS_S2;
                    battle_animation_start_call(entry,0x800a23e8u,0x800a2b50u);
                    AS_W(AS_S0-4u,0);
                }
            }
            /* The third-channel null branch increments S1 before any
             * protection read. Its protection branch sets V0 in delay. */
            AS_A1=AS_R(AS_S0);AS_S1++;
            if(AS_A1) {
                AS_V0=AS_B(AS_A1+3u);keep=AS_V0==AS_S4;
                AS_V0=(uint32_t)((int32_t)AS_S1<(int32_t)AS_S3);
                if(!keep) {
                    AS_A0=AS_S2;
                    battle_animation_start_call(entry,0x800a23e8u,0x800a2b7cu);
                    AS_W(AS_S0,0);
                    AS_V0=(uint32_t)((int32_t)AS_S1<(int32_t)AS_S3);
                }
            } else AS_V0=(uint32_t)((int32_t)AS_S1<(int32_t)AS_S3);
            AS_S0+=0x7cu;
        } while(AS_V0);
    }
    /* Zero count deliberately leaves V0 unchanged. */
    p=AS_R(AS_SP+0x2cu);q=AS_R(AS_SP+0x28u);AS_RA=p;
    p=AS_R(AS_SP+0x24u);AS_S4=q;
    q=AS_R(AS_SP+0x20u);AS_S3=p;
    p=AS_R(AS_SP+0x1cu);AS_S2=q;
    q=AS_R(AS_SP+0x18u);AS_S1=p;
    AS_SP+=0x30u;AS_S0=q;
}

static int battle_animation_start_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x800a2434u,720u,"caaba7ed66d982ae863a9136db5a0f2d07bac493eaea5dc47466de87e9b4d2bd"},
        {0x800a2accu,236u,"020a0eba7e385c5fa2917a872a19d5c1f7bb4f9f579419e8f4aa6aa947e5344c"}
    };
    unsigned index=0;while(index<2u&&bodies[index].target!=target)index++;
    if(index==2u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ANIMATION_START");
    if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800a2434u)battle_animation_start_run(&entry);
    else battle_animation_release_all_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef AS_R
#undef AS_H
#undef AS_B
#undef AS_W
#undef AS_SH
#undef AS_SB
#undef AS_V0
#undef AS_V1
#undef AS_A0
#undef AS_A1
#undef AS_A2
#undef AS_A3
#undef AS_T0
#undef AS_S0
#undef AS_S1
#undef AS_S2
#undef AS_S3
#undef AS_S4
#undef AS_S5
#undef AS_S6
#undef AS_S7
#undef AS_SP
#undef AS_FP
#undef AS_RA
