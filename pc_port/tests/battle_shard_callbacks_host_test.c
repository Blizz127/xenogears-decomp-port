/* Original CPU execution is a test-only oracle. Native callback runs forbid
 * it. SDK integration shares the native GTE backend; separate leaf checks
 * execute original RotMatrix/AddPrim, including Q12 rounding and tag aliases. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "psx/gtereg.h"
#include <openssl/sha.h>

uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
int g_GPUDisabledState;
int g_splitIndex;
void GR_UpdateVRAM(void) {}
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
int PcPort_QuickCheckpointFieldIsActive(void) { return 0; }
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
extern int32_t ReadGeomScreen(void);
extern int32_t func_80021AD8(int32_t, int32_t);
/* Instrumented exact-signature forwarders also check calls into C++ SDK TUs
 * compiled by GCC, which does not emit Clang's function-type metadata. */
static MATRIX *sdk_translate(MATRIX *m, VECTOR *v) { return TransMatrix(m, v); }
static void sdk_rotation(MATRIX *m) { SetRotMatrix(m); }
static void sdk_translation(MATRIX *m) { SetTransMatrix(m); }
static void sdk_screen(int h) { SetGeomScreen(h); }
static int sdk_project(SVECTOR *a, SVECTOR *b, SVECTOR *c, long *x, long *y,
                       long *z, long *p, long *f) { return RotTransPers3(a,b,c,x,y,z,p,f); }
static int sdk_sync(int mode) { return DrawSync(mode); }
static unsigned int released;
static unsigned int sdk_free(void *p) { assert(p == PSX_ADDR(0x80100000u)); return ++released; }

static BattleMipsRuntime rt;
static uint8_t battle_code[0x458], exe_code[0x50000];
static uint8_t shared[0x10000], expected_shared[0x10000], expected_ram[PSX_RAM_SIZE];
static uint8_t expected_scratch[4096];
static GTERegisters expected_gte;
static unsigned calls, variant, stop_target, pending_return, projections;
static uint32_t heap, frame_base, entry_address;
static int candidate, compare, failed, actual_sdk;
typedef struct Trace {
    uint32_t target, ra, sp, saved[9], args[8], hi, lo;
    uint8_t frame[0x90], shared_hash[32], heap_hash[32], ot_hash[32], gte_hash[32];
} Trace;
static Trace traces[4000];
static void fail(const char *label, uint32_t value)
{
    fprintf(stderr, "BATTLE SHARD CALLBACKS FAIL variant=%u entry=%08x call=%u %s %08x\n",
            variant, entry_address, calls, label, value);
    failed = 1;
}
static uint32_t read_at(uint32_t a, unsigned w)
{
    uint32_t v; assert(runtime_read(&rt, a, w, &v) == 0); return v;
}
static void write_at(uint32_t a, unsigned w, uint32_t v)
{
    assert(runtime_write(&rt, a, w, v) == 0);
}
static int model(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (stop_target && target - stop_target < 12u) return 0;
    if (target == BATTLE_SHARD_UPDATE_ADDRESS || target == BATTLE_SHARD_RENDER_ADDRESS ||
        target == BATTLE_SHARD_CLEANUP_ADDRESS)
        return candidate ? runtime_bridge(opaque, cpu, target) : 0;
    if (target - BATTLE_SHARD_UPDATE_ADDRESS < sizeof(battle_code)) return 0;
    unsigned argc;
    switch (target) {
    case 0x8004a0dcu: argc = 0; break;
    case 0x80049efcu: case 0x80049f8cu: case 0x8004a14cu:
    case 0x800445d0u: case 0x800320e8u: argc = 1; break;
    case 0x8004a67cu: argc = 8; break;
    case 0x80021ad8u: case 0x8004a0bcu: case 0x8004a12cu:
    case 0x8003f738u: case 0x80049d9cu: case 0x80043b48u: argc = 2; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= sizeof(traces)/sizeof(traces[0])) { fail("trace overflow", target); return -1; }
    Trace t = {.target=target, .ra=cpu->gpr[31], .sp=cpu->gpr[29], .hi=cpu->hi, .lo=cpu->lo};
    memcpy(t.saved, cpu->gpr + 16, 8 * sizeof(uint32_t)); t.saved[8] = cpu->gpr[30];
    for (unsigned i=0; i<argc; i++) t.args[i] = i<4 ? cpu->gpr[4+i] : read_at(t.sp+16+(i-4)*4,4);
    uint8_t *frame = resolve_memory(&rt, frame_base, sizeof(t.frame), 0); assert(frame);
    memcpy(t.frame, frame, sizeof(t.frame));
    assert(SHA256(shared, sizeof(shared), t.shared_hash));
    assert(SHA256(PSX_ADDR(heap)-16, 0x10f7c+32, t.heap_hash));
    assert(SHA256(PSX_ADDR(0x80120000u), 0x4000, t.ot_hash));
    assert(SHA256((uint8_t *)&gteRegs, sizeof(gteRegs), t.gte_hash));
    if (compare && memcmp(&t, &traces[calls], sizeof(t))) {
        Trace *want=&traces[calls];
        if (t.target!=want->target || t.ra!=want->ra || t.sp!=want->sp) fail("target/RA/SP",t.target);
        for (unsigned i=0; i<9; i++) if(t.saved[i]!=want->saved[i]) fail("saved register",i);
        for (unsigned i=0; i<8; i++) if(t.args[i]!=want->args[i]) fail("argument",i);
        for (unsigned i=0; i<sizeof(t.frame); i++) if(t.frame[i]!=want->frame[i]) { fail("frame byte",frame_base+i); break; }
        if (t.hi!=want->hi || t.lo!=want->lo) fail("HI/LO", t.lo);
        fail("callee memory/state",target); return -1;
    }
    if (!compare) traces[calls]=t;
    calls++;
    if (actual_sdk) return runtime_bridge(opaque,cpu,target);
    cpu->gpr[2]=0x50000000u+calls; cpu->gpr[3]=0x60000000u+calls;
    cpu->hi=0x70000000u+calls; cpu->lo=0x80000000u+calls;
    write_at(0x80059520u,4,calls);
    switch (target) {
    case 0x80021ad8u:
        cpu->gpr[2]=(uint32_t)func_80021AD8((int32_t)t.args[0],(int32_t)t.args[1]);
        if (variant & 4u) {
            /* The selected triangle remains fixed for these three calls. */
            write_at(0x800ccb34u,4,calls & 1u);
            uint32_t tri=cpu->gpr[16];
            write_at(tri+5,1,read_at(tri+5,1)+3u);
            write_at(tri+6,1,read_at(tri+6,1)+5u);
        }
        break;
    case 0x8004a0bcu: write_at(t.args[0],4,(uint32_t)-37); write_at(t.args[1],4,255); break;
    case 0x8004a0dcu: cpu->gpr[2]=0xfffff123u; break;
    case 0x8003f738u:
        for (unsigned i=0;i<18;i++) write_at(t.args[1]+i,1,read_at(t.args[0]+i%6,1)+i);
        cpu->gpr[2]=t.args[1]; break;
    case 0x80049d9cu:
        for (unsigned i=0;i<3;i++) write_at(t.args[0]+20+i*4,4,read_at(t.args[1]+i*4,4));
        cpu->gpr[2]=t.args[0]; break;
    case 0x8004a67cu:
        projections++;
        for (unsigned i=3;i<8;i++) write_at(t.args[i],4,0x81234567u+calls*7+i);
        cpu->gpr[2]=(variant & 8u) ? 0xfffff000u+projections%0x1000 : projections*31u;
        break;
    case 0x80043b48u:
        write_at(t.args[1],4,(read_at(t.args[1],4)&0xff000000u)|(read_at(t.args[0],4)&0xffffffu));
        write_at(t.args[0],4,(read_at(t.args[0],4)&0xff000000u)|(t.args[1]&0xffffffu));
        if (variant & 4u) {
            write_at(0x800ccb34u,4,calls & 1u);
            write_at(0x800c3cb4u,4,0x80121000u+(calls%2)*0x1000u);
        }
        break;
    case 0x800320e8u: write_at(t.args[0],4,0xdeadc0deu); break;
    }
    if(target==pending_return) { cpu->load_valid=1; cpu->load_reg=2; cpu->load_value=123; cpu->load_mask=0; }
    return 1;
}
static void setup(PcPortMipsCpu *cpu, uint32_t entry)
{
    entry_address=entry;
    memset(&rt,0,sizeof(rt)); memset(g_PsxRam,0x35+variant,sizeof(g_PsxRam));
    memset(g_PsxScratchpad,0x69,sizeof(g_PsxScratchpad)); memset(&gteRegs,0,sizeof(gteRegs));
    memcpy(PSX_ADDR(0x8000f800u),exe_code,sizeof(exe_code));
    memcpy(shared,PSX_ADDR(0x80050000u),sizeof(shared));
    memcpy(PSX_ADDR(BATTLE_SHARD_UPDATE_ADDRESS),battle_code,sizeof(battle_code));
    rt.data[0]=(ResolvedData){0x80050000u,sizeof(shared),shared,"shared"};
    rt.data_count=1; rt.initialized=1;
    const ResolvedFunction sdk[] = {
        {0x80021ad8u,(void *)func_80021AD8,"func_80021AD8"},
        {0x800320e8u,(void *)sdk_free,"HeapFree"},
        {0x8003f738u,(void *)RotMatrix,"RotMatrix"},
        {0x80043b48u,(void *)AddPrim,"AddPrim"},
        {0x800445d0u,(void *)sdk_sync,"DrawSync"},
        {0x80049d9cu,(void *)sdk_translate,"TransMatrix"},
        {0x80049efcu,(void *)sdk_rotation,"SetRotMatrix"},
        {0x80049f8cu,(void *)sdk_translation,"SetTransMatrix"},
        {0x8004a0bcu,(void *)ReadGeomOffset,"ReadGeomOffset"},
        {0x8004a0dcu,(void *)ReadGeomScreen,"ReadGeomScreen"},
        {0x8004a12cu,(void *)SetGeomOffset,"SetGeomOffset"},
        {0x8004a14cu,(void *)sdk_screen,"SetGeomScreen"},
        {0x8004a67cu,(void *)sdk_project,"RotTransPers3"}
    };
    memcpy(rt.functions,sdk,sizeof(sdk)); rt.function_count=sizeof(sdk)/sizeof(sdk[0]);
    g_ActiveBattleRuntime=&rt; initialize_cpu(cpu,&rt); cpu->bus.bridge=model;
    for(unsigned r=1;r<31;r++) if(r!=29) cpu->gpr[r]=0x12340000u+variant*256u+r;
    cpu->gpr[29] = variant & 2u ? 0x1f8003f8u : (variant & 1u ? 0xa01fff00u : 0x801fff00u);
    unsigned frame=entry==BATTLE_SHARD_UPDATE_ADDRESS?0x48:entry==BATTLE_SHARD_RENDER_ADDRESS?0x88:0x18;
    frame_base=cpu->gpr[29]-frame;
    heap=variant & 1u ? 0xa0100000u : 0x80100000u;
    cpu->gpr[4]=heap; cpu->hi=0xabcdef01u; cpu->lo=0x12345678u;
    calls=stop_target=pending_return=projections=released=0;
    write_at(heap+4,4,heap); write_at(heap+0x38,4,UINT32_MAX-variant);
    write_at(0x800ccb34u,4,variant&1u); write_at(0x800c3cb4u,4,0x80121000u);
    for(unsigned i=0;i<560;i++) {
        uint32_t p=heap+0x3c+i*0x7c;
        for(unsigned j=0;j<3;j++) {
            write_at(p+j*2,2,variant*1451+i*171+j*593);
            write_at(p+8+j*2,2,0xfff0+i*11+j*473);
            write_at(p+0x10+j*4,4,actual_sdk ? i%640-320u : i*99999991u+j);
            static const uint32_t velocities[]={0,1,15,16,0xffffffffu,0xfffffff1u,0xfffffff0u,0x80000000u,0x7fffffffu,0x80010001u};
            write_at(p+0x68+j*4,4,velocities[(variant+i+j)%10]);
        }
        uint32_t z=variant%4==0?64u:variant%4==1?63u:variant%4==2?(uint32_t)-1:512u+i*2u;
        if(i%5==0) z=1024u;
        write_at(p+0x18,4,z);
        write_at(p+0x64,4,variant%4==0?0:variant%4==1?1:variant%4==2?UINT32_MAX:i%3);
        write_at(p+0x78,4,0x70800u+i*12345u);
        for(unsigned b=0;b<2;b++) {
            write_at(p+0x24+b*32,4,0x07123456u+i);
            write_at(p+0x28+b*32,4,0x24000000u|((i*901u+variant*11)&0xffffffu));
        }
    }
    for(unsigned i=0;i<0x1000;i++) write_at(0x80120000u+i*4,4,0x00ffffffu);
    for(unsigned i=0;i<6;i++) {
        write_at(0x800c3594u+i*8,2,i&1u?16:(uint32_t)-16);
        write_at(0x800c3596u+i*8,2,i%3?16:(uint32_t)-16);
        write_at(0x800c3598u+i*8,2,0); write_at(0x800c359au+i*8,2,0);
    }
    CTC2((uint32_t)-37 << 16,24); CTC2(255u << 16,25); CTC2(1234,26);
}
static int run_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;
    setup(&oracle,entry); candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,1000000)!=PC_PORT_MIPS_HALTED) {
        fail(oracle.error,oracle.pc); return 0;
    }
    unsigned expected_calls=calls;
    assert(oracle.steps);
    memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam)); memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_scratch,g_PsxScratchpad,sizeof(expected_scratch)); expected_gte=gteRegs;
    setup(&native,entry); candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,1000000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc); return 0; }
    if(calls!=expected_calls || native.steps || rt.bridge_cpu ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("count/context/CPU",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] ||
       native.hi!=oracle.hi || native.lo!=oracle.lo) fail("return/HI/LO",native.gpr[3]);
    if(memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
       memcmp(g_PsxScratchpad,expected_scratch,sizeof(expected_scratch)) || memcmp(&gteRegs,&expected_gte,sizeof(gteRegs)))
        fail("final RAM/shared/scratch/GTE",0);
    if(!failed) fprintf(stderr,"BATTLE SHARD CALLBACKS case PASS variant=%u entry=%08x sdk=%d calls=%u\n",variant,entry,actual_sdk,calls);
    return !failed;
}
static int controls(void)
{
    const uint32_t addresses[]={BATTLE_SHARD_UPDATE_ADDRESS,BATTLE_SHARD_RENDER_ADDRESS,BATTLE_SHARD_CLEANUP_ADDRESS};
    const unsigned sizes[]={BATTLE_SHARD_UPDATE_BYTES,BATTLE_SHARD_RENDER_BYTES,BATTLE_SHARD_CLEANUP_BYTES};
    PcPortMipsCpu cpu; variant=0; candidate=1; compare=0;
    for(unsigned unit=0;unit<3;unit++) {
        setup(&cpu,addresses[unit]); rt.bridge_cpu=&cpu;
        unsigned offsets[]={0,sizes[unit]/2,sizes[unit]-1};
        for(unsigned j=0;j<3;j++) {
            uint8_t *p=PSX_ADDR(addresses[unit]+offsets[j]); *p^=1;
            if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("changed identity",offsets[j]);
            *p^=1;
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("entry load",unit);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("forced oracle",unit);
        rt.force_interpret=0;g_ActiveBattleRuntime=NULL;
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("inactive context",unit);
        g_ActiveBattleRuntime=&rt;rt.initialized=0;
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("uninitialized",unit);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("wrong CPU",unit);
        rt.bridge_cpu=&cpu;assert(setenv("XENO_BATTLE_NATIVE_SHARD_CALLBACKS","0",1)==0);
        if(battle_shard_callbacks_try_host(&rt,&cpu,addresses[unit])) fail("environment control",unit);
        assert(unsetenv("XENO_BATTLE_NATIVE_SHARD_CALLBACKS")==0);
        if(calls) fail("refusal changed state",calls);
    }
    const uint32_t targets[]={0x80021ad8u,0x8004a0bcu,0x8004a0dcu,0x8004a12cu,0x8004a14cu,
        0x8003f738u,0x80049d9cu,0x80049efcu,0x80049f8cu,0x8004a67cu,0x80043b48u,0x800445d0u,0x800320e8u};
    for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);i++) {
        uint32_t entry=i==0?addresses[0]:i>=11?addresses[2]:addresses[1];
        setup(&cpu,entry); pending_return=targets[i];
        if(PcPortMipsRun(&cpu,entry,BATTLE_HALT_PC,1000000)!=PC_PORT_MIPS_UNRESOLVED_CALL || !calls || rt.bridge_cpu)
            fail("pending return load",targets[i]);
        setup(&cpu,entry); stop_target=targets[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        if(PcPortMipsRun(&cpu,entry,BATTLE_HALT_PC,1000000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
           PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1) fail("disabled refusal",targets[i]);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        if(i==0 && read_at(heap+0x38,4)!=0) fail("refusal lost completed write",i);
    }
    return !failed;
}
static int original_leaf(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if(candidate) return runtime_bridge(opaque,cpu,target);
    return 0;
}
static int leaf_cases(void)
{
    for(unsigned i=0;i<520;i++) {
        uint32_t target=i<512?0x8003f738u:0x80043b48u;
        PcPortMipsCpu oracle,native; variant=i%4; setup(&oracle,target);
        uint32_t a=i<512?0x80150000u:0x80151000u,b=i<512?0x80150020u:(i&1u?0x80151000u:0x80151020u);
        if(i&2u) {a|=0x20000000u;b|=0x20000000u;}
        for(unsigned j=0;j<3;j++) write_at(a+j*2,2,i*32771u+j*3571u);
        write_at(b,4,0x07ffffffu);
        oracle.gpr[4]=a;oracle.gpr[5]=b;oracle.bus.bridge=original_leaf;candidate=0;
        if(PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,10000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
        memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam));memcpy(expected_shared,shared,sizeof(shared));expected_gte=gteRegs;
        setup(&native,target);
        for(unsigned j=0;j<3;j++) write_at(a+j*2,2,i*32771u+j*3571u);
        write_at(b,4,0x07ffffffu);
        native.gpr[4]=i>=516?(a&0x1fffffffu):a;native.gpr[5]=i>=516?(b&0x1fffffffu):b;
        native.bus.bridge=original_leaf;candidate=1;
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        int rc=PcPortMipsRun(&native,target,BATTLE_HALT_PC,10000);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        if(rc!=PC_PORT_MIPS_HALTED || native.steps || native.gpr[2]!=oracle.gpr[2] ||
           native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo ||
           memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
           memcmp(&gteRegs,&expected_gte,sizeof(gteRegs))) {fail("original matrix/tag leaf",i);return 0;}
    }
    puts("BATTLE SHARD CALLBACKS LEAF PASS 512 original RotMatrix and 8 original AddPrim comparisons; Q12, HI/LO, tag aliases, physical pointers");
    return 1;
}
int main(int argc,char **argv)
{
    FILE *f=fopen("disc/battle.bin","rb");assert(f);
    assert(fseek(f,BATTLE_SHARD_UPDATE_ADDRESS-BATTLE_BASE,SEEK_SET)==0);
    assert(fread(battle_code,1,sizeof(battle_code),f)==sizeof(battle_code));assert(fclose(f)==0);
    f=fopen("disc/SLUS_006.64","rb");assert(f);
    assert(fread(exe_code,1,sizeof(exe_code),f)>=0x46bf0u);assert(fclose(f)==0);
    assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    if(argc==2 && strcmp(argv[1],"--leaf")==0) return leaf_cases()?0:1;
    actual_sdk=argc==2 && strcmp(argv[1],"--sdk")==0;
    for(variant=0;variant<(actual_sdk?8u:16u);variant++) {
        if(!run_case(BATTLE_SHARD_UPDATE_ADDRESS) || !run_case(BATTLE_SHARD_RENDER_ADDRESS) ||
           !run_case(BATTLE_SHARD_CLEANUP_ADDRESS)) return 1;
    }
    if(!actual_sdk && !controls()) return 1;
    printf("BATTLE SHARD CALLBACKS PASS %u %s cases; frames, full RAM/shared/scratch/GTE, declared calls; native steps=0\n",actual_sdk?24:48,actual_sdk?"SDK integration":"modeled");
    return 0;
}
