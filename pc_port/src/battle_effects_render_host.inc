/* Complete effect-render coordinator with fixed native control flow.
 * The caller retains packed guest pointers and both normal/scratch stacks. */
#define ER_R32(a) battle_entry_read(entry,(a),4)
#define ER_R8(a) battle_entry_read(entry,(a),1)
#define ER_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define ER_V0 (entry->cpu->gpr[2])
#define ER_V1 (entry->cpu->gpr[3])
#define ER_A0 (entry->cpu->gpr[4])
#define ER_A1 (entry->cpu->gpr[5])
#define ER_A2 (entry->cpu->gpr[6])
#define ER_A3 (entry->cpu->gpr[7])
#define ER_T0 (entry->cpu->gpr[8])
#define ER_T1 (entry->cpu->gpr[9])
#define ER_S0 (entry->cpu->gpr[16])
#define ER_S1 (entry->cpu->gpr[17])
#define ER_S2 (entry->cpu->gpr[18])
#define ER_S3 (entry->cpu->gpr[19])
#define ER_S4 (entry->cpu->gpr[20])
#define ER_S5 (entry->cpu->gpr[21])
#define ER_S6 (entry->cpu->gpr[22])
#define ER_S7 (entry->cpu->gpr[23])
#define ER_SP (entry->cpu->gpr[29])
#define ER_FP (entry->cpu->gpr[30])
#define ER_RA (entry->cpu->gpr[31])
#define ER_CALL(t,s) battle_effects_render_call(entry,(t),(s))

static void battle_effects_render_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    ER_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-effects-render] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"effects render callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending effects render callee return",target);
}
static void battle_effects_render_run(BattleEntryCall *entry)
{
    ER_SP-=0x58u;ER_W32(ER_SP+0x44u,ER_S5);
    uint32_t p=ER_R32(ER_SP+0x68u);ER_W32(ER_SP+0x48u,ER_S6);ER_S5=p;
    p=ER_R32(ER_SP+0x6cu);ER_W32(ER_SP+0x4cu,ER_S7);ER_S6=p;
    p=ER_R32(ER_SP+0x70u);ER_W32(ER_SP+0x50u,ER_FP);ER_S7=p;
    p=ER_R32(ER_SP+0x74u);ER_W32(ER_SP+0x3cu,ER_S3);ER_FP=p;
    ER_S3=ER_A0;ER_W32(ER_SP+0x38u,ER_S2);ER_S2=ER_A1;
    ER_W32(ER_SP+0x40u,ER_S4);ER_S4=ER_A3;ER_W32(ER_SP+0x54u,ER_RA);
    ER_W32(ER_SP+0x34u,ER_S1);ER_W32(ER_SP+0x30u,ER_S0);
    int light=ER_S2!=0;ER_W32(ER_SP+0x20u,ER_A2);
    if(light){ER_A0=ER_S2;ER_CALL(0x80049f2cu,0x800a46a4u);}
    ER_V0=0x800d0000u;ER_V0=ER_R32(ER_V0+0x33e4u);
    p=ER_R8(ER_V0+0x10eu);uint32_t q=ER_R32(ER_V0+0x118u);ER_V1=p;
    int records=(int32_t)ER_V1>0;ER_S1=q;ER_S0=0;
    while(records){
        ER_A1=0x800d0000u;p=ER_R32(ER_A1-0x33a4u);ER_RA=0x800a46d8u;ER_A1=p;ER_A0=ER_S1;
        ER_CALL(0x800a3e98u,0x800a46d0u);ER_V0=0x800d0000u;
        p=ER_R32(ER_V0+0x33e4u);ER_S0++;ER_V0=p;ER_V0=ER_R8(ER_V0+0x10eu);
        ER_V0=(int32_t)ER_S0<(int32_t)ER_V0;records=ER_V0!=0;ER_S1+=0x30u;
    }
    ER_T1=0x1f8003fcu;ER_T0=ER_T1;ER_W32(ER_T0,ER_SP);ER_T0-=4u;ER_SP=ER_T0;
    ER_CALL(0x800a6ae8u,0x800a4710u);
    ER_SP+=4u;p=ER_R32(ER_SP);ER_S0=0;ER_SP=p;ER_S1=0x800c0000u;ER_S1+=0x3da0u;
    int again;
    do{
        ER_A0=ER_S1;ER_CALL(0x80027eacu,0x800a472cu);ER_S0++;
        ER_V0=(int32_t)ER_S0<2;again=ER_V0!=0;ER_S1+=0x18u;
    }while(again);
    ER_A1=0x800c0000u;ER_A1=ER_R32(ER_A1+0x3e38u);int models=ER_A1!=0;ER_S0=0;
    if(models){
        ER_A2=ER_S3;ER_A0=0x800c0000u;p=ER_R32(ER_A0+0x3e48u);q=ER_R32(ER_SP+0x20u);ER_A0=p;
        ER_A3=ER_S2;ER_T1=q;ER_W32(ER_SP+0x14u,ER_S4);ER_W32(ER_SP+0x18u,ER_S5);ER_W32(ER_SP+0x1cu,ER_FP);
        ER_RA=0x800a4780u;ER_W32(ER_SP+0x10u,ER_T1);ER_CALL(0x800a48ecu,0x800a4778u);
    }
    ER_V0=ER_FP<<2;ER_V0=ER_S4+ER_V0;ER_S2=ER_V0-4u;ER_S1=0x800c0000u;ER_S1+=0x3d50u;ER_A1=ER_S6;
    do{
        ER_A2=ER_S7;ER_A3=ER_S3;ER_W32(ER_SP+0x10u,ER_S2);ER_W32(ER_SP+0x14u,ER_S5);
        p=ER_R32(ER_S1);ER_S1+=4u;ER_A0=p;ER_RA=0x800a47b8u;ER_S0++;
        ER_CALL(0x800273c4u,0x800a47b0u);ER_V0=(int32_t)ER_S0<2;again=ER_V0!=0;ER_A1=ER_S6;
    }while(again);
    ER_A2=ER_S7;ER_A3=ER_S3;ER_V0=ER_FP<<2;ER_V0=ER_S4+ER_V0;ER_A0=0x800c0000u;
    p=ER_R32(ER_A0+0x3ea0u);ER_V0-=4u;ER_A0=p;ER_W32(ER_SP+0x10u,ER_V0);
    ER_RA=0x800a47ecu;ER_W32(ER_SP+0x14u,ER_S5);ER_CALL(0x800a4db8u,0x800a47e4u);
    p=ER_R32(ER_SP+0x54u);q=ER_R32(ER_SP+0x50u);ER_RA=p;
    p=ER_R32(ER_SP+0x4cu);ER_FP=q;q=ER_R32(ER_SP+0x48u);ER_S7=p;
    p=ER_R32(ER_SP+0x44u);ER_S6=q;q=ER_R32(ER_SP+0x40u);ER_S5=p;
    p=ER_R32(ER_SP+0x3cu);ER_S4=q;q=ER_R32(ER_SP+0x38u);ER_S3=p;
    p=ER_R32(ER_SP+0x34u);ER_S2=q;q=ER_R32(ER_SP+0x30u);ER_S1=p;
    ER_SP+=0x58u;ER_S0=q;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_effects_render_bodies[]={
    {0x800a4654u,460u,"7e2d3aedd3e7437fd8dea61a4b6d9ddcf566ffad2c49e8195228699032ca6392"},
};
static int battle_effects_render_target(uint32_t target)
{return target==0x800a4654u;}
static int battle_effects_render_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_effects_render_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECTS_RENDER");if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),battle_effects_render_bodies[0].bytes,digest);
    const char *expected=battle_effects_render_bodies[0].sha;
    for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_effects_render_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef ER_R32
#undef ER_R8
#undef ER_W32
#undef ER_V0
#undef ER_V1
#undef ER_A0
#undef ER_A1
#undef ER_A2
#undef ER_A3
#undef ER_T0
#undef ER_T1
#undef ER_S0
#undef ER_S1
#undef ER_S2
#undef ER_S3
#undef ER_S4
#undef ER_S5
#undef ER_S6
#undef ER_S7
#undef ER_SP
#undef ER_FP
#undef ER_RA
#undef ER_CALL
