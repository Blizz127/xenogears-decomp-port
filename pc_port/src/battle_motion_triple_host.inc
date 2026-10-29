/* Complete motion-triple updater, retail 800A1B50..800A1CF4.
 * Nodes retain their packed 124-byte layout and guest handle words. Reads,
 * short-circuit comparisons and dirty writes use the production data bus;
 * no original CPU instructions are fetched or decoded by this body. */
#define MP_R(a) battle_entry_read(entry,(a),4)
#define MP_H(a) battle_entry_read(entry,(a),2)
#define MP_S(a) ((uint32_t)(int32_t)(int16_t)MP_H(a))
#define MP_B(a) battle_entry_read(entry,(a),1)
#define MP_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MP_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MP_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MP_V0 (entry->cpu->gpr[2])
#define MP_V1 (entry->cpu->gpr[3])
#define MP_A0 (entry->cpu->gpr[4])
#define MP_A1 (entry->cpu->gpr[5])
#define MP_A2 (entry->cpu->gpr[6])
#define MP_A3 (entry->cpu->gpr[7])
#define MP_T0 (entry->cpu->gpr[8])
#define MP_T1 (entry->cpu->gpr[9])
#define MP_T2 (entry->cpu->gpr[10])
#define MP_T3 (entry->cpu->gpr[11])
#define MP_T4 (entry->cpu->gpr[12])
#define MP_T5 (entry->cpu->gpr[13])
#define MP_T6 (entry->cpu->gpr[14])
#define MP_T7 (entry->cpu->gpr[15])
#define MP_T8 (entry->cpu->gpr[24])
#define MP_T9 (entry->cpu->gpr[25])
#define MP_SP (entry->cpu->gpr[29])

static void battle_motion_triple_run(BattleEntryCall *entry)
{
    MP_SP-=8u;MP_T2=0;MP_T1=0;
    /* Each header load commits only after the next access succeeds. This
     * preserves the register prefix when that following access faults. */
    uint32_t p=MP_S(MP_A1+6u),q=MP_H(MP_A1+0xcu);MP_T0=p;
    p=MP_H(MP_A1+0xeu);MP_T5=q;
    q=MP_H(MP_A1+4u);MP_T9=p;MP_A2=q;
    MP_A1+=0x18u;
    if(!MP_T0) {
        MP_V1=MP_T5+1u;
        MP_V0=MP_V1<<1;MP_V0+=MP_V1;MP_V0<<=1;
        MP_A1+=MP_V0;
    }
    MP_V0=MP_H(MP_A0+0xau);MP_T3=MP_V0-1u;
    MP_V0=MP_T3&0xffffu;MP_T0=0;
    if(MP_V0) {
        MP_T8=MP_A2&1u;MP_T6=0xffu;MP_T4=1u;MP_T7=MP_A2&2u;
        MP_A0+=4u;
        do {
            MP_A0+=0x7cu;
            if(!MP_T8) {
                MP_V0=MP_T2&0xffffu;MP_V0=MP_V0<MP_T5;
                if(MP_V0) {
                    MP_A3=MP_S(MP_A1);MP_A1+=2u;
                    MP_A2=MP_S(MP_A1);MP_A1+=2u;
                    MP_V1=MP_S(MP_A1);MP_A1+=2u;
                    MP_V0=MP_S(MP_A0+0x50u);MP_T2++;
                    if(MP_V0!=MP_A3)goto rotation_changed;
                    MP_V0=MP_S(MP_A0+0x52u);
                    if(MP_V0!=MP_A2)goto rotation_changed;
                    MP_V0=MP_S(MP_A0+0x54u);
                    if(MP_V0==MP_V1)goto translation;
rotation_changed:
                    MP_V0=MP_R(MP_A0+0x6cu);
                    if(MP_V0) {
                        MP_V0=MP_B(MP_V0+3u);
                        if(MP_V0==MP_T6)goto translation;
                    }
                    MP_SH(MP_A0+0x50u,MP_A3);
                    MP_SH(MP_A0+0x52u,MP_A2);
                    MP_SH(MP_A0+0x54u,MP_V1);
                    MP_SB(MP_A0,MP_T4);MP_SB(MP_A0+1u,MP_T4);
                }
            }
translation:
            MP_V0=MP_T1&0xffffu;
            if(!MP_T7) {
                MP_V0=MP_V0<MP_T9;
                if(MP_V0) {
                    MP_A3=MP_S(MP_A1);MP_A1+=2u;
                    MP_A2=MP_S(MP_A1);MP_A1+=2u;
                    MP_V1=MP_S(MP_A1);MP_A1+=2u;
                    MP_V0=MP_R(MP_A0+0x58u);MP_T1++;
                    if(MP_V0!=MP_A3)goto translation_changed;
                    MP_V0=MP_R(MP_A0+0x5cu);
                    if(MP_V0!=MP_A2)goto translation_changed;
                    MP_V0=MP_R(MP_A0+0x60u);
                    if(MP_V0==MP_V1)goto next_node;
translation_changed:
                    MP_V0=MP_R(MP_A0+0x70u);
                    if(MP_V0) {
                        MP_V0=MP_B(MP_V0+3u);
                        if(MP_V0==MP_T6)goto next_node;
                    }
                    MP_W(MP_A0+0x58u,MP_A3);
                    MP_W(MP_A0+0x5cu,MP_A2);
                    MP_W(MP_A0+0x60u,MP_V1);
                    MP_SB(MP_A0,MP_T4);
                }
            }
next_node:
            MP_T0++;MP_V0=MP_T3&0xffffu;
            /* The loop branch's delay slot leaves this masked count in V0,
             * including the wrapped 65535-node case for a zero header. */
        } while((int32_t)MP_T0<(int32_t)MP_V0);
    }
    MP_SP+=8u;
}

static int battle_motion_triple_try_host(BattleMipsRuntime *runtime,
                                          PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a1b50u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MOTION_TRIPLE");
    if(control&&strcmp(control,"0")==0)return 0;
    static const char expected[]=
        "e12a4093ee078d91cb5fbfd8699e8ced3af26cd2f1fe848bacc52b1e57bef7d4";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),420u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||
           file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_motion_triple_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MP_R
#undef MP_H
#undef MP_S
#undef MP_B
#undef MP_W
#undef MP_SH
#undef MP_SB
#undef MP_V0
#undef MP_V1
#undef MP_A0
#undef MP_A1
#undef MP_A2
#undef MP_A3
#undef MP_T0
#undef MP_T1
#undef MP_T2
#undef MP_T3
#undef MP_T4
#undef MP_T5
#undef MP_T6
#undef MP_T7
#undef MP_T8
#undef MP_T9
#undef MP_SP
