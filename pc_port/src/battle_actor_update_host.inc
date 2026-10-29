/* Complete actor update/render coordinator. Actor slots and nested records
 * retain their original 32-bit layout. Loads remain live across child calls. */
#define AU_R(a) battle_entry_read(entry,(a),4)
#define AU_H(a) battle_entry_read(entry,(a),2)
#define AU_B(a) battle_entry_read(entry,(a),1)
#define AU_W(a,v) battle_entry_write(entry,(a),4,(v))
#define AU_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define AU_WB(a,v) battle_entry_write(entry,(a),1,(v))
#define AU_AT (entry->cpu->gpr[1])
#define AU_V0 (entry->cpu->gpr[2])
#define AU_V1 (entry->cpu->gpr[3])
#define AU_A0 (entry->cpu->gpr[4])
#define AU_A1 (entry->cpu->gpr[5])
#define AU_A2 (entry->cpu->gpr[6])
#define AU_A3 (entry->cpu->gpr[7])
#define AU_S0 (entry->cpu->gpr[16])
#define AU_S1 (entry->cpu->gpr[17])
#define AU_S2 (entry->cpu->gpr[18])
#define AU_S3 (entry->cpu->gpr[19])
#define AU_S4 (entry->cpu->gpr[20])
#define AU_S5 (entry->cpu->gpr[21])
#define AU_S6 (entry->cpu->gpr[22])
#define AU_S7 (entry->cpu->gpr[23])
#define AU_SP (entry->cpu->gpr[29])
#define AU_FP (entry->cpu->gpr[30])
#define AU_RA (entry->cpu->gpr[31])
#define AU_CALL(t,s) battle_actor_update_call(entry,(t),(s))

static void battle_actor_update_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    AU_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-actor-update] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"actor update callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending actor update callee return",target);
}
static void battle_actor_update_run(BattleEntryCall *entry)
{
    AU_V0=0x800c0000u;uint32_t p=AU_R(AU_V0+0x3e88u);
    AU_V1=0x800d0000u;AU_V0=p;p=AU_R(AU_V1-0x33a4u);AU_SP-=0x60u;AU_V1=p;
    AU_W(AU_SP+0x54u,AU_S7);AU_S7=AU_A0;AU_W(AU_SP+0x58u,AU_FP);AU_FP=AU_A2;
    AU_W(AU_SP+0x50u,AU_S6);AU_S6=AU_A3;AU_W(AU_SP+0x48u,AU_S4);
    AU_W(AU_SP+0x5cu,AU_RA);AU_W(AU_SP+0x4cu,AU_S5);AU_W(AU_SP+0x44u,AU_S3);
    AU_W(AU_SP+0x40u,AU_S2);AU_W(AU_SP+0x3cu,AU_S1);AU_W(AU_SP+0x38u,AU_S0);AU_W(AU_SP+0x28u,AU_A1);
    /* Rate addition wraps before the signed upper clamp. Negative cadence is
     * retained, including values produced by overflow. */
    AU_V0++;AU_V0+=AU_V1;AU_AT=0x800c0000u;AU_W(AU_AT+0x3e88u,AU_V0);
    AU_V0=(int32_t)AU_V0<7;int under_limit=AU_V0!=0;AU_S4=0;
    if(!under_limit){AU_V0=6;AU_AT=0x800c0000u;AU_W(AU_AT+0x3e88u,AU_V0);}
    AU_V1=0x800c0000u;AU_V1=AU_R(AU_V1+0x3e88u);AU_V0=(int32_t)AU_V1<2;
    int ticks=AU_V0==0;AU_V0=AU_S4<<3;
    while(ticks){
        AU_V0=AU_V1-2u;AU_AT=0x800c0000u;AU_W(AU_AT+0x3e88u,AU_V0);
        AU_V1=AU_V0;AU_V0=(int32_t)AU_V1<2;ticks=AU_V0==0;AU_S4++;
        if(!ticks)AU_V0=AU_S4<<3;
    }
    AU_V0-=AU_S4;AU_V1=0x800c0000u;p=AU_H(AU_V1+0x3cf0u);AU_V0<<=3;AU_V1=p;
    AU_V1+=AU_V0;AU_A0=AU_V1<<16;AU_AT=0x800c0000u;AU_WH(AU_AT+0x3cf0u,AU_V1);
    AU_RA=0x800a9b24u;AU_A0=(uint32_t)((int32_t)AU_A0>>16);AU_CALL(0x8003f8ccu,0x800a9b1cu);
    AU_V1=0x51eb851fu;AU_V0+=0x1000u;
    int64_t product=(int64_t)(int32_t)AU_V0*(int32_t)AU_V1;
    entry->cpu->hi=(uint32_t)((uint64_t)product>>32);entry->cpu->lo=(uint32_t)product;
    AU_V1=0x800c0000u;p=AU_H(AU_V1+0x3b7cu);AU_V0=(uint32_t)((int32_t)AU_V0>>31);AU_V1=p;
    AU_V1+=0x80u;AU_A0=AU_V1<<16;AU_AT=0x800c0000u;AU_WH(AU_AT+0x3b7cu,AU_V1);
    AU_V1=entry->cpu->hi;AU_V1=(uint32_t)((int32_t)AU_V1>>8);AU_V1-=AU_V0;AU_V1+=4u;
    AU_AT=0x800d0000u;AU_WH(AU_AT+0x39e8u,AU_V1);
    AU_RA=0x800a9b70u;AU_A0=(uint32_t)((int32_t)AU_A0>>16);AU_CALL(0x8003f8ccu,0x800a9b68u);
    AU_V1=AU_V0;AU_V0=AU_V1+0x1000u;int positive=(int32_t)AU_V0>=0;
    AU_V0=(uint32_t)((int32_t)AU_V0>>5);
    if(!positive){AU_V0=AU_V1+0x101fu;AU_V0=(uint32_t)((int32_t)AU_V0>>5);}
    AU_A0=AU_V0<<16;AU_V1=0x80060000u;p=AU_R(AU_V1+0x58c8u);AU_A0=(uint32_t)((int32_t)AU_A0>>16);AU_V1=p;
    AU_AT=0x800c0000u;AU_WH(AU_AT+0x3b80u,AU_V0);p=AU_B(AU_V1+0x474u);
    AU_RA=0x800a9bacu;AU_A2=p;AU_A1=32;AU_CALL(0x800aa514u,0x800a9ba4u);
    AU_A0=0x800c0000u;p=AU_H(AU_A0+0x3b80u);AU_V1=0x80060000u;AU_A0=(uint32_t)(int32_t)(int16_t)p;
    p=AU_R(AU_V1+0x58c8u);AU_WB(AU_SP+0x20u,AU_V0);AU_V1=p;p=AU_B(AU_V1+0x475u);
    AU_RA=0x800a9bccu;AU_A2=p;AU_A1=32;AU_CALL(0x800aa514u,0x800a9bc4u);
    AU_A0=0x800c0000u;p=AU_H(AU_A0+0x3b80u);AU_V1=0x80060000u;AU_A0=(uint32_t)(int32_t)(int16_t)p;
    p=AU_R(AU_V1+0x58c8u);AU_WB(AU_SP+0x21u,AU_V0);AU_V1=p;p=AU_B(AU_V1+0x476u);
    AU_RA=0x800a9becu;AU_A2=p;AU_A1=32;AU_CALL(0x800aa514u,0x800a9be4u);
    AU_V1=0x800c0000u;p=AU_H(AU_V1+0x3d40u);AU_WB(AU_SP+0x22u,AU_V0);AU_V1=p;
    AU_V1<<=2;AU_AT=0x800d0000u;AU_AT+=AU_V1;p=AU_R(AU_AT+0x3368u);
    AU_V1=0x800d0000u;AU_V0=p;AU_V1+=0x3368u;int selected=AU_V0!=0;AU_S1=0;
    if(selected){AU_V0=AU_H(AU_V0+0x4au);AU_V0&=32u;selected=AU_V0==0;}
    if(selected){
        AU_S2=0;AU_S3=AU_V1;AU_S5=AU_V1;
        int again;
        do{
            AU_A0=AU_R(AU_S3);
            if(AU_A0){
                AU_A1=0x800c0000u;AU_A1=AU_H(AU_A1+0x3d40u);
                int other=AU_A1!=AU_S2;AU_V0=255;
                if(other){
                    AU_V1=AU_B(AU_A0+0x5cu);
                    if(AU_V1==AU_V0){
                        AU_V0=0x800c0000u;AU_V0=AU_H(AU_V0+0x3e30u);
                        AU_V0=(uint32_t)((int32_t)AU_V0>>(AU_S2&31u));AU_V0&=1;
                        if(!AU_V0){
                            AU_V0=AU_B(AU_A0+0x34u);int active=AU_V0!=0;AU_V0=AU_A1<<2;
                            if(active){
                                AU_V0+=AU_S5;AU_V0=AU_R(AU_V0);p=AU_R(AU_V0+4u);
                                AU_RA=0x800a9cb0u;AU_S0=p;AU_A0=AU_S2;AU_CALL(0x800aa600u,0x800a9ca8u);
                                AU_V1=AU_R(AU_S3);AU_S1=AU_R(AU_V1+4u);
                                p=AU_R(AU_S1+0x60u);uint32_t q=AU_R(AU_S0+0x60u);AU_V1=p;
                                AU_V1-=AU_V0;AU_A0=q;AU_V1=(int32_t)AU_V1<(int32_t)AU_A0;
                                if(AU_V1){
                                    AU_A0=0x800c0000u;p=AU_H(AU_A0+0x3d40u);AU_RA=0x800a9ce8u;AU_A0=p;
                                    AU_CALL(0x800aa600u,0x800a9ce0u);
                                    AU_V1=0x800c0000u;AU_V1=AU_H(AU_V1+0x3d40u);AU_V1<<=2;AU_V1+=AU_S5;
                                    AU_V1=AU_R(AU_V1);AU_V1=AU_R(AU_V1+4u);p=AU_R(AU_V1+0x60u);
                                    q=AU_R(AU_S1+0x60u);AU_V1=p;AU_V1-=AU_V0;AU_A0=q;
                                    AU_V1=(int32_t)AU_V1<(int32_t)AU_A0;
                                    if(AU_V1){
                                        AU_RA=0x800a9d2cu;AU_A0=AU_S2;AU_CALL(0x800aa650u,0x800a9d24u);
                                        AU_A0=0x800c0000u;p=AU_H(AU_A0+0x3d40u);AU_RA=0x800a9d3cu;AU_A0=p;AU_S0=AU_V0;
                                        AU_CALL(0x800aa650u,0x800a9d34u);
                                        AU_V1=AU_R(AU_S3);p=AU_R(AU_V1+4u);AU_A0=0x800c0000u;AU_V1=p;
                                        p=AU_H(AU_A0+0x3d40u);q=AU_R(AU_V1+0x5cu);AU_A0=p;
                                        p=AU_R(AU_V1+0x64u);AU_A1=q;AU_RA=0x800a9d60u;AU_A2=p;AU_A3=AU_S0+AU_V0;
                                        AU_CALL(0x800b10ecu,0x800a9d58u);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            AU_S2++;AU_V0=(int32_t)AU_S2<11;again=AU_V0!=0;AU_S3+=4u;
        }while(again);
        AU_A0=0x800c0000u;p=AU_H(AU_A0+0x3d40u);AU_RA=0x800a9d80u;AU_A0=p;AU_S1=0;
        AU_CALL(0x800a4cf8u,0x800a9d78u);
    }
    AU_S0=0x800d3368u;int again;
    do{
        AU_V0=AU_R(AU_S0);int present=AU_V0!=0;AU_S1++;
        if(present){
            AU_A1=0x800c3d0cu;AU_V0=0x800d0000u;p=AU_R(AU_V0-0x33a4u);
            AU_A2=AU_S4;AU_V0=p;AU_W(AU_SP+0x10u,AU_V0);p=AU_R(AU_S0);
            AU_RA=0x800a9dbcu;AU_A0=p;AU_A3=AU_S6;AU_CALL(0x800aaa20u,0x800a9db4u);
        }
        AU_V0=(int32_t)AU_S1<32;again=AU_V0!=0;AU_S0+=4u;
    }while(again);
    AU_V0=0x800c0000u;AU_V0=AU_B(AU_V0+0x3df8u);int extras=AU_V0!=0;AU_S1=0;
    if(extras){AU_A0=0x800c3d0cu;AU_A1=AU_S4;AU_A2=0;AU_RA=0x800a9df4u;AU_A3=AU_S6;AU_CALL(0x800b026cu,0x800a9decu);}
    AU_S0=0x800d3368u;
    do{
        AU_A0=AU_R(AU_S0);int present=AU_A0!=0;AU_S0+=4u;
        if(present){AU_V0=AU_B(AU_A0+0x5cu);AU_V0=AU_V0<255u;if(AU_V0)AU_CALL(0x800aab34u,0x800a9e20u);}
        AU_S1++;AU_V0=(int32_t)AU_S1<32;again=AU_V0!=0;
    }while(again);
    AU_S0=0x800d3368u;AU_RA=0x800a9e48u;AU_A0=AU_S0;AU_CALL(0x800a44c0u,0x800a9e40u);
    AU_A0=0x800d0000u;p=AU_R(AU_A0+0x2fc0u);AU_RA=0x800a9e58u;AU_A0=p;AU_CALL(0x80049f5cu,0x800a9e50u);
    AU_V0=0x800c0000u;AU_V0=AU_B(AU_V0+0x3b74u);
    if(AU_V0){
        AU_S1=0;
        do{
            AU_V0=AU_R(AU_S0);
            if(AU_V0){
                AU_V0=0x800c0000u;AU_V0=AU_H(AU_V0+0x3d14u);
                AU_V0=(uint32_t)((int32_t)AU_V0>>(AU_S1&31u));AU_V0&=1;
                if(AU_V0){
                    p=AU_B(AU_SP+0x20u);uint32_t q=AU_B(AU_SP+0x21u);AU_A0=p;
                    p=AU_B(AU_SP+0x22u);AU_A1=q;AU_A2=p;
                }else{
                    AU_V0=0x80060000u;AU_V0=AU_R(AU_V0+0x58c8u);
                    p=AU_B(AU_V0+0x474u);uint32_t q=AU_B(AU_V0+0x475u);AU_A0=p;
                    p=AU_B(AU_V0+0x476u);AU_A1=q;AU_A2=p;
                }
                AU_CALL(0x8004a0ecu,0x800a9ec8u);
                AU_V0=AU_R(AU_S0);AU_V0=AU_H(AU_V0+0x4au);AU_V0&=64u;
                int alternate=AU_V0!=0;AU_V0=1;AU_AT=0x80050000u;AU_W(AU_AT+0x104u,alternate?0:AU_V0);
                AU_V0=0x800d0000u;p=AU_R(AU_V0-0x33a4u);uint32_t q=AU_R(AU_SP+0x28u);AU_V0=p;
                AU_A1=AU_S7;AU_A2=q;AU_W(AU_SP+0x14u,AU_FP);AU_W(AU_SP+0x18u,AU_S6);AU_W(AU_SP+0x10u,AU_V0);
                p=AU_R(AU_S0);AU_RA=0x800a9f2cu;AU_A0=p;AU_A3=1;AU_CALL(0x8009f844u,0x800a9f24u);
                /* Clear only after a successful renderer return. */
                AU_AT=0x80050000u;AU_W(AU_AT+0x104u,0);
            }
            /* The draw pass intentionally visits slots 0 through 30. */
            AU_S1++;AU_V0=(int32_t)AU_S1<31;again=AU_V0!=0;AU_S0+=4u;
        }while(again);
    }
    AU_W(AU_SP+0x10u,AU_S6);AU_A0=0x800c3d04u;AU_A1=AU_S7;AU_A2=AU_S4;
    AU_RA=0x800a9f60u;AU_A3=AU_FP;AU_CALL(0x800a2fd8u,0x800a9f58u);
    p=AU_R(AU_SP+0x5cu);uint32_t q=AU_R(AU_SP+0x58u);AU_RA=p;
    p=AU_R(AU_SP+0x54u);AU_FP=q;q=AU_R(AU_SP+0x50u);AU_S7=p;
    p=AU_R(AU_SP+0x4cu);AU_S6=q;q=AU_R(AU_SP+0x48u);AU_S5=p;
    p=AU_R(AU_SP+0x44u);AU_S4=q;q=AU_R(AU_SP+0x40u);AU_S3=p;
    p=AU_R(AU_SP+0x3cu);AU_S2=q;q=AU_R(AU_SP+0x38u);AU_S1=p;
    AU_SP+=0x60u;AU_S0=q;
}
static int battle_actor_update_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a9a50u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ACTOR_UPDATE");if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="44c84dd358551240285c22dec8b58eeecb1a4869f13092b8b283ff6f2a60f3f2";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),1348u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_actor_update_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef AU_R
#undef AU_H
#undef AU_B
#undef AU_W
#undef AU_WH
#undef AU_WB
#undef AU_AT
#undef AU_V0
#undef AU_V1
#undef AU_A0
#undef AU_A1
#undef AU_A2
#undef AU_A3
#undef AU_S0
#undef AU_S1
#undef AU_S2
#undef AU_S3
#undef AU_S4
#undef AU_S5
#undef AU_S6
#undef AU_S7
#undef AU_SP
#undef AU_FP
#undef AU_RA
#undef AU_CALL
