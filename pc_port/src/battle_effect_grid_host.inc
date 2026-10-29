/* Complete native effect-grid renderer, 800A4DB8..800A577C.
 * Fixed compiled control flow; packed guest packets and actual native GTE. */
#define EG_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define EG_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define EG_S(a) ((uint32_t)(int32_t)(int16_t)EG_H(a))
#define EG_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define EG_WH(a,v) battle_entry_write(entry,shard_sdk_address(a),2,(v))
#define EG_WB(a,v) battle_entry_write(entry,shard_sdk_address(a),1,(v))
#define EG_V0 (entry->cpu->gpr[2])
#define EG_V1 (entry->cpu->gpr[3])
#define EG_A0 (entry->cpu->gpr[4])
#define EG_A1 (entry->cpu->gpr[5])
#define EG_A2 (entry->cpu->gpr[6])
#define EG_A3 (entry->cpu->gpr[7])
#define EG_T0 (entry->cpu->gpr[8])
#define EG_T1 (entry->cpu->gpr[9])
#define EG_T2 (entry->cpu->gpr[10])
#define EG_T3 (entry->cpu->gpr[11])
#define EG_T4 (entry->cpu->gpr[12])
#define EG_T5 (entry->cpu->gpr[13])
#define EG_T6 (entry->cpu->gpr[14])
#define EG_T7 (entry->cpu->gpr[15])
#define EG_S0 (entry->cpu->gpr[16])
#define EG_S1 (entry->cpu->gpr[17])
#define EG_S2 (entry->cpu->gpr[18])
#define EG_S3 (entry->cpu->gpr[19])
#define EG_S4 (entry->cpu->gpr[20])
#define EG_S5 (entry->cpu->gpr[21])
#define EG_S6 (entry->cpu->gpr[22])
#define EG_S7 (entry->cpu->gpr[23])
#define EG_T8 (entry->cpu->gpr[24])
#define EG_SP (entry->cpu->gpr[29])
#define EG_FP (entry->cpu->gpr[30])
#define EG_RA (entry->cpu->gpr[31])
#define EG_LO (entry->cpu->lo)
#define EG_HI (entry->cpu->hi)
#define EG_ASR(v,n) ((uint32_t)((int32_t)(v)>>(n)))
#define EG_CALL(t,s) battle_effect_grid_call(entry,(t),(s))
static void battle_effect_grid_mult(BattleEntryCall *entry,uint32_t a,uint32_t b)
{
    int64_t product=(int64_t)(int32_t)a*(int32_t)b;
    entry->cpu->lo=(uint32_t)product;entry->cpu->hi=(uint32_t)((uint64_t)product>>32);
}
#define EG_MULT(a,b) battle_effect_grid_mult(entry,(a),(b))
static void battle_effect_grid_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    EG_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-effect-grid] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"effect grid callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending effect grid callee return",target);
}
/* Same packed tag operation at three fixed sites. Preserve the second OT
 * read after the packet write: packet and OT may alias. */
static void battle_effect_grid_link_a0(BattleEntryCall *entry,uint32_t base,
                                       uint32_t offset,uint32_t low,uint32_t high)
{
    uint32_t p=EG_R(base+offset),q=EG_R(EG_S4);EG_A0=p;
    EG_A0&=high;EG_V0=q;EG_V0&=low;EG_A0|=EG_V0;EG_W(base+offset,EG_A0);
    p=EG_R(EG_S4);EG_V1&=low;EG_V0=p;EG_V0&=high;EG_V0|=EG_V1;EG_W(EG_S4,EG_V0);
}
static void battle_effect_grid_link_v1(BattleEntryCall *entry,uint32_t offset)
{
    uint32_t p=EG_R(EG_S4);EG_V1&=EG_A3;EG_V0=p;EG_V0&=EG_A2;
    EG_V1|=EG_V0;EG_W(EG_A1+offset,EG_V1);
    p=EG_R(EG_S4);EG_A0&=EG_A2;EG_V0=p;EG_V0&=EG_A3;EG_V0|=EG_A0;EG_W(EG_S4,EG_V0);
}
/* Two complete scaled-point calculations differ only in the coefficient
 * halfword at +0E or +0C. All input reads, low products and biases retain
 * their original order, including faults before pending load commits. */
static void battle_effect_grid_scaled_point(BattleEntryCall *entry,uint32_t coefficient)
{
    uint32_t p=EG_S(EG_SP+0x98u),q=EG_S(EG_S3+0x18u);EG_V1=p;EG_V0=q;
    EG_MULT(EG_V1,EG_V0);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xfffu;
    p=EG_S(EG_S3+coefficient);EG_V0=EG_ASR(EG_V0,12);EG_V1=p;
    EG_MULT(EG_V0,EG_V1);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xffu;
    p=EG_H(EG_S0);EG_V0=EG_ASR(EG_V0,8);EG_V1=p;EG_V0+=EG_V1;EG_WH(EG_SP+0x90u,EG_V0);
    p=EG_S(EG_S3+0x16u);q=EG_S(EG_S3+coefficient);EG_V1=p;EG_V0=q;
    EG_MULT(EG_V1,EG_V0);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xffu;
    p=EG_S(EG_SP+0x9cu);EG_V0=EG_ASR(EG_V0,8);EG_V1=p;EG_WH(EG_SP+0x92u,EG_V0);
    EG_V0=EG_S(EG_S3+0x18u);EG_MULT(EG_V1,EG_V0);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xfffu;
    p=EG_S(EG_S3+coefficient);EG_V0=EG_ASR(EG_V0,12);EG_V1=p;
    EG_MULT(EG_V0,EG_V1);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xffu;
    p=EG_H(EG_S0+4u);EG_V0=EG_ASR(EG_V0,8);EG_V1=p;EG_V0+=EG_V1;EG_WH(EG_SP+0x94u,EG_V0);
}
static void battle_effect_grid_run(BattleEntryCall *entry)
{
    EG_SP-=0xe8u;EG_W(EG_SP+0xd0u,EG_S4);
    uint32_t p=EG_R(EG_SP+0xf8u);EG_W(EG_SP+0xdcu,EG_S7);EG_S4=p;
    p=EG_R(EG_SP+0xfcu);EG_W(EG_SP+0xccu,EG_S3);EG_S7=p;
    EG_S3=EG_A0;EG_W(EG_SP+0xd8u,EG_S6);EG_S6=EG_A1;EG_W(EG_SP+0xc0u,EG_S0);EG_S0=EG_A2;
    EG_W(EG_SP+0xe0u,EG_FP);EG_FP=EG_A3;EG_W(EG_SP+0xe4u,EG_RA);
    EG_W(EG_SP+0xd4u,EG_S5);EG_W(EG_SP+0xc8u,EG_S2);int empty=EG_S3==0;EG_W(EG_SP+0xc4u,EG_S1);
    uint32_t q;int take;
    if(empty)goto restore;
    EG_S1=0x00ffffffu;EG_V1=EG_S7<<1;EG_V1+=EG_S7;EG_V1<<=2;EG_A1=EG_V1+EG_S3;
    EG_S2=0xff000000u;EG_V1+=0x3cu;EG_V1+=EG_S3;
    battle_effect_grid_link_a0(entry,EG_A1,0x3cu,EG_S1,EG_S2);
    p=EG_S(EG_S0);q=EG_S(EG_S6);EG_V0=p;EG_A0=EG_SP+0x80u;EG_V1=q;
    EG_W(EG_SP+0x84u,0);EG_V0-=EG_V1;EG_W(EG_SP+0x80u,EG_V0);
    p=EG_S(EG_S0+4u);q=EG_S(EG_S6+4u);EG_V0=p;EG_A1=EG_SP+0x98u;EG_V1=q;EG_V0-=EG_V1;
    EG_RA=0x800a4e80u;EG_W(EG_SP+0x88u,EG_V0);EG_CALL(0x80048d68u,0x800a4e78u);
    p=EG_S(EG_SP+0x98u);q=EG_S(EG_S3+0x18u);EG_V1=p;EG_V0=q;
    EG_MULT(EG_V1,EG_V0);EG_V0=EG_LO;if((int32_t)EG_V0<0)EG_V0+=0xfffu;
    p=EG_H(EG_S0);EG_V0=EG_ASR(EG_V0,12);EG_V1=p;EG_V0+=EG_V1;EG_WH(EG_SP+0x90u,EG_V0);
    p=EG_H(EG_S3+0x16u);q=EG_S(EG_SP+0x9cu);EG_V0=p;EG_WH(EG_SP+0x92u,EG_V0);EG_V1=q;
    EG_V0=EG_S(EG_S3+0x18u);EG_MULT(EG_V1,EG_V0);EG_V0=EG_LO;take=(int32_t)EG_V0>=0;EG_A0=EG_FP;
    if(!take)EG_V0+=0xfffu;
    p=EG_H(EG_S0+4u);EG_V0=EG_ASR(EG_V0,12);EG_V1=p;EG_V0+=EG_V1;
    EG_RA=0x800a4eecu;EG_WH(EG_SP+0x94u,EG_V0);EG_CALL(0x80049efcu,0x800a4ee4u);
    EG_RA=0x800a4ef4u;EG_A0=EG_FP;EG_CALL(0x80049f8cu,0x800a4eecu);
    EG_A0=EG_SP+0x90u;MTC2(EG_R(EG_A0),0);MTC2(EG_R(EG_A0+4u),1);doCOP2(0x4a180001);
    EG_V0=EG_SP+0x70u;EG_W(EG_V0,MFC2(14));EG_V0=EG_H(EG_SP+0x72u);EG_WH(EG_SP+0x70u,EG_V0);
    EG_V0<<=16;EG_V0=EG_ASR(EG_V0,16);EG_V0=(int32_t)EG_V0<241;take=EG_V0!=0;EG_V0=240;
    if(!take)EG_WH(EG_SP+0x72u,EG_V0);
    battle_effect_grid_scaled_point(entry,0xeu);
    MTC2(EG_R(EG_A0),0);MTC2(EG_R(EG_A0+4u),1);doCOP2(0x4a180001);
    EG_V0=EG_SP+0x78u;EG_W(EG_V0,MFC2(14));EG_V0=EG_S(EG_SP+0x7au);int below=(int32_t)EG_V0<0;
    if(!below){
        EG_V0=EG_S(EG_SP+0x72u);EG_A0=EG_V0;EG_V0=(int32_t)EG_V0<240;take=EG_V0!=0;EG_V1=EG_S7+2u;
        if(take){
            EG_V0=EG_V1<<3;EG_V0+=EG_V1;EG_V0<<=2;EG_V0+=EG_S3;EG_WH(EG_V0+0x1746u,EG_A0);
            EG_V1=EG_H(EG_SP+0x72u);EG_WH(EG_V0+0x174eu,EG_V1);
            EG_V1=EG_H(EG_SP+0x7au);EG_WH(EG_V0+0x1756u,EG_V1);
            EG_V1=EG_H(EG_SP+0x7au);EG_WH(EG_V0+0x175eu,EG_V1);
            EG_V1=EG_S7<<3;EG_V1+=EG_S7;EG_V1<<=2;EG_A1=EG_V1+EG_S3;EG_V1+=0x1784u;EG_V1+=EG_S3;
            battle_effect_grid_link_a0(entry,EG_A1,0x1784u,EG_S1,EG_S2);
        }
        EG_V0=EG_S(EG_SP+0x7au);below=(int32_t)EG_V0<0;EG_A0=EG_V0;
    }
    if(below){EG_WH(EG_SP+0x7au,0);EG_V0=EG_S(EG_SP+0x7au);EG_A0=EG_V0;}
    EG_V0=(int32_t)EG_V0<240;take=EG_V0!=0;EG_A3=0x00ff0000u;
    if(take){
        EG_V1=EG_S7+2u;EG_V0=EG_V1<<1;EG_V0+=EG_V1;EG_V0<<=3;EG_V0+=EG_S3;EG_A3|=0xffffu;
        EG_WH(EG_V0+0x16e6u,EG_A0);p=EG_H(EG_SP+0x7au);EG_A2=0xff000000u;EG_V1=p;EG_WH(EG_V0+0x16eau,EG_V1);
        EG_V1=EG_S7<<1;EG_V1+=EG_S7;EG_V1<<=3;EG_A1=EG_V1+EG_S3;EG_V1+=0x170cu;EG_V1+=EG_S3;
        battle_effect_grid_link_a0(entry,EG_A1,0x170cu,EG_A3,EG_A2);
    }
    battle_effect_grid_scaled_point(entry,0xcu);
    EG_V0=EG_SP+0x90u;MTC2(EG_R(EG_V0),0);MTC2(EG_R(EG_V0+4u),1);doCOP2(0x4a180001);
    EG_V0=EG_SP+0x78u;EG_W(EG_V0,MFC2(14));p=EG_S(EG_SP+0x72u);q=EG_H(EG_SP+0x7au);EG_V0=p;
    p=EG_H(EG_SP+0x70u);EG_V1=q;EG_V0<<=1;EG_A2=p;EG_V0-=EG_V1;EG_V1=EG_V0-8u;
    EG_V0=EG_A2<480u;take=EG_V0!=0;EG_WH(EG_SP+0x7au,EG_V1);
    if(take){
        EG_V0=EG_V1<<16;EG_V0=EG_ASR(EG_V0,16);EG_V0=(int32_t)EG_V0<240;take=EG_V0!=0;EG_A3=0xff000000u;
        if(take){
            EG_A0=EG_S7<<3;EG_A0+=EG_S7;EG_A0<<=2;EG_A1=EG_S3+EG_A0;EG_WH(EG_A1+0x1746u,EG_A2);
            EG_A2=0x00ffffffu;p=EG_H(EG_SP+0x70u);q=EG_R(EG_A1+0x173cu);EG_V0=p;
            EG_WH(EG_A1+0x174eu,EG_V0);EG_V1=q;p=EG_H(EG_SP+0x7au);EG_A0+=0x173cu;EG_V0=p;
            EG_WH(EG_A1+0x1756u,EG_V0);p=EG_H(EG_SP+0x7au);EG_A0+=EG_S3;EG_V0=p;EG_WH(EG_A1+0x175eu,EG_V0);
            battle_effect_grid_link_v1(entry,0x173cu);
        }
    }
    p=EG_S(EG_S0);q=EG_S(EG_S6);EG_V0=p;EG_V1=q;EG_V0-=EG_V1;EG_MULT(EG_V0,EG_V0);
    EG_W(EG_SP+0x64u,0);EG_W(EG_SP+0x60u,EG_V0);
    p=EG_S(EG_S0+4u);q=EG_S(EG_S6+4u);EG_V0=p;EG_A1=EG_LO;EG_V1=q;EG_V0-=EG_V1;
    EG_MULT(EG_V0,EG_V0);EG_W(EG_SP+0x68u,EG_V0);EG_A0=EG_LO;
    EG_RA=0x800a5314u;EG_A0+=EG_A1;EG_CALL(0x80048c4cu,0x800a530cu);
    p=EG_S(EG_S0+2u);q=EG_S(EG_S6+2u);EG_V1=p;EG_A1=EG_V0;EG_A0=q;
    EG_RA=0x800a5328u;EG_A0=EG_V1-EG_A0;EG_CALL(0x8004b32cu,0x800a5320u);
    EG_A1=EG_V0;p=EG_S(EG_S3+0x12u);EG_V0=EG_A1-0x100u;EG_V1=p;
    EG_MULT(EG_V0,EG_V1);EG_A0=EG_LO;take=(int32_t)EG_A0>=0;EG_V0=0x400u;
    if(!take)EG_A0+=0x1ffu;
    EG_A0=EG_ASR(EG_A0,9);take=(int32_t)EG_A1>=0;EG_V1=EG_A1;if(!take)EG_V1=0u-EG_V1;
    EG_V0-=EG_V1;EG_MULT(EG_A0,EG_V0);EG_S1=EG_LO;if((int32_t)EG_S1<0)EG_S1+=0x3ffu;
    EG_A0=EG_S(EG_S3+0x10u);EG_RA=0x800a537cu;EG_S1=EG_ASR(EG_S1,10);EG_CALL(0x8004a14cu,0x800a5374u);
    p=EG_R(EG_SP+0x60u);q=EG_R(EG_SP+0x68u);EG_A0=p;EG_RA=0x800a538cu;EG_A1=q;
    EG_WH(EG_SP+0x58u,0);EG_CALL(0x8004b32cu,0x800a5384u);
    EG_S0=EG_SP+0x58u;EG_A0=EG_S0;EG_S5=EG_SP+0x38u;EG_A1=EG_S5;
    EG_V0=0u-EG_V0;EG_WH(EG_SP+0x5au,EG_V0);EG_RA=0x800a53acu;EG_WH(EG_SP+0x5cu,0);EG_CALL(0x8003f738u,0x800a53a4u);
    EG_A0=EG_S0;EG_S2=EG_SP+0x18u;p=EG_H(EG_SP+0x5au);EG_A1=EG_S2;EG_V0=p;
    EG_W(EG_SP+0x4cu,0);EG_W(EG_SP+0x50u,0);EG_W(EG_SP+0x54u,0);EG_WH(EG_SP+0x58u,EG_S1);
    EG_V0=0u-EG_V0;EG_RA=0x800a53d8u;EG_WH(EG_SP+0x5au,EG_V0);EG_CALL(0x8004a92cu,0x800a53d0u);
    EG_A0=EG_SP+0x60u;EG_RA=0x800a53e4u;EG_A1=EG_S0;EG_CALL(0x80048d68u,0x800a53dcu);
    p=EG_S(EG_SP+0x58u);q=EG_S(EG_S6);EG_V0=p;EG_V0<<=1;EG_V1=q;EG_T0=EG_V1+EG_V0;EG_W(EG_SP+0x60u,EG_T0);
    EG_V1=EG_S(EG_S6+2u);take=(int32_t)EG_V1>=0;EG_A0=EG_S2;if(!take)EG_V1+=3u;
    EG_A1=EG_S5;p=EG_S(EG_S3+0xau);EG_V1=EG_ASR(EG_V1,2);EG_V0=p;EG_V1-=EG_V0;EG_W(EG_SP+0x64u,EG_V1);
    p=EG_S(EG_S6+4u);q=EG_S(EG_SP+0x5cu);EG_A3=p;EG_A2=EG_S2;EG_V0=q;
    EG_W(EG_SP+0x2cu,EG_T0);EG_W(EG_SP+0x30u,EG_V1);EG_V0<<=1;EG_A3+=EG_V0;EG_W(EG_SP+0x68u,EG_A3);
    EG_RA=0x800a5448u;EG_W(EG_SP+0x34u,EG_A3);EG_CALL(0x8004931cu,0x800a5440u);
    EG_A0=EG_FP;EG_A1=EG_S2;EG_RA=0x800a5458u;EG_A2=EG_S2;EG_CALL(0x8004931cu,0x800a5450u);
    EG_RA=0x800a5460u;EG_A0=EG_S2;EG_CALL(0x80049efcu,0x800a5458u);
    EG_RA=0x800a5468u;EG_A0=EG_S2;EG_CALL(0x80049f8cu,0x800a5460u);
    p=EG_H(EG_S3);q=EG_H(EG_S3+4u);EG_V0=p;EG_V1=q;EG_V0+=EG_V1;
    p=EG_H(EG_S3+2u);EG_WH(EG_S3,EG_V0);EG_V1=p;
    p=EG_H(EG_S3+6u);q=EG_S(EG_S3);EG_V0=p;EG_V0=EG_V1+EG_V0;EG_A0=q;
    take=(int32_t)EG_A0>=0;EG_WH(EG_S3+2u,EG_V0);if(!take)EG_A0+=0xfu;
    EG_A2=0x2aaa0000u;p=EG_R(EG_SP+0x60u);EG_A2|=0xaaabu;EG_V1=p;EG_MULT(EG_V1,EG_A2);
    EG_V0<<=16;EG_A1=EG_ASR(EG_V0,16);EG_A0=EG_ASR(EG_A0,4);EG_V1=EG_ASR(EG_V1,31);
    EG_V0=EG_HI;EG_V0=EG_ASR(EG_V0,1);EG_V0-=EG_V1;
    p=EG_H(EG_S3+8u);EG_A0+=EG_V0;EG_V1=p;EG_T1=EG_V1-1u;
    take=(int32_t)EG_A1>=0;EG_T0=EG_A0&EG_T1;if(!take)EG_A1+=0xfu;
    p=EG_R(EG_SP+0x68u);EG_T6=EG_S7<<6;EG_A0=p;EG_A3=EG_S3+0x54u;EG_MULT(EG_A0,EG_A2);
    EG_V0=EG_V1<<16;EG_V1=EG_ASR(EG_V0,16);EG_V0>>=31;EG_V1+=EG_V0;EG_V1=EG_ASR(EG_V1,1);
    EG_S6=EG_V1;EG_T7=0;EG_S2=EG_V1;EG_T8=EG_T0;EG_V1=EG_ASR(EG_A1,4);EG_A0=EG_ASR(EG_A0,31);
    EG_V0=EG_HI;EG_V0=EG_ASR(EG_V0,1);EG_V0-=EG_A0;EG_V1+=EG_V0;EG_S5=EG_V1&EG_T1;
    do{
        EG_V0=EG_T7&1u;EG_MULT(EG_V0,EG_S2);EG_T0=0;EG_T4=0x00ffffffu;EG_S1=0xff000000u;
        EG_V0=EG_T6<<2;EG_V0+=EG_T6;EG_V0<<=3;EG_A2=EG_V0+EG_S3;EG_T1=EG_V0+0x2dcu;
        EG_S0=EG_S6;EG_V0=EG_LO;EG_T3=EG_V0+EG_S5;EG_V0=EG_T3+EG_S0;EG_T5=EG_V0-1u;
        do{
            EG_T2=EG_A3+8u;EG_V0=EG_A3+0x48u;
            MTC2(EG_R(EG_A3),0);MTC2(EG_R(EG_A3+4u),1);MTC2(EG_R(EG_T2),2);MTC2(EG_R(EG_T2+4u),3);
            MTC2(EG_R(EG_V0),4);MTC2(EG_R(EG_V0+4u),5);doCOP2(0x4a280030);doCOP2(0x4b400006);
            EG_V0=EG_SP+0xa0u;EG_W(EG_V0,MFC2(24));EG_V0=EG_R(EG_SP+0xa0u);
            take=(int32_t)EG_V0<0;EG_T6++;
            if(!take){
                EG_A1=EG_S3+EG_T1;EG_A0=EG_A1+8u;EG_V1=EG_A1+0x10u;EG_V0=EG_A1+0x18u;
                EG_W(EG_A0,MFC2(12));EG_W(EG_V1,MFC2(13));EG_W(EG_V0,MFC2(14));
                EG_V0=EG_A3+0x50u;MTC2(EG_R(EG_V0),0);MTC2(EG_R(EG_V0+4u),1);doCOP2(0x4a180001);
                EG_V0=EG_A1+0x20u;EG_W(EG_V0,MFC2(14));EG_V0=EG_T0&1u;EG_MULT(EG_V0,EG_S2);
                EG_WB(EG_A2+0x2e9u,EG_T3);EG_WB(EG_A2+0x2f1u,EG_T3);EG_WB(EG_A2+0x2f9u,EG_T5);EG_WB(EG_A2+0x301u,EG_T5);
                EG_V0=EG_LO;EG_V0+=EG_T8;EG_V1=EG_V0+EG_S0;EG_WB(EG_A2+0x2e8u,EG_V0);EG_WB(EG_A2+0x2f8u,EG_V0);
                p=EG_R(EG_A2+0x2dcu);EG_V1--;EG_V0=p;EG_WB(EG_A2+0x2f0u,EG_V1);EG_WB(EG_A2+0x300u,EG_V1);
                p=EG_R(EG_S4);EG_V0&=EG_S1;EG_V1=p;EG_V1&=EG_T4;EG_V0|=EG_V1;EG_W(EG_A2+0x2dcu,EG_V0);
                p=EG_R(EG_S4);EG_A1&=EG_T4;EG_V0=p;EG_V0&=EG_S1;EG_V0|=EG_A1;EG_W(EG_S4,EG_V0);
            }
            EG_T0++;EG_A2+=0x28u;EG_T1+=0x28u;EG_V0=(int32_t)EG_T0<8;take=EG_V0!=0;EG_A3=EG_T2;
        }while(take);
        EG_T7++;EG_V0=(int32_t)EG_T7<8;take=EG_V0!=0;EG_A3+=8u;
    }while(take);
    EG_RA=0x800a568cu;EG_A0=512;EG_CALL(0x8004a14cu,0x800a5684u);
    EG_V0=EG_S(EG_SP+0x72u);take=(int32_t)EG_V0<0;EG_V1=EG_V0;
    if(!take){
        EG_A2=0x00ff0000u;EG_A0=EG_S7<<1;EG_A0+=EG_S7;EG_A0<<=3;EG_A1=EG_S3+EG_A0;
        EG_A2|=0xffffu;EG_A3=0xff000000u;EG_A0+=0x16dcu;EG_WH(EG_A1+0x16eeu,EG_V1);
        p=EG_H(EG_SP+0x72u);q=EG_R(EG_A1+0x16dcu);EG_V0=p;EG_A0+=EG_S3;EG_V1=q;EG_WH(EG_A1+0x16f2u,EG_V0);
        battle_effect_grid_link_v1(entry,0x16dcu);
    }
    EG_A1=0x00ffffffu;EG_V1=EG_S7<<1;EG_V1+=EG_S7;EG_V1<<=2;EG_A2=EG_V1+EG_S3;
    EG_A3=0xff000000u;EG_V1+=0x24u;EG_V1+=EG_S3;
    battle_effect_grid_link_a0(entry,EG_A2,0x24u,EG_A1,EG_A3);
restore:
    p=EG_R(EG_SP+0xe4u);q=EG_R(EG_SP+0xe0u);EG_RA=p;
    p=EG_R(EG_SP+0xdcu);EG_FP=q;q=EG_R(EG_SP+0xd8u);EG_S7=p;
    p=EG_R(EG_SP+0xd4u);EG_S6=q;q=EG_R(EG_SP+0xd0u);EG_S5=p;
    p=EG_R(EG_SP+0xccu);EG_S4=q;q=EG_R(EG_SP+0xc8u);EG_S3=p;
    p=EG_R(EG_SP+0xc4u);EG_S2=q;q=EG_R(EG_SP+0xc0u);EG_S1=p;EG_SP+=0xe8u;EG_S0=q;
}
static int battle_effect_grid_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a4db8u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_GRID");if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="93aa82abdf0041d3295926bc2b94cd7235baca996c97c9da8115561eef02593a";
    uint8_t hash[32];file1_digest(PSX_ADDR(target),2500u,hash);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[i*2]||file1_hex_digit(hash[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_effect_grid_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef EG_R
#undef EG_H
#undef EG_S
#undef EG_W
#undef EG_WH
#undef EG_WB
#undef EG_V0
#undef EG_V1
#undef EG_A0
#undef EG_A1
#undef EG_A2
#undef EG_A3
#undef EG_T0
#undef EG_T1
#undef EG_T2
#undef EG_T3
#undef EG_T4
#undef EG_T5
#undef EG_T6
#undef EG_T7
#undef EG_S0
#undef EG_S1
#undef EG_S2
#undef EG_S3
#undef EG_S4
#undef EG_S5
#undef EG_S6
#undef EG_S7
#undef EG_T8
#undef EG_SP
#undef EG_FP
#undef EG_RA
#undef EG_LO
#undef EG_HI
#undef EG_ASR
#undef EG_CALL
#undef EG_MULT
