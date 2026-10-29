/* Complete per-frame camera update and view/effects wrapper. Fixed native
 * arithmetic and control flow; no original CPU or GTE opcode dispatch. */
#define CU_R32(a) battle_entry_read(entry,(a),4)
#define CU_R16(a) battle_entry_read(entry,(a),2)
#define CU_H(a) ((uint32_t)(int32_t)(int16_t)CU_R16(a))
#define CU_R8(a) battle_entry_read(entry,(a),1)
#define CU_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define CU_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define CU_V0 (entry->cpu->gpr[2])
#define CU_V1 (entry->cpu->gpr[3])
#define CU_A0 (entry->cpu->gpr[4])
#define CU_A1 (entry->cpu->gpr[5])
#define CU_A2 (entry->cpu->gpr[6])
#define CU_A3 (entry->cpu->gpr[7])
#define CU_T0 (entry->cpu->gpr[8])
#define CU_S0 (entry->cpu->gpr[16])
#define CU_S1 (entry->cpu->gpr[17])
#define CU_S2 (entry->cpu->gpr[18])
#define CU_SP (entry->cpu->gpr[29])
#define CU_RA (entry->cpu->gpr[31])
#define CU_ASR(v,n) battle_target_camera_asr((v),(n))
#define CU_MULT(a,b) battle_target_camera_product(entry->cpu,(a),(b))
#define CU_CALL(t,s) battle_camera_update_call(entry,(t),(s))

static void battle_camera_update_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    CU_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-camera-update] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"camera update callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending camera update callee return",target);
}
/* The preceding frame stores require an aligned SP. Each paired unaligned
 * instruction therefore covers a full aligned word here, but its duplicate
 * reads/writes and delayed register commits remain observable on faults. */
static void battle_camera_update_copy(BattleEntryCall *entry,uint32_t source,uint32_t destination)
{
    uint32_t p=CU_R32(source),q=CU_R32(source);CU_V1=p;
    p=CU_R32(source+4u);CU_V1=q;q=CU_R32(source+4u);CU_A0=p;
    (void)CU_R32(destination);CU_W32(destination,CU_V1);CU_A0=q;
    (void)CU_R32(destination);CU_W32(destination,CU_V1);
    (void)CU_R32(destination+4u);CU_W32(destination+4u,CU_A0);
    (void)CU_R32(destination+4u);CU_W32(destination+4u,CU_A0);
}
/* Fixed GPF(sf=1,lm=0). Products fit int32 before the arithmetic shift.
 * IR upper halves survive until the actual SWC2 reads; flags and RGB FIFO
 * retain the existing graphics-register representation. */
static void battle_camera_update_product(uint32_t factor,const uint32_t delta[3])
{
    uint32_t flags=0,rgb=MFC2(6)&0xff000000u;
    for(unsigned i=0;i<3;i++){
        int32_t product=(int32_t)(int16_t)factor*(int32_t)(int16_t)delta[i];
        int32_t scaled=(int32_t)CU_ASR((uint32_t)product,12),ir=scaled,color=scaled>>4;
        if(ir>32767){ir=32767;flags|=1u<<(24u-i);}
        else if(ir< -32768){ir= -32768;flags|=1u<<(24u-i);}
        if(color>255){color=255;flags|=1u<<(21u-i);}
        else if(color<0){color=0;flags|=1u<<(21u-i);}
        MTC2((uint32_t)scaled,(int)(25u+i));
        MTC2((delta[i]&0xffff0000u)|(uint16_t)ir,(int)(9u+i));
        rgb|=(uint32_t)color<<(i*8u);
    }
    MTC2(MFC2(21),20);MTC2(MFC2(22),21);MTC2(rgb,22);CTC2(flags,31);
}
static void battle_camera_update_smooth(BattleEntryCall *entry,uint32_t factor)
{
    for(unsigned pass=0;pass<2;pass++){
        uint32_t desired=4u+pass*8u,current=0x3354u+pass*8u;
        CU_V0=0x800d0000u;CU_A3=CU_V0+0x309cu;CU_T0=0x800d0000u;
        uint32_t p=CU_H(CU_A3+desired),q=CU_H(CU_T0+current);CU_V0=p;
        CU_A2=CU_T0+current;CU_V1=q;CU_V0-=CU_V1;CU_W32(CU_SP+0x10u,CU_V0);
        p=CU_H(CU_A3+desired+2u);q=CU_H(CU_A2+2u);CU_V0=p;
        p=CU_H(CU_A3+desired+4u);CU_A0=q;q=CU_H(CU_A2+4u);CU_V1=p;
        CU_V0-=CU_A0;CU_A1=q;CU_V1-=CU_A1;CU_W32(CU_SP+0x14u,CU_V0);
        CU_V0=CU_SP+0x10u;CU_W32(CU_SP+0x18u,CU_V1);
        uint32_t delta[3];
        for(unsigned i=0;i<3;i++){delta[i]=CU_R32(CU_V0+i*4u);MTC2(delta[i],(int)(9u+i));}
        battle_camera_update_product(factor,delta);
        for(unsigned i=0;i<3;i++)CU_W32(CU_V0+i*4u,MFC2((int)(9u+i)));
        p=CU_R32(CU_SP+0x10u);q=CU_R32(CU_SP+0x18u);CU_V0=p;
        p=CU_R32(CU_SP+0x14u);CU_V1=q;CU_V0|=CU_V1;CU_A0=p;CU_V0|=CU_A0;
        int increment=CU_V0!=0;CU_V0=CU_A3+desired;
        if(increment){
            p=CU_R16(CU_T0+current);q=CU_R16(CU_SP+0x10u);CU_V0=p;
            p=CU_R16(CU_SP+0x14u);CU_V1=q;q=CU_R16(CU_SP+0x18u);CU_A0=p;
            CU_V0+=CU_V1;CU_A1=q;CU_W16(CU_T0+current,CU_V0);
            p=CU_R16(CU_A2+2u);q=CU_R16(CU_A2+4u);CU_V0=p;
            CU_V0+=CU_A0;CU_V1=q;CU_V1+=CU_A1;CU_W16(CU_A2+2u,CU_V0);CU_W16(CU_A2+4u,CU_V1);
        }else{
            p=CU_R16(CU_A3+desired);q=CU_R16(CU_V0+2u);CU_V1=p;
            p=CU_R16(CU_V0+4u);CU_A0=q;CU_W16(CU_T0+current,CU_V1);CU_V0=p;
            CU_W16(CU_A2+2u,CU_A0);CU_W16(CU_A2+4u,CU_V0);
        }
    }
    CU_A0=CU_SP+0x40u;
}
static void battle_camera_update_tick(BattleEntryCall *entry)
{
    CU_SP-=0x80u;CU_V0=0x800c0000u;uint32_t p=CU_R32(CU_V0+0x3cc0u);CU_V0=1;CU_V1=p;
    CU_W32(CU_SP+0x78u,CU_RA);CU_W32(CU_SP+0x74u,CU_S1);
    int framing=CU_V1==CU_V0;CU_W32(CU_SP+0x70u,CU_S0);
    if(framing){
        CU_V0=0x800c0000u;p=CU_R32(CU_V0+0x3678u);CU_RA=0x800bbb10u;CU_A0=p;
        CU_CALL(0x800bc460u,0x800bbb08u);CU_V0=0x800c0000u;
    }else{
        CU_V0=(int32_t)CU_V1<2;int lower=CU_V0!=0;CU_V0=0x800c0000u;
        if(!lower){
            CU_V0=2;int packed=CU_V1==CU_V0;CU_V0=3;
            if(packed){
                CU_A0=0x80070000u;CU_V1=0x800d0000u;p=CU_R32(CU_A0-0x664u);CU_A0-=0x664u;CU_V0=p;
                CU_A1=0x80070000u;CU_V0=CU_ASR(CU_V0,16);CU_W16(CU_V1+0x30a0u,CU_V0);CU_V1+=0x30a0u;
                p=CU_R32(CU_A1-0x654u);CU_A1-=0x654u;CU_V0=p;
                uint32_t q=CU_H(CU_A0+6u),r=CU_H(CU_A0+10u);CU_A2=q;CU_V0=CU_ASR(CU_V0,16);CU_A0=r;
                CU_W16(CU_V1+2u,CU_A2);CU_W16(CU_V1+4u,CU_A0);CU_W16(CU_V1+8u,CU_V0);
                p=CU_H(CU_A1+6u);q=CU_H(CU_A1+10u);CU_V0=p;CU_V1+=8u;CU_A0=q;
                CU_W16(CU_V1+2u,CU_V0);CU_W16(CU_V1+4u,CU_A0);CU_V0=0x800c0000u;
            }else{
                int orbit=CU_V1==CU_V0;CU_V0=0x800c0000u;
                if(orbit){
                    CU_V0=0x800d0000u;CU_S1=0x800c0000u;p=CU_R32(CU_V0+0x39ecu);
                    uint32_t q=CU_H(CU_S1+0x373cu);CU_V1=p;p=CU_H(CU_V1+2u);CU_A0=q;CU_V0=p;
                    CU_W16(CU_SP+0x10u,CU_V0);CU_W16(CU_SP+0x18u,CU_V0);
                    CU_V0=CU_H(CU_V1+6u);CU_W16(CU_SP+0x12u,CU_V0);
                    p=CU_H(CU_V1+10u);CU_RA=0x800bbba8u;CU_V0=p;CU_W16(CU_SP+0x14u,CU_V0);CU_CALL(0x8003f8ccu,0x800bbba0u);
                    CU_S0=0x800c0000u;CU_V1=CU_R32(CU_S0+0x3738u);CU_MULT(CU_V0,CU_V1);CU_V1=entry->cpu->lo;
                    if((int32_t)CU_V1<0)CU_V1+=0xfffu;
                    CU_V1=CU_ASR(CU_V1,12);
                    p=CU_R16(CU_SP+0x14u);q=CU_H(CU_S1+0x373cu);CU_V0=p;CU_V0-=CU_V1;CU_A0=q;
                    CU_RA=0x800bbbe0u;CU_W16(CU_SP+0x1cu,CU_V0);CU_CALL(0x8003f8b0u,0x800bbbd8u);
                    CU_V1=CU_R32(CU_S0+0x3738u);CU_MULT(CU_V0,CU_V1);CU_V1=entry->cpu->lo;
                    if((int32_t)CU_V1<0)CU_V1+=0xfffu;
                    p=CU_R16(CU_SP+0x12u);CU_V1=CU_ASR(CU_V1,12);CU_V0=p;CU_V0-=CU_V1;CU_W16(CU_SP+0x1au,CU_V0);
                    CU_V0=0x800d0000u;CU_V0+=0x309cu;
                    battle_camera_update_copy(entry,CU_SP+0x18u,CU_V0+4u);
                    battle_camera_update_copy(entry,CU_SP+0x10u,CU_V0+12u);CU_V0=0x800c0000u;
                }
            }
        }
    }
    p=CU_R32(CU_V0+0x3cbcu);CU_V0=1;CU_V1=p;int smooth=CU_V1==CU_V0;CU_A0=CU_SP+0x40u;
    if(smooth){CU_V0=0x800c0000u;CU_V0=CU_R32(CU_V0+0x3674u);MTC2(CU_V0,8);battle_camera_update_smooth(entry,CU_V0);}
    CU_V1=0x800d0000u;CU_A2=0x800d0000u;CU_A1=CU_SP+0x60u;
    p=CU_H(CU_V1+0x335cu);CU_V1+=0x335cu;CU_V0=p;
    p=CU_H(CU_A2+0x3354u);CU_A2+=0x3354u;CU_A3=p;CU_V0-=CU_A3;CU_W32(CU_SP+0x40u,CU_V0);
    p=CU_H(CU_V1+2u);uint32_t q=CU_H(CU_A2+2u);CU_V0=p;
    p=CU_H(CU_V1+4u);CU_A3=q;q=CU_H(CU_A2+4u);CU_V1=p;
    CU_V0-=CU_A3;CU_A2=q;CU_V1-=CU_A2;CU_W32(CU_SP+0x44u,CU_V0);
    CU_RA=0x800bbe64u;CU_W32(CU_SP+0x48u,CU_V1);CU_CALL(0x8004a414u,0x800bbe5cu);
    p=CU_R32(CU_SP+0x60u);q=CU_R32(CU_SP+0x68u);CU_V0=p;CU_RA=0x800bbe74u;CU_A0=q;
    CU_A0=CU_V0+CU_A0;CU_CALL(0x80048c4cu,0x800bbe6cu);CU_S1=CU_V0;
    p=CU_R32(CU_SP+0x60u);q=CU_R32(CU_SP+0x64u);CU_A0=p;
    p=CU_R32(CU_SP+0x68u);CU_V1=q;CU_A0+=CU_V1;CU_A1=p;CU_RA=0x800bbe90u;
    CU_A0+=CU_A1;CU_CALL(0x80048c4cu,0x800bbe88u);
    CU_S0=0x800d0000u;p=CU_R32(CU_SP+0x48u);q=CU_R32(CU_SP+0x40u);CU_A0=p;
    CU_S0+=0x309cu;CU_A1=q;CU_RA=0x800bbea8u;CU_W32(CU_S0+0x1cu,CU_V0);CU_CALL(0x8004b32cu,0x800bbea0u);
    CU_A1=CU_S1;p=CU_R32(CU_SP+0x44u);CU_V0=0u-CU_V0;CU_A0=p;
    CU_RA=0x800bbebcu;CU_W16(CU_S0+0x16u,CU_V0);CU_CALL(0x8004b32cu,0x800bbeb4u);
    CU_V0=0u-CU_V0;CU_W16(CU_S0+0x14u,CU_V0);CU_W16(CU_S0+0x18u,0);
    p=CU_R32(CU_SP+0x78u);q=CU_R32(CU_SP+0x74u);CU_RA=p;
    p=CU_R32(CU_SP+0x70u);CU_S1=q;CU_SP+=0x80u;CU_S0=p;
}
static void battle_camera_update_view(BattleEntryCall *entry)
{
    CU_SP-=0x30u;CU_V0=0x800d0000u;CU_W32(CU_SP+0x20u,CU_S0);CU_S0=CU_V0+0x30bcu;CU_A0=CU_S0;
    CU_V0=0x800d0000u;CU_W32(CU_SP+0x28u,CU_S2);CU_S2=CU_V0+0x3354u;CU_A1=CU_S2;
    CU_V0=0x800d0000u;CU_W32(CU_SP+0x24u,CU_S1);CU_S1=CU_V0+0x335cu;CU_A2=CU_S1;
    CU_A3=0x800c0000u;CU_W32(CU_SP+0x2cu,CU_RA);CU_A3+=0x3730u;CU_CALL(0x800bb844u,0x800bba10u);
    CU_A0=CU_S0-0x20u;CU_V0=0x800c0000u;CU_A3=CU_V0+0x354cu;
    uint32_t p=CU_H(CU_V0+0x354cu),q=CU_R32(CU_A0+0x34u);CU_A1=p;
    p=CU_H(CU_A3+2u);CU_V0=q;q=CU_R32(CU_A0+0x38u);CU_A2=p;
    CU_V0+=CU_A1;CU_V1=q;CU_V1+=CU_A2;CU_W32(CU_A0+0x38u,CU_V1);
    CU_V1=0x800c0000u;CU_W32(CU_A0+0x34u,CU_V0);
    p=CU_H(CU_A3+4u);q=CU_R32(CU_A0+0x3cu);CU_A1=p;p=CU_R8(CU_V1+0x372cu);CU_V0=q;
    CU_V0+=CU_A1;CU_V1=p;int skip=CU_V1!=0;CU_W32(CU_A0+0x3cu,CU_V0);
    if(!skip){
        CU_A0=CU_S0;CU_A1=0;CU_V1=0x800c0000u;CU_V1+=0x3eb0u;CU_V0=0x8000u;CU_V1+=CU_V0;
        p=CU_R32(CU_V1+0xc54u);CU_A2=CU_A1;CU_A3=p;CU_W32(CU_SP+0x14u,CU_S2);CU_W32(CU_SP+0x18u,CU_S1);
        p=CU_R32(CU_V1+0xc84u);CU_V0=0x1000u;CU_V1=p;CU_W32(CU_SP+0x1cu,CU_V0);
        CU_RA=0x800bba9cu;CU_W32(CU_SP+0x10u,CU_V1);CU_CALL(0x800a4654u,0x800bba94u);
    }
    p=CU_R32(CU_SP+0x2cu);q=CU_R32(CU_SP+0x28u);CU_RA=p;
    p=CU_R32(CU_SP+0x24u);CU_S2=q;q=CU_R32(CU_SP+0x20u);CU_S1=p;CU_SP+=0x30u;CU_S0=q;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_camera_update_bodies[]={
    {0x800bbab8u,1064u,"84f51ca52a388f4ab102571dc64e9371803eb8015c99ceaff5e85c7edc525fda"},
    {0x800bb9d4u,228u,"5e2150e561c022d0d6ebd14c6aaebd855eb7791488e911e2fc28735425da02e3"},
};
static int battle_camera_update_target(uint32_t target)
{return target==0x800bbab8u||target==0x800bb9d4u;}
static int battle_camera_update_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_camera_update_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_CAMERA_UPDATE");if(control&&strcmp(control,"0")==0)return 0;
    unsigned i=target==0x800bbab8u?0:1;uint8_t digest[32];
    file1_digest(PSX_ADDR(target),battle_camera_update_bodies[i].bytes,digest);const char *expected=battle_camera_update_bodies[i].sha;
    for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(i==0)battle_camera_update_tick(&entry);else battle_camera_update_view(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef CU_R32
#undef CU_R16
#undef CU_H
#undef CU_R8
#undef CU_W32
#undef CU_W16
#undef CU_V0
#undef CU_V1
#undef CU_A0
#undef CU_A1
#undef CU_A2
#undef CU_A3
#undef CU_T0
#undef CU_S0
#undef CU_S1
#undef CU_S2
#undef CU_SP
#undef CU_RA
#undef CU_ASR
#undef CU_MULT
#undef CU_CALL
