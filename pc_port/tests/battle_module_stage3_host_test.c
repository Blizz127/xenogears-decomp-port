/* Original six stage-three bodies plus dispatcher versus native dispatch.
 * External game services are controlled models. The SDK leg uses the actual
 * atlas getter and PsyCross packet setters with typed game-service models. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

extern void func_80026338(uint8_t *,int32_t,uint32_t *,int32_t *,int32_t *,int32_t *,int32_t *,int32_t *);
static _Alignas(4) uint8_t stage_host[0x30000],expected_host[sizeof(stage_host)];
static const uint32_t stage_entries[]={0x801e5924,0x801e5ee8,0x801e5d2c,0x801e5e78,0x801e6290,0x801e62b8,0x801e5840};
static const unsigned stage_sizes[]={1032,936,332,112,40,40,228};
static int actual_sdk;
static unsigned service_calls;
static uint8_t clut_code[24];
typedef struct StageTrace {
    uint32_t target,ra,sp,saved[9],args[8],hi,lo;
    uint8_t frame[0x100],ram_hash[32],shared_hash[32],pad_hash[32],host_hash[32];
} StageTrace;
static StageTrace stage_traces[1024];
static uint32_t menu_owner(unsigned bank)
{
    return variant%3==2 ? (uint32_t)(uintptr_t)stage_host+bank*0x10000u :
        (variant%3==1 ? 0xa0100000u : 0x80100000u)+bank*0x20000u;
}
static uint32_t state_owner(unsigned bank)
{
    const uint32_t owners[]={0x80150000,0xa0150000,0x1f800800,(uint32_t)(uintptr_t)stage_host+0x24000u};
    return owners[(variant/4)%4]+bank*0x200u;
}
static void stage_trace(PcPortMipsCpu *cpu,uint32_t target,unsigned argc)
{
    if (target==running_entry && cpu->gpr[31]==BATTLE_HALT_PC) return;
    if (calls>=1024) {fail("stage3 trace capacity",calls);exit(2);}
    StageTrace row={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
    memcpy(row.saved,cpu->gpr+16,8*sizeof(uint32_t));row.saved[8]=cpu->gpr[30];
    for(unsigned i=0;i<argc;i++) row.args[i]=i<4 ? cpu->gpr[4+i] : read_at(cpu->gpr[29]+0x10u+(i-4)*4u,4);
    memcpy(row.frame,resolve_memory(&rt,frame_base,sizeof(row.frame),0),sizeof(row.frame));
    assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram_hash));assert(SHA256(shared,sizeof(shared),row.shared_hash));
    assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad_hash));assert(SHA256(stage_host,sizeof(stage_host),row.host_hash));
    if(compare && memcmp(&row,&stage_traces[calls],sizeof(row))) {
        StageTrace *want=&stage_traces[calls];
        if(row.target!=want->target) fail("stage3 target",want->target);
        if(row.ra!=want->ra || row.sp!=want->sp) fail("stage3 RA/SP",row.ra);
        for(unsigned i=0;i<9;i++) if(row.saved[i]!=want->saved[i]) fail("stage3 saved register",i);
        for(unsigned i=0;i<8;i++) if(row.args[i]!=want->args[i]) fail("stage3 argument",i);
        if(memcmp(row.frame,want->frame,sizeof(row.frame))) fail("stage3 frame",frame_base);
        fail("stage3 call/memory",target);
    }
    if(!compare) stage_traces[calls]=row;
    calls++;
}

/* Controlled, observable external game-service effects; not a claim that the
 * original glyph renderer, allocator or text renderer is executed here. */
static uint32_t game_effect(uint32_t target,const uint32_t *a)
{
    if(target==0x80076a10u || target==0x80076a6cu) {
        write_at(a[1],4,a[0]^a[2]^(a[3]<<16));
        return ((variant>>3)+a[0])%3;
    }
    if(target==0x80076c34u) {write_at(a[0]+4,1,0x40);write_at(a[0]+0x16,2,read_at(a[0]+0x16,2)|0x40);return 0;}
    if(target==0x8008ac00u) return 0x80170000u+(service_calls%16)*0x100u;
    if(target==0x800338d8u) return 0x80180000u+a[0]*0x20u;
    assert(target==0x80034eacu);
    write_at(a[1],4,read_at(a[0],4)^a[2]);return 0x87654321u+service_calls;
}
static int32_t typed_glyph(int32_t index,uint8_t *p,int16_t x,int16_t y)
{
    PcPortMipsCpu *cpu=rt.bridge_cpu;assert(cpu);
    assert(p==module_memory_pointer(&rt,cpu->gpr[5],1,1));
    assert(index==(int32_t)cpu->gpr[4] && x==(int16_t)cpu->gpr[6] && y==(int16_t)cpu->gpr[7]);
    uint32_t a[]={cpu->gpr[4],cpu->gpr[5],(uint32_t)(int32_t)x,(uint32_t)(int32_t)y};
    return (int32_t)game_effect(0x80076a10u,a);
}
static void typed_style(uint8_t *p)
{
    assert(p==module_memory_pointer(&rt,rt.bridge_cpu->gpr[4],40,1));
    game_effect(0x80076c34u,rt.bridge_cpu->gpr+4);
}
static void *typed_allocate(int32_t size)
{
    assert(size==(int32_t)rt.bridge_cpu->gpr[4]);
    return PSX_ADDR(game_effect(0x8008ac00u,rt.bridge_cpu->gpr+4));
}
static void *typed_string(int32_t index)
{
    assert(index==(int32_t)rt.bridge_cpu->gpr[4]);
    return PSX_ADDR(game_effect(0x800338d8u,rt.bridge_cpu->gpr+4));
}
static int32_t typed_render(void *text,void *buffer,int32_t height,int32_t flag)
{
    PcPortMipsCpu *cpu=rt.bridge_cpu;assert(text==module_memory_pointer(&rt,cpu->gpr[4],1,0));
    assert(buffer==module_memory_pointer(&rt,cpu->gpr[5],1,1));
    assert(height==(int32_t)cpu->gpr[6] && flag==(int32_t)cpu->gpr[7]);
    return (int32_t)game_effect(0x80034eacu,cpu->gpr+4);
}
/* Exact C signatures make Clang's function sanitizer inspect the bridge,
 * while the actual packet setter bodies remain in the linked SDK object. */
static void typed_gt4(POLY_GT4 *p) {SetPolyGT4(p);}
static void typed_g4(POLY_G4 *p) {SetPolyG4(p);}
static void typed_f4(POLY_F4 *p) {SetPolyF4(p);}
static void typed_line(LINE_F2 *p) {SetLineF2(p);}
static void typed_semi(void *p,int v) {SetSemiTrans(p,v);}
static void typed_shade(void *p,int v) {SetShadeTex(p,v);}
static void typed_draw(DR_MODE *p,int dfe,int dtd,int page,RECT16 *r) {SetDrawMode(p,dfe,dtd,page,r);}
static u_short typed_page(int a,int b,int x,int y) {return GetTPage(a,b,x,y);}
static u_short unused_clut(int x,int y) {(void)x;(void)y;abort();}

static int stage_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if(stop_target && target-stop_target<12u) return 0;
    for(unsigned i=0;i<7;i++) {
        if(target==stage_entries[i]) {
            stage_trace(cpu,target,i==6 ? 1:0);
            int rc=candidate ? runtime_bridge(opaque,cpu,target):0;
            if(rc==1 && pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;}
            return rc;
        }
        if(target-stage_entries[i]<stage_sizes[i]) return 0;
    }
    unsigned argc;int game=0;
    switch(target) {
    case 0x80026338u:argc=8;break;
    case 0x80043a58u:case 0x80043c24u:case 0x80043bfcu:argc=2;break;
    case 0x80043a1cu:argc=4;break;
    case 0x800454dcu:argc=5;break;
    case 0x80043cd8u:case 0x80043cc4u:case 0x80043c9cu:case 0x80043d78u:argc=1;break;
    case 0x80076a6cu:case 0x80076a10u:case 0x80034eacu:argc=4;game=1;break;
    case 0x80076c34u:case 0x8008ac00u:case 0x800338d8u:argc=1;game=1;break;
    default:fail("stage3 unexpected callee",target);return -1;
    }
    stage_trace(cpu,target,argc);service_calls++;
    uint32_t a[8];for(unsigned i=0;i<argc;i++) a[i]=i<4 ? cpu->gpr[4+i] : read_at(cpu->gpr[29]+0x10u+(i-4)*4u,4);
    if(actual_sdk) {
        int handled;
        if(game) {
            /* These services are deliberately modeled here even after their
             * real bodies gain early native dispatch. Exercise their typed
             * service boundary explicitly; adoption is proved separately. */
            PcPortMipsCpu *saved_cpu=rt.bridge_cpu;rt.bridge_cpu=cpu;
            handled=bridge_stage3_services(&rt,cpu,find_function(&rt,target));
            rt.bridge_cpu=saved_cpu;
        } else handled=runtime_bridge(opaque,cpu,target);
        if(handled!=1) {fail("stage3 actual SDK bridge",target);return -1;}
    } else {
        cpu->gpr[2]=0x91000000u+service_calls;cpu->gpr[3]=0x62000000u+service_calls;
        cpu->hi=0x12340000u+service_calls;cpu->lo=0x56780000u+service_calls;
        if(game) cpu->gpr[2]=game_effect(target,a);
        else if(target==0x80026338u) {
            for(unsigned i=0;i<6;i++) write_at(a[i+2],4,0x8000fff0u+i*7u+variant);
        } else if(target==0x800454dcu) {
            write_at(a[0]+4,4,a[3]);write_at(a[0]+8,4,read_at(a[4],4)^read_at(a[4]+4,4));
        } else if(target!=0x80043a1cu && target!=0x80043a58u) write_at(a[0]+7,1,target&255u);
    }
    write_at(0x80059470u,4,service_calls);
    if(variant&16u) {
        write_at(0x800c3ea4u,4,menu_owner(service_calls&1u));
        write_at(0x800d2d28u,4,state_owner((service_calls/3)&1u));
        write_at(0x800d3280u,1,(variant+service_calls)&255u);
        write_at(0x800ccb34u,4,service_calls&1u);
    }
    if(pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;}
    return failed ? -1:1;
}

static void stage_setup(PcPortMipsCpu *cpu)
{
    setup(cpu);memset(stage_host,0xb6,sizeof(stage_host));assert((uintptr_t)stage_host<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)stage_host,(uintptr_t)stage_host+sizeof(stage_host),1};rt.host_range_count=1;
    cpu->bus.bridge=stage_bridge;cpu->gpr[4]=0xabcd0003u;service_calls=0;
    const ResolvedFunction funcs[]={
        {0x80026338u,func_80026338,"func_80026338"},{0x80043a58u,unused_clut,"GetClut"},
        {0x80043cd8u,typed_gt4,"SetPolyGT4"},{0x80043cc4u,typed_g4,"SetPolyG4"},
        {0x80043c9cu,typed_f4,"SetPolyF4"},{0x80043d78u,typed_line,"SetLineF2"},
        {0x80043c24u,typed_shade,"SetShadeTex"},{0x80043bfcu,typed_semi,"SetSemiTrans"},
        {0x80043a1cu,typed_page,"GetTPage"},{0x800454dcu,typed_draw,"SetDrawMode"},
        {0x80076a10u,typed_glyph,"func_80076A10"},{0x80076a6cu,typed_glyph,"func_80076A6C"},
        {0x80076c34u,typed_style,"func_80076C34"},{0x8008ac00u,typed_allocate,"func_8008AC00"},
        {0x800338d8u,typed_string,"func_800338D8"},{0x80034eacu,typed_render,"SystemRenderStringEntry"}};
    memcpy(rt.functions,funcs,sizeof(funcs));rt.function_count=sizeof(funcs)/sizeof(funcs[0]);
    qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
    for(unsigned bank=0;bank<2;bank++) {
        uint32_t owner=menu_owner(bank),state=state_owner(bank);
        for(unsigned i=0;i<0xb000;i++) write_at(owner+i,1,(i*17+variant+bank)&255u);
        for(unsigned i=0;i<0x110;i++) write_at(state+i,1,i*7+variant);
        const uint8_t counts[]={0,1,254,255};
        for(unsigned i=0;i<3;i++) write_at(state+0x78u+i,1,counts[(variant/8)%4]);
        for(unsigned i=0;i<6;i++) write_at(owner+0xa234u+i*4u,4,i==1 ? 1:i==3 ? 480:i==4 ? 256:64);
    }
    write_at(0x800c3ea4u,4,menu_owner(0));write_at(0x800d2d28u,4,state_owner(0));
    if((variant&8u) && !actual_sdk && running_entry==BATTLE_STAGE3_GLYPHS_ADDRESS) {
        /* Live state count bytes can share the original saved-register frame. */
        write_at(0x800d2d28u,4,cpu->gpr[29]-0x38u+0x10u-0x78u);
    }
    for(unsigned i=0;i<3;i++) write_at(0x800c3eb6u+i*28u,1,variant&(1u<<i) ? (i==2?255:i):127);
    write_at(0x800d3280u,1,(variant*37u)&255u);write_at(0x800ccb34u,4,variant&1u);
    const uint16_t xs[]={0,0x7fff,0x8000,0xffff,1,0xff80,0x7fa0,640};
    for(unsigned i=0;i<768;i++) write_at(0x800c3254u+i*2u,2,xs[(i+variant)%8]);
    write_at(0x800d2f5cu,4,0x80160000u);
    write_at(0x80160000u+4+0x5c*2u,2,0x400);
    for(unsigned i=0;i<0x30;i+=2) write_at(0x80160400u+i,2,0);
    write_at(0x80160400u,2,1);write_at(0x80160404u,2,64);
    write_at(0x80160414u,2,1);write_at(0x80160416u,2,0x300);write_at(0x80160418u,2,480);
    write_at(0x8016041au,2,256);write_at(0x8016041cu,2,256);
    for(unsigned i=0;i<0x140;i++) write_at(0x80180000u+i,1,i*9+variant);
    frame_base=cpu->gpr[29]-0x100u;
}

static void stage_snapshot(void)
{
    memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));memcpy(expected_host,stage_host,sizeof(stage_host));
}
static int stage_equal(void)
{
    return !memcmp(expected_ram,g_PsxRam,sizeof(g_PsxRam)) && !memcmp(expected_shared,shared,sizeof(shared)) &&
        !memcmp(expected_pad,g_PsxScratchpad,sizeof(expected_pad)) && !memcmp(expected_host,stage_host,sizeof(stage_host));
}
static int stage_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;running_entry=entry;stage_setup(&oracle);candidate=compare=0;
    if(entry==BATTLE_MODULE_DISPATCH_ADDRESS) printf("BATTLE MODULE STAGE3 dispatcher variant=%u sdk=%d phase=original\n",variant,actual_sdk);
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,1000000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
    unsigned expected_calls=calls;stage_snapshot();stage_setup(&native);candidate=compare=1;
    if(entry==BATTLE_MODULE_DISPATCH_ADDRESS) printf("BATTLE MODULE STAGE3 dispatcher variant=%u sdk=%d phase=native calls=%u\n",variant,actual_sdk,expected_calls);
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,1000000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) {fail(native.error,native.pc);return 0;}
    if(native.steps || rt.bridge_cpu || calls!=expected_calls || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("stage3 native context/calls/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("stage3 preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
        fail("stage3 return/HI/LO",entry);
        fprintf(stderr,"native V0/V1=%08x/%08x original=%08x/%08x\n",native.gpr[2],native.gpr[3],oracle.gpr[2],oracle.gpr[3]);
    }
    if(!stage_equal()) fail("stage3 final memory",entry);
    return !failed;
}

static int stage_controls(void)
{
    PcPortMipsCpu cpu;variant=1;candidate=1;compare=0;actual_sdk=0;
    for(unsigned t=0;t<6;t++) {
        running_entry=stage_entries[t];stage_setup(&cpu);rt.bridge_cpu=&cpu;stage_snapshot();
        const unsigned offsets[]={0,stage_sizes[t]/2,stage_sizes[t]-1};
        for(unsigned i=0;i<3;i++) {
            uint8_t *p=PSX_ADDR(stage_entries[t]+offsets[i]);*p^=1;
            if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 body identity",i);
            *p^=1;
        }
        uint8_t *dispatcher=PSX_ADDR(BATTLE_MODULE_DISPATCH_ADDRESS);*dispatcher^=1;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 resident identity",t);
        *dispatcher^=1;cpu.load_valid=1;cpu.load_reg=4;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 pending entry",t);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 force reference",t);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_MODULE_STAGE3","0",1)==0);
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 environment",t);
        assert(unsetenv("XENO_BATTLE_NATIVE_MODULE_STAGE3")==0);g_ActiveBattleRuntime=NULL;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 inactive",t);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 CPU context",t);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_module_stage3_try_host(&rt,&cpu,stage_entries[t])) fail("stage3 init",t);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(calls || !stage_equal()) fail("stage3 rejection writes",t);
    }
    const uint32_t targets[]={0x801e6290,0x801e5924,0x80026338,0x80043a58,0x801e5ee8,0x80076a6c,0x80076a10,0x801e62b8,0x801e5d2c,0x800454dc,0x801e5e78,0x8008ac00,0x800338d8,0x80034eac};
    for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);i++) for(unsigned pending=0;pending<2;pending++) {
        running_entry=BATTLE_MODULE_DISPATCH_ADDRESS;stage_setup(&cpu);
        if(pending) pending_return=targets[i];else stop_target=targets[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,1000000);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
           PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("stage3 nested refusal",targets[i]);
    }
    running_entry=BATTLE_STAGE3_UI_ADDRESS;stage_setup(&cpu);write_at(0x800d2d28u,4,0x1f800f83u);
    candidate=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,10000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || read_at(0x1f800fffu,1)!=1 || rt.bridge_cpu) fail("stage3 partial state write",0);
    return !failed;
}

static int clut_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    return candidate && target==0x80043a58u ? runtime_bridge(opaque,cpu,target):0;
}
static int clut_checks(void)
{
    const uint32_t values[]={0,1,0x7fff,0x8000,0xffff,0xffffffff,0x80000000,0x7fffffff};
    variant=0;running_entry=0x80043a58;
    for(unsigned x=0;x<8;x++) for(unsigned y=0;y<8;y++) {
        PcPortMipsCpu oracle,native;stage_setup(&oracle);oracle.bus.bridge=clut_bridge;
        memcpy(PSX_ADDR(running_entry),clut_code,sizeof(clut_code));
        oracle.gpr[4]=values[x];oracle.gpr[5]=values[y];candidate=0;
        assert(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);
        stage_setup(&native);native.bus.bridge=clut_bridge;native.gpr[4]=values[x];native.gpr[5]=values[y];candidate=1;
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        assert(PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        if(native.steps || native.gpr[2]!=oracle.gpr[2]) fail("stage3 original GetClut result",values[y]);
    }
    puts("BATTLE MODULE STAGE3 PASS 64 original GetClut return comparisons; native steps=0");
    return !failed;
}

#ifndef BATTLE_STAGE3_TEST_MAIN
#define BATTLE_STAGE3_TEST_MAIN main
#endif
int BATTLE_STAGE3_TEST_MAIN(int argc,char **argv)
{
    assert(argc==2 || argc==3);actual_sdk=argc==3;
    assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
    FILE *f=fopen(argv[1],"rb");assert(f && fread(module,1,sizeof(module),f)==sizeof(module));assert(!fclose(f));
    f=fopen("disc/SLUS_006.64","rb");assert(f && !fseek(f,0x80043a58u-0x8000f800u,SEEK_SET));
    assert(fread(clut_code,1,sizeof(clut_code),f)==sizeof(clut_code));assert(!fclose(f));
    assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    if(argc==3 && !strcmp(argv[2],"--abi-only")) {
        variant=0;return stage_case(BATTLE_STAGE3_PANELS_ADDRESS) ? 0:1;
    }
    for(unsigned entry=0;entry<7;entry++) {
        for(unsigned v=0;v<(actual_sdk?16u:32u);v++) {
            variant=actual_sdk && v>=8 ? v+16:v;
            if(!stage_case(stage_entries[entry])) return 1;
        }
        printf("BATTLE MODULE STAGE3 completed entry=%08x sdk=%d\n",stage_entries[entry],actual_sdk);
    }
    if(!actual_sdk && !stage_controls()) return 1;
    if(actual_sdk && !clut_checks()) return 1;
    printf("BATTLE MODULE STAGE3 PASS %u cases sdk=%d; external game services modeled; original frames/full memory/HI-LO and call traces; native steps=0\n",actual_sdk?112:224,actual_sdk);
    return 0;
}
