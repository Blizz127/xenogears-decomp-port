/* Complete battle frame wrapper and frame tick. Fixed host control flow;
 * no guest instruction fetching or decoding. External callees retain their
 * existing conversion gates, and unaudited indirect callbacks refuse. */
#define BF_R32(a) battle_entry_read(entry,(a),4)
#define BF_R16(a) battle_entry_read(entry,(a),2)
#define BF_H(a) ((uint32_t)(int32_t)(int16_t)BF_R16(a))
#define BF_R8(a) battle_entry_read(entry,(a),1)
#define BF_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define BF_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define BF_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define BF_V0 (entry->cpu->gpr[2])
#define BF_V1 (entry->cpu->gpr[3])
#define BF_A0 (entry->cpu->gpr[4])
#define BF_A1 (entry->cpu->gpr[5])
#define BF_A2 (entry->cpu->gpr[6])
#define BF_A3 (entry->cpu->gpr[7])
#define BF_T0 (entry->cpu->gpr[8])
#define BF_T1 (entry->cpu->gpr[9])
#define BF_S0 (entry->cpu->gpr[16])
#define BF_S1 (entry->cpu->gpr[17])
#define BF_S2 (entry->cpu->gpr[18])
#define BF_S3 (entry->cpu->gpr[19])
#define BF_SP (entry->cpu->gpr[29])
#define BF_RA (entry->cpu->gpr[31])
#define BF_CALL(t,s) battle_frame_call(entry,(t),(s))

static void battle_frame_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    BF_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-frame] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"battle frame callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending battle frame callee return",target);
}
static void battle_frame_wrapper(BattleEntryCall *entry)
{
    BF_V0=0x80060000u;uint32_t p=BF_R32(BF_V0-0x6e84u);
    BF_SP-=0x18u;BF_V0=p;BF_W32(BF_SP+0x10u,BF_RA);
    p=BF_R32(BF_V0);BF_V0=UINT32_MAX;BF_V1=p;
    if(BF_V1!=BF_V0)BF_CALL(0x8028022cu,0x800716f8u);
    BF_CALL(0x800be790u,0x80071700u);
    BF_V0=0;p=BF_R32(BF_SP+0x10u);BF_SP+=0x18u;BF_RA=p;
}
static void battle_frame_tick(BattleEntryCall *entry)
{
    BF_V1=0x800c0000u;uint32_t p=BF_R32(BF_V1+0x37d0u);BF_SP-=0x28u;BF_V0=p;
    BF_W32(BF_SP+0x20u,BF_RA);BF_W32(BF_SP+0x1cu,BF_S3);
    BF_W32(BF_SP+0x18u,BF_S2);BF_W32(BF_SP+0x14u,BF_S1);BF_W32(BF_SP+0x10u,BF_S0);
    BF_V0++;BF_RA=0x800be7bcu;BF_W32(BF_V1+0x37d0u,BF_V0);BF_CALL(0x80019ca0u,0x800be7b4u);
    BF_A0=UINT32_MAX;BF_CALL(0x8004b54cu,0x800be7bcu);
    BF_V1=0x800c0000u;BF_A1=BF_V1+0x3eb0u;BF_A2=BF_A1+0xb70u;BF_A0=0x800d0000u;
    BF_V1=0x8000u;BF_S0=BF_A1+BF_V1;p=BF_R32(BF_S0+0xc50u);BF_S1=BF_A0+0x309cu;BF_V1=p;
    int alternate=BF_V1==BF_A2;BF_W32(BF_S1+0x48u,BF_V0);if(alternate)BF_A2=BF_A1+0x4be0u;
    BF_A0=BF_A2+0x70u;BF_A1=0x1000u;BF_W32(BF_S0+0xc50u,BF_A2);
    BF_RA=0x800be804u;BF_W32(BF_S0+0xc54u,BF_A0);BF_CALL(0x80044ad8u,0x800be7fcu);
    BF_V0=1;p=BF_R32(BF_S0+0xc84u);BF_S3=0x80010000u;BF_V1=p;
    BF_V0-=BF_V1;BF_W32(BF_S0+0xc84u,BF_V0);p=BF_R32(BF_S3);BF_S2=UINT32_MAX;BF_V0=p;
    if(BF_V0!=BF_S2){
        BF_CALL(0x800bebc4u,0x800be828u);
        /* The original BREAK is unconditional after this debug branch. */
        battle_entry_fail(entry,"battle frame debug BREAK",0x800be830u);
    }
    p=BF_R32(BF_S0+0xc84u);BF_RA=0x800be848u;BF_A0=p;BF_S1+=0x20u;BF_CALL(0x800250e0u,0x800be840u);
    BF_CALL(0x800bbab8u,0x800be848u);BF_CALL(0x800bb9d4u,0x800be850u);
    BF_A0=BF_S1;BF_CALL(0x80024ff4u,0x800be858u);
    p=BF_R32(BF_S0+0xc54u);BF_RA=0x800be86cu;BF_A0=p;BF_CALL(0x80024fe4u,0x800be864u);
    BF_V0=BF_R32(BF_S3);int font=BF_V0!=BF_S2;BF_A0=BF_S1;
    if(font){p=BF_R32(BF_S0+0xc54u);BF_RA=0x800be888u;BF_A0=p;BF_CALL(0x80037324u,0x800be880u);BF_A0=BF_S1;}
    BF_A1=0x800d0000u;BF_V0=0x80060000u;
    p=BF_R32(BF_V0-0x6a94u);uint32_t q=BF_R32(BF_S0+0xc84u);BF_A2=p;
    BF_RA=0x800be8a4u;BF_A3=q;BF_A1-=0x346cu;BF_CALL(0x800a9a50u,0x800be89cu);
    BF_T1=0x1f800000u;BF_T1|=0x3fcu;BF_T0=BF_T1;BF_W32(BF_T0,BF_SP);
    BF_T0-=4u;BF_SP=BF_T0;
    BF_CALL(0x8001d468u,0x800be8bcu);BF_CALL(0x8001c9f8u,0x800be8c4u);BF_CALL(0x8001c964u,0x800be8ccu);
    BF_V0=0x80060000u;BF_S0=BF_H(BF_V0-0x6b6cu);BF_S0--;
    int timers=BF_S0!=BF_S2;BF_S1=UINT32_MAX;
    if(timers)do{
        BF_S0--;BF_CALL(0x800bbab8u,0x800be8ecu);BF_CALL(0x8001c964u,0x800be8f4u);
    }while(BF_S0!=BF_S1);
    BF_SP+=4u;p=BF_R32(BF_SP);BF_RA=0x800be914u;BF_SP=p;BF_S0=0x80060000u;BF_CALL(0x80076544u,0x800be90cu);
    BF_A0=0;BF_CALL(0x8008a9c0u,0x800be914u);
    p=BF_R16(BF_S0-0x6b6cu);BF_V1=UINT32_MAX;BF_V0=p;BF_V0--;
    BF_W16(BF_S0-0x6b6cu,BF_V0);BF_V0=(uint32_t)(int32_t)(int16_t)BF_V0;
    int repeat=BF_V0!=BF_V1;BF_S1=BF_V1;
    if(repeat)do{
        BF_A0=1;BF_CALL(0x8008a9c0u,0x800be93cu);
        BF_V0=BF_R16(BF_S0-0x6b6cu);BF_V0--;BF_W16(BF_S0-0x6b6cu,BF_V0);
        BF_V0=(uint32_t)(int32_t)(int16_t)BF_V0;
    }while(BF_V0!=BF_S1);
    BF_A0=1;BF_CALL(0x8004b54cu,0x800be964u);
    BF_A0=0;BF_S0=0x800d0000u;BF_S0+=0x309cu;
    BF_RA=0x800be980u;BF_W32(BF_S0+0x40u,BF_V0);BF_CALL(0x800445d0u,0x800be978u);
    BF_A0=1;BF_CALL(0x8004b54cu,0x800be980u);
    BF_A0=UINT32_MAX;BF_RA=0x800be994u;BF_W32(BF_S0+0x44u,BF_V0);BF_CALL(0x8004b54cu,0x800be98cu);
    BF_S1=0x80060000u;BF_A1=0x80060000u;p=BF_R16(BF_S0+0x48u);q=BF_R16(BF_A1-0x6e68u);BF_V1=p;
    BF_V0-=BF_V1;BF_A0=q;BF_V0-=BF_A0;BF_W16(BF_S1-0x6b6cu,BF_V0);BF_V0<<=16;
    if((int32_t)BF_V0<0)BF_W16(BF_S1-0x6b6cu,0);
    BF_V0=BF_H(BF_S1-0x6b6cu);BF_V0=(int32_t)BF_V0<5;
    int clamp=BF_V0==0;BF_V0=0x800c0000u;
    if(clamp){BF_V0=4;BF_W16(BF_S1-0x6b6cu,BF_V0);BF_V0=0x800c0000u;}
    BF_V0+=0x3eb0u;BF_A0=0x8000u;p=BF_H(BF_S1-0x6b6cu);q=BF_R32(BF_A1-0x6e68u);BF_V1=p;
    BF_V0+=BF_A0;BF_A1=q;BF_V1+=BF_A1;BF_W32(BF_V0+0xdacu,BF_V1);
    int interval=BF_A1!=0;BF_A0=0;if(interval)BF_A0=BF_A1+1u;
    BF_S0=0x800c0000u;BF_CALL(0x8004b54cu,0x800bea08u);
    BF_S0+=0x3eb0u;BF_V0=0x8000u;BF_S0+=BF_V0;
    p=BF_R32(BF_S0+0xc50u);BF_RA=0x800bea28u;BF_A0=p;BF_A0+=0x5cu;BF_CALL(0x80044e9cu,0x800bea20u);
    p=BF_R32(BF_S0+0xc50u);BF_RA=0x800bea34u;BF_A0=p;BF_CALL(0x80044c44u,0x800bea2cu);
    BF_CALL(0x80025044u,0x800bea34u);
    p=BF_R32(BF_S0+0xc50u);BF_RA=0x800bea48u;BF_A0=p;BF_A0+=0x406cu;BF_CALL(0x80044bd0u,0x800bea40u);
    BF_CALL(0x800beb04u,0x800bea48u);
    BF_V0=0x800c0000u;p=BF_R32(BF_V0+0x37d0u);BF_V0=1;BF_V1=p;
    int outer=BF_V1==BF_V0;BF_V1=0x800c0000u;
    if(outer){
        BF_V0=0x800c0000u;BF_A0=BF_R32(BF_V0+0x3610u);int callback=BF_A0!=0;BF_V1=0x80060000u;
        if(callback){BF_V0=BF_R32(BF_A0+8u);uint32_t target=BF_V0;BF_CALL(target,0x800bea80u);BF_V1=0x80060000u;}
        BF_V0=BF_R16(BF_V1-0x6e4cu);int sound=BF_V0!=0;BF_A0=BF_V0;
        if(sound){BF_RA=0x800beaa4u;BF_W16(BF_V1-0x6e4cu,0);BF_CALL(0x800b8068u,0x800bea9cu);}
        BF_S0=0x800c0000u;BF_V0=BF_R8(BF_S0+0x37ccu);int loading=BF_V0!=0;BF_V1=0x800c0000u;
        if(loading){
            BF_CALL(0x800286ccu,0x800beab8u);int loaded=BF_V0==0;BF_V1=0x800c0000u;
            if(loaded){BF_RA=0x800bead0u;BF_W8(BF_S0+0x37ccu,0);BF_CALL(0x800b136cu,0x800beac8u);BF_V1=0x800c0000u;}
        }
    }
    BF_V0=BF_R32(BF_V1+0x37d0u);BF_V0--;BF_W32(BF_V1+0x37d0u,BF_V0);
    p=BF_R32(BF_SP+0x20u);q=BF_R32(BF_SP+0x1cu);BF_RA=p;
    p=BF_R32(BF_SP+0x18u);BF_S3=q;q=BF_R32(BF_SP+0x14u);BF_S2=p;
    p=BF_R32(BF_SP+0x10u);BF_S1=q;BF_SP+=0x28u;BF_S0=p;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_frame_bodies[]={
    {0x800716d8u,68u,"1cc5264c72a93353380fa5a79be7a6079d87d3b43ce42229ac0c6121c411bc42"},
    {0x800be790u,884u,"d1cc8dc513869017f5673e389702d6308aae81d1de3a7c82729f79bde6aa7c1f"},
};
static int battle_frame_target(uint32_t target)
{return target==0x800716d8u||target==0x800be790u;}
static int battle_frame_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_frame_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_FRAME");if(control&&strcmp(control,"0")==0)return 0;
    unsigned i=target==0x800716d8u?0:1;uint8_t digest[32];
    file1_digest(PSX_ADDR(target),battle_frame_bodies[i].bytes,digest);const char *expected=battle_frame_bodies[i].sha;
    for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(i==0)battle_frame_wrapper(&entry);else battle_frame_tick(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef BF_R32
#undef BF_R16
#undef BF_H
#undef BF_R8
#undef BF_W32
#undef BF_W16
#undef BF_W8
#undef BF_V0
#undef BF_V1
#undef BF_A0
#undef BF_A1
#undef BF_A2
#undef BF_A3
#undef BF_T0
#undef BF_T1
#undef BF_S0
#undef BF_S1
#undef BF_S2
#undef BF_S3
#undef BF_SP
#undef BF_RA
#undef BF_CALL
