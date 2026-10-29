/* Complete motion-track and pool-slot release bodies. Packed addresses stay
 * guest words, including the unsigned handle/base difference. All data uses
 * the production bus; these bodies never fetch or decode CPU instructions. */
#define MT_R(a) battle_entry_read(entry,(a),4)
#define MT_H(a) battle_entry_read(entry,(a),2)
#define MT_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MT_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MT_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MT_V0 (entry->cpu->gpr[2])
#define MT_V1 (entry->cpu->gpr[3])
#define MT_A0 (entry->cpu->gpr[4])
#define MT_A1 (entry->cpu->gpr[5])
#define MT_A2 (entry->cpu->gpr[6])
#define MT_A3 (entry->cpu->gpr[7])
#define MT_S0 (entry->cpu->gpr[16])
#define MT_S1 (entry->cpu->gpr[17])
#define MT_S2 (entry->cpu->gpr[18])
#define MT_SP (entry->cpu->gpr[29])
#define MT_RA (entry->cpu->gpr[31])

static void battle_motion_track_call(BattleEntryCall *entry,uint32_t site)
{
    MT_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x800a23e8u);
    int rc=PcPortMipsRun(&callee,0x800a23e8u,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-motion-track] callee 0x800a23e8 from 0x%08x: %s\n",site,callee.error);
        battle_entry_fail(entry,"motion-track callee stopped",0x800a23e8u);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending motion-track callee return",0x800a23e8u);
}

static void battle_motion_track_release_run(BattleEntryCall *entry)
{
    MT_SP-=0x20u;
    MT_W(MT_SP+0x18u,MT_S2);MT_S2=MT_A0;
    MT_W(MT_SP+0x10u,MT_S0);MT_S0=MT_A1;
    MT_W(MT_SP+0x1cu,MT_RA);MT_W(MT_SP+0x14u,MT_S1);
    MT_V0=MT_H(MT_S0+0xau);
    MT_V0=(int32_t)MT_A2<(int32_t)MT_V0;
    MT_S1=MT_A3;
    if(MT_V0) {
        MT_V0=MT_A2<<5;MT_V0-=MT_A2;MT_V0<<=2;MT_S0+=MT_V0;
        MT_A1=MT_R(MT_S0+0x70u);MT_V0=MT_S1&1u;
        if(MT_A1&&MT_V0) {
            battle_motion_track_call(entry,0x800a21c4u);
            MT_W(MT_S0+0x70u,0);
        }
        MT_A1=MT_R(MT_S0+0x74u);MT_V0=MT_S1&2u;
        if(MT_A1&&MT_V0) {
            MT_A0=MT_S2;battle_motion_track_call(entry,0x800a21e8u);
            MT_W(MT_S0+0x74u,0);
        }
        MT_A1=MT_R(MT_S0+0x78u);MT_V0=MT_S1&4u;
        if(MT_A1&&MT_V0) {
            MT_A0=MT_S2;battle_motion_track_call(entry,0x800a220cu);
            MT_W(MT_S0+0x78u,0);
        }
    }
    /* Reads and register commits follow the original load delays. SP and
     * saved-register values are live after each nested child. */
    uint32_t p=MT_R(MT_SP+0x1cu),q=MT_R(MT_SP+0x18u);MT_RA=p;
    p=MT_R(MT_SP+0x14u);MT_S2=q;
    q=MT_R(MT_SP+0x10u);MT_S1=p;MT_SP+=0x20u;MT_S0=q;
}

static void battle_motion_slot_release_run(BattleEntryCall *entry)
{
    MT_V1=0xcccc0000u;
    if(!MT_A1) {MT_V0=0xffffffffu;return;}
    uint32_t base=MT_R(MT_A0);MT_V1|=0xcccdu;
    MT_V0=MT_A1-base;
    uint64_t product=(uint64_t)MT_V0*MT_V1;
    entry->cpu->hi=(uint32_t)(product>>32);entry->cpu->lo=(uint32_t)product;
    uint32_t first_free=MT_H(MT_A0+4u);
    MT_V1=entry->cpu->hi;MT_V0=first_free;MT_V1>>=4;
    MT_V0=(int32_t)MT_V1<(int32_t)MT_V0;
    if(MT_V0)MT_SH(MT_A0+4u,MT_V1);
    MT_SB(MT_A1,0);MT_V0=MT_V1;
}

static int battle_motion_track_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x800a216cu,200u,"3f1ca115989d17605c9d3c2a28007437f3dcf00c06f61e2a0c941bfcea4a37d9"},
        {0x800a23e8u,76u,"fe43ec9555086fac2eb4fe394f84f3e8a6ca57ee8656260b7d4f0e47aef9cbfa"}
    };
    unsigned index=0;while(index<2u&&bodies[index].target!=target)index++;
    if(index==2u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MOTION_TRACK");
    if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800a216cu)battle_motion_track_release_run(&entry);
    else battle_motion_slot_release_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MT_R
#undef MT_H
#undef MT_W
#undef MT_SH
#undef MT_SB
#undef MT_V0
#undef MT_V1
#undef MT_A0
#undef MT_A1
#undef MT_A2
#undef MT_A3
#undef MT_S0
#undef MT_S1
#undef MT_S2
#undef MT_SP
#undef MT_RA
