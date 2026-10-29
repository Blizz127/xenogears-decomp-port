/* Model-list cleanup and optional secondary-buffer release. Pointer slots
 * remain 32-bit bus addresses; list counts, pointers and flags are read live.
 * Identity for the main-executable helper uses the verified retail cache. */
#define MR_R(a) battle_entry_read(entry,(a),4)
#define MR_H(a) battle_entry_read(entry,(a),2)
#define MR_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MR_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MR_V0 (entry->cpu->gpr[2])
#define MR_V1 (entry->cpu->gpr[3])
#define MR_A0 (entry->cpu->gpr[4])
#define MR_A1 (entry->cpu->gpr[5])
#define MR_S0 (entry->cpu->gpr[16])
#define MR_S1 (entry->cpu->gpr[17])
#define MR_S2 (entry->cpu->gpr[18])
#define MR_SP (entry->cpu->gpr[29])
#define MR_RA (entry->cpu->gpr[31])

static void battle_model_release_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    MR_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-model-release] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"model-release callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending model-release callee return load",target);
}

static void battle_model_secondary_release_run(BattleEntryCall *entry)
{
    MR_SP-=0x18u;uint32_t sp=MR_SP;
    MR_W(sp+0x10u,MR_S0);MR_S0=MR_A0;MR_W(sp+0x14u,MR_RA);
    MR_V0=MR_H(MR_S0);MR_V0&=1u;
    if(MR_V0) {
        MR_A0=MR_R(MR_S0+0x18u);
        battle_model_release_call(entry,0x800320e8u,0x8002cbe4u);
        MR_V0=MR_H(MR_S0);MR_V0&=0xfffeu;MR_SH(MR_S0,MR_V0);
    }
    uint32_t ra=MR_R(sp+0x14u),s0=MR_R(sp+0x10u);MR_RA=ra;
    MR_SP=sp+0x18u;MR_S0=s0;
}

static void battle_model_list_release_run(BattleEntryCall *entry)
{
    MR_SP-=0x28u;uint32_t sp=MR_SP;
    MR_W(sp+0x1cu,MR_S1);MR_S1=MR_A0;MR_W(sp+0x20u,MR_S2);MR_S2=MR_A1;
    MR_W(sp+0x24u,MR_RA);MR_W(sp+0x18u,MR_S0);
    if(MR_S1) {
        MR_V0=MR_R(MR_S1+4u);MR_S0=0;
        if(MR_V0) {
            do {
                MR_V1=MR_R(MR_S1);MR_V0=MR_S0<<2;
                if(MR_V1) {
                    MR_V0+=MR_V1;MR_A0=MR_R(MR_V0);
                    if(MR_A0&&MR_S2)
                        battle_model_release_call(entry,0x8002cbbcu,0x8009f7f0u);
                }
                uint32_t count=MR_R(MR_S1+4u);MR_S0++;MR_V0=MR_S0<count;
            } while(MR_V0);
        }
        MR_A0=MR_R(MR_S1);
        if(MR_A0) {
            battle_model_release_call(entry,0x800320e8u,0x8009f81cu);
            MR_W(MR_S1,0);
        }
    }
    uint32_t ra=MR_R(sp+0x24u),s2=MR_R(sp+0x20u);MR_RA=ra;
    uint32_t s1=MR_R(sp+0x1cu);MR_S2=s2;
    uint32_t s0=MR_R(sp+0x18u);MR_S1=s1;MR_SP=sp+0x28u;MR_S0=s0;
}

static int battle_model_release_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x8009f794u,176u,"e5993739ad3629bfbd3c86a337b5e52df6444b5ed0b841ac62608d62fbd3ce2d"},
        {0x8002cbbcu,84u,"068f185d0c08afc9cc57a091b7fc33cb587688f62a3fc4d159daa4b350b25ee0"}
    };
    unsigned index=0;while(index<sizeof(bodies)/sizeof(bodies[0])&&bodies[index].target!=target)index++;
    if(index==sizeof(bodies)/sizeof(bodies[0])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_RELEASE");if(control&&strcmp(control,"0")==0)return 0;
    const uint8_t *code=PSX_ADDR(target);uint32_t bytes=bodies[index].bytes;
    if(target<BATTLE_BASE) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||!image->bytes||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8009f794u)battle_model_list_release_run(&entry);
    else battle_model_secondary_release_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MR_R
#undef MR_H
#undef MR_W
#undef MR_SH
#undef MR_V0
#undef MR_V1
#undef MR_A0
#undef MR_A1
#undef MR_S0
#undef MR_S1
#undef MR_S2
#undef MR_SP
#undef MR_RA
