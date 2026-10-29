/* Complete target-camera wrapper, framing and view-matrix construction.
 * Fixed native loops retain the live guest reads and original call/frame order.
 * SDK graphics services use checked native boundaries; no CPU decoding occurs. */
#define TC_R32(a) battle_entry_read(entry,(a),4)
#define TC_R16(a) battle_entry_read(entry,(a),2)
#define TC_H(a) ((uint32_t)(int32_t)(int16_t)TC_R16(a))
#define TC_R8(a) battle_entry_read(entry,(a),1)
#define TC_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define TC_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define TC_CALL(t,s) battle_target_camera_call(entry,(t),(s))
#define TC_V0 (entry->cpu->gpr[2])
#define TC_V1 (entry->cpu->gpr[3])
#define TC_A0 (entry->cpu->gpr[4])
#define TC_A1 (entry->cpu->gpr[5])
#define TC_A2 (entry->cpu->gpr[6])
#define TC_A3 (entry->cpu->gpr[7])
#define TC_T0 (entry->cpu->gpr[8])
#define TC_T1 (entry->cpu->gpr[9])
#define TC_T2 (entry->cpu->gpr[10])
#define TC_T3 (entry->cpu->gpr[11])
#define TC_T4 (entry->cpu->gpr[12])
#define TC_T5 (entry->cpu->gpr[13])
#define TC_T6 (entry->cpu->gpr[14])
#define TC_T7 (entry->cpu->gpr[15])
#define TC_S0 (entry->cpu->gpr[16])
#define TC_S1 (entry->cpu->gpr[17])
#define TC_S2 (entry->cpu->gpr[18])
#define TC_S3 (entry->cpu->gpr[19])
#define TC_S4 (entry->cpu->gpr[20])
#define TC_S5 (entry->cpu->gpr[21])
#define TC_S6 (entry->cpu->gpr[22])
#define TC_S7 (entry->cpu->gpr[23])
#define TC_SP (entry->cpu->gpr[29])
#define TC_FP (entry->cpu->gpr[30])
#define TC_RA (entry->cpu->gpr[31])

static uint32_t battle_target_camera_asr(uint32_t value,unsigned shift)
{return (value>>shift)|((0u-(value>>31))<<(32u-shift));}
static void battle_target_camera_product(PcPortMipsCpu *cpu,uint32_t a,uint32_t b)
{
    int64_t product=(int64_t)(int32_t)a*(int32_t)b;
    cpu->lo=(uint32_t)product;cpu->hi=(uint32_t)((uint64_t)product>>32);
}
static void battle_target_camera_divide(PcPortMipsCpu *cpu,uint32_t a,uint32_t b)
{
    if(!b){cpu->lo=(int32_t)a<0?1u:UINT32_MAX;cpu->hi=a;}
    else if(a==0x80000000u&&b==UINT32_MAX){cpu->lo=a;cpu->hi=0;}
    else{cpu->lo=(uint32_t)((int64_t)(int32_t)a/(int32_t)b);cpu->hi=(uint32_t)((int64_t)(int32_t)a%(int32_t)b);}
}
#define TC_ASR(v,n) battle_target_camera_asr((v),(n))
#define TC_MULT(a,b) battle_target_camera_product(entry->cpu,(a),(b))
#define TC_DIV(a,b) battle_target_camera_divide(entry->cpu,(a),(b))
static void battle_target_camera_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    TC_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-target-camera] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"target camera callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending target camera callee return",target);
}
static void battle_target_camera_wrapper(BattleEntryCall *entry)
{
    TC_V0=0x800c0000u;uint32_t flag=TC_R8(0x800c37c8u);TC_SP-=0x18u;TC_V0=flag;
    TC_W32(TC_SP+0x10u,TC_S0);TC_S0=TC_A0;int skip=TC_V0!=0;TC_W32(TC_SP+0x14u,TC_RA);
    if(!skip){TC_A0=1;TC_CALL(0x800bc2f0u,0x800bc420u);TC_A0=TC_S0;TC_CALL(0x800bc460u,0x800bc428u);}
    TC_V0=0x800c0000u;uint32_t half=TC_R16(0x800c3cdcu);TC_V0=0x80060000u;TC_V1=half;
    TC_W16(TC_V0-0x6bacu,TC_V1);uint32_t ra=TC_R32(TC_SP+0x14u),s0=TC_R32(TC_SP+0x10u);
    TC_RA=ra;TC_SP+=0x18u;TC_S0=s0;
}
static void battle_target_camera_view(BattleEntryCall *entry)
{
    TC_SP-=0x68u;TC_W32(TC_SP+0x54u,TC_S1);TC_S1=TC_A0;TC_W32(TC_SP+0x60u,TC_S4);TC_S4=TC_A1;
    TC_W32(TC_SP+0x50u,TC_S0);TC_S0=TC_A3;TC_A0=TC_SP+0x10u;
    TC_W32(TC_SP+0x64u,TC_RA);TC_W32(TC_SP+0x5cu,TC_S3);TC_W32(TC_SP+0x58u,TC_S2);
    uint32_t p=TC_H(TC_A2),q=TC_H(TC_S4);TC_V0=p;p=TC_H(TC_A2+2u);TC_A1=q;
    q=TC_H(TC_A2+4u);TC_T0=p;p=TC_H(TC_S4+4u);TC_V1=q;TC_A1=TC_V0-TC_A1;TC_A3=p;
    p=TC_H(TC_S4+2u);TC_A3=TC_V1-TC_A3;TC_V0=p;TC_A2=TC_T0-TC_V0;TC_CALL(0x80021b14u,0x800bb890u);
    p=TC_H(TC_S0);TC_A0=TC_SP+0x10u;TC_V0=p;TC_W32(TC_SP+0x40u,TC_V0);
    p=TC_H(TC_S0+2u);TC_S2=TC_SP+0x20u;TC_V0=p;TC_W32(TC_SP+0x44u,TC_V0);
    p=TC_H(TC_S0+4u);TC_A1=TC_S2;TC_V0=p;TC_RA=0x800bb8c0u;TC_W32(TC_SP+0x48u,TC_V0);TC_CALL(0x80048d7cu,0x800bb8b8u);
    TC_S3=TC_SP+0x40u;TC_A0=TC_S3;TC_A1=TC_S2;TC_A2=TC_SP+0x10u;TC_CALL(0x8004a480u,0x800bb8ccu);
    TC_A0=TC_SP+0x10u;TC_S0=TC_SP+0x30u;TC_A1=TC_S0;TC_CALL(0x80048d7cu,0x800bb8dcu);
    TC_A0=TC_S2;TC_A1=TC_S0;TC_A2=TC_SP+0x10u;TC_CALL(0x8004a480u,0x800bb8ecu);
    TC_A0=TC_SP+0x10u;TC_A1=TC_S3;TC_CALL(0x80048d7cu,0x800bb8f8u);
    const uint32_t rows[]={0x30u,0x40u,0x20u};
    for(unsigned row=0;row<3;row++)for(unsigned axis=0;axis<3;axis++){
        TC_V0=TC_R16(TC_SP+rows[row]+axis*4u);
        if(row==2&&axis==2)TC_RA=0x800bb96cu;
        TC_W16(TC_S1+row*6u+axis*2u,TC_V0);
    }
    TC_CALL(0x8004960cu,0x800bb964u);
    TC_A0=TC_S1;TC_A1=TC_S4;TC_A2=TC_SP+0x10u;TC_CALL(0x80049cecu,0x800bb974u);
    for(unsigned axis=0;axis<3;axis++){
        TC_V0=TC_R32(TC_SP+0x10u+axis*4u);TC_V0=0u-TC_V0;
        if(axis==2)TC_RA=0x800bb9b0u;
        TC_W32(TC_S1+0x14u+axis*4u,TC_V0);
    }
    TC_CALL(0x800496acu,0x800bb9a8u);
    const unsigned saved[]={31,20,19,18,17,16};p=TC_R32(TC_SP+0x64u);
    for(unsigned i=1;i<6;i++){q=TC_R32(TC_SP+0x64u-i*4u);entry->cpu->gpr[saved[i-1]]=p;p=q;}
    TC_SP+=0x68u;TC_S0=p;
}
/* Both projection sites have the same signed, narrowed radius accumulation. */
static void battle_target_camera_radius(BattleEntryCall *entry)
{
    TC_A0=TC_R16(TC_SP+0x68u);TC_A0-=160u;TC_V0=TC_A0<<18;TC_V0=TC_ASR(TC_V0,16);TC_MULT(TC_V0,TC_V0);
    TC_V1=TC_R16(TC_SP+0x6au);TC_V1-=164u;TC_A1=entry->cpu->lo;TC_V0=TC_V1<<18;
    TC_V0=TC_ASR(TC_V0,16);TC_MULT(TC_V0,TC_V0);TC_W16(TC_SP+0x68u,TC_A0);TC_A0<<=2;
    TC_W16(TC_SP+0x6au,TC_V1);TC_V1<<=2;TC_W16(TC_SP+0x68u,TC_A0);TC_T0=entry->cpu->lo;
    TC_A1+=TC_T0;TC_V0=(int32_t)TC_S5<(int32_t)TC_A1;int replace=TC_V0!=0;
    TC_W16(TC_SP+0x6au,TC_V1);if(replace)TC_S5=TC_A1;
}
static void battle_target_camera_frame(BattleEntryCall *entry)
{
    TC_SP-=0x120u;TC_W32(TC_SP+0x104u,TC_S3);TC_S3=TC_A0;TC_A0=TC_SP+0x10u;TC_A1=0;TC_A2=16;
    const unsigned save[]={31,30,23,22,21,20,18,17};
    const uint32_t offsets[]={0x11cu,0x118u,0x114u,0x110u,0x10cu,0x108u,0x100u,0xfcu};
    for(unsigned i=0;i<8;i++)TC_W32(TC_SP+offsets[i],entry->cpu->gpr[save[i]]);
    TC_RA=0x800bc4a0u;TC_W32(TC_SP+0xf8u,TC_S0);TC_CALL(0x8003fa08u,0x800bc498u);
    TC_S5=0;TC_V0=0x800c0000u;TC_S2=TC_S5;TC_A2=TC_S2;TC_S1=TC_S3;TC_T0=0x8c8cu;TC_A3=11;
    TC_W32(TC_V0+0x3678u,TC_S3);TC_V0=0x800c0000u;TC_A0=TC_V0+0x3eb0u;TC_A1=TC_A0;
    do {
        TC_V0=TC_S1&1u;
        if(TC_V0){
            TC_V0=TC_R8(TC_A0+7u);int suppressed=TC_V0!=0;TC_V0=TC_A1+TC_T0;
            if(!suppressed){
                TC_S0=TC_R32(TC_V0);
                if(TC_S0){
                    TC_A2++;
                    for(unsigned axis=0;axis<3;axis++){
                        uint32_t p=TC_R32(TC_S0+axis*4u),q=TC_R32(TC_SP+0x10u+axis*4u);
                        TC_V0=p;TC_V0=TC_ASR(TC_V0,1);TC_V1=q;TC_V1+=TC_V0;TC_W32(TC_SP+0x10u+axis*4u,TC_V1);
                    }
                }
            }
        }
        TC_A1+=4u;TC_A0+=0x1cu;TC_S2++;TC_V0=TC_S1&0xffffu;TC_S1=TC_V0>>1;
    }while(TC_S2!=TC_A3);
    int empty=TC_A2==0;TC_S2=0;
    if(!empty){
        TC_V1=TC_R32(TC_SP+0x10u);TC_DIV(TC_V1,TC_A2);TC_V1=entry->cpu->lo;
        TC_A0=TC_R32(TC_SP+0x18u);TC_DIV(TC_A0,TC_A2);TC_A0=entry->cpu->lo;
        TC_A1=TC_R32(TC_SP+0x14u);TC_DIV(TC_A1,TC_A2);TC_A1=entry->cpu->lo;
        TC_S1=TC_S3;TC_T6=0x8c8cu;TC_T5=11;TC_V0=0x800c0000u;TC_T0=TC_V0+0x3eb0u;TC_A3=TC_T0;
        TC_V1<<=1;TC_T3=TC_V1;TC_A0<<=1;TC_T2=TC_A0;TC_T4=TC_T2;TC_A1<<=1;TC_T1=TC_A1;TC_A2=TC_T1;
        TC_W32(TC_SP+0x10u,TC_V1);TC_W32(TC_SP+0x14u,TC_T1);TC_W32(TC_SP+0x18u,TC_T2);
        do {
            TC_V0=TC_S1&1u;
            if(TC_V0){
                TC_V0=TC_R8(TC_T0+7u);int suppressed=TC_V0!=0;TC_V0=TC_A3+TC_T6;
                if(!suppressed){
                    TC_S0=TC_R32(TC_V0);
                    if(TC_S0){
                        TC_A0=TC_R32(TC_S0);TC_V0=(int32_t)TC_T3<(int32_t)TC_A0;int higher=TC_V0!=0;
                        TC_V0=(int32_t)TC_A0<(int32_t)TC_V1;if(higher)TC_T3=TC_A0;if(TC_V0)TC_V1=TC_A0;
                        TC_A0=TC_R32(TC_S0+8u);TC_V0=(int32_t)TC_T4<(int32_t)TC_A0;higher=TC_V0!=0;
                        TC_V0=(int32_t)TC_A0<(int32_t)TC_T2;if(higher)TC_T4=TC_A0;if(TC_V0)TC_T2=TC_A0;
                        TC_A0=TC_R32(TC_S0+4u);TC_V0=(int32_t)TC_A2<(int32_t)TC_A0;higher=TC_V0!=0;
                        TC_V0=(int32_t)TC_A0<(int32_t)TC_T1;if(higher)TC_A2=TC_A0;if(TC_V0)TC_T1=TC_A0;
                    }
                }
            }
            TC_A3+=4u;TC_T0+=0x1cu;TC_S2++;TC_V0=TC_S1&0xffffu;TC_S1=TC_V0>>1;
        }while(TC_S2!=TC_T5);
        TC_A0=0x800c3740u;TC_S0=TC_SP+0x30u;TC_A1=TC_S0;TC_A3=TC_V1+TC_T3;
        TC_V0=TC_A3>>31;TC_A3+=TC_V0;TC_V0=TC_ASR(TC_A3,1);TC_A2+=TC_T1;TC_W32(TC_SP+0x10u,TC_V0);
        TC_V0=TC_A2>>31;TC_A2+=TC_V0;TC_V0=TC_ASR(TC_A2,1);TC_W32(TC_SP+0x14u,TC_V0);
        TC_V0=TC_T2+TC_T4;TC_V1=TC_V0>>31;TC_V0+=TC_V1;TC_V1=TC_ASR(TC_V0,1);
        TC_A3=TC_ASR(TC_A3,17);TC_A2=TC_ASR(TC_A2,17);TC_V0=TC_ASR(TC_V0,17);
        TC_W32(TC_SP+0x18u,TC_V1);TC_W32(TC_SP+0x10u,TC_A3);TC_W32(TC_SP+0x14u,TC_A2);
        TC_RA=0x800bc6e4u;TC_W32(TC_SP+0x18u,TC_V0);TC_CALL(0x8004abbcu,0x800bc6dcu);
        TC_W16(TC_SP+0x70u,0);TC_RA=0x800bc6f0u;TC_W16(TC_SP+0x72u,0);TC_CALL(0x8004a0dcu,0x800bc6e8u);
        TC_A0=TC_S0;TC_A1=TC_SP+0x70u;TC_A2=TC_SP+0x50u;TC_V0<<=3;
        TC_RA=0x800bc708u;TC_W16(TC_SP+0x74u,TC_V0);TC_CALL(0x80049cecu,0x800bc700u);
        TC_A0=TC_S0;TC_A1=TC_SP+0x20u;TC_A2=TC_SP+0x28u;TC_A3=0x800c3730u;TC_S2=0;TC_T7=TC_SP+0x60u;TC_FP=TC_SP+0x68u;
        uint32_t p=TC_R16(TC_SP+0x10u),q=TC_R16(TC_SP+0x14u);TC_V0=p;p=TC_R16(TC_SP+0x18u);TC_V1=q;
        q=TC_R16(TC_SP+0x50u);TC_T0=p;TC_S7=TC_SP+0x78u;TC_T1=q;
        TC_W16(TC_SP+0x20u,TC_V0);TC_W16(TC_SP+0x28u,TC_V0);TC_V0-=TC_T1;TC_W16(TC_SP+0x20u,TC_V0);
        p=TC_R16(TC_SP+0x54u);q=TC_R16(TC_SP+0x58u);TC_V0=p;TC_S6=TC_SP+0x7cu;TC_T1=q;
        TC_W32(TC_SP+0xf0u,TC_T7);TC_W16(TC_SP+0x22u,TC_V1);TC_W16(TC_SP+0x24u,TC_T0);
        TC_W16(TC_SP+0x2au,TC_V1);TC_W16(TC_SP+0x2cu,TC_T0);TC_V1+=TC_V0;TC_T0-=TC_T1;TC_W16(TC_SP+0x22u,TC_V1);
        TC_RA=0x800bc780u;TC_W16(TC_SP+0x24u,TC_T0);TC_CALL(0x800bb844u,0x800bc778u);
        TC_A0=TC_S0;TC_CALL(0x80049efcu,0x800bc780u);TC_A0=TC_S0;TC_CALL(0x80049f8cu,0x800bc788u);
        TC_S1=TC_S3;TC_V0=0x800c0000u;TC_S3=TC_V0+0x3eb0u;TC_S4=TC_S3;
        do {
            TC_V0=TC_S1&1u;
            if(TC_V0){
                TC_V0=TC_R8(TC_S3+7u);int suppressed=TC_V0!=0;TC_V0=0x8c8cu;
                if(!suppressed){
                    TC_V0+=TC_S4;TC_S0=TC_R32(TC_V0);int absent=TC_S0==0;TC_A1=TC_FP;
                    if(!absent){
                        p=TC_R32(TC_SP+0xf0u);q=TC_H(TC_S0+2u);TC_A0=p;TC_V0=q;TC_W16(TC_SP+0x60u,TC_V0);
                        p=TC_H(TC_S0+6u);TC_A2=TC_S7;TC_V0=p;TC_W16(TC_SP+0x62u,TC_V0);
                        p=TC_H(TC_S0+10u);TC_A3=TC_S6;TC_V0=p;TC_RA=0x800bc7fcu;TC_W16(TC_SP+0x64u,TC_V0);
                        TC_CALL(0x8004a64cu,0x800bc7f4u);battle_target_camera_radius(entry);
                        TC_V0=TC_R8(TC_S3+8u);int has_height=TC_V0!=0;TC_V0=0x800c0000u;
                        if(has_height){
                            TC_V0=TC_R8(TC_V0+0x3688u);int gated=TC_V0!=0;TC_A1=TC_FP;
                            if(!gated){
                                TC_A2=TC_S7;TC_A3=TC_S6;
                                p=TC_R16(TC_SP+0x62u);q=TC_R16(TC_S0+0x36u);TC_V0=p;
                                p=TC_R32(TC_SP+0xf0u);TC_V1=q;TC_V0-=TC_V1;TC_A0=p;
                                TC_RA=0x800bc89cu;TC_W16(TC_SP+0x62u,TC_V0);TC_CALL(0x8004a64cu,0x800bc894u);
                                battle_target_camera_radius(entry);
                            }
                        }
                    }
                }
            }
            TC_S4+=4u;TC_S2++;TC_V0=TC_S1&0xffffu;TC_S1=TC_V0>>1;TC_V0=11;TC_S3+=0x1cu;
        }while(TC_S2!=TC_V0);
        TC_A0=TC_S5;TC_CALL(0x80048c4cu,0x800bc918u);TC_S5=TC_V0;TC_V0=(int32_t)TC_S5<120;
        int small=TC_V0!=0;TC_A0=0x800c0000u;
        if(small){
            TC_A0+=0x3740u;TC_S0=TC_SP+0x80u;TC_A1=TC_S0;TC_CALL(0x8004abbcu,0x800bc938u);
            TC_W16(TC_SP+0xb0u,0);TC_RA=0x800bc94cu;TC_W16(TC_SP+0xb2u,0);TC_CALL(0x8004a0dcu,0x800bc944u);
            TC_V0<<=1;TC_RA=0x800bc958u;TC_W16(TC_SP+0xb4u,TC_V0);TC_CALL(0x8004a0dcu,0x800bc950u);
            TC_A0=TC_S0;TC_A1=TC_SP+0xb0u;TC_A2=TC_SP+0x60u;TC_V1=0x800c0000u;TC_V0<<=1;
            TC_RA=0x800bc974u;TC_W32(TC_V1+0x3cdcu,TC_V0);TC_CALL(0x80049cecu,0x800bc96cu);
            p=TC_R16(TC_SP+0x10u);q=TC_R16(TC_SP+0x14u);TC_A2=p;p=TC_R16(TC_SP+0x18u);TC_A3=q;
            q=TC_R16(TC_SP+0x60u);TC_T0=p;p=TC_R16(TC_SP+0x64u);TC_A1=q;q=TC_R16(TC_SP+0x68u);TC_V1=p;
            TC_A0=q;TC_V0=0x800d0000u;
        }else{
            TC_V0=0x88888889u;TC_V1=TC_S5<<14;TC_MULT(TC_V1,TC_V0);TC_T7=entry->cpu->hi;
            TC_V0=TC_T7+TC_V1;TC_V0=TC_ASR(TC_V0,6);TC_V1=TC_ASR(TC_V1,31);TC_S1=TC_V0-TC_V1;
            TC_CALL(0x8004a0dcu,0x800bc9b4u);TC_V1=TC_S1<<1;TC_MULT(TC_V1,TC_V0);
            TC_A0=0x800c3740u;TC_S0=TC_SP+0xb8u;TC_A1=TC_S0;TC_V0=0x800c0000u;
            TC_S1=entry->cpu->lo;TC_S1=TC_ASR(TC_S1,14);TC_RA=0x800bc9e8u;TC_W32(TC_V0+0x3cdcu,TC_S1);
            TC_CALL(0x8004abbcu,0x800bc9e0u);TC_A0=TC_S0;TC_A1=TC_SP+0xe8u;TC_A2=TC_SP+0xa0u;
            TC_W16(TC_SP+0xe8u,0);TC_W16(TC_SP+0xeau,0);TC_RA=0x800bca04u;TC_W16(TC_SP+0xecu,TC_S1);
            TC_CALL(0x80049cecu,0x800bc9fcu);TC_V0=0x800d0000u;
            p=TC_R16(TC_SP+0x10u);q=TC_R16(TC_SP+0x14u);TC_A2=p;p=TC_R16(TC_SP+0x18u);TC_A3=q;
            q=TC_R16(TC_SP+0xa0u);TC_T0=p;p=TC_R16(TC_SP+0xa4u);TC_A1=q;q=TC_R16(TC_SP+0xa8u);TC_V1=p;
            TC_A1=TC_A2-TC_A1;TC_A0=q;
        }
        if(small)TC_A1=TC_A2-TC_A1;
        TC_V1=TC_A3+TC_V1;TC_A0=TC_T0-TC_A0;TC_W16(TC_V0+0x30a0u,TC_A1);TC_V0+=0x30a0u;
        TC_W16(TC_SP+0x20u,TC_A2);TC_W16(TC_SP+0x22u,TC_A3);TC_W16(TC_SP+0x24u,TC_T0);
        TC_W16(TC_SP+0x28u,TC_A2);TC_W16(TC_SP+0x2au,TC_A3);TC_W16(TC_SP+0x2cu,TC_T0);
        TC_W16(TC_SP+0x20u,TC_A1);TC_W16(TC_SP+0x22u,TC_V1);TC_W16(TC_SP+0x24u,TC_A0);
        TC_W16(TC_V0+2u,TC_V1);TC_W16(TC_V0+4u,TC_A0);TC_W16(TC_V0+8u,TC_A2);TC_V0+=8u;
        TC_W16(TC_V0+2u,TC_A3);TC_W16(TC_V0+4u,TC_T0);
    }
    const unsigned saved[]={31,30,23,22,21,20,19,18,17,16};uint32_t pending=TC_R32(TC_SP+0x11cu);
    for(unsigned i=1;i<10;i++){uint32_t next=TC_R32(TC_SP+0x11cu-i*4u);entry->cpu->gpr[saved[i-1]]=pending;pending=next;}
    TC_SP+=0x120u;TC_S0=pending;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_target_camera_bodies[]={
    {0x800bc404u,80u,"bf56810e8fd31bea70329998fcaf4757aa7c7700b88b773c1d1aee47ce91e8d8"},
    {0x800bc460u,1604u,"4549ebf7a618d484078505b6dd7d5d925e37c579ff7b529778bce077a6814996"},
    {0x800bb844u,400u,"c1f965e0a378d3a0439d7c2cdcdcab085ddbf7df3ed7f0ed49821afe7f112a29"},
    {0x80021b14u,16u,"28901a8f332a5d7a6d47145f0306c2c5bdc640b7aa1c3bafe4b12df7d63c4d66"},
};
static int battle_target_camera_target(uint32_t target)
{
    for(unsigned i=0;i<sizeof(battle_target_camera_bodies)/sizeof(battle_target_camera_bodies[0]);i++)
        if(battle_target_camera_bodies[i].target==target)return 1;
    return 0;
}
static int battle_target_camera_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_target_camera_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TARGET_CAMERA");if(control&&strcmp(control,"0")==0)return 0;
    for(unsigned i=0;i<sizeof(battle_target_camera_bodies)/sizeof(battle_target_camera_bodies[0]);i++)if(battle_target_camera_bodies[i].target==target){
        const uint8_t *code=PSX_ADDR(target);unsigned bytes=battle_target_camera_bodies[i].bytes;
        if(target==0x80021b14u){
            const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
            if(!image||!image->bytes||image->size<offset+bytes)return 0;
            code=image->bytes+offset;
        }
        uint8_t digest[32];file1_digest(code,bytes,digest);const char *expected=battle_target_camera_bodies[i].sha;
        for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800bc404u)battle_target_camera_wrapper(&entry);
    else if(target==0x800bc460u)battle_target_camera_frame(&entry);
    else if(target==0x800bb844u)battle_target_camera_view(&entry);
    else for(unsigned axis=0;axis<3;axis++)battle_entry_write(&entry,cpu->gpr[4]+axis*4u,4,cpu->gpr[5+axis]);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef TC_R32
#undef TC_R16
#undef TC_H
#undef TC_R8
#undef TC_W32
#undef TC_W16
#undef TC_CALL
#undef TC_V0
#undef TC_V1
#undef TC_A0
#undef TC_A1
#undef TC_A2
#undef TC_A3
#undef TC_T0
#undef TC_T1
#undef TC_T2
#undef TC_T3
#undef TC_T4
#undef TC_T5
#undef TC_T6
#undef TC_T7
#undef TC_S0
#undef TC_S1
#undef TC_S2
#undef TC_S3
#undef TC_S4
#undef TC_S5
#undef TC_S6
#undef TC_S7
#undef TC_SP
#undef TC_FP
#undef TC_RA
#undef TC_ASR
#undef TC_MULT
#undef TC_DIV
