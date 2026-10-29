/* Original stage-1 callers versus native C. Callees 97D5C/9B098 are modeled;
 * the separate SDK suite uses real libc and the canonical native mask body. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>
/* PsyCross's strings.h shadows the system header without declaring bzero. */
extern void bzero(void *, size_t);

uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
unsigned int MFC2(int r) { (void)r; abort(); }
unsigned int CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned int v,int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned int v,int r) { (void)v; (void)r; abort(); }
int doCOP2(int op) { (void)op; abort(); }
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
int PcPort_QuickCheckpointFieldIsActive(void) { return 0; }
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p,int s) { (void)p; (void)s; abort(); }
extern uint16_t func_80089C08(uint8_t index);
/* Forwarders let Clang's function-type sanitizer inspect the actual call. */
static void *native_move(void *d,const void *s,size_t n) { return memmove(d,s,n); }
static void native_zero(void *d,size_t n) { bzero(d,n); }

static BattleMipsRuntime rt;
static uint8_t sdk_mask_body[28];
static uint8_t module[0x56c0], shared[0x20000], expected_shared[sizeof(shared)];
static uint8_t expected_ram[PSX_RAM_SIZE], expected_pad[4096];
static unsigned variant, calls;
static uint32_t frame_base, stop_target, pending_return, running_entry;
static int candidate, compare, sdk, failed;
static const uint32_t entries[]={0x801e4048u,0x801e4160u,0x801e4870u,0x801e4ac0u,0x801e5840u};
static const unsigned sizes[]={0x118,0x710,0x250,0x210,0xe4};
typedef struct Trace {
    uint32_t target,ra,sp,saved[9],args[3],hi,lo;
    uint8_t frame[0x80],shared_hash[32],ram_hash[32],pad_hash[32];
} Trace;
static Trace traces[64];

static void fail(const char *label,uint32_t value)
{
    fprintf(stderr,"BATTLE MODULE STAGE1 FAIL entry=%08x variant=%u call=%u %s %08x\n",
            running_entry,variant,calls,label,value); failed=1;
}
static uint32_t read_at(uint32_t address,unsigned width)
{
    uint32_t value; assert(runtime_read(&rt,address,width,&value)==0); return value;
}
static void write_at(uint32_t address,unsigned width,uint32_t value)
{
    assert(runtime_write(&rt,address,width,value)==0);
}
static int model(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if (stop_target && target-stop_target<12u) return 0;
    for (unsigned i=0;i<5;i++) {
        if (candidate && target==entries[i]) return runtime_bridge(opaque,cpu,target);
        if (target-entries[i]<sizes[i]) return 0;
    }
    unsigned argc;
    switch (target) {
    case 0x80089c08u: argc=1; break;
    case 0x8003f99cu: argc=3; break;
    case 0x8003f8e8u: argc=2; break;
    case 0x80097d5cu: case 0x8009b098u: argc=0; break;
    default: fail("unexpected callee",target); return -1;
    }
    if (calls>=64) { fail("trace capacity",calls); return -1; }
    Trace trace={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
    memcpy(trace.saved,cpu->gpr+16,8*sizeof(uint32_t));trace.saved[8]=cpu->gpr[30];
    memcpy(trace.frame,resolve_memory(&rt,frame_base,sizeof(trace.frame),0),sizeof(trace.frame));
    memcpy(trace.args,cpu->gpr+4,argc*sizeof(uint32_t));
    assert(SHA256(shared,sizeof(shared),trace.shared_hash));
    assert(SHA256(g_PsxRam,sizeof(g_PsxRam),trace.ram_hash));
    assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),trace.pad_hash));
    if (compare && memcmp(&trace,&traces[calls],sizeof(trace))) {
        Trace *want=&traces[calls];
        if (trace.target!=want->target) fail("target",want->target);
        if (trace.ra!=want->ra || trace.sp!=want->sp) fail("RA/SP",trace.ra);
        for (unsigned i=0;i<9;i++) if(trace.saved[i]!=want->saved[i]) fail("saved register",i);
        for (unsigned i=0;i<3;i++) if(trace.args[i]!=want->args[i]) fail("argument",i);
        if(memcmp(trace.frame,want->frame,sizeof(trace.frame))) fail("frame",frame_base);
        fail("call/memory differs",target);return -1;
    }
    if (!compare) traces[calls]=trace;
    calls++;
    if (sdk && (target==0x80089c08u || target==0x8003f99cu || target==0x8003f8e8u)) {
        int rc=runtime_bridge(opaque,cpu,target); if(rc!=1) fail("typed boundary",target); return rc;
    }
    cpu->gpr[2]=0x90000000u+calls;cpu->gpr[3]=0x60000000u+calls;
    cpu->hi=0x12300000u+calls;cpu->lo=0x45600000u+calls;
    write_at(0x80059470u,4,calls);
    switch (target) {
    case 0x80089c08u:
        cpu->gpr[2]=0xabcd0000u|(1u<<(trace.args[0]&15u));
        if (variant&16u) write_at(cpu->gpr[16],1,(read_at(cpu->gpr[16],1)+1u)&3u);
        break;
    case 0x8003f99cu:
        assert(trace.args[2]==0x170);
        memmove(resolve_memory(&rt,trace.args[0],trace.args[2],1),
                resolve_memory(&rt,trace.args[1],trace.args[2],0),trace.args[2]);
        if (variant&16u) {
            write_at(cpu->gpr[19],1,(read_at(cpu->gpr[19],1)+1u)%8u);
            write_at(0x800c3dd0u,4,read_at(0x800c3dd0u,4)^0x10000u);
        }
        break;
    case 0x8003f8e8u:
        assert(trace.args[1]==0x170);memset(resolve_memory(&rt,trace.args[0],trace.args[1],1),0,trace.args[1]);break;
    case 0x80097d5cu:
        if (variant&16u) {
            write_at(0x800d3294u,1,read_at(0x800d3294u,1)^1u);
            write_at(0x800c3ea4u,4,read_at(0x800c3ea4u,4)^0x10000u);
            write_at(0x800c3eb6u,1,7);write_at(0x800c3eb8u,1,1);
        }
        for(unsigned i=0;i<3;i++) {
            write_at(0x800ccd4au+i*0x170u,1,variant+i+17);
            write_at(0x800ccd4bu+i*0x170u,1,variant+i+127);
        }
        break;
    case 0x8009b098u:
        if(variant&16u) write_at(0x800c3eb8u+0x1cu,1,0);
        break;
    }
    if (pending_return==target) {
        cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;
    }
    return 1;
}

static void setup(PcPortMipsCpu *cpu)
{
    memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
    memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));
    memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
    if(sdk) memcpy(PSX_ADDR(0x80089c08u),sdk_mask_body,sizeof(sdk_mask_body));
    rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};
    rt.data[1]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};
    rt.data_count=2;rt.initialized=1;g_ActiveBattleRuntime=&rt;
    const ResolvedFunction functions[]={
        {0x8003f8e8u,native_zero,"bzero"},{0x8003f99cu,native_move,"memmove"},
        {0x80089c08u,func_80089C08,"func_80089C08"}};
    memcpy(rt.functions,functions,sizeof(functions));rt.function_count=3;
    initialize_cpu(cpu,&rt);cpu->bus.bridge=model;
    for(unsigned r=1;r<29;r++) cpu->gpr[r]=0x12340000u+variant*256u+r;
    cpu->gpr[30]=0x87650000u+variant;cpu->gpr[4]=0xabcd0001u;
    if(variant&2u) cpu->gpr[29]=0x1f8003f8u;
    else if(variant&4u) cpu->gpr[29]|=0x20000000u;
    frame_base=cpu->gpr[29]-0x80u;
    cpu->hi=0x98765432;cpu->lo=0xfedcba98;
    calls=0;stop_target=pending_return=0;
    uint32_t alias=variant&8u ? 0x20000000u : 0;
    write_at(0x8005949cu,4,0x80170000u|alias);
    write_at(0x800c3dd0u,4,0x80140000u|alias);
    write_at(0x800c3ea4u,4,0x80100000u|alias);
    static const uint8_t specials[]={0,1,0x80,0xff};
    write_at(0x800d3294u,1,specials[variant%4]);
    for(unsigned i=0;i<0x400;i+=2) write_at(0x80170000u+i,2,i*13+variant);
    for(unsigned i=0;i<256;i++) write_at(0x800c3448u+i*2u,2,(1u<<(i&15))^0x8181);
    for(unsigned table=0;table<2;table++) {
        uint32_t base=0x80140000u+table*0x10000u;
        for(unsigned i=0;i<0x1200;i++) write_at(base+i,1,i*7+table*13+variant);
        write_at(base+0x30u,2,0x5000);
        for(unsigned actor=0;actor<16;actor++) {
            uint32_t header=base+0x4000u+actor*0x20u;
            write_at(base+actor*2u,2,0x4000u+actor*0x20u);
            write_at(header,2,0x80+actor);write_at(header+2,2,0x100+actor);
            write_at(header+4,2,(actor+variant)&1u ? 0xffff:0x140);
            write_at(header+6,2,(actor+variant)&2u ? 0xffff:0x180);
        }
    }
    for(unsigned i=0;i<11;i++) {
        unsigned actor=(i+variant)%8u;
        if(variant%6u==0 || (variant%6u==2 && i%2)) actor=0x7f;
        if(i==0 && variant%6u==5) actor=0xff;
        unsigned gear=variant%6u==1 ? 0 : specials[(variant+i)&3];
        write_at(0x800c3eb4u+i*0x1cu,1,i%3);
        write_at(0x800c3eb5u+i*0x1cu,1,i%2);
        write_at(0x800c3eb6u+i*0x1cu,1,actor);
        write_at(0x800c3eb8u+i*0x1cu,1,gear);
        write_at(0x8006f8e5u+i,1,gear);
        if(i<3) {
            write_at(0x800d2d24u+i,1,actor);write_at(0x8006f9e0u+i,1,0x80u|(i%2u));
        } else {
            write_at(0x8006f9e1u+i,1,actor|(gear?0x80u:0));
            write_at(0x8006f9e9u+i,1,(i&1)?0x81:0);
            write_at(0x8006f9f1u+i,1,0x80u|(i%3u));
        }
    }
}

static int run_case(uint32_t entry)
{
    running_entry=entry;
    PcPortMipsCpu oracle,native;
    setup(&oracle);candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_HALTED) {
        fail(oracle.error,oracle.pc);return 0;
    }
    unsigned expected_calls=calls;
    if(!oracle.steps) fail("oracle did not execute",0);
    memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));
    setup(&native);candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,50000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc);return 0; }
    if(calls!=expected_calls || native.steps || rt.bridge_cpu ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("calls/context/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] ||
       native.hi!=oracle.hi || native.lo!=oracle.lo) {
        fail("return values",native.gpr[3]);fprintf(stderr,"expected v1=%08x\n",oracle.gpr[3]);
    }
    if(memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
       memcmp(g_PsxScratchpad,expected_pad,sizeof(expected_pad))) fail("final memory",0);
    return !failed;
}

static int controls(void)
{
    PcPortMipsCpu cpu;variant=1;candidate=1;compare=0;sdk=0;setup(&cpu);rt.bridge_cpu=&cpu;
    memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));
    for(unsigned t=0;t<4;t++) {
        for(unsigned region=0;region<2;region++) {
            unsigned r=region?t:4, offsets[]={0,sizes[r]/2,sizes[r]-1};
            for(unsigned j=0;j<3;j++) {
                uint8_t *p=PSX_ADDR(entries[r]+offsets[j]);*p^=1;
                if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("modified code accepted",r);
                *p^=1;
            }
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("pending load",t);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("forced reference",t);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_MODULE_STAGE1","0",1)==0);
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("reference switch",t);
        assert(unsetenv("XENO_BATTLE_NATIVE_MODULE_STAGE1")==0);g_ActiveBattleRuntime=NULL;
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("inactive runtime",t);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("wrong CPU",t);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_module_stage1_try_host(&rt,&cpu,entries[t])) fail("uninitialized runtime",t);
        rt.initialized=1;
    }
    if(calls || memcmp(g_PsxRam,expected_ram,sizeof(expected_ram))) fail("rejection effects",calls);
    const uint32_t stops[]={0x80089c08u,0x8003f99cu,0x80097d5cu};
    for(unsigned i=0;i<3;i++) {
        setup(&cpu);stop_target=stops[i];assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        if(PcPortMipsRun(&cpu,entries[4],BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_UNRESOLVED_CALL ||
           cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1)
            fail("missing callee/context",i);
        if(read_at(0x800d3338u,1)!=0) fail("completed writes lost",i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        setup(&cpu);pending_return=stops[i];
        if(PcPortMipsRun(&cpu,entries[4],BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_UNRESOLVED_CALL)
            fail("pending callee return",i);
    }
    rt.bridge_cpu=NULL;
    return !failed;
}

static int boundary_cases(void)
{
    PcPortMipsCpu cpu;variant=1;setup(&cpu);
    const uint32_t bases[]={0x80120000u,0xa0120000u,0x120000u,0x80060000u,0x60000u,0x1f800100u};
    for(unsigned i=0;i<6;i++) for(unsigned reverse=0;reverse<2;reverse++) {
        uint8_t want[96],*p=resolve_memory(&rt,shard_sdk_address(bases[i]),96,1);
        for(unsigned j=0;j<96;j++) p[j]=want[j]=(uint8_t)(j*7);
        unsigned d=reverse?0:7,s=reverse?7:0;
        memmove(want+d,want+s,64);
        cpu.gpr[4]=bases[i]+d;cpu.gpr[5]=bases[i]+s;cpu.gpr[6]=64;
        if(runtime_bridge(&rt,&cpu,0x8003f99cu)!=1 || cpu.gpr[2]!=bases[i]+d || cpu.gpr[3]!=0 || memcmp(p,want,96))
            fail("memmove overlap/alias/return",i);
        cpu.gpr[4]=bases[i]+3;cpu.gpr[5]=39;cpu.gpr[2]=0x12345678;cpu.gpr[3]=0xabcdef98;
        memset(want+3,0,39);
        if(runtime_bridge(&rt,&cpu,0x8003f8e8u)!=1 || memcmp(p,want,96) ||
           cpu.gpr[2]!=0x12345678 || cpu.gpr[3]!=0xabcdef98) fail("bzero alias/return",i);
    }
    for(unsigned i=0;i<256;i++) {
        cpu.gpr[4]=0x80000000u+i;
        if(runtime_bridge(&rt,&cpu,0x80089c08u)!=1 || cpu.gpr[2]!=read_at(0x800c3448u+i*2u,2))
            fail("mask argument width",i);
    }
    const uint32_t bad[]={0x801ffff0u,0x1f800ff0u};
    for(unsigned i=0;i<2;i++) {
        cpu.gpr[4]=bad[i];cpu.gpr[5]=0x80120000u;cpu.gpr[6]=32;
        if(runtime_bridge(&rt,&cpu,0x8003f99cu)!=-1) fail("copy range",i);
        cpu.gpr[5]=UINT32_MAX;
        if(runtime_bridge(&rt,&cpu,0x8003f8e8u)!=-1) fail("zero range overflow",i);
    }
    cpu.gpr[4]=cpu.gpr[5]=cpu.gpr[6]=0;
    if(runtime_bridge(&rt,&cpu,0x8003f99cu)!=1 || cpu.gpr[2]!=0 || runtime_bridge(&rt,&cpu,0x8003f8e8u)!=1)
        fail("empty ranges",0);
    return !failed;
}

int main(int argc,char **argv)
{
    assert(argc==2 || argc==3);sdk=argc==3;assert(!sdk || strcmp(argv[2],"--sdk")==0);
    FILE *image=fopen(argv[1],"rb");assert(image && fread(module,1,sizeof(module),image)==sizeof(module));
    assert(fclose(image)==0);assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    if(sdk) {
        image=fopen("disc/battle.bin","rb");
        assert(image && fseek(image,0x1a118,SEEK_SET)==0 &&
               fread(sdk_mask_body,1,sizeof(sdk_mask_body),image)==sizeof(sdk_mask_body) && fclose(image)==0);
        uint8_t digest[32];char hex[65];assert(SHA256(sdk_mask_body,sizeof(sdk_mask_body),digest));
        for(unsigned i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);
        assert(strcmp(hex,"74c04258a3cf89081e9e82cef4ed802aff2c03b830a9fc7bb204291532cb45c0")==0);
        for(variant=0;variant<24;variant++) if(!run_case(entries[4])) return 1;
        if(!boundary_cases()) return 1;
        puts("BATTLE MODULE STAGE1 SDK PASS 24 composed cases; 24 libc alias/overlap cases, 256 mask widths, bounds/empty controls; 97D5C/9B098 modeled");
    } else {
        for(variant=0;variant<24;variant++) for(unsigned i=0;i<5;i++) if(!run_case(entries[i])) return 1;
        if(!controls()) return 1;
        puts("BATTLE MODULE STAGE1 PASS 120 modeled-callee differential cases; full RAM/shared/scratch, call frames, mutable callees, identity/refusal controls; native steps=0");
    }
    return 0;
}
