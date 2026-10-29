/* Original reset/speed/flag bodies versus native code, with both synthetic
 * RNG boundaries and the actual native RNG services. The independent service
 * checks execute the original main-executable range/RNG routines as oracles. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

extern uint32_t g_RandomSeed;
extern uint8_t func_8001BD40(uint8_t minimum,uint8_t maximum);
static uint8_t battle_image[343936],range_code[156],random_code[48];
static _Alignas(4) uint8_t workspace[0x30000],expected_workspace[sizeof(workspace)];
static uint32_t bases[2],output_pointer;
static unsigned random_draws;
static int sdk_rng,raw_services,service_only;
static const uint32_t turn_entries[]={0x80078508u,0x80098af8u,0x80094d24u,0x801e4e7cu};
static const unsigned turn_sizes[]={204,372,448,408};
typedef struct TurnTrace {
    uint32_t target,ra,sp,saved[9],args[2],hi,lo,seed;
    uint8_t frame[0x100],ram_hash[32],shared_hash[32],pad_hash[32],workspace_hash[32];
} TurnTrace;
static TurnTrace turn_traces[256];

static void turn_trace(PcPortMipsCpu *cpu,uint32_t target,unsigned argc)
{
    if(service_only || (target==running_entry && cpu->gpr[31]==BATTLE_HALT_PC)) return;
    if(calls>=256) { fail("turn trace capacity",calls);exit(2); }
    TurnTrace trace={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo,.seed=g_RandomSeed};
    memcpy(trace.saved,cpu->gpr+16,8*sizeof(uint32_t));trace.saved[8]=cpu->gpr[30];
    memcpy(trace.args,cpu->gpr+4,argc*sizeof(uint32_t));
    memcpy(trace.frame,resolve_memory(&rt,frame_base,sizeof(trace.frame),0),sizeof(trace.frame));
    assert(SHA256(g_PsxRam,sizeof(g_PsxRam),trace.ram_hash));assert(SHA256(shared,sizeof(shared),trace.shared_hash));
    assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),trace.pad_hash));
    assert(SHA256(workspace,sizeof(workspace),trace.workspace_hash));
    if(compare && memcmp(&trace,&turn_traces[calls],sizeof(trace))) {
        const TurnTrace *want=&turn_traces[calls];
        if(trace.target!=want->target) fail("turn target",want->target);
        if(trace.ra!=want->ra || trace.sp!=want->sp) fail("turn RA/SP",trace.ra);
        for(unsigned i=0;i<9;i++) if(trace.saved[i]!=want->saved[i]) fail("turn saved register",i);
        for(unsigned i=0;i<2;i++) if(trace.args[i]!=want->args[i]) fail("turn argument",i);
        if(memcmp(trace.frame,want->frame,sizeof(trace.frame))) fail("turn frame",frame_base);
        fail("turn call/memory/state",target);
    }
    if(!compare) turn_traces[calls]=trace;
    calls++;
}

static int turn_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if(stop_target && target-stop_target<12u) return 0;
    if(raw_services && (target-0x8001bd40u<sizeof(range_code) || target-0x8003fa38u<sizeof(random_code))) return 0;
    for(unsigned i=0;i<4;i++) {
        if(target==turn_entries[i]) {
            turn_trace(cpu,target,i==0?1:i==1?2:0);
            int rc=candidate?runtime_bridge(opaque,cpu,target):0;
            if(rc==1 && pending_return==target) {
                cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;
            }
            return rc;
        }
        if(target-turn_entries[i]<turn_sizes[i]) return 0;
    }
    if(target!=0x8003fa38u && target!=0x8001bd40u) { fail("unexpected turn callee",target);return -1; }
    turn_trace(cpu,target,target==0x8001bd40u?2:0);
    int rc=1;
    if(sdk_rng || target==0x8001bd40u) rc=runtime_bridge(opaque,cpu,target);
    else {
        const uint32_t values[]={0,1,7,8,0x7fffffffu,0x80000000u,0xfffffffdu,0xfffffffbu};
        cpu->gpr[2]=values[(variant+random_draws)%8];cpu->gpr[3]=0x63000000u+calls;
        cpu->hi=0x93000000u+calls;cpu->lo=0x33000000u+calls;
        g_RandomSeed=g_RandomSeed*1664525u+1013904223u;
        if(variant&64u) {
            write_at(0x800c34b0u,4,read_at(0x800c34b0u,4)==bases[0]?bases[1]:bases[0]);
            write_at(0x800d2dccu+1u+(random_draws%10u),1,0);
        }
    }
    if(target==0x8003fa38u) random_draws++;
    if(rc==1 && pending_return==target) {
        cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;
    }
    return rc;
}

static void turn_setup(PcPortMipsCpu *cpu,uint32_t entry)
{
    setup(cpu);cpu->bus.bridge=turn_bridge;random_draws=0;raw_services=service_only=0;
    frame_base=cpu->gpr[29]-0x100u;
    for(unsigned i=0;i<3;i++) memcpy(PSX_ADDR(turn_entries[i]),battle_image+(turn_entries[i]-BATTLE_BASE),turn_sizes[i]);
    memcpy(PSX_ADDR(0x8001bd40u),range_code,sizeof(range_code));
    memcpy(PSX_ADDR(0x8003fa38u),random_code,sizeof(random_code));
    g_RandomSeed=0x789abcdeu+variant*0x45d9f3bu;
    rt.data[0]=(ResolvedData){0x80050000u,0xa1fcu,shared,"shared-5-prefix"};
    rt.data[1]=(ResolvedData){0x8005a1fcu,4,(uint8_t *)&g_RandomSeed,"rng"};
    rt.data[2]=(ResolvedData){0x8005a200u,0x5e00u,shared+0xa200u,"shared-5-suffix"};
    rt.data[3]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};
    rt.data_count=4;
    const ResolvedFunction functions[]={
        {0x8001bd40u,(void *)func_8001BD40,"func_8001BD40"},
        {0x8003fa38u,(void *)rand,"rand"}};
    memcpy(rt.functions,functions,sizeof(functions));rt.function_count=2;
    memset(workspace,0xa7,sizeof(workspace));assert((uintptr_t)workspace<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)workspace,(uintptr_t)workspace+sizeof(workspace),1};rt.host_range_count=1;
    const uint32_t choices[]={0x80120000u,0xa0120000u,(uint32_t)(uintptr_t)workspace};
    bases[0]=choices[variant%3];bases[1]=bases[0]+0x18000u;
    const uint8_t stats[]={0,1,9,18,19,20,165,255};
    const uint8_t penalties[]={0,1,255,128,127,17,18,20};
    const uint8_t selections[]={0,1,7,37,255};
    const uint16_t flags[]={0,2,0x4000,0x4002,0x8000,0x8002,0xc000,0xc002};
    for(unsigned b=0;b<2;b++) {
        uint32_t base=bases[b],selected=selections[(variant/16+b)%5];
        write_at(base+0x5fc2u,1,selected);
        for(unsigned i=0;i<256;i++) {
            uint32_t record=base+i*0x170u;
            write_at(record+0x5au,1,stats[(variant/8+i+b)%8]);
            write_at(record+0x13cu,1,stats[(variant/16+i+b+3)%8]);
            write_at(record+0x15au,1,(variant+i)&16u?0x80:0);
        }
        for(unsigned i=0;i<3;i++) {
            uint32_t record=base+i*0x170u;
            write_at(record+0x7cu,2,flags[(variant>>(i*3))&7]|((i*0x314u+variant*8u)&0x3ffdu));
            write_at(record+0x7au,2,0x9a00u+i+variant);
            write_at(base+0x1058u+i*0x5f0u+selected*0x28u+0x27u,1,penalties[(variant/32+i+b)%8]);
            write_at(base+0x2228u+i*0x690u+selected*0x28u+0x27u,1,penalties[(variant/8+i+b)%8]);
        }
    }
    if(entry==BATTLE_TURN_FLAGS_ADDRESS && variant%4==2) {
        uint32_t scratch=0x1f800800u;
        for(unsigned i=0;i<3;i++) {
            write_at(scratch+i*0x170u+0x7cu,2,read_at(bases[0]+i*0x170u+0x7cu,2));
            write_at(scratch+i*0x170u+0x7au,2,0xabcd);
        }
        bases[0]=scratch;
    }
    write_at(0x800c34b0u,4,bases[0]);
    for(unsigned i=0;i<3;i++) write_at(0x800c3aa4u+i*2u,2,0x8137u+i*0x143u+variant);
    for(unsigned i=0;i<11;i++) {
        unsigned active=variant%4==0?0:variant%4==1?1:((i+variant)&1u);
        write_at(0x800d2dccu+i,1,active);
        write_at(0x800ccd1cu+i*0x170u,2,(variant+i)&2u?0x200:0);
    }
    const uint32_t outputs[]={0x80190000u,0xa0190000u,0x1f800e00u,(uint32_t)(uintptr_t)workspace+0x17000u,
                              0x800d2dcdu,0x800d2df0u,cpu->gpr[29]-0x20u,0x800c3aa4u};
    output_pointer=outputs[(variant/4)%8];
    if(entry==BATTLE_TURN_RESET_ADDRESS) cpu->gpr[4]=output_pointer;
    else if(entry==BATTLE_TURN_SPEED_ADDRESS) {
        const uint8_t slots[]={0,1,2,3,7,10,127,255};cpu->gpr[4]=0xdead8000u|slots[variant%8];
    }
}

static int turn_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;running_entry=entry;
    turn_setup(&oracle,entry);candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,200000)!=PC_PORT_MIPS_HALTED) { fail(oracle.error,oracle.pc);return 0; }
    unsigned expected_calls=calls;uint32_t seed=g_RandomSeed;
    if(!oracle.steps) fail("turn oracle did not execute",entry);
    memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));memcpy(expected_workspace,workspace,sizeof(workspace));
    turn_setup(&native,entry);candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,200000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc);return 0; }
    if(native.steps || rt.bridge_cpu || calls!=expected_calls ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("turn context/calls/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("turn preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo || seed!=g_RandomSeed) {
        fail("turn return/HI/LO/RNG",entry);
        fprintf(stderr,"native %08x %08x %08x %08x expected %08x %08x %08x %08x\n",
            native.gpr[2],native.gpr[3],native.hi,native.lo,oracle.gpr[2],oracle.gpr[3],oracle.hi,oracle.lo);
    }
    if(memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
       memcmp(g_PsxScratchpad,expected_pad,sizeof(expected_pad)) || memcmp(workspace,expected_workspace,sizeof(workspace))) {
        fail("turn final memory",entry);unsigned shown=0;
        for(unsigned i=0;i<sizeof(g_PsxRam) && shown<12;i++) if(g_PsxRam[i]!=expected_ram[i]) {
            fprintf(stderr,"RAM %08x native=%02x expected=%02x\n",0x80000000u+i,g_PsxRam[i],expected_ram[i]);shown++;
        }
    }
    return !failed;
}

static int service_cases(void)
{
    PcPortMipsCpu cpu;variant=1;turn_setup(&cpu,BATTLE_TURN_SPEED_ADDRESS);service_only=1;sdk_rng=1;compare=0;
    for(unsigned group=0;group<2;group++) {
        uint32_t entry=group?0x8001bd40u:0x8003fa38u;running_entry=entry;
        unsigned count=group?65536u:512u;
        for(unsigned i=0;i<count;i++) {
            uint32_t seed=0x8badf00du+i*0x45d9f3bu;
            g_RandomSeed=seed;initialize_cpu(&cpu,&rt);cpu.bus.bridge=turn_bridge;
            cpu.gpr[4]=0x80000000u|(i>>8);cpu.gpr[5]=0xa0000000u|(i&255u);
            raw_services=1;candidate=0;
            if(PcPortMipsRun(&cpu,entry,BATTLE_HALT_PC,1000)!=PC_PORT_MIPS_HALTED) { fail(cpu.error,cpu.pc);return 0; }
            uint32_t value=cpu.gpr[2],expected_seed=g_RandomSeed;
            g_RandomSeed=seed;initialize_cpu(&cpu,&rt);cpu.bus.bridge=turn_bridge;
            cpu.gpr[4]=0x80000000u|(i>>8);cpu.gpr[5]=0xa0000000u|(i&255u);
            raw_services=0;candidate=1;
            PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
            if(runtime_bridge(&rt,&cpu,entry)!=1 || cpu.gpr[2]!=value || g_RandomSeed!=expected_seed || cpu.steps ||
               PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) {
                fail("native RNG service return/state",i);return 0;
            }
        }
    }
    service_only=0;
    puts("BATTLE TURN RNG PASS 512 original RNG seeds and 65536 original byte-range pairs; return/RNG-state contract, native steps=0");
    return !failed;
}

static int turn_controls(void)
{
    PcPortMipsCpu cpu;variant=1;candidate=1;compare=0;sdk_rng=1;
    for(unsigned t=0;t<3;t++) {
        turn_setup(&cpu,turn_entries[t]);rt.bridge_cpu=&cpu;
        memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));memcpy(expected_shared,shared,sizeof(shared));
        const unsigned offsets[]={0,turn_sizes[t]/2,turn_sizes[t]-1};
        for(unsigned i=0;i<3;i++) {
            uint8_t *p=PSX_ADDR(turn_entries[t]+offsets[i]);*p^=1;
            if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn code identity",i);
            *p^=1;
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn pending entry",t);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn forced reference",t);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_TURN_SPEED","0",1)==0);
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn reference switch",t);
        assert(unsetenv("XENO_BATTLE_NATIVE_TURN_SPEED")==0);g_ActiveBattleRuntime=NULL;
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn inactive runtime",t);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn wrong CPU",t);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_turn_speed_try_host(&rt,&cpu,turn_entries[t])) fail("turn uninitialized",t);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(calls || memcmp(expected_ram,g_PsxRam,sizeof(g_PsxRam)) || memcmp(expected_shared,shared,sizeof(shared)))
            fail("turn rejection effects",t);
    }
    const uint32_t targets[]={BATTLE_TURN_SPEED_ADDRESS,BATTLE_TURN_FLAGS_ADDRESS,0x8003fa38u,0x8001bd40u};
    for(unsigned i=0;i<4;i++) for(unsigned pending=0;pending<2;pending++) {
        running_entry=BATTLE_STAGE2_ORDER_ADDRESS;turn_setup(&cpu,running_entry);
        if(pending) pending_return=targets[i];else stop_target=targets[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        if(PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,200000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
           PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions ||
           (!pending && PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1))
            fail("turn nested refusal",i);
        if(read_at(0x800d2caau,1)!=0) fail("turn earlier writes lost",i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    }
    running_entry=BATTLE_TURN_RESET_ADDRESS;turn_setup(&cpu,running_entry);cpu.gpr[4]=0x1f801000u;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    if(PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,200000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu)
        fail("turn invalid output",0);
    if(read_at(0x800d2de4u,1)!=0) fail("turn completed slot writes lost",0);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    return !failed;
}

int main(int argc,char **argv)
{
    assert(argc==2 || argc==3);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
    FILE *image=fopen("disc/battle.bin","rb");assert(image && fread(battle_image,1,sizeof(battle_image),image)==sizeof(battle_image));assert(fclose(image)==0);
    image=fopen("disc/SLUS_006.64","rb");assert(image);
    assert(fseek(image,0x8001bd40u-0x8000f800u,SEEK_SET)==0 && fread(range_code,1,sizeof(range_code),image)==sizeof(range_code));
    assert(fseek(image,0x8003fa38u-0x8000f800u,SEEK_SET)==0 && fread(random_code,1,sizeof(random_code),image)==sizeof(random_code));assert(fclose(image)==0);
    image=fopen(argv[1],"rb");assert(image && fread(module,1,sizeof(module),image)==sizeof(module));assert(fclose(image)==0);
    assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    if(!service_cases()) return 1;
    if(argc==3) { assert(strcmp(argv[2],"--range-only")==0);return 0; }
    sdk_rng=0;
    for(variant=0;variant<512;variant++) if(!turn_case(BATTLE_TURN_FLAGS_ADDRESS)) return 1;
    puts("BATTLE TURN FLAGS PASS 512 original-body cases");
    for(variant=0;variant<256;variant++) if(!turn_case(BATTLE_TURN_SPEED_ADDRESS)) return 1;
    for(variant=0;variant<64;variant++) if(!turn_case(BATTLE_TURN_RESET_ADDRESS)) return 1;
    puts("BATTLE TURN SPEED PASS 320 game-body cases with synthetic RNG boundary");
    sdk_rng=1;
    for(variant=0;variant<64;variant++) if(!turn_case(BATTLE_TURN_RESET_ADDRESS)) return 1;
    for(variant=0;variant<32;variant++) if(!turn_case(BATTLE_STAGE2_ORDER_ADDRESS)) return 1;
    if(!turn_controls()) return 1;
    puts("BATTLE TURN SPEED PASS 928 game-body differential cases: 512 flags, 256 speed, 128 reset, 32 full turn-order; 96 actual RNG/range integrations; full memory/frames/HI-LO, aliases/identity/refusal; native steps=0");
    return 0;
}
