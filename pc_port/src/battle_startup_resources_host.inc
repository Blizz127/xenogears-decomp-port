/* Complete startup resource loader, 801E6314..801E6710. The palette table,
 * slot map and outgoing arguments remain on the original guest stack. */
#define SR_R(a) battle_entry_read(entry,(a),4)
#define SR_B(a) battle_entry_read(entry,(a),1)
#define SR_H(a) battle_entry_read(entry,(a),2)
#define SR_W(a,v) battle_entry_write(entry,(a),4,(v))
#define SR_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define SR_V0 (entry->cpu->gpr[2])
#define SR_V1 (entry->cpu->gpr[3])
#define SR_A0 (entry->cpu->gpr[4])
#define SR_A1 (entry->cpu->gpr[5])
#define SR_A2 (entry->cpu->gpr[6])
#define SR_A3 (entry->cpu->gpr[7])
#define SR_T0 (entry->cpu->gpr[8])
#define SR_T1 (entry->cpu->gpr[9])
#define SR_S0 (entry->cpu->gpr[16])
#define SR_S1 (entry->cpu->gpr[17])
#define SR_S2 (entry->cpu->gpr[18])
#define SR_S3 (entry->cpu->gpr[19])
#define SR_S4 (entry->cpu->gpr[20])
#define SR_S5 (entry->cpu->gpr[21])
#define SR_S6 (entry->cpu->gpr[22])
#define SR_S7 (entry->cpu->gpr[23])
#define SR_SP (entry->cpu->gpr[29])
#define SR_S8 (entry->cpu->gpr[30])
#define SR_RA (entry->cpu->gpr[31])
static void battle_startup_resources_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    SR_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-startup-resources] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"startup resources callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending startup resources callee return",target);
}
static void battle_startup_resources_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    SR_SP-=0xc8u;SR_T0=3;SR_W(SR_SP+0xb0u,SR_S4);SR_S4=0;SR_A1=11;SR_V0=0x800c0000u;
    SR_W(SR_SP+0xc8u,SR_A0);SR_A0=SR_V0+0x3eb0u;SR_V1=SR_SP+0x28u;
    SR_W(SR_SP+0xc4u,SR_RA);SR_W(SR_SP+0xc0u,SR_S8);SR_W(SR_SP+0xbcu,SR_S7);
    SR_W(SR_SP+0xb8u,SR_S6);SR_W(SR_SP+0xb4u,SR_S5);SR_W(SR_SP+0xacu,SR_S3);
    SR_W(SR_SP+0xa8u,SR_S2);SR_W(SR_SP+0xa4u,SR_S1);SR_W(SR_SP+0xa0u,SR_S0);
    SR_W(SR_SP+0x88u,0);SR_W(SR_SP+0x8cu,SR_T0);
    do{
        p=SR_B(SR_A0+6u);SR_A0+=0x1cu;SR_V0=p;SR_S4++;SR_W(SR_V1+0x20u,SR_V0);
        int again=SR_S4!=SR_A1;SR_V1+=4u;if(!again)break;
    }while(1);
    SR_T1=SR_R(SR_SP+0xc8u);p=SR_B(SR_T1+1u);SR_A1=0;SR_V0=p;SR_W(SR_SP+0x84u,0);
    p=SR_B(SR_T1);SR_V0<<=6;SR_T0=p;SR_S4=SR_V0+0x140u;
    SR_S0=SR_T0<<1;SR_S0+=SR_T0;SR_S0<<=2;SR_W(SR_SP+0x80u,SR_T0);
    p=SR_R(SR_T1+4u);SR_S0+=8u;SR_A0=p;SR_RA=0x801e63bcu;SR_A0-=SR_S0;
    battle_startup_resources_call(entry,0x80031bdcu,0x801e63b4u);
    SR_A0=SR_V0;p=SR_R(SR_SP+0xc8u);SR_S1=0x800d0000u;SR_T1=p;SR_W(SR_S1+0x39c8u,SR_A0);
    p=SR_R(SR_T1+4u);SR_A1=SR_T1+SR_S0;SR_A2=p;SR_RA=0x801e63dcu;SR_A2-=SR_S0;
    battle_startup_resources_call(entry,0x8003f968u,0x801e63d4u);
    p=SR_R(SR_S1+0x39c8u);q=SR_R(SR_SP+0xc8u);SR_V0=p;p=SR_R(SR_SP+0x80u);SR_T0=q;
    SR_S8=SR_T0+8u;SR_T1=p;SR_V0-=SR_S0;int have_records=SR_T1!=0;SR_W(SR_SP+0x90u,SR_V0);
    if(have_records){
        SR_S7=SR_SP+0x28u;SR_V1=0x800c3eb0u;
        p=SR_R(SR_SP+0x8cu);q=SR_R(SR_SP+0xc8u);SR_T0=p;SR_V0=SR_T0<<1;SR_T1=q;
        SR_V0+=SR_T0;SR_V0<<=2;SR_V0+=SR_V1;SR_S5=SR_T1+0x13u;SR_W(SR_SP+0x9cu,SR_V0);
        do{
            SR_V0=SR_B(SR_S5-3u);int skip=0;
            if(SR_V0){
                SR_V0=SR_R(SR_S5-7u);SR_V0=SR_V0<8u;skip=SR_V0!=0;SR_S3=0;
                if(!skip){
                    SR_V0=0x8000u;SR_S4+=0x40u;SR_A1=3;SR_S2=SR_S3;SR_A0=11;SR_V1=SR_S7+0xcu;
                    p=SR_R(SR_SP+0x8cu);q=SR_R(SR_SP+0x9cu);SR_T0=p;SR_S6=SR_T0-3u;SR_T1=q;
                    SR_V0+=SR_T1;SR_W(SR_V0+0xd24u,0);
                    do{
                        SR_V0=SR_R(SR_V1+0x20u);if(SR_S6==SR_V0)SR_S2++;
                        SR_A1++;int again=SR_A1!=SR_A0;SR_V1+=4u;if(!again)break;
                    }while(1);
                    p=SR_R(SR_SP+0x84u);q=SR_R(SR_SP+0x80u);SR_T0=p;SR_A1=SR_T0+1u;SR_T1=q;
                    int scan=SR_A1!=SR_T1;SR_S0=3;
                    if(scan){
                        SR_A2=SR_S8+0x10u;
                        do{
                            SR_V0=SR_B(SR_A2+4u);
                            if(SR_V0){
                                p=SR_R(SR_A2);q=SR_R(SR_SP+0x84u);SR_V0=p;SR_T0=q;
                                if(SR_V0==SR_T0){
                                    SR_A0=3;SR_A3=11;SR_V1=SR_S7+0xcu;
                                    do{
                                        SR_V0=SR_R(SR_V1+0x20u);
                                        if(SR_A1==SR_V0){SR_W(SR_V1+0x20u,SR_S6);SR_S2++;}
                                        SR_A0++;int again=SR_A0!=SR_A3;SR_V1+=4u;if(!again)break;
                                    }while(1);
                                }
                            }
                            SR_A1++;SR_T1=SR_R(SR_SP+0x80u);int again=SR_A1!=SR_T1;SR_A2+=0xcu;if(!again)break;
                        }while(1);
                        SR_S0=3;
                    }
                    SR_V0=(int32_t)SR_S2<2;SR_V0^=1;SR_V0=0u-SR_V0;SR_W(SR_SP+0x98u,SR_V0);
                    SR_V0=SR_S4-0x40u;SR_V0<<=16;SR_V0=(uint32_t)((int32_t)SR_V0>>16);SR_W(SR_SP+0x94u,SR_V0);
                    SR_V0=SR_S0<<2;
                    do{
                        SR_V0+=SR_S7;SR_V1=SR_R(SR_V0+0x20u);SR_V0=(int32_t)SR_V1<8;
                        if(SR_V0&&SR_V1==SR_S6){
                            int create=SR_S0!=0;SR_S1=SR_S0;
                            if(create){
                                p=SR_R(SR_SP+0x98u);int first=SR_S3==0;SR_T0=p;SR_A1=SR_T0&2u;
                                if(!first){SR_V0=SR_S3+1u;int last=SR_S2==SR_V0;SR_A1=7;if(last)SR_A1=5;}
                                SR_A1|=0x80u;SR_A0=SR_S1;SR_A1&=0xffffu;SR_V1=SR_S3-0x1c0u;SR_S3++;
                                SR_V0=0x100u;SR_V1=SR_S1-SR_V1;SR_V1<<=16;
                                p=SR_R(SR_S8);q=SR_R(SR_S5-7u);SR_A2=p;p=SR_R(SR_SP+0x94u);SR_A3=q;
                                q=SR_R(SR_SP+0x90u);SR_T1=p;SR_V1=(uint32_t)((int32_t)SR_V1>>16);SR_T0=q;
                                SR_W(SR_SP+0x14u,SR_V0);SR_W(SR_SP+0x18u,0);SR_W(SR_SP+0x1cu,SR_V1);
                                SR_W(SR_SP+0x20u,0);SR_W(SR_SP+0x10u,SR_T1);
                                p=SR_R(SR_SP+0xc8u);SR_A2+=SR_T0;SR_T1=p;SR_RA=0x801e65d8u;SR_A3+=SR_T1;
                                battle_startup_resources_call(entry,0x800a8bf0u,0x801e65d0u);
                                SR_RA=0x801e65e0u;SR_A0=SR_S1;battle_startup_resources_call(entry,0x800bb350u,0x801e65d8u);
                            }
                        }
                        SR_S0++;SR_V0=11;int again=SR_S0!=SR_V0;SR_V0=SR_S0<<2;if(!again)break;
                    }while(1);
                    SR_S5+=0xcu;
                }
            }else{
                SR_A0=SR_R(SR_S5-7u);SR_V0=SR_A0<8u;int reuse=SR_V0!=0;SR_S0=SR_A0;
                if(!reuse){
                    SR_A1=SR_S4;p=SR_R(SR_SP+0xc8u);SR_A2=0x100u;SR_T0=p;SR_RA=0x801e6620u;SR_A0+=SR_T0;
                    battle_startup_resources_call(entry,0x80022a70u,0x801e6618u);
                    SR_S0=SR_R(SR_SP+0x88u);SR_V0=SR_S0<<1;SR_V0+=SR_S7;SR_WH(SR_V0,SR_S4);
                    p=SR_R(SR_S5-7u);q=SR_R(SR_SP+0xc8u);SR_A0=p;SR_T1=SR_S0+1u;SR_T0=q;
                    SR_W(SR_SP+0x88u,SR_T1);SR_RA=0x801e664cu;SR_A0+=SR_T0;
                    battle_startup_resources_call(entry,0x80022a00u,0x801e6644u);
                    SR_V0<<=6;SR_S4+=SR_V0;SR_V0=(int32_t)SR_S4<0x2c1;int keep=SR_V0!=0;SR_A0=0x8000u;
                    if(!keep){SR_S4=0;SR_A0=0x8000u;}
                }else SR_A0=0x8000u;
                SR_V0=0x100u;p=SR_R(SR_SP+0x9cu);q=SR_R(SR_S8);SR_T1=p;SR_A0+=SR_T1;SR_V1=q;
                SR_WH(SR_A0+0xd2au,SR_V0);SR_V0=SR_S0<<1;p=SR_R(SR_SP+0x90u);SR_V0+=SR_S7;SR_T0=p;
                SR_V1+=SR_T0;SR_W(SR_A0+0xd24u,SR_V1);SR_V0=SR_H(SR_V0);SR_WH(SR_A0+0xd28u,SR_V0);
                SR_V0=SR_B(SR_S5);SR_W(SR_A0+0xd2cu,SR_V0);SR_S5+=0xcu;
            }
            if(!skip){
                SR_S8+=0xcu;p=SR_R(SR_SP+0x9cu);q=SR_R(SR_SP+0x8cu);SR_T1=p;SR_T1+=0xcu;SR_T0=q;
                SR_T0++;SR_W(SR_SP+0x9cu,SR_T1);SR_W(SR_SP+0x8cu,SR_T0);
            }
            p=SR_R(SR_SP+0x84u);q=SR_R(SR_SP+0x80u);SR_T1=p;SR_T1++;SR_T0=q;
            int again=SR_T1!=SR_T0;SR_W(SR_SP+0x84u,SR_T1);if(!again)break;
        }while(1);
    }
    p=SR_R(SR_SP+0xc4u);q=SR_R(SR_SP+0xc0u);SR_RA=p;p=SR_R(SR_SP+0xbcu);SR_S8=q;
    q=SR_R(SR_SP+0xb8u);SR_S7=p;p=SR_R(SR_SP+0xb4u);SR_S6=q;q=SR_R(SR_SP+0xb0u);SR_S5=p;
    p=SR_R(SR_SP+0xacu);SR_S4=q;q=SR_R(SR_SP+0xa8u);SR_S3=p;p=SR_R(SR_SP+0xa4u);SR_S2=q;
    q=SR_R(SR_SP+0xa0u);SR_S1=p;SR_SP+=0xc8u;SR_S0=q;
}
static int battle_startup_resources_identity(uint32_t address,unsigned bytes,const char *sha,int executable)
{
    const uint8_t *code=PSX_ADDR(address);
    if(executable){
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t off=XENO_RD_SLUS_OFF(address);
        if(!image||!image->bytes||image->size<off+bytes)return 0;
        code=image->bytes+off;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[i*2]||file1_hex_digit(digest[i]&15u)!=sha[i*2+1])return 0;
    return 1;
}
static int battle_startup_resources_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x801e6314u||runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||
       runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES");if(control&&strcmp(control,"0")==0)return 0;
    if(!battle_startup_resources_identity(target,1020,"4581d128c6e3d82317b5bd330bb6439fe22208f572dfdcb74e165da57e120a5f",0)||
       !battle_startup_resources_identity(0x801e4000u,32,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140",0)||
       !battle_startup_resources_identity(0x801e5840u,228,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2",0))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_startup_resources_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
/* The startup copy is the original signed-length forward byte copy, including
 * overlap and null-destination behavior. These bindings have no unused stack
 * arguments and are confined to the two calls in the loader. */
static int battle_startup_resources_service_target(uint32_t target,uint32_t caller)
{
    return (target==0x8003f968u&&caller==0x801e63dcu)||
           (target==0x80022a00u&&caller==0x801e664cu);
}
static void battle_startup_resources_service_run(BattleEntryCall *entry,uint32_t target)
{
    if(target==0x80022a00u){SR_V0=SR_R(SR_A0);return;}
    int destination=SR_A0!=0;SR_V0=0;
    if(destination){
        int copy=(int32_t)SR_A2>0;SR_V1=SR_A0;
        if(copy)do{
            uint32_t p=SR_B(SR_A1);SR_A1++;SR_V0=p;SR_A2--;
            battle_entry_write(entry,SR_A0,1,SR_V0);
            int again=(int32_t)SR_A2>0;SR_A0++;if(!again)break;
        }while(1);
        SR_V0=SR_V1;
    }
}
static int battle_startup_resources_service_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_startup_resources_service_target(target,cpu->gpr[31])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES");if(control&&strcmp(control,"0")==0)return 0;
    if(!battle_startup_resources_identity(target,target==0x8003f968u?52u:12u,target==0x8003f968u?
       "0ce68d87981de9301e11a186d5b7b97a838ff7b244145d7261b6ffab7cb185df":
       "4957f5372e34b70d76a56a232f14cdfbde36664457628f8b757aac861b817110",1))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_startup_resources_service_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef SR_R
#undef SR_B
#undef SR_H
#undef SR_W
#undef SR_WH
#undef SR_V0
#undef SR_V1
#undef SR_A0
#undef SR_A1
#undef SR_A2
#undef SR_A3
#undef SR_T0
#undef SR_T1
#undef SR_S0
#undef SR_S1
#undef SR_S2
#undef SR_S3
#undef SR_S4
#undef SR_S5
#undef SR_S6
#undef SR_S7
#undef SR_SP
#undef SR_S8
#undef SR_RA
