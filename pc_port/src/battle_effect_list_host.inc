/* Complete fixed native effect-list update and release bodies.
 * Guest words, live accesses, delayed loads, frames and GTE effects stay visible. */
#define EL_R(a) battle_entry_read(entry,(a),4)
#define EL_H(a) battle_entry_read(entry,(a),2)
#define EL_SH(a) ((uint32_t)(int32_t)(int16_t)EL_H(a))
#define EL_W(a,v) battle_entry_write(entry,(a),4,(v))
#define EL_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define EL_WB(a,v) battle_entry_write(entry,(a),1,(v))
#define EL_V0 (entry->cpu->gpr[2])
#define EL_V1 (entry->cpu->gpr[3])
#define EL_A0 (entry->cpu->gpr[4])
#define EL_A1 (entry->cpu->gpr[5])
#define EL_A2 (entry->cpu->gpr[6])
#define EL_A3 (entry->cpu->gpr[7])
#define EL_T4 (entry->cpu->gpr[12])
#define EL_S0 (entry->cpu->gpr[16])
#define EL_S1 (entry->cpu->gpr[17])
#define EL_S2 (entry->cpu->gpr[18])
#define EL_S3 (entry->cpu->gpr[19])
#define EL_S4 (entry->cpu->gpr[20])
#define EL_S5 (entry->cpu->gpr[21])
#define EL_S6 (entry->cpu->gpr[22])
#define EL_S7 (entry->cpu->gpr[23])
#define EL_SP (entry->cpu->gpr[29])
#define EL_FP (entry->cpu->gpr[30])
#define EL_RA (entry->cpu->gpr[31])
static void battle_effect_list_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    EL_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-effect-list] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"effect list callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending effect list callee return",target);
}
static void battle_effect_list_mult(BattleEntryCall *entry)
{
    uint64_t product=(uint64_t)((int64_t)(int32_t)EL_V0*(int64_t)(int32_t)EL_S3);
    entry->cpu->hi=(uint32_t)(product>>32);entry->cpu->lo=(uint32_t)product;
}
static void battle_effect_list_release(BattleEntryCall *entry)
{
    EL_V1=0x21080000u;uint32_t p=EL_R(EL_A0);EL_V1|=0x4211u;EL_V0=p;
    EL_V0=EL_A1-EL_V0;EL_V0>>=2;
    uint64_t product=(uint64_t)EL_V0*EL_V1;entry->cpu->hi=(uint32_t)(product>>32);entry->cpu->lo=(uint32_t)product;
    p=EL_SH(EL_A0+6u);EL_V1=entry->cpu->hi;EL_V0=p;EL_V1>>=2;
    EL_V0=(int32_t)EL_V0<(int32_t)EL_V1;
    if(!EL_V0)EL_WH(EL_A0+6u,EL_V1);
    EL_V0=0xffffffffu;EL_WH(EL_A1+0x16u,EL_V0);EL_V0=EL_V1;
}
static void battle_effect_list_update(BattleEntryCall *entry)
{
    EL_SP-=0x48u;EL_W(EL_SP+0x3cu,EL_S7);EL_S7=EL_A0;
    EL_W(EL_SP+0x20u,EL_S0);EL_S0=EL_A1;EL_W(EL_SP+0x2cu,EL_S3);EL_S3=EL_A2;
    EL_W(EL_SP+0x30u,EL_S4);EL_S4=EL_A3;EL_W(EL_SP+0x40u,EL_FP);
    uint32_t p=EL_R(EL_SP+0x58u);EL_A0=EL_S0;EL_FP=p;
    EL_W(EL_SP+0x44u,EL_RA);EL_W(EL_SP+0x38u,EL_S6);EL_W(EL_SP+0x34u,EL_S5);EL_W(EL_SP+0x28u,EL_S2);
    EL_RA=0x800a3020u;EL_W(EL_SP+0x24u,EL_S1);battle_effect_list_call(entry,0x80049efcu,0x800a3018u);
    EL_RA=0x800a3028u;EL_A0=EL_S0;battle_effect_list_call(entry,0x80049f8cu,0x800a3020u);
    p=EL_SH(EL_S7+4u);uint32_t q=EL_R(EL_S7);EL_V0=p;
    int present=(int32_t)EL_V0>0;EL_S1=q;EL_S5=0;
    if(present){
        EL_S2=0x00ffffffu;EL_S6=0xff000000u;EL_S0=EL_S1+0x24u;int again;
        do{
            p=EL_SH(EL_S0-0xeu);EL_V0=0xffffffffu;EL_V1=p;
            if(EL_V1!=EL_V0){
                EL_V0=EL_SH(EL_S0-6u);EL_V0=(int32_t)EL_V1<(int32_t)EL_V0;
                int live=EL_V0!=0;EL_V0=EL_FP<<2;
                if(!live){
                    EL_A0=EL_S7;EL_RA=0x800a3078u;EL_A1=EL_S1;battle_effect_list_call(entry,0x800a2f94u,0x800a3070u);
                }else{
                    EL_V0+=EL_FP;EL_A1=EL_V0<<3;p=EL_H(EL_S0-4u);EL_A2=EL_S1+EL_A1;EL_V1=p;
                    EL_V1>>=6;EL_WB(EL_A2+0x30u,EL_V1);
                    EL_V0=EL_H(EL_S0-2u);EL_V0>>=6;EL_WB(EL_A2+0x31u,EL_V0);
                    EL_V0=EL_H(EL_S0);EL_V0>>=6;EL_WB(EL_A2+0x32u,EL_V0);
                    EL_V0=EL_SH(EL_S0-0x16u);int projected=EL_V0!=0;EL_V1=EL_S1+8u;
                    if(!projected){
                        p=EL_H(EL_S1);q=EL_R(EL_A2+0x2cu);EL_V0=p;EL_WH(EL_A2+0x34u,EL_V0);EL_V1=q;
                        EL_V0=EL_H(EL_S0-0x22u);EL_WH(EL_A2+0x36u,EL_V0);
                        EL_V0=EL_H(EL_S0-0x1cu);EL_WH(EL_A2+0x3cu,EL_V0);
                        EL_V0=EL_H(EL_S0-0x1au);EL_WH(EL_A2+0x3eu,EL_V0);
                        EL_V0=EL_H(EL_S0-0x14u);EL_WH(EL_A2+0x44u,EL_V0);
                        EL_V0=EL_H(EL_S0-0x12u);EL_WH(EL_A2+0x46u,EL_V0);
                        EL_V0=EL_H(EL_S0-0xcu);EL_WH(EL_A2+0x4cu,EL_V0);
                        EL_V0=EL_H(EL_S0-0xau);EL_WH(EL_A2+0x4eu,EL_V0);
                        p=EL_R(EL_S4);EL_V1&=EL_S6;EL_V0=p;EL_V0&=EL_S2;EL_V1|=EL_V0;
                        EL_V0=EL_A1+0x2cu;EL_V0+=EL_S1;EL_W(EL_A2+0x2cu,EL_V1);
                        p=EL_R(EL_S4);EL_V0&=EL_S2;EL_V1=p;EL_V1&=EL_S6;EL_V1|=EL_V0;EL_W(EL_S4,EL_V1);
                    }else{
                        EL_V0=EL_S1+0x10u;MTC2(EL_R(EL_S1),0);MTC2(EL_R(EL_S1+4u),1);
                        MTC2(EL_R(EL_V1),2);MTC2(EL_R(EL_V1+4u),3);MTC2(EL_R(EL_V0),4);MTC2(EL_R(EL_V0+4u),5);
                        doCOP2(0x4a280030);
                        EL_A1+=0x2cu;EL_A1+=EL_S1;EL_A0=EL_A1+8u;EL_V1=EL_A1+0x10u;EL_V0=EL_A1+0x18u;
                        EL_W(EL_A0,MFC2(12));EL_W(EL_V1,MFC2(13));EL_W(EL_V0,MFC2(14));
                        EL_V0=EL_SP+0x10u;EL_T4=MFC2(19);EL_T4=(uint32_t)((int32_t)EL_T4>>2);EL_W(EL_V0,EL_T4);
                        p=EL_R(EL_SP+0x10u);EL_V1=0x80050000u;EL_V0=p;EL_V1=EL_R(EL_V1+0x100u);
                        EL_V0=(uint32_t)((int32_t)EL_V0>>(EL_V1&31u));EL_W(EL_SP+0x10u,EL_V0);
                        EL_V0=EL_S1+0x18u;MTC2(EL_R(EL_V0),0);MTC2(EL_R(EL_V0+4u),1);doCOP2(0x4a180001);
                        EL_V0=EL_A1+0x20u;EL_W(EL_V0,MFC2(14));
                        p=EL_R(EL_SP+0x10u);q=EL_R(EL_A2+0x2cu);EL_A0=p;EL_A0<<=2;EL_V1=q;EL_A0+=EL_S4;
                        p=EL_R(EL_A0);EL_V1&=EL_S6;EL_V0=p;EL_V0&=EL_S2;EL_V1|=EL_V0;EL_W(EL_A2+0x2cu,EL_V1);
                        p=EL_R(EL_A0);EL_A1&=EL_S2;EL_V0=p;EL_V0&=EL_S6;EL_V0|=EL_A1;EL_W(EL_A0,EL_V0);
                    }
                    EL_V0=EL_SH(EL_S0+2u);battle_effect_list_mult(entry);EL_A1=entry->cpu->lo;
                    EL_V0=EL_SH(EL_S0+4u);battle_effect_list_mult(entry);EL_A0=entry->cpu->lo;
                    EL_V0=EL_SH(EL_S0+6u);battle_effect_list_mult(entry);
                    p=EL_H(EL_S0-0xeu);q=EL_H(EL_S0-2u);EL_V0=p;EL_V0+=EL_S3;EL_V1=q;EL_WH(EL_S0-0xeu,EL_V0);
                    p=EL_H(EL_S0-4u);EL_V1-=EL_A0;EL_V0=p;EL_WH(EL_S0-2u,EL_V1);EL_V0-=EL_A1;EL_WH(EL_S0-4u,EL_V0);
                    p=EL_H(EL_S0);EL_V1=entry->cpu->lo;EL_V0=p;EL_V0-=EL_V1;EL_WH(EL_S0,EL_V0);
                }
            }
            EL_S5++;EL_S0+=0x7cu;EL_V0=EL_SH(EL_S7+4u);EL_V0=(int32_t)EL_S5<(int32_t)EL_V0;
            again=EL_V0!=0;EL_S1+=0x7cu;
        }while(again);
    }
    p=EL_R(EL_SP+0x44u);q=EL_R(EL_SP+0x40u);EL_RA=p;
    p=EL_R(EL_SP+0x3cu);EL_FP=q;q=EL_R(EL_SP+0x38u);EL_S7=p;
    p=EL_R(EL_SP+0x34u);EL_S6=q;q=EL_R(EL_SP+0x30u);EL_S5=p;
    p=EL_R(EL_SP+0x2cu);EL_S4=q;q=EL_R(EL_SP+0x28u);EL_S3=p;
    p=EL_R(EL_SP+0x24u);EL_S2=q;q=EL_R(EL_SP+0x20u);EL_S1=p;EL_SP+=0x48u;EL_S0=q;
}
static int battle_effect_list_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    unsigned size;const char *expected;
    if(target==0x800a2fd8u){size=768;expected="54616eb908fdbcf974ed8fe21dc2d4b74c938283af12d788af2a24b27b23181d";}
    else if(target==0x800a2f94u){size=68;expected="91e3293a33d45b2086a530876a9c0605a3bde08c2245f2ab7368cd874bad50ce";}
    else return 0;
    if(runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_LIST");if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),size,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800a2fd8u)battle_effect_list_update(&entry);else battle_effect_list_release(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef EL_R
#undef EL_H
#undef EL_SH
#undef EL_W
#undef EL_WH
#undef EL_WB
#undef EL_V0
#undef EL_V1
#undef EL_A0
#undef EL_A1
#undef EL_A2
#undef EL_A3
#undef EL_T4
#undef EL_S0
#undef EL_S1
#undef EL_S2
#undef EL_S3
#undef EL_S4
#undef EL_S5
#undef EL_S6
#undef EL_S7
#undef EL_SP
#undef EL_FP
#undef EL_RA
