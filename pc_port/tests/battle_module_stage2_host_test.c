/* Original stage-two callers versus native C, including the dispatcher.
 * Game callees are explicit models; this does not accept their native bodies. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

static const uint32_t stage2_entries[]={0x801e4cd0u,0x801e4e7cu,0x801e5014u,0x801e5840u};
static const unsigned stage2_sizes[]={0x1ac,0x198,0x370,0xe4};
static _Alignas(4) uint8_t host_menus[0x2000],expected_menus[sizeof(host_menus)];
static uint32_t menu_bases[2];
static unsigned random_draws;
static uint8_t menu_trace_hash[64][32];

static void stage2_write(uint32_t address,unsigned width,uint32_t value)
{
    if(runtime_write(&rt,address,width,value)!=0) {
        fail("stage-two fixture address",address);exit(2);
    }
}
#define write_at stage2_write

static int stage2_model(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    if(stop_target && target-stop_target<12u) return 0;
    for(unsigned i=0;i<4;i++) {
        if(candidate && target==stage2_entries[i]) return runtime_bridge(opaque,cpu,target);
        if(target-stage2_entries[i]<stage2_sizes[i]) return 0;
    }
    unsigned argc;
    switch(target) {
    case 0x80078508u:case 0x800841e0u:argc=1;break;
    case 0x8001bd40u:case 0x80085310u:case 0x8008abb8u:case 0x8003f8e8u:argc=2;break;
    default:fail("unexpected stage-two callee",target);return -1;
    }
    if(calls>=64) { fail("stage-two trace capacity",calls);return -1; }
    Trace trace={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
    memcpy(trace.saved,cpu->gpr+16,8*sizeof(uint32_t));trace.saved[8]=cpu->gpr[30];
    memcpy(trace.frame,resolve_memory(&rt,frame_base,sizeof(trace.frame),0),sizeof(trace.frame));
    memcpy(trace.args,cpu->gpr+4,argc*sizeof(uint32_t));
    assert(SHA256(shared,sizeof(shared),trace.shared_hash));
    assert(SHA256(g_PsxRam,sizeof(g_PsxRam),trace.ram_hash));
    assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),trace.pad_hash));
    uint8_t menu_hash[32];assert(SHA256(host_menus,sizeof(host_menus),menu_hash));
    if(compare && memcmp(menu_hash,menu_trace_hash[calls],sizeof(menu_hash)))
        fail("registered menu memory at callee",calls);
    if(!compare) memcpy(menu_trace_hash[calls],menu_hash,sizeof(menu_hash));
    if(compare && memcmp(&trace,&traces[calls],sizeof(trace))) {
        const Trace *want=&traces[calls];
        if(trace.target!=want->target) fail("target",want->target);
        if(trace.ra!=want->ra || trace.sp!=want->sp) fail("RA/SP",trace.ra);
        for(unsigned i=0;i<9;i++) if(trace.saved[i]!=want->saved[i]) fail("saved register",i);
        for(unsigned i=0;i<3;i++) if(trace.args[i]!=want->args[i]) fail("argument",i);
        if(memcmp(trace.frame,want->frame,sizeof(trace.frame))) fail("frame",frame_base);
        fail("stage-two call/memory",target);return -1;
    }
    if(!compare) traces[calls]=trace;
    calls++;
    cpu->gpr[2]=0x90000000u+calls;cpu->gpr[3]=0x60000000u+calls;
    cpu->hi=0x12300000u+calls;cpu->lo=0x45600000u+calls;
    write_at(0x80059470u,4,calls);
    switch(target) {
    case 0x80078508u:
        for(unsigned i=0;i<11;i++) write_at(trace.args[0]+i,1,0);
        if(variant&16u) write_at(0x800d2dccu+5,1,0);
        break;
    case 0x8001bd40u:
        if(trace.args[0]!=0 || trace.args[1]!=10) fail("random range",trace.args[1]);
        cpu->gpr[2]=0xface0000u|((variant*7u+(random_draws/(variant&8u?2u:1u))*3u)%11u);
        random_draws++;
        break;
    case 0x800841e0u:case 0x80085310u: {
        uint32_t slot=trace.args[0];
        if(slot>=11) { fail("command slot",slot);return -1; }
        cpu->gpr[2]=target==0x800841e0u ? 0xface00f0u+variant+slot : 0xbeef00e0u+(trace.args[1]^slot);
        if(variant&16u) {
            write_at(0x800c3eacu,4,read_at(0x800c3eacu,4)==menu_bases[0]?menu_bases[1]:menu_bases[0]);
            write_at(0x8006f9ddu,1,read_at(0x8006f9ddu,1)^0xe0u);
            write_at(0x800ccd62u+slot*0x170u,2,read_at(0x800ccd62u+slot*0x170u,2)^0x24a5u);
            write_at(0x800c3242u,2,read_at(0x800c3242u,2)^0x4020u);
            if(slot<3) write_at(0x800d2d24u+slot,1,7);
        }
        break;
    }
    case 0x8008abb8u:
        if(trace.args[0]!=0xec || trace.args[1]!=0) fail("allocation args",trace.args[0]);
        cpu->gpr[2]=variant&8u ? 0xa0180000u:0x80180000u;break;
    case 0x8003f8e8u:
        if(trace.args[1]!=0xec) fail("zero size",trace.args[1]);
        for(unsigned i=0;i<trace.args[1];i++) write_at(trace.args[0]+i,1,0);
        break;
    }
    if(pending_return==target) {
        cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeed0100;cpu->load_mask=0;
    }
    return 1;
}

static void stage2_setup(PcPortMipsCpu *cpu,uint32_t entry)
{
    setup(cpu);sdk=0;cpu->bus.bridge=stage2_model;cpu->gpr[4]=0xabcd0002u;random_draws=0;
    memset(host_menus,0x93,sizeof(host_menus));assert((uintptr_t)host_menus<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)host_menus,(uintptr_t)host_menus+sizeof(host_menus),1};
    rt.host_range_count=1;
    const uint32_t bases[]={0x80120000u,0xa0120000u,0x1f800800u,(uint32_t)(uintptr_t)host_menus};
    menu_bases[0]=bases[variant%4];
    menu_bases[1]=menu_bases[0]+(variant%4==3?0x1000u:variant%4==2?0x400u:0x10000u);
    if(entry==BATTLE_STAGE2_COMMANDS_ADDRESS && variant%16==14) menu_bases[0]=0x800c3234u;
    for(unsigned j=0;j<2;j++) for(unsigned i=0;i<0x400;i++) write_at(menu_bases[j]+i,1,i*19+variant+j);
    write_at(0x800c3eacu,4,menu_bases[0]);
    if(entry==BATTLE_STAGE2_ITEMS_ADDRESS && variant%16==14)
        write_at(0x800c3eacu,4,0x8006f3d0u-0x2d9u);
    for(unsigned i=0;i<256;i++) write_at(0x8006ed0bu+i*32u,1,i*13+variant);
    const uint8_t ids[]={0,1,48,49,50,72,73,255},quantities[]={0,1,98,99,100,255};
    const unsigned primary_counts[]={0,1,47,48,49,149,150,73};
    for(unsigned i=0;i<150;i++) {
        uint32_t id=i<primary_counts[variant%8] ? (variant&8u?ids[(i+variant)%8]:1u+i%48u):0;
        write_at(0x8006f65au+i,1,id);
        write_at(0x8006f5c4u+i,1,variant&32u?quantities[(i+variant)%6]:100u+i%156u);
    }
    const unsigned secondary_counts[]={0,1,47,48,49,72,99,100};
    for(unsigned i=0;i<100;i++) {
        uint32_t id=i<secondary_counts[(variant/2)%8] ? 50u+(i+variant)%23u:73;
        if(variant&32u) id=ids[(i+variant)%8];
        write_at(0x8006f3d0u+i,1,id);write_at(0x8006f36cu+i,1,quantities[(i+variant)%6]);
    }
    const uint16_t times[]={0,1,2,0x7fff,0x8000,0xffff,200,900};
    for(unsigned i=0;i<11;i++) {
        unsigned active=variant%4==0?0:variant%4==1?1:(i+variant)%2;
        write_at(0x800d2dccu+i,1,active);
        write_at(0x800d2df0u+i*2u,2,times[(i+variant+3)%8]);
        write_at(0x800d2e06u+i*2u,2,times[(i+variant)%8]);
        write_at(0x800ccd1cu+i*0x170u,2,(variant+i)&2u?0x200:0);
        write_at(0x800ccd3eu+i*0x170u,1,(i+variant)%16);
        write_at(0x800ccd62u+i*0x170u,2,(i*0x4567u+variant)^0xa591u);
        write_at(0x800ccd88u+i*0x170u,1,(i+variant)&1u?0xff:0xfe);
    }
    const uint8_t party[]={0,7,9,0x7f,0xff};
    for(unsigned i=0;i<3;i++) write_at(0x800d2d24u+i,1,party[(i+variant)%5]);
    for(unsigned i=0;i<19;i++) {
        write_at(0x800c20f0u+i*4u,4,0x801a0000u+i*16u+(variant&4u?0x20000000u:0));
        for(unsigned j=0;j<16;j++) write_at(0x801a0000u+i*16u+j,1,i*7+j+variant);
    }
    for(unsigned i=0;i<16;i++) write_at(0x800c3234u+i*2u,2,(1u<<i)^((variant&8u)?0x8001u:0));
    write_at(0x8006f9ddu,1,(variant%8)*32u);
}

static int stage2_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;running_entry=entry;
    stage2_setup(&oracle,entry);candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_HALTED) {
        fail(oracle.error,oracle.pc);return 0;
    }
    unsigned expected_calls=calls;
    if(!oracle.steps) fail("stage-two oracle did not execute",entry);
    memcpy(expected_ram,g_PsxRam,sizeof(g_PsxRam));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));memcpy(expected_menus,host_menus,sizeof(host_menus));
    stage2_setup(&native,entry);candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,50000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc);return 0; }
    if(native.steps || rt.bridge_cpu || calls!=expected_calls ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("stage-two context/calls/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
        fail("stage-two return/HI/LO",native.gpr[3]);
        fprintf(stderr,"native v0/v1=%08x/%08x expected=%08x/%08x\n",native.gpr[2],native.gpr[3],oracle.gpr[2],oracle.gpr[3]);
    }
    if(memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
       memcmp(g_PsxScratchpad,expected_pad,sizeof(expected_pad)) || memcmp(host_menus,expected_menus,sizeof(host_menus))) {
        fail("stage-two final memory",entry);unsigned shown=0;
        for(unsigned i=0;i<sizeof(g_PsxRam) && shown<12;i++) if(g_PsxRam[i]!=expected_ram[i]) {
            fprintf(stderr,"RAM %08x native=%02x expected=%02x\n",0x80000000u+i,g_PsxRam[i],expected_ram[i]);shown++;
        }
    }
    return !failed;
}

static int stage2_controls(void)
{
    PcPortMipsCpu cpu;variant=1;candidate=1;compare=0;
    for(unsigned t=0;t<3;t++) {
        stage2_setup(&cpu,stage2_entries[t]);rt.bridge_cpu=&cpu;
        memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));memcpy(expected_shared,shared,sizeof(shared));
        for(unsigned region=0;region<2;region++) {
            unsigned r=region?t:3,offsets[]={0,stage2_sizes[r]/2,stage2_sizes[r]-1};
            for(unsigned j=0;j<3;j++) {
                uint8_t *p=PSX_ADDR(stage2_entries[r]+offsets[j]);*p^=1;
                if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two code identity",r);
                *p^=1;
            }
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two pending entry",t);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two forced reference",t);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_MODULE_STAGE2","0",1)==0);
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two reference switch",t);
        assert(unsetenv("XENO_BATTLE_NATIVE_MODULE_STAGE2")==0);g_ActiveBattleRuntime=NULL;
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two inactive runtime",t);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two wrong CPU",t);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_module_stage2_try_host(&rt,&cpu,stage2_entries[t])) fail("stage-two uninitialized",t);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(calls || memcmp(expected_ram,g_PsxRam,sizeof(g_PsxRam)) || memcmp(expected_shared,shared,sizeof(shared)))
            fail("stage-two rejection effects",t);
    }
    const uint32_t stops[]={0x80078508u,0x8001bd40u,0x800841e0u,0x80085310u,0x8008abb8u,0x8003f8e8u};
    for(unsigned i=0;i<6;i++) {
        for(unsigned pending=0;pending<2;pending++) {
            stage2_setup(&cpu,BATTLE_MODULE_DISPATCH_ADDRESS);
            if(pending) pending_return=stops[i];else stop_target=stops[i];
            assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
            PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
            if(PcPortMipsRun(&cpu,BATTLE_MODULE_DISPATCH_ADDRESS,BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_UNRESOLVED_CALL ||
               cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions ||
               (!pending && PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1))
                fail("stage-two callee refusal",i);
            if(read_at(read_at(0x800c3eacu,4)+0x2d9u,1)!=47) fail("stage-two earlier writes lost",i);
            assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        }
    }
    stage2_setup(&cpu,BATTLE_STAGE2_ITEMS_ADDRESS);write_at(0x800c3eacu,4,0x1f800ff0u);
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    if(PcPortMipsRun(&cpu,BATTLE_STAGE2_ITEMS_ADDRESS,BATTLE_HALT_PC,50000)!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu)
        fail("stage-two invalid menu pointer",0);
    if(read_at(0x800d2fe4u+47u,1)!=0) fail("stage-two invalid range lost writes",0);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    return !failed;
}

int main(int argc,char **argv)
{
    assert(argc==2);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
    FILE *image=fopen(argv[1],"rb");assert(image && fread(module,1,sizeof(module),image)==sizeof(module));
    assert(fclose(image)==0);assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    for(variant=0;variant<96;variant++) if(!stage2_case(BATTLE_STAGE2_ITEMS_ADDRESS)) return 1;
    puts("BATTLE MODULE STAGE2 PASS 96 original-body inventory cases");
    for(unsigned i=1;i<4;i++) for(variant=0;variant<32;variant++) if(!stage2_case(stage2_entries[i])) return 1;
    if(!stage2_controls()) return 1;
    puts("BATTLE MODULE STAGE2 PASS 192 differential cases: 96 inventory, 32 turn order, 32 commands, 32 composed; modeled callees; full memory/frames/HI-LO, live pointers, aliases and refusal controls; native steps=0");
    return 0;
}
