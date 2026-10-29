/* Packed model relocation (8002C3E8) and pointer-list construction (8009EBA8).
 * All offsets, entries and additions retain their original 32-bit domain.
 * The structured bodies never fetch or execute original CPU instructions. */
#include "retail_data.h"
#define ML_R(a) battle_entry_read(entry,(a),4)
#define ML_W(a,v) battle_entry_write(entry,(a),4,(v))
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
#define ML_S0 (entry->cpu->gpr[16])
#define ML_S1 (entry->cpu->gpr[17])
#define ML_S2 (entry->cpu->gpr[18])
#define ML_SP (entry->cpu->gpr[29])
#define ML_RA (entry->cpu->gpr[31])

static void battle_model_relocate_run(BattleEntryCall *entry)
{
    /* Consecutive loads commit only after the next access succeeds. Keep
     * their ordering when a later read or store faults partway through. */
    uint32_t flags=ML_R(ML_A0+4u),count=ML_R(ML_A0);
    ML_T0=flags;ML_V0=ML_T0&1u;ML_T2=count;ML_SP-=8u;
    if(!ML_V0) {
        ML_V0=ML_T0|1u;ML_W(ML_A0+4u,ML_V0);ML_T0=0;
        if((int32_t)ML_T2>0) {
            ML_T3=UINT32_MAX;ML_A3=ML_A0+0x2cu;
            do {
                uint32_t first=ML_R(ML_A3-0x14u),third=ML_R(ML_A3-0xcu);
                ML_V0=first+ML_A0;ML_V1=third;ML_W(ML_A3-0x14u,ML_V0);
                uint32_t second=ML_R(ML_A3-0x10u);
                ML_V1+=ML_A0;ML_V0=second;ML_W(ML_A3-0xcu,ML_V1);
                ML_V0+=ML_A0;ML_W(ML_A3-0x10u,ML_V0);
                uint32_t fourth=ML_R(ML_A3-8u),sub=ML_R(ML_A3);
                ML_V0=fourth+ML_A0;ML_V1=sub;ML_W(ML_A3-8u,ML_V0);
                if(ML_V1) {
                    ML_V0=ML_V1+ML_A0;ML_W(ML_A3,ML_V0);
                    ML_A2=ML_R(ML_V0);ML_V1=ML_V0+4u;
                    if(ML_A2!=ML_T3) {
                        ML_T1=UINT32_MAX;ML_V0=((ML_A2<<1)+ML_A2)<<2;
                        ML_A1=ML_V0+ML_V1;
                        /* The header is the last index: zero visits one
                         * record, -1 skips. Unsigned arithmetic also retains
                         * the original behavior for other negative words. */
                        do {
                            ML_A2--;
                            uint32_t left=ML_R(ML_A1+4u),right=ML_R(ML_A1+8u);
                            ML_V0=left+ML_A0;ML_V1=right+ML_A0;
                            ML_W(ML_A1+4u,ML_V0);ML_W(ML_A1+8u,ML_V1);
                            ML_A1-=12u;
                        } while(ML_A2!=ML_T1);
                    }
                }
                ML_T0++;ML_V0=(int32_t)ML_T0<(int32_t)ML_T2;ML_A3+=0x38u;
            } while(ML_V0);
        }
    }
    ML_V0=ML_T2;ML_SP+=8u;
}

static void battle_model_list_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    ML_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    /* A partially completed relocator owns visible register/memory changes
     * even when it refuses. Propagate them before terminating the caller. */
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-model-list] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"model-list callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending model-list callee return load",target);
}

static void battle_model_list_run(BattleEntryCall *entry)
{
    ML_SP-=0x28u;uint32_t sp=ML_SP;
    ML_W(sp+0x20u,ML_S2);ML_S2=ML_A0;ML_W(sp+0x1cu,ML_S1);ML_S1=ML_A1;
    ML_A0=4;ML_A1=0;ML_W(sp+0x24u,ML_RA);ML_RA=0x8009ebd0u;ML_W(sp+0x18u,ML_S0);
    battle_model_list_call(entry,0x80032498u,0x8009ebc8u);
    ML_A0=ML_S2;battle_model_list_call(entry,0x8002c3e8u,0x8009ebd0u);
    ML_S0=ML_V0;ML_A0=ML_S0<<2;ML_A1=0;
    battle_model_list_call(entry,0x80031bdcu,0x8009ebe0u);
    ML_W(ML_S1,ML_V0);ML_V0=ML_R(ML_S1);ML_W(ML_S1+4u,ML_S0);
    if(ML_V0) {
        ML_A1=0;
        if(ML_S0) {
            ML_A2=0x10u;
            do {
                ML_A0=ML_S2+ML_A2;ML_V0=ML_A1<<2;
                ML_V1=ML_R(ML_S1);ML_A1++;ML_V0+=ML_V1;ML_W(ML_V0,ML_A0);
                ML_V0=ML_A1<ML_S0;ML_A2+=0x38u;
            } while(ML_V0);
        }
    }
    ML_V0=ML_S1;
    /* Keep load/commit ordering across an aliased or faulting saved frame. */
    uint32_t ra=ML_R(sp+0x24u),s2=ML_R(sp+0x20u);ML_RA=ra;
    uint32_t s1=ML_R(sp+0x1cu);ML_S2=s2;
    uint32_t s0=ML_R(sp+0x18u);ML_S1=s1;ML_SP=sp+0x28u;ML_S0=s0;
}

static int battle_model_list_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if((target!=0x8009eba8u&&target!=0x8002c3e8u)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_LIST");if(control&&strcmp(control,"0")==0)return 0;
    uint32_t bytes=target==0x8009eba8u?164u:212u;
    const char *sha=target==0x8009eba8u?
        "ed37d38e4361912726b17fd6aeb8902f96e13650ba9caf105aa5347b7aa4e971":
        "369298efec3d5f839491870269ab85363b74e9eba9f6faa5a95d78a776ea4e17";
    const uint8_t *code=PSX_ADDR(target);
    if(target==0x8002c3e8u) {
        /* Main-executable text is intentionally absent from game RAM.
         * Consult the verified user-asset cache for identity only. */
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(target);
        if(image==NULL||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[i*2]||file1_hex_digit(digest[i]&15u)!=sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8009eba8u)battle_model_list_run(&entry);else battle_model_relocate_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef ML_R
#undef ML_W
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
#undef ML_S0
#undef ML_S1
#undef ML_S2
#undef ML_SP
#undef ML_RA
