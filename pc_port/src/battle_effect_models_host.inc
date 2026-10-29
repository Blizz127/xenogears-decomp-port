/* Complete effect model-list caller. Entries, matrix storage and incoming
 * arguments remain guest words; count zero retains unsigned underflow. */
#define EM_R32(a) battle_entry_read(entry,(a),4)
#define EM_R16(a) battle_entry_read(entry,(a),2)
#define EM_R8(a) battle_entry_read(entry,(a),1)
#define EM_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define EM_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define EM_AT (entry->cpu->gpr[1])
#define EM_V0 (entry->cpu->gpr[2])
#define EM_V1 (entry->cpu->gpr[3])
#define EM_A0 (entry->cpu->gpr[4])
#define EM_A1 (entry->cpu->gpr[5])
#define EM_A2 (entry->cpu->gpr[6])
#define EM_A3 (entry->cpu->gpr[7])
#define EM_T0 (entry->cpu->gpr[8])
#define EM_S0 (entry->cpu->gpr[16])
#define EM_S1 (entry->cpu->gpr[17])
#define EM_S2 (entry->cpu->gpr[18])
#define EM_S3 (entry->cpu->gpr[19])
#define EM_S4 (entry->cpu->gpr[20])
#define EM_S5 (entry->cpu->gpr[21])
#define EM_S6 (entry->cpu->gpr[22])
#define EM_S7 (entry->cpu->gpr[23])
#define EM_SP (entry->cpu->gpr[29])
#define EM_FP (entry->cpu->gpr[30])
#define EM_RA (entry->cpu->gpr[31])
#define EM_CALL(t,s) battle_effect_models_call(entry,(t),(s))

static void battle_effect_models_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    EM_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-effect-models] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"effect models callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending effect models callee return",target);
}
static void battle_effect_models_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    EM_SP-=0x40u;EM_W32(EM_SP+0x2cu,EM_S5);EM_S5=EM_A0;
    EM_W32(EM_SP+0x20u,EM_S2);EM_S2=EM_A1;EM_W32(EM_SP+0x24u,EM_S3);EM_S3=EM_A2;
    EM_W32(EM_SP+0x18u,EM_S0);EM_S0=0x1f800000u;EM_S0|=0x40u;
    EM_W32(EM_SP+0x3cu,EM_RA);EM_W32(EM_SP+0x38u,EM_FP);EM_W32(EM_SP+0x34u,EM_S7);
    EM_W32(EM_SP+0x30u,EM_S6);EM_W32(EM_SP+0x28u,EM_S4);EM_W32(EM_SP+0x1cu,EM_S1);
    p=EM_R16(EM_S2+0xau);EM_S2+=0x7cu;EM_V0=p;EM_S6=0x80050000u;
    p=EM_R32(EM_S6+0x100u);EM_S7=EM_V0-1u;EM_S6=p;
    int again=EM_S7!=0;EM_S4=0;
    if(again){
        EM_FP=0x1000u;EM_S1=EM_S2+8u;
        do{
            p=EM_R16(EM_S1);EM_T0=0xffffu;EM_V0=p;
            if(EM_V0!=EM_T0){
                EM_V0=EM_R8(EM_S1-1u);int active=EM_V0!=0;EM_A0=EM_S3;
                if(active){
                    EM_A1=EM_S2+0x2cu;EM_A2=EM_S0;EM_CALL(0x8004931cu,0x800a4974u);
                    p=EM_R16(EM_S1+0x4au);EM_T0=0xffffu;EM_V0=p;
                    EM_V0=(EM_V0+EM_T0)&0xffffu;EM_V0=EM_V0<2u;
                    int billboard=EM_V0!=0;EM_V0=1;
                    if(billboard){
                        EM_W16(EM_S0,EM_FP);EM_W16(EM_S0+4u,0);EM_W16(EM_S0+6u,0);
                        EM_W16(EM_S0+0xau,0);EM_W16(EM_S0+0xcu,0);EM_W16(EM_S0+0x10u,EM_FP);
                        EM_V1=(uint32_t)(int32_t)(int16_t)EM_R16(EM_S1+0x4au);
                        if(EM_V1==EM_V0){
                            EM_V0=EM_R16(EM_S3+2u);EM_W16(EM_S0+2u,EM_V0);
                            EM_V0=EM_R16(EM_S3+8u);EM_W16(EM_S0+8u,EM_V0);
                            EM_V0=EM_R16(EM_S3+0xeu);EM_W16(EM_S0+0xeu,EM_V0);
                        }else{EM_W16(EM_S0+2u,0);EM_W16(EM_S0+8u,EM_FP);EM_W16(EM_S0+0xeu,0);}
                    }
                    EM_A0=EM_S0;EM_CALL(0x80049efcu,0x800a49f0u);EM_A0=EM_S0;EM_CALL(0x80049f8cu,0x800a49f8u);
                    p=(uint32_t)(int32_t)(int16_t)EM_R16(EM_S1+0x4au);EM_V0=5;EM_V1=p;
                    int five=EM_V1==EM_V0;EM_V0=(int32_t)EM_V1<6;
                    if(five)EM_A3=3;
                    else{
                        int low=EM_V0!=0;EM_V0=4;
                        if(low){int four=EM_V1==EM_V0;EM_A3=2;if(!four)EM_A3=0;}
                        else{
                            EM_V0=6;int six=EM_V1==EM_V0;EM_V0=7;
                            if(six)EM_A3=4;
                            else{int seven=EM_V1==EM_V0;EM_A3=5;if(!seven)EM_A3=0;}
                        }
                    }
                    EM_V0=(uint32_t)(int32_t)(int16_t)EM_R16(EM_S1+0x4au);
                    if(!EM_V0){
                        EM_T0=EM_R32(EM_SP+0x5cu);EM_A2=EM_T0<<2;
                        EM_T0=EM_R32(EM_SP+0x54u);EM_A2=EM_T0+EM_A2;
                        p=EM_R32(EM_SP+0x58u);q=EM_R32(EM_S5);EM_T0=p;EM_V0=EM_T0<<2;EM_V1=q;EM_V0+=EM_S2;
                        p=EM_R32(EM_V0+0x68u);q=EM_R16(EM_S1);EM_A1=p;EM_V0=q;
                        EM_V0<<=2;EM_V0+=EM_V1;p=EM_R32(EM_V0);EM_V0=0x10u;EM_A0=p;
                        EM_AT=0x80050000u;EM_W32(EM_AT+0x100u,EM_V0);EM_A2-=4u;
                    }else{
                        p=EM_R32(EM_SP+0x58u);q=EM_R32(EM_SP+0x54u);EM_T0=p;EM_V0=EM_T0<<2;EM_A2=q;EM_V0+=EM_S2;
                        p=EM_R32(EM_V0+0x68u);q=EM_R16(EM_S1);EM_A1=p;p=EM_R32(EM_S5);EM_V0=q;
                        EM_V0<<=2;EM_V1=p;EM_V0+=EM_V1;p=EM_R32(EM_V0);EM_AT=0x80050000u;EM_A0=p;
                        EM_W32(EM_AT+0x100u,EM_S6);
                    }
                    EM_CALL(0x8002c700u,0x800a4ae4u);
                }
            }
            EM_S4++;EM_S1+=0x7cu;EM_V0=EM_S4<EM_S7;again=EM_V0!=0;EM_S2+=0x7cu;
        }while(again);
    }
    EM_AT=0x80050000u;EM_W32(EM_AT+0x100u,EM_S6);
    p=EM_R32(EM_SP+0x3cu);q=EM_R32(EM_SP+0x38u);EM_RA=p;
    p=EM_R32(EM_SP+0x34u);EM_FP=q;q=EM_R32(EM_SP+0x30u);EM_S7=p;
    p=EM_R32(EM_SP+0x2cu);EM_S6=q;q=EM_R32(EM_SP+0x28u);EM_S5=p;
    p=EM_R32(EM_SP+0x24u);EM_S4=q;q=EM_R32(EM_SP+0x20u);EM_S3=p;
    p=EM_R32(EM_SP+0x1cu);EM_S2=q;q=EM_R32(EM_SP+0x18u);EM_S1=p;
    EM_SP+=0x40u;EM_S0=q;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_effect_models_bodies[]={
    {0x800a48ecu,592u,"6528c03d2c56e3728a8a8fe2fdf34e37fcd0e1712588f7f7ac54887f9b9f193b"},
};
static int battle_effect_models_target(uint32_t target)
{return target==0x800a48ecu;}
static int battle_effect_models_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_effect_models_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_MODELS");if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),battle_effect_models_bodies[0].bytes,digest);
    const char *expected=battle_effect_models_bodies[0].sha;
    for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_effect_models_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef EM_R32
#undef EM_R16
#undef EM_R8
#undef EM_W32
#undef EM_W16
#undef EM_AT
#undef EM_V0
#undef EM_V1
#undef EM_A0
#undef EM_A1
#undef EM_A2
#undef EM_A3
#undef EM_T0
#undef EM_S0
#undef EM_S1
#undef EM_S2
#undef EM_S3
#undef EM_S4
#undef EM_S5
#undef EM_S6
#undef EM_S7
#undef EM_SP
#undef EM_FP
#undef EM_RA
#undef EM_CALL
