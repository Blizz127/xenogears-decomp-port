/* Undo relocation in the original address domain. Passing a translated host
 * base to legacy C would subtract a different base from guest pointer words. */
#define MU_R(a) battle_entry_read(entry,(a),4)
#define MU_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MU_V0 (entry->cpu->gpr[2])
#define MU_V1 (entry->cpu->gpr[3])
#define MU_A0 (entry->cpu->gpr[4])
#define MU_A1 (entry->cpu->gpr[5])
#define MU_A2 (entry->cpu->gpr[6])
#define MU_A3 (entry->cpu->gpr[7])
#define MU_T0 (entry->cpu->gpr[8])
#define MU_T1 (entry->cpu->gpr[9])
#define MU_T2 (entry->cpu->gpr[10])
#define MU_T3 (entry->cpu->gpr[11])
#define MU_SP (entry->cpu->gpr[29])
static void battle_model_unrelocate_run(BattleEntryCall *entry)
{
    uint32_t p=MU_R(MU_A0+4u),q=MU_R(MU_A0);MU_T0=p;MU_V0=MU_T0&1u;MU_T2=q;
    int relocated=MU_V0!=0;MU_SP-=8u;
    if(relocated){
        MU_V0=0xfffffffeu;MU_V0=MU_T0&MU_V0;MU_W(MU_A0+4u,MU_V0);MU_T0=0;
        if((int32_t)MU_T2>0){
            MU_T3=0xffffffffu;MU_A3=MU_A0+0x2cu;
            do{
                p=MU_R(MU_A3-0x14u);q=MU_R(MU_A3-0xcu);MU_V0=p;MU_V0-=MU_A0;MU_V1=q;
                MU_W(MU_A3-0x14u,MU_V0);p=MU_R(MU_A3-0x10u);MU_V1-=MU_A0;MU_V0=p;
                MU_W(MU_A3-0xcu,MU_V1);MU_V0-=MU_A0;MU_W(MU_A3-0x10u,MU_V0);
                p=MU_R(MU_A3-8u);q=MU_R(MU_A3);MU_V0=p;MU_V0-=MU_A0;MU_V1=q;
                int sub=MU_V1!=0;MU_W(MU_A3-8u,MU_V0);
                if(sub){
                    MU_A2=MU_R(MU_V1);int items=MU_A2!=MU_T3;MU_V1+=4u;
                    if(items){
                        MU_T1=0xffffffffu;MU_V0=MU_A2<<1;MU_V0+=MU_A2;MU_V0<<=2;MU_A1=MU_V0+MU_V1;
                        int more;
                        do{
                            MU_A2--;p=MU_R(MU_A1+4u);q=MU_R(MU_A1+8u);MU_V0=p;MU_V0-=MU_A0;MU_V1=q;MU_V1-=MU_A0;
                            MU_W(MU_A1+4u,MU_V0);MU_W(MU_A1+8u,MU_V1);more=MU_A2!=MU_T1;MU_A1-=12u;
                        }while(more);
                    }
                    MU_V0=MU_R(MU_A3);MU_V0-=MU_A0;MU_W(MU_A3,MU_V0);
                }
                MU_T0++;MU_V0=(int32_t)MU_T0<(int32_t)MU_T2;MU_A3+=0x38u;
            }while(MU_V0);
        }
    }
    MU_V0=MU_T2;MU_SP+=8u;
}
static int battle_model_unrelocate_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8002c4bcu||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_UNRELOCATE");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image||image->size<offset+224u)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,224u,digest);
    const char *expected="c6345773d4df83161dadb3b0be7410d3f9867a92b56c95ca926833bd18add745";
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_model_unrelocate_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef MU_R
#undef MU_W
#undef MU_V0
#undef MU_V1
#undef MU_A0
#undef MU_A1
#undef MU_A2
#undef MU_A3
#undef MU_T0
#undef MU_T1
#undef MU_T2
#undef MU_T3
#undef MU_SP
