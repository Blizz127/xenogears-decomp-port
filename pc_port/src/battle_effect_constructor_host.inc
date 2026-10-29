/* Complete native effect-image constructor, 800A3640..800A3E98.
 * Fixed compiled control flow, nineteen original argument words, packed
 * guest records; no instruction decoder or machine-code execution. */
#define EC_R(a) battle_entry_read(entry,(a),4)
#define EC_H(a) battle_entry_read(entry,(a),2)
#define EC_B(a) battle_entry_read(entry,(a),1)
#define EC_W(a,v) battle_entry_write(entry,(a),4,(v))
#define EC_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define EC_WB(a,v) battle_entry_write(entry,(a),1,(v))
#define EC_V0 (entry->cpu->gpr[2])
#define EC_V1 (entry->cpu->gpr[3])
#define EC_A0 (entry->cpu->gpr[4])
#define EC_A1 (entry->cpu->gpr[5])
#define EC_A2 (entry->cpu->gpr[6])
#define EC_A3 (entry->cpu->gpr[7])
#define EC_T0 (entry->cpu->gpr[8])
#define EC_T1 (entry->cpu->gpr[9])
#define EC_T2 (entry->cpu->gpr[10])
#define EC_T3 (entry->cpu->gpr[11])
#define EC_T4 (entry->cpu->gpr[12])
#define EC_T5 (entry->cpu->gpr[13])
#define EC_T6 (entry->cpu->gpr[14])
#define EC_S0 (entry->cpu->gpr[16])
#define EC_S1 (entry->cpu->gpr[17])
#define EC_S2 (entry->cpu->gpr[18])
#define EC_S3 (entry->cpu->gpr[19])
#define EC_S4 (entry->cpu->gpr[20])
#define EC_S5 (entry->cpu->gpr[21])
#define EC_S6 (entry->cpu->gpr[22])
#define EC_S7 (entry->cpu->gpr[23])
#define EC_SP (entry->cpu->gpr[29])
#define EC_FP (entry->cpu->gpr[30])
#define EC_RA (entry->cpu->gpr[31])
#define EC_LO (entry->cpu->lo)
#define EC_HI (entry->cpu->hi)
#define EC_ASR(v,n) ((uint32_t)((int32_t)(v)>>(n)))
#define EC_CALL(t,s) battle_effect_constructor_call(entry,(t),(s))
#define EC_MULT(a,b) battle_effect_constructor_mult(entry,(a),(b))
static void battle_effect_constructor_mult(BattleEntryCall *entry,uint32_t a,uint32_t b)
{
    int64_t product=(int64_t)(int32_t)a*(int32_t)b;
    EC_LO=(uint32_t)product;EC_HI=(uint32_t)((uint64_t)product>>32);
}
static void battle_effect_constructor_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    EC_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    EC_HI=callee.hi;EC_LO=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-effect-constructor] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"effect constructor callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending effect constructor callee return",target);
}
/* Common signed allocation calculation at six fixed sites. V0 contains
 * the original branch-delay shifted width even when allocation is skipped. */
static void battle_effect_constructor_allocate(BattleEntryCall *entry,uint32_t site,uint32_t slot)
{
    EC_V0=EC_ASR(EC_V0,16);EC_V1=EC_S2<<16;EC_V1=EC_ASR(EC_V1,16);
    EC_MULT(EC_V0,EC_V1);EC_A1=0;EC_A0=EC_LO;EC_RA=site+8u;EC_A0<<=1;
    EC_CALL(0x80031bdcu,site);EC_W(EC_S0+slot,EC_V0);
}
/* Capture a rectangular plane through the existing typed native SDK bridge.
 * The original frame rectangle is visible to both callees. */
static void battle_effect_constructor_capture(BattleEntryCall *entry,uint32_t values,uint32_t slot,uint32_t site)
{
    EC_T6=EC_H(EC_SP+values);EC_WH(EC_SP+0x10u,EC_T6);
    uint32_t p=EC_H(EC_SP+values+8u);EC_WH(EC_SP+0x14u,EC_S3);EC_T6=p;
    EC_WH(EC_SP+0x16u,EC_S2);EC_WH(EC_SP+0x12u,EC_T6);
    p=EC_R(EC_S0+slot);EC_RA=site+8u;EC_A1=p;EC_A0=EC_SP+0x10u;
    EC_CALL(0x800448f8u,site);EC_RA=site+16u;EC_A0=0;EC_CALL(0x800445d0u,site+8u);
}
/* Packed-color fill. The destination pointer is deliberately reloaded for
 * every pixel, so overlap with the record retains the original ordering. */
static void battle_effect_constructor_color(BattleEntryCall *entry,uint32_t values,uint32_t slot)
{
    EC_V1=EC_ASR(EC_V1,16);EC_V0=EC_S2<<16;EC_V0=EC_ASR(EC_V0,16);EC_MULT(EC_V1,EC_V0);
    EC_T6=EC_H(EC_SP+values+16u);EC_V1=EC_T6&63u;
    uint32_t p=EC_H(EC_SP+values+8u);EC_V1<<=10;EC_T6=p;
    EC_V0=EC_T6&31u;EC_V0<<=5;p=EC_H(EC_SP+values);EC_V1+=EC_V0;EC_T6=p;
    EC_V0=EC_T6&31u;EC_A0=EC_V1|EC_V0;EC_V0=EC_LO;int fill=(int32_t)EC_V0>0;EC_A3=0;
    if(fill){
        EC_A1=EC_V0;EC_V0=EC_A3<<1;int again;
        do{
            p=EC_R(EC_S0+slot);EC_A3++;EC_V1=p;EC_V0+=EC_V1;EC_WH(EC_V0,EC_A0);
            EC_V0=(int32_t)EC_A3<(int32_t)EC_A1;again=EC_V0!=0;EC_V0=EC_A3<<1;
        }while(again);
    }
}
/* Types 4/5 copy from a source with six bytes per row. Width, coordinates,
 * live source and destination pointers retain their exact guest arithmetic.
 * Plane two clears A2 in its outer branch delay, unlike plane one. */
static void battle_effect_constructor_copy(BattleEntryCall *entry,uint32_t values,uint32_t slot,int second)
{
    EC_V0=EC_S2<<16;int rows=(int32_t)EC_V0>0;EC_A3=0;if(!rows)return;
    EC_T3=EC_S3<<16;uint32_t p=EC_H(EC_SP+values+8u);EC_T5=EC_ASR(EC_T3,16);EC_T6=p;
    EC_V0=EC_T6<<16;p=EC_H(EC_SP+values);EC_T4=EC_ASR(EC_V0,16);EC_T6=p;
    EC_V0=EC_T6<<16;EC_T2=EC_ASR(EC_V0,16);EC_A2=0;
    do{
        if(!second)EC_A2=0;
        int columns=(int32_t)EC_T5>0;EC_T1=EC_T3;
        if(columns){
            EC_V1=EC_T4+EC_A3;EC_V0=EC_V1<<1;EC_V0+=EC_V1;
            p=EC_R(EC_SP+0x30u);EC_V0<<=1;EC_T6=p;EC_T0=EC_V0+EC_T6;int again;
            do{
                EC_A1=EC_ASR(EC_T1,16);EC_MULT(EC_A3,EC_A1);EC_V0=EC_T2+EC_A2;EC_V0<<=1;EC_V0+=EC_T0;
                p=EC_R(EC_S0+slot);uint32_t q=EC_H(EC_V0);EC_A0=p;EC_V1=EC_LO;EC_V0=q;
                EC_V1+=EC_A2;EC_A2++;EC_V1<<=1;EC_V1+=EC_A0;EC_A1=(int32_t)EC_A2<(int32_t)EC_A1;
                again=EC_A1!=0;EC_WH(EC_V1,EC_V0);
            }while(again);
        }
        EC_A3++;EC_V0=EC_S2<<16;EC_V0=EC_ASR(EC_V0,16);EC_V0=(int32_t)EC_A3<(int32_t)EC_V0;
        rows=EC_V0!=0;if(second)EC_A2=0;else EC_V0=EC_S5>>4;
    }while(rows);
}
/* Signed row remainder chooses one of three words. A negative remainder
 * leaves S1 intact, including the caller's incoming S1 on the first plane.
 * Component words and destination pointers stay live on every column. */
static void battle_effect_constructor_stripes(BattleEntryCall *entry,uint32_t values,uint32_t slot)
{
    int rows=(int32_t)EC_V0>0;EC_A3=0;if(!rows)return;
    EC_V0=EC_S3<<16;EC_T0=EC_ASR(EC_V0,16);uint32_t p=EC_H(EC_SP+0x70u);
    EC_T1=0x55550000u;EC_T6=p;EC_T1|=0x5556u;EC_A1=0;EC_V0=EC_T6<<16;EC_T2=EC_ASR(EC_V0,16);
    do{
        int columns=(int32_t)EC_T0>0;EC_A2=0;
        if(columns){
            EC_A0=EC_T2+EC_A3;EC_MULT(EC_A0,EC_T1);EC_V0=EC_ASR(EC_A0,31);EC_V1=EC_HI;
            EC_V1-=EC_V0;EC_V0=EC_V1<<1;EC_V0+=EC_V1;EC_A0-=EC_V0;EC_V0=1;int again;
            do{
                int middle=EC_A0==EC_V0;EC_V0=(int32_t)EC_A0<2;
                if(middle){p=EC_H(EC_SP+values+8u);EC_S1=p;EC_V0=EC_A1+EC_A2;}
                else if(EC_V0){
                    int first=EC_A0==0;EC_V0=EC_A1+EC_A2;
                    if(first)EC_S1=EC_H(EC_SP+values);
                }else{
                    EC_V0=2;int last=EC_A0==EC_V0;EC_V0=EC_A1+EC_A2;
                    if(last){p=EC_H(EC_SP+values+16u);EC_A2++;EC_S1=p;goto loaded_last;}
                }
                EC_A2++;
loaded_last:
                p=EC_R(EC_S0+slot);EC_V0<<=1;EC_V1=p;EC_V0+=EC_V1;EC_WH(EC_V0,EC_S1);
                EC_V0=(int32_t)EC_A2<(int32_t)EC_T0;again=EC_V0!=0;EC_V0=1;
            }while(again);
        }
        EC_A3++;EC_V0=EC_S2<<16;EC_V0=EC_ASR(EC_V0,16);EC_V0=(int32_t)EC_A3<(int32_t)EC_V0;
        rows=EC_V0!=0;EC_A1+=EC_T0;
    }while(rows);
}
static void battle_effect_constructor_run(BattleEntryCall *entry)
{
    EC_SP-=0x108u;EC_W(EC_SP+0xf4u,EC_S5);EC_W(EC_SP+0x28u,EC_A3);
    uint32_t p=EC_H(EC_SP+0x28u),q=EC_R(EC_SP+0x13cu);EC_S5=p;
    EC_W(EC_SP+0xecu,EC_S3);EC_T6=q;EC_W(EC_SP+0x78u,EC_T6);
    p=EC_R(EC_SP+0x140u);q=EC_H(EC_SP+0x78u);EC_T6=p;EC_W(EC_SP+0xe8u,EC_S2);EC_S3=q;
    EC_W(EC_SP+0x80u,EC_T6);p=EC_R(EC_SP+0x118u);q=EC_H(EC_SP+0x80u);EC_T6=p;
    EC_W(EC_SP+0x30u,EC_T6);EC_S2=q;p=EC_R(EC_SP+0x150u);EC_W(EC_SP+0xf0u,EC_S4);EC_T6=p;
    EC_W(EC_SP+0x88u,EC_T6);p=EC_H(EC_SP+0x11cu);EC_S4=EC_A2;EC_T6=p;EC_WH(EC_SP+0x38u,EC_T6);
    p=EC_H(EC_SP+0x120u);EC_W(EC_SP+0x104u,EC_RA);EC_T6=p;EC_W(EC_SP+0x100u,EC_FP);
    EC_W(EC_SP+0xfcu,EC_S7);EC_W(EC_SP+0xf8u,EC_S6);EC_W(EC_SP+0xe4u,EC_S1);EC_W(EC_SP+0xe0u,EC_S0);
    EC_W(EC_SP+0x18u,EC_A1);EC_WH(EC_SP+0x20u,EC_S4);EC_WH(EC_SP+0x40u,EC_T6);
    EC_T6=EC_H(EC_SP+0x124u);EC_WH(EC_SP+0x48u,EC_T6);EC_T6=EC_H(EC_SP+0x128u);EC_WH(EC_SP+0x50u,EC_T6);
    p=EC_H(EC_SP+0x12cu);q=EC_H(EC_SP+0x144u);EC_T6=p;p=EC_H(EC_SP+0x148u);EC_S7=q;
    EC_WH(EC_SP+0x58u,EC_T6);EC_FP=p;EC_T6=EC_H(EC_SP+0x130u);EC_WH(EC_SP+0x60u,EC_T6);
    EC_T6=EC_H(EC_SP+0x134u);EC_WH(EC_SP+0x68u,EC_T6);p=EC_H(EC_SP+0x138u);EC_S0=EC_A0;EC_T6=p;
    EC_WH(EC_SP+0x70u,EC_T6);p=EC_H(EC_S0+0x1au);q=EC_H(EC_SP+0x14cu);EC_V0=p;
    int take=EC_V0!=0;EC_S6=q;EC_V0=0;if(take)goto restore;
    EC_A0=4;EC_RA=0x800a3720u;EC_A1=0;EC_CALL(0x80032498u,0x800a3718u);
    EC_V0=1;EC_WH(EC_S0+0x1au,EC_V0);p=EC_B(EC_SP+0x20u);EC_WB(EC_S0+0x11u,0);EC_T6=p;
    EC_WB(EC_S0+0x10u,EC_T6);p=EC_R(EC_SP+0x18u);EC_V0=0xffffu;EC_T6=p;EC_W(EC_S0,EC_T6);
    EC_WH(EC_S0+0x14u,0);EC_WH(EC_S0+0x18u,EC_V0);EC_WH(EC_S0+0x16u,EC_S7);
    EC_WH(EC_S0+0x20u,EC_FP);EC_WH(EC_S0+0x22u,EC_S6);EC_T6=EC_R(EC_SP+0x88u);EC_W(EC_S0+0x24u,EC_T6);
    p=EC_R(EC_SP+0x30u);EC_V0=EC_S4&1u;EC_T6=p;take=EC_V0!=0;EC_W(EC_S0+0x1cu,EC_T6);
    if(!take){EC_T6=EC_R(EC_SP+0x28u);EC_S5=EC_T6&0xfd0fu;}
    EC_V0=EC_S4&4u;if(EC_V0)EC_S5&=0xfeffu;
    EC_T6=EC_R(EC_SP+0x78u);EC_V0=EC_T6<<16;if(!EC_V0)EC_S3=0x100u;
    EC_T6=EC_R(EC_SP+0x80u);EC_V0=EC_T6<<16;if(!EC_V0)EC_S2=0x100u;
    EC_V1=EC_H(EC_SP+0x20u);take=(int32_t)EC_V1<0;EC_V0=(int32_t)EC_V1<2;if(take)goto result;
    take=EC_V0!=0;EC_V0=EC_S3<<16;if(!take){
        EC_V0=(int32_t)EC_V1<6;take=EC_V0==0;EC_V0=(int32_t)EC_V1<4;if(take)goto result;
        take=EC_V0!=0;EC_V0=EC_S0;if(take)goto restore;EC_MULT(EC_S3,EC_S2);goto source_planes;
    }
    EC_V0=EC_ASR(EC_V0,16);EC_V0++;EC_V1=EC_V0>>31;EC_V0+=EC_V1;EC_S1=EC_ASR(EC_V0,1);
    EC_V0=EC_S1<<1;EC_MULT(EC_V0,EC_S2);EC_T6=EC_H(EC_SP+0x68u);EC_WH(EC_S0+0x28u,EC_T6);
    p=EC_H(EC_SP+0x70u);EC_S3=EC_V0;EC_T6=p;EC_WH(EC_S0+0x2au,EC_T6);
    EC_WH(EC_S0+0x2cu,EC_S3);EC_WH(EC_S0+0x2eu,EC_S2);EC_V0=EC_LO;EC_WH(EC_S0+0x12u,EC_V0);
    EC_V0=EC_S5&0x100u;take=EC_V0!=0;EC_V0=EC_S1<<17;if(take)battle_effect_constructor_allocate(entry,0x800a3858u,12);
    EC_V0=EC_S5&0x200u;take=EC_V0!=0;EC_V0=EC_S1<<17;if(take)battle_effect_constructor_allocate(entry,0x800a3888u,8);
    EC_V0=EC_S5&0x400u;take=EC_V0!=0;EC_V0=EC_S1<<17;if(take)battle_effect_constructor_allocate(entry,0x800a38b8u,4);
    EC_V1=EC_S5&15u;EC_V0=1;take=EC_V1==EC_V0;EC_V0=2;
    if(take)battle_effect_constructor_capture(entry,0x38u,4,0x800a3904u);
    else{take=EC_V1==EC_V0;EC_V1=EC_S1<<17;if(take)battle_effect_constructor_color(entry,0x38u,4);}
    EC_V0=EC_S5>>4;EC_V1=EC_V0&15u;EC_V0=1;take=EC_V1==EC_V0;EC_V0=2;
    if(take)battle_effect_constructor_capture(entry,0x50u,8,0x800a39ccu);
    else{take=EC_V1==EC_V0;EC_V1=EC_S3<<16;if(take)battle_effect_constructor_color(entry,0x50u,8);}
    goto result;
source_planes:
    EC_T6=EC_H(EC_SP+0x68u);EC_WH(EC_S0+0x28u,EC_T6);p=EC_H(EC_SP+0x70u);
    EC_WH(EC_S0+0x2cu,EC_S3);EC_T6=p;EC_WH(EC_S0+0x2eu,EC_S2);EC_WH(EC_S0+0x2au,EC_T6);
    EC_V0=EC_LO;EC_WH(EC_S0+0x12u,EC_V0);
    EC_V0=EC_S5&0x100u;take=EC_V0!=0;EC_V0=EC_S3<<16;if(take)battle_effect_constructor_allocate(entry,0x800a3aa0u,12);
    EC_V0=EC_S5&0x200u;take=EC_V0!=0;EC_V0=EC_S3<<16;if(take)battle_effect_constructor_allocate(entry,0x800a3ad0u,8);
    EC_V0=EC_S5&0x400u;take=EC_V0!=0;EC_V0=EC_S3<<16;if(take)battle_effect_constructor_allocate(entry,0x800a3b00u,4);
    EC_V1=EC_S5&15u;EC_V0=1;take=EC_V1==EC_V0;EC_V0=2;
    if(take)battle_effect_constructor_copy(entry,0x38u,4,0);
    else{take=EC_V1==EC_V0;EC_V0=EC_S2<<16;if(take)battle_effect_constructor_stripes(entry,0x38u,4);}
    EC_V0=EC_S5>>4;EC_V1=EC_V0&15u;EC_V0=1;take=EC_V1==EC_V0;EC_V0=2;
    if(take)battle_effect_constructor_copy(entry,0x50u,8,1);
    else{take=EC_V1==EC_V0;EC_V0=EC_S2<<16;if(take)battle_effect_constructor_stripes(entry,0x50u,8);}
result:
    EC_V0=EC_S0;
restore:
    p=EC_R(EC_SP+0x104u);q=EC_R(EC_SP+0x100u);EC_RA=p;
    p=EC_R(EC_SP+0xfcu);EC_FP=q;q=EC_R(EC_SP+0xf8u);EC_S7=p;
    p=EC_R(EC_SP+0xf4u);EC_S6=q;q=EC_R(EC_SP+0xf0u);EC_S5=p;
    p=EC_R(EC_SP+0xecu);EC_S4=q;q=EC_R(EC_SP+0xe8u);EC_S3=p;
    p=EC_R(EC_SP+0xe4u);EC_S2=q;q=EC_R(EC_SP+0xe0u);EC_S1=p;EC_SP+=0x108u;EC_S0=q;
}
static int battle_effect_constructor_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a3640u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_CONSTRUCTOR");if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="c3673076fb507130a0d0f073437cb130c589ef92b40cebbb109c1257423a6426";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),2136u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_effect_constructor_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef EC_R
#undef EC_H
#undef EC_B
#undef EC_W
#undef EC_WH
#undef EC_WB
#undef EC_V0
#undef EC_V1
#undef EC_A0
#undef EC_A1
#undef EC_A2
#undef EC_A3
#undef EC_T0
#undef EC_T1
#undef EC_T2
#undef EC_T3
#undef EC_T4
#undef EC_T5
#undef EC_T6
#undef EC_S0
#undef EC_S1
#undef EC_S2
#undef EC_S3
#undef EC_S4
#undef EC_S5
#undef EC_S6
#undef EC_S7
#undef EC_SP
#undef EC_FP
#undef EC_RA
#undef EC_LO
#undef EC_HI
#undef EC_ASR
#undef EC_CALL
#undef EC_MULT
