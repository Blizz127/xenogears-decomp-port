/* Actual original target/list/priority bodies and the complete command caller
 * versus native dispatch. No game callees are modeled in this suite. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

static uint8_t target_image[343936];
static _Alignas(4) uint8_t target_memory[0x6000],expected_target_memory[sizeof(target_memory)];
static const uint32_t target_entries[]={0x800841e0u,0x80083ff4u,0x80085310u,0x801e5014u};
static const unsigned target_sizes[]={872,276,64,880};
typedef struct TargetTrace {
    uint32_t target,ra,sp,saved[9],args[2],hi,lo;
    uint8_t frame[0x100],ram_hash[32],shared_hash[32],pad_hash[32],host_hash[32];
} TargetTrace;
static TargetTrace target_traces[256];

static void target_write(uint32_t address,unsigned width,uint32_t value)
{
    if(runtime_write(&rt,address,width,value)!=0) { fail("target fixture address",address);exit(2); }
}
#define write_at target_write

static void target_trace(PcPortMipsCpu *cpu,uint32_t target,unsigned argc)
{
    if(target==running_entry && cpu->gpr[31]==BATTLE_HALT_PC) return;
    if(calls>=256) { fail("target trace capacity",calls);exit(2); }
    TargetTrace trace={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
    memcpy(trace.saved,cpu->gpr+16,8*sizeof(uint32_t));trace.saved[8]=cpu->gpr[30];
    memcpy(trace.args,cpu->gpr+4,argc*sizeof(uint32_t));
    memcpy(trace.frame,resolve_memory(&rt,frame_base,sizeof(trace.frame),0),sizeof(trace.frame));
    assert(SHA256(g_PsxRam,sizeof(g_PsxRam),trace.ram_hash));assert(SHA256(shared,sizeof(shared),trace.shared_hash));
    assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),trace.pad_hash));
    assert(SHA256(target_memory,sizeof(target_memory),trace.host_hash));
    if(compare && memcmp(&trace,&target_traces[calls],sizeof(trace))) {
        const TargetTrace *want=&target_traces[calls];
        if(trace.target!=want->target) fail("target callee",want->target);
        if(trace.ra!=want->ra || trace.sp!=want->sp) fail("target RA/SP",trace.ra);
        for(unsigned i=0;i<9;i++) if(trace.saved[i]!=want->saved[i]) fail("target saved register",i);
        for(unsigned i=0;i<2;i++) if(trace.args[i]!=want->args[i]) fail("target argument",i);
        if(memcmp(trace.frame,want->frame,sizeof(trace.frame))) fail("target frame",frame_base);
        fail("target call/memory",target);
    }
    if(!compare) target_traces[calls]=trace;
    calls++;
}

static int target_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if(stop_target && target-stop_target<12u) return 0;
    for(unsigned i=0;i<4;i++) {
        if(target==target_entries[i]) {
            target_trace(cpu,target,i==0?1:i<3?2:0);
            int rc=candidate?runtime_bridge(opaque,cpu,target):0;
            if(rc==1 && pending_return==target) {
                cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;
            }
            return rc;
        }
        if(target-target_entries[i]<target_sizes[i]) return 0;
    }
    fail("unexpected target callee",target);return -1;
}

static void target_setup(PcPortMipsCpu *cpu,uint32_t entry)
{
    setup(cpu);cpu->bus.bridge=target_bridge;
    memset(target_memory,0,sizeof(target_memory));assert((uintptr_t)target_memory<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)target_memory,(uintptr_t)target_memory+sizeof(target_memory),1};rt.host_range_count=1;
    for(unsigned i=0;i<3;i++) memcpy(PSX_ADDR(target_entries[i]),target_image+(target_entries[i]-BATTLE_BASE),target_sizes[i]);
    const uint8_t slots[]={0,1,2,3,7,10,127,255};
    const uint16_t ranks[]={0,1,0x7fff,0x8000,0xffff,0xfffe,90,90};
    const uint16_t statuses[]={0,1,0x4000,0x8000,0xc000,0xc001,0x3ffe,0xffff};
    uint32_t actor=slots[(variant/8)%8],target=slots[(variant/64)%8];
    cpu->gpr[4]=0xdead8000u|actor;cpu->gpr[5]=0xbeef8000u|target;
    const uint32_t owners[]={0x80130000u,0xa0130000u,0x1f800400u,(uint32_t)(uintptr_t)target_memory};
    uint32_t owner=owners[variant%4];
    if(variant%4!=3) for(unsigned i=0;i<(variant%4==2?0x400u:0x5000u);i++) write_at(owner+i,1,0);
    for(unsigned i=0;i<256;i++) {
        unsigned group;
        switch((variant/8)%4) {
        case 0:group=0;break;
        case 1:group=i%3;break;
        case 2:group=(i/3)%3;break;
        default:group=variant%4==2?(i&3u):(i&1u?255u:7u);break;
        }
        write_at(0x800c3eb4u+i*28u,1,group);
        write_at(0x800c3eb7u+i*28u,1,(variant&64u) && i%4==2 ? 1:0);
        write_at(0x800d32a1u+i*8u,1,(variant&32u)?i&1u:0);
        unsigned active=variant%4==0?0:variant%4==1?1:(i+variant)&1u;
        write_at(0x800d2dccu+i,1,active);
        write_at(0x800ccd64u+i*0x170u,2,(variant&128u)?statuses[(i+variant)%8]:0);
        write_at(0x800cce08u+i*0x170u,2,(variant&128u)?statuses[(i+variant+3)%8]:0);
        unsigned rank_mode=(variant/32)%4;
        uint32_t rank=rank_mode==0?i*251u:rank_mode==1?65535u-i*251u:rank_mode==2?0x8000u:ranks[(i+variant)%8];
        write_at(0x800ccd34u+i*0x170u,2,rank);
        write_at(0x800c3ebeu+i*28u,2,ranks[(i+variant)%8]);
    }
    write_at(0x800d3364u,4,owner);
    if(entry==BATTLE_TARGET_ELIGIBILITY_ADDRESS) {
        write_at(0x800d2dccu+target,1,(variant&1u)?0x80:0);
        write_at(0x800c3eb7u+target*28u,1,(variant&2u)?0xff:0);
        write_at(0x800d32a1u+target*8u,1,(variant&4u)?0xff:0);
        write_at(0x800ccd64u+target*0x170u,2,statuses[(variant/16)%8]);
        write_at(0x800cce08u+target*0x170u,2,statuses[(variant/64)%8]);
        uint32_t delta=read_at(0x800c3eb4u+actor*28u,1)*64u+read_at(0x800c3eb4u+target*28u,1)*8u;
        write_at(owner+0x140u+delta,1,(variant&8u)?0x80:0);
    }
    if(entry==BATTLE_TARGET_PRIORITY_ADDRESS) {
        actor=slots[variant%8];target=slots[(variant/8)%8];
        cpu->gpr[4]=0xdead8000u|actor;cpu->gpr[5]=0xbeef8000u|target;
        write_at(0x800c3ebeu+actor*28u,2,ranks[(variant/32)%8]);
        write_at(0x800c3ebeu+target*28u,2,ranks[(variant/8)%8]);
    }
    if(entry==BATTLE_TARGET_LIST_ADDRESS) {
        const uint32_t stacks[]={cpu->gpr[29],0x800c3ea8u,0xa00c3ea8u,0x800d32b4u};
        if(variant%16==13) cpu->gpr[29]=stacks[(variant/16)%4];
        unsigned overlap=(variant/16)%4;
        if(overlap && variant%8==1) {
            for(unsigned i=0;i<256;i++) write_at(0x800c3eb4u+i*28u,1,0);
            const unsigned offsets[]={0,0x10,0x20,0x38};
            write_at(0x800d3364u,4,cpu->gpr[29]-0x50u+offsets[overlap]-0x140u);
        }
    }
    if(entry==BATTLE_STAGE2_COMMANDS_ADDRESS) {
        const uint32_t menus[]={0x80120000u,0xa0120000u,0x1f800800u,(uint32_t)(uintptr_t)target_memory+0x5000u};
        uint32_t menu=variant%16==14?0x800c3234u:menus[variant%4];
        for(unsigned i=0;i<0x400;i++) write_at(menu+i,1,i*19+variant);
        write_at(0x800c3eacu,4,menu);
        for(unsigned i=0;i<256;i++) write_at(0x8006ed0bu+i*32u,1,i*13+variant);
        const uint8_t party[]={0,7,9,127,255};
        for(unsigned i=0;i<11;i++) {
            write_at(0x800d2d24u+i,1,party[(i+variant)%5]);
            write_at(0x800ccd3eu+i*0x170u,1,(i+variant)%16);
            write_at(0x800ccd62u+i*0x170u,2,(i*0x4567u+variant)^0xa591u);
            write_at(0x800ccd88u+i*0x170u,1,(i+variant)&1u?0xff:0xfe);
        }
        for(unsigned i=0;i<19;i++) {
            write_at(0x800c20f0u+i*4u,4,0x801a0000u+i*16u+(variant&4u?0x20000000u:0));
            for(unsigned j=0;j<16;j++) write_at(0x801a0000u+i*16u+j,1,i*7+j+variant);
        }
        for(unsigned i=0;i<16;i++) write_at(0x800c3234u+i*2u,2,(1u<<i)^((variant&8u)?0x8001u:0));
        write_at(0x8006f9ddu,1,(variant%8)*32u);
    }
    frame_base=cpu->gpr[29]-0x100u;
}

static void target_snapshot(void)
{
    memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));memcpy(expected_target_memory,target_memory,sizeof(target_memory));
}

static int target_memory_equal(void)
{
    return !memcmp(expected_ram,g_PsxRam,sizeof(g_PsxRam)) && !memcmp(expected_shared,shared,sizeof(shared)) &&
           !memcmp(expected_pad,g_PsxScratchpad,sizeof(expected_pad)) && !memcmp(expected_target_memory,target_memory,sizeof(target_memory));
}

static int target_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;running_entry=entry;
    target_setup(&oracle,entry);candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_HALTED) { fail(oracle.error,oracle.pc);return 0; }
    unsigned expected_calls=calls;if(!oracle.steps) fail("target oracle did not execute",entry);target_snapshot();
    target_setup(&native,entry);candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,100000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc);return 0; }
    if(native.steps || rt.bridge_cpu || calls!=expected_calls ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("target context/calls/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("target preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
        fail("target return/HI/LO",entry);
        fprintf(stderr,"native V0/V1=%08x/%08x expected=%08x/%08x\n",native.gpr[2],native.gpr[3],oracle.gpr[2],oracle.gpr[3]);
    }
    if(!target_memory_equal()) {
        fail("target final memory",entry);unsigned shown=0;
        for(unsigned i=0;i<sizeof(g_PsxRam) && shown<12;i++) if(g_PsxRam[i]!=expected_ram[i]) {
            fprintf(stderr,"RAM %08x native=%02x expected=%02x\n",0x80000000u+i,g_PsxRam[i],expected_ram[i]);shown++;
        }
    }
    return !failed;
}

static int target_controls(void)
{
    PcPortMipsCpu cpu;variant=1;candidate=1;compare=0;
    for(unsigned t=0;t<3;t++) {
        running_entry=target_entries[t];target_setup(&cpu,running_entry);rt.bridge_cpu=&cpu;target_snapshot();
        const unsigned offsets[]={0,target_sizes[t]/2,target_sizes[t]-1};
        for(unsigned i=0;i<3;i++) {
            uint8_t *p=PSX_ADDR(target_entries[t]+offsets[i]);*p^=1;
            if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target code identity",i);
            *p^=1;
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target pending entry",t);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target forced reference",t);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_TARGET_LIST","0",1)==0);
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target reference switch",t);
        assert(unsetenv("XENO_BATTLE_NATIVE_TARGET_LIST")==0);g_ActiveBattleRuntime=NULL;
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target inactive runtime",t);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target wrong CPU",t);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_target_list_try_host(&rt,&cpu,target_entries[t])) fail("target uninitialized",t);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(calls || !target_memory_equal()) fail("target rejection effects",t);
    }
    for(unsigned i=0;i<3;i++) for(unsigned pending=0;pending<2;pending++) {
        running_entry=BATTLE_STAGE2_COMMANDS_ADDRESS;target_setup(&cpu,running_entry);
        if(pending) pending_return=target_entries[i];else stop_target=target_entries[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        if(PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
           PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions ||
           (!pending && PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1))
            fail("target nested refusal",i);
        if(read_at(0x800c3d48u,1)!=1) fail("target earlier command write lost",i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    }
    PcPortMipsCpu oracle;running_entry=BATTLE_TARGET_LIST_ADDRESS;target_setup(&oracle,running_entry);oracle.gpr[29]=0x80000020u;
    candidate=compare=0;
    if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,10000)==PC_PORT_MIPS_HALTED || !strstr(oracle.error,"write8 fault at 0x7fffffe0"))
        fail("target original invalid-frame prefix",oracle.pc);
    target_snapshot();target_setup(&cpu,running_entry);cpu.gpr[29]=0x80000020u;candidate=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    if(PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,10000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
       cpu.gpr[29]!=oracle.gpr[29] || !target_memory_equal()) fail("target invalid-frame prefix",0);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    target_setup(&cpu,running_entry);write_at(0x800d3364u,4,0x1f801000u);
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    if(PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,10000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu)
        fail("target invalid adjacency pointer",0);
    if(read_at(0x800d3274u,1)!=0 || read_at(0x800c3e90u,1)!=255) fail("target invalid pointer lost writes",0);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    return !failed;
}

int main(int argc,char **argv)
{
    assert(argc==2);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
    FILE *image=fopen("disc/battle.bin","rb");assert(image && fread(target_image,1,sizeof(target_image),image)==sizeof(target_image));assert(fclose(image)==0);
    image=fopen(argv[1],"rb");assert(image && fread(module,1,sizeof(module),image)==sizeof(module));assert(fclose(image)==0);
    assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    for(variant=0;variant<512;variant++) if(!target_case(BATTLE_TARGET_ELIGIBILITY_ADDRESS)) return 1;
    for(variant=0;variant<256;variant++) if(!target_case(BATTLE_TARGET_PRIORITY_ADDRESS)) return 1;
    puts("BATTLE TARGET LIST PASS 768 original eligibility/priority cases");
    for(variant=0;variant<256;variant++) if(!target_case(BATTLE_TARGET_LIST_ADDRESS)) return 1;
    for(variant=0;variant<32;variant++) if(!target_case(BATTLE_STAGE2_COMMANDS_ADDRESS)) return 1;
    if(!target_controls()) return 1;
    puts("BATTLE TARGET LIST PASS 1056 differential cases: 512 eligibility, 256 priority, 256 lists, 32 complete command callers; no modeled game callees; full memory/frames/HI-LO, aliases/identity/refusal; native steps=0");
    return 0;
}
