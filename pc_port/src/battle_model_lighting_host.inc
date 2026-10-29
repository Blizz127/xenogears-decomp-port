/* Complete face-normal and color-lighting bodies. Fixed GTE commands use the
 * native backend; no original CPU instruction is fetched or decoded here.
 * OuterProduct0/SquareRoot0 adoption is scoped to this face-normal caller. */
#define ML_R(a) battle_entry_read(entry,(a),4)
#define ML_H(a) ((uint32_t)(int32_t)(int16_t)battle_entry_read(entry,(a),2))
#define ML_W(a,v) battle_entry_write(entry,(a),4,(v))
#define ML_AT (entry->cpu->gpr[1])
#define ML_V0 (entry->cpu->gpr[2])
#define ML_V1 (entry->cpu->gpr[3])
#define ML_A0 (entry->cpu->gpr[4])
#define ML_A1 (entry->cpu->gpr[5])
#define ML_A2 (entry->cpu->gpr[6])
#define ML_A3 (entry->cpu->gpr[7])
#define ML_T0 (entry->cpu->gpr[8])
#define ML_T1 (entry->cpu->gpr[9])
#define ML_T2 (entry->cpu->gpr[10])
#define ML_T3 (entry->cpu->gpr[11])
#define ML_T4 (entry->cpu->gpr[12])
#define ML_T5 (entry->cpu->gpr[13])
#define ML_T6 (entry->cpu->gpr[14])
#define ML_T7 (entry->cpu->gpr[15])
#define ML_S0 (entry->cpu->gpr[16])
#define ML_S1 (entry->cpu->gpr[17])
#define ML_SP (entry->cpu->gpr[29])
#define ML_RA (entry->cpu->gpr[31])

static void battle_model_lighting_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    ML_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-model-lighting] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"model lighting callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending model lighting callee return",target);
}
static void battle_model_lighting_divide(BattleEntryCall *entry,uint32_t numerator,uint32_t divisor)
{
    int64_t n=(int32_t)numerator,d=(int32_t)divisor;
    if(!d){entry->cpu->lo=n<0?1u:UINT32_MAX;entry->cpu->hi=numerator;}
    else {entry->cpu->lo=(uint32_t)(n/d);entry->cpu->hi=(uint32_t)(n%d);}
}
static void battle_model_face_normal_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    ML_SP-=0x50u;ML_T0=ML_A0;
    ML_W(ML_SP+0x48u,ML_RA);ML_W(ML_SP+0x44u,ML_S1);ML_W(ML_SP+0x40u,ML_S0);
    p=ML_H(ML_A1);q=ML_H(ML_T0);ML_V0=p;ML_V1=q;
    ML_V0-=ML_V1;ML_W(ML_SP+0x10u,ML_V0);
    p=ML_H(ML_A1+2u);q=ML_H(ML_T0+2u);ML_V0=p;ML_S1=ML_A3;ML_V1=q;
    ML_V0-=ML_V1;ML_W(ML_SP+0x14u,ML_V0);
    p=ML_H(ML_A1+4u);q=ML_H(ML_T0+4u);ML_V0=p;ML_A0=ML_SP+0x20u;ML_V1=q;
    ML_V0-=ML_V1;ML_W(ML_SP+0x18u,ML_V0);
    p=ML_H(ML_A2);q=ML_H(ML_T0);ML_V0=p;ML_S0=ML_SP+0x30u;ML_V1=q;
    ML_V0-=ML_V1;ML_W(ML_SP+0x20u,ML_V0);
    p=ML_H(ML_A2+2u);q=ML_H(ML_T0+2u);ML_V0=p;ML_A1=ML_SP+0x10u;ML_V1=q;
    ML_V0-=ML_V1;ML_W(ML_SP+0x24u,ML_V0);
    p=ML_H(ML_A2+4u);q=ML_H(ML_T0+4u);ML_V0=p;ML_A2=ML_S0;ML_V1=q;
    ML_V0-=ML_V1;ML_RA=0x8002dc14u;ML_W(ML_SP+0x28u,ML_V0);
    battle_model_lighting_call(entry,0x8004a4d8u,0x8002dc0cu);
    p=ML_R(ML_SP+0x30u);q=ML_R(ML_SP+0x34u);ML_A0=p;
    p=ML_R(ML_SP+0x38u);ML_A1=q;ML_RA=0x8002dc28u;ML_A2=p;
    battle_model_lighting_call(entry,0x8002dc9cu,0x8002dc20u);
    if((int32_t)ML_V0<0)ML_V0=0u-ML_V0;
    ML_RA=0x8002dc3cu;ML_A0=ML_V0;
    battle_model_lighting_call(entry,0x80048c4cu,0x8002dc34u);
    ML_A3=ML_R(ML_SP+0x30u);battle_model_lighting_divide(entry,ML_A3,ML_V0);ML_A3=entry->cpu->lo;
    ML_A2=ML_R(ML_SP+0x34u);battle_model_lighting_divide(entry,ML_A2,ML_V0);ML_A2=entry->cpu->lo;
    ML_V1=ML_R(ML_SP+0x38u);battle_model_lighting_divide(entry,ML_V1,ML_V0);ML_V1=entry->cpu->lo;
    ML_A0=ML_S0;ML_A1=ML_S1;ML_W(ML_SP+0x30u,ML_A3);ML_W(ML_SP+0x34u,ML_A2);
    ML_RA=0x8002dc84u;ML_W(ML_SP+0x38u,ML_V1);
    battle_model_lighting_call(entry,0x80048d68u,0x8002dc7cu);
    p=ML_R(ML_SP+0x48u);q=ML_R(ML_SP+0x44u);ML_RA=p;
    p=ML_R(ML_SP+0x40u);ML_S1=q;ML_SP+=0x50u;ML_S0=p;
}
static void battle_model_dominant_run(BattleEntryCall *entry)
{
    ML_V1=ML_A0;ML_A3=ML_A1;ML_T0=ML_A2;
    if((int32_t)ML_V1<0)ML_V1=0u-ML_V1;
    if((int32_t)ML_A3<0)ML_A3=0u-ML_A3;
    if((int32_t)ML_A2<0)ML_T0=0u-ML_A2;
    /* Signed comparisons of wrapped magnitudes: INT_MIN remains negative.
     * The total order makes the assembly's residual return-one path unreachable. */
    if((int32_t)ML_V1>=(int32_t)ML_A3&&(int32_t)ML_V1>=(int32_t)ML_T0)ML_V0=ML_A0;
    else if((int32_t)ML_A3>=(int32_t)ML_V1&&(int32_t)ML_A3>=(int32_t)ML_T0)ML_V0=ML_A1;
    else ML_V0=ML_A2;
}
static void battle_model_outer_product_run(BattleEntryCall *entry)
{
    uint32_t p=CFC2(0),q=CFC2(2);ML_T5=p;p=CFC2(4);ML_T6=q;
    q=ML_R(ML_A0);ML_T7=p;p=ML_R(ML_A0+4u);ML_T0=q;
    q=ML_R(ML_A0+8u);ML_T1=p;CTC2(ML_T0,0);ML_T2=q;
    CTC2(ML_T1,2);CTC2(ML_T2,4);
    MTC2(ML_R(ML_A1+8u),11);MTC2(ML_R(ML_A1),9);MTC2(ML_R(ML_A1+4u),10);
    doCOP2(0x4b70000c); /* OP(0): diagonal/IR operands use signed low16. */
    ML_W(ML_A2,MFC2(25));ML_W(ML_A2+4u,MFC2(26));ML_W(ML_A2+8u,MFC2(27));
    CTC2(ML_T5,0);CTC2(ML_T6,2);CTC2(ML_T7,4);
}
static uint32_t battle_model_lighting_checked(BattleEntryCall *entry,int64_t v,uint32_t site)
{
    if(v<INT32_MIN||v>INT32_MAX)battle_entry_fail(entry,"model lighting signed overflow",site);
    return (uint32_t)v;
}
static void battle_model_square_root_run(BattleEntryCall *entry)
{
    MTC2(ML_A0,30);uint32_t p=MFC2(31);ML_AT=32;ML_V0=p;
    if(ML_V0==ML_AT){ML_V0=0;return;}
    ML_T0=ML_V0&1u;ML_T2=ML_V0&0xfffffffeu;ML_T1=31;
    ML_T1=battle_model_lighting_checked(entry,(int64_t)(int32_t)ML_T1-(int32_t)ML_T2,0x80048c78u);
    ML_T1=(uint32_t)((int32_t)ML_T1>>1);
    ML_T3=battle_model_lighting_checked(entry,(int64_t)(int32_t)ML_T2-24,0x80048c80u);
    if((int32_t)ML_T3>=0){ML_T4=ML_A0<<(ML_T3&31u);ML_T3=24;}
    else{
        ML_T3=24;ML_T3=battle_model_lighting_checked(entry,(int64_t)(int32_t)ML_T3-(int32_t)ML_T2,0x80048c98u);
        ML_T4=(uint32_t)((int32_t)ML_A0>>(ML_T3&31u));
    }
    ML_T4=battle_model_lighting_checked(entry,(int64_t)(int32_t)ML_T4-64,0x80048ca0u);
    ML_T4<<=1;ML_T5=0x80050000u;ML_T5+=ML_T4;
    ML_T5=ML_H(ML_T5+0x6a00u);ML_T5<<=(ML_T1&31u);ML_V0=ML_T5>>12;
}
static void battle_model_color_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t p,q;
    MTC2(ML_R(ML_A0),0);MTC2(ML_R(ML_A0+4u),1);
    if(target==0x8004a19cu){
        doCOP2(0x4ac8041e);ML_W(ML_A1,MFC2(22));return;
    }
    if(target==0x8004a260u){
        MTC2(ML_R(ML_A1),6);doCOP2(0x4b08041b);ML_W(ML_A2,MFC2(22));return;
    }
    MTC2(ML_R(ML_A1),2);MTC2(ML_R(ML_A1+4u),3);
    MTC2(ML_R(ML_A2),4);MTC2(ML_R(ML_A2+4u),5);
    if(target==0x8004a1b8u){
        doCOP2(0x4ad80420);
        p=ML_R(ML_SP+0x10u);q=ML_R(ML_SP+0x14u);ML_T0=p;
        ML_W(ML_A3,MFC2(20));ML_T1=q;
        ML_W(ML_T0,MFC2(21));ML_W(ML_T1,MFC2(22));return;
    }
    MTC2(ML_R(ML_A3),6);doCOP2(0x4b18043f);
    p=ML_R(ML_SP+0x10u);q=ML_R(ML_SP+0x14u);ML_T0=p;
    p=ML_R(ML_SP+0x18u);ML_T1=q;
    ML_W(ML_T0,MFC2(20));ML_T2=p;
    ML_W(ML_T1,MFC2(21));ML_W(ML_T2,MFC2(22));
}
static int battle_model_lighting_target(uint32_t target,uint32_t caller)
{
    switch(target){
    case 0x8002db84u:case 0x8002dc9cu:case 0x8004a19cu:case 0x8004a1b8u:
    case 0x8004a260u:case 0x8004a280u:return 1;
    case 0x8004a4d8u:return caller==0x8002dc14u;
    case 0x80048c4cu:return caller==0x8002dc3cu;
    default:return 0;
    }
}
static int battle_model_lighting_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_model_lighting_target(target,cpu->gpr[31])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_LIGHTING");
    if(control&&strcmp(control,"0")==0)return 0;
    static const struct {uint32_t target,size;const char *hash;} spans[]={
        {0x8002db84u,280,"bde5a633327e7da1e571bf9a67662fac025bf79f44ee58bcaa280400f0769481"},
        {0x8002dc9cu,132,"df730f3e645ac59c4236740cc3e26d681a5d04b4f23c81f570f14ac6e41231ca"},
        {0x8004a4d8u,88,"b0df3db5528e30315b774c82d909799911f21a661fc9d23bf7ce2dbb33ef3f53"},
        {0x80048c4cu,132,"fda79f78e9d9d817a0a8e1612f3a55e14e3783c220b4d6e26bff8fdaf32ed391"},
        {0x8004a19cu,28,"51a99181363b8badf91ed61044388e61a4b9a8bfcce30664da1e27c0a4173ce3"},
        {0x8004a1b8u,60,"e7e7d9b5915dcc3c8628f660347c1c23fbbefb3f92cdae8a2088e7bfae998462"},
        {0x8004a260u,32,"3c1aabf3316d9ba80fcc1c1f1a830b2a722af41e056051e5a5630c69650e9d19"},
        {0x8004a280u,68,"f44adfbab7eab389a99255c38be00c699268a786d109538552a6116799ed1fc9"}
    };
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);if(!image)return 0;
    unsigned n=0;while(n<sizeof(spans)/sizeof(spans[0])&&spans[n].target!=target)n++;
    if(n==sizeof(spans)/sizeof(spans[0]))return 0;
    size_t offset=XENO_RD_SLUS_OFF(target);if(image->size<offset+spans[n].size)return 0;
    uint8_t hash[32];file1_digest(image->bytes+offset,spans[n].size,hash);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=spans[n].hash[i*2]||file1_hex_digit(hash[i]&15u)!=spans[n].hash[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8002db84u)battle_model_face_normal_run(&entry);
    else if(target==0x8002dc9cu)battle_model_dominant_run(&entry);
    else if(target==0x8004a4d8u)battle_model_outer_product_run(&entry);
    else if(target==0x80048c4cu)battle_model_square_root_run(&entry);
    else battle_model_color_run(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef ML_R
#undef ML_H
#undef ML_W
#undef ML_AT
#undef ML_V0
#undef ML_V1
#undef ML_A0
#undef ML_A1
#undef ML_A2
#undef ML_A3
#undef ML_T0
#undef ML_T1
#undef ML_T2
#undef ML_T3
#undef ML_T4
#undef ML_T5
#undef ML_T6
#undef ML_T7
#undef ML_S0
#undef ML_S1
#undef ML_SP
#undef ML_RA
