/* Reuse the stage-1 bus, SDK fixtures and full-memory comparisons. Unlike its
 * modeled stat callees, this suite executes both original stat bodies and the
 * production native bodies, including through modes and the full dispatcher. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

static uint8_t battle_image[343936];
static _Alignas(4) uint8_t host_workspace[0x6000],expected_host[0x6000];
static uint32_t workspace_address;

static int party_model(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    const uint32_t addresses[]={BATTLE_PARTY_STATS_ADDRESS,BATTLE_PARTY_SPECIAL_ADDRESS};
    const unsigned lengths[]={BATTLE_PARTY_STATS_BYTES,BATTLE_PARTY_SPECIAL_BYTES};
    for(unsigned i=0;i<2;i++) {
        if(candidate && target==addresses[i]) return runtime_bridge(opaque,cpu,target);
        if(target-addresses[i]<lengths[i]) return 0;
    }
    return model(opaque,cpu,target);
}

static void party_setup(PcPortMipsCpu *cpu,uint32_t entry)
{
    setup(cpu);sdk=1;cpu->bus.bridge=party_model;
    memcpy(PSX_ADDR(BATTLE_PARTY_STATS_ADDRESS),battle_image+(BATTLE_PARTY_STATS_ADDRESS-BATTLE_BASE),BATTLE_PARTY_STATS_BYTES);
    memcpy(PSX_ADDR(BATTLE_PARTY_SPECIAL_ADDRESS),battle_image+(BATTLE_PARTY_SPECIAL_ADDRESS-BATTLE_BASE),BATTLE_PARTY_SPECIAL_BYTES);
    memset(host_workspace,0x69,sizeof(host_workspace));
    assert((uintptr_t)host_workspace<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)host_workspace,(uintptr_t)host_workspace+sizeof(host_workspace),1};
    rt.host_range_count=1;
    if(entry==BATTLE_MODULE_DISPATCH_ADDRESS || entry==BATTLE_STAGE1_MODES_ADDRESS)
        workspace_address=0x800ccce8u;
    else if(entry==BATTLE_PARTY_SPECIAL_ADDRESS && variant%4==3)
        workspace_address=0x1f800800u;
    else {
        const uint32_t bases[]={0x80120000u,0xa0120000u,(uint32_t)(uintptr_t)host_workspace};
        workspace_address=bases[variant%3];
    }
    uint32_t seed=0x6d2b79f5u^(variant*0x45d9f3bu);
    unsigned length=entry==BATTLE_PARTY_SPECIAL_ADDRESS ? 0x600 : 0x6000;
    for(unsigned i=0;i<length;i++) {
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
        write_at(workspace_address+i,1,seed>>24);
    }
    write_at(0x800c34b0u,4,workspace_address);
    const uint8_t ids[]={0,4,7,9,10,31,0x7f,0xff};
    const uint16_t elements[]={0,0x1c00,0x380,0x70,0x1ff0,0x80,0x1000,0xffff};
    for(unsigned i=0;i<8;i++) {
        write_at(0x8006d634u+0x16c4u+ids[i]*0x20u,2,elements[(variant+i)%8]);
        write_at(0x8006d634u+0x16dau+ids[i]*0x20u,2,0xffffu-variant);
    }
    const uint16_t hp[]={0,1,19,20,99,100,999,1000,65535,65000};
    const uint8_t scales[]={0,1,19,20,21,255};
    const uint32_t gear_hp[]={0,9,10,99,1000,0x7fffffffu,0x80000000u,0xffffffffu};
    const uint8_t special[]={0,0x0f,0x12,0x11};
    for(unsigned i=0;i<3;i++) {
        uint32_t record=workspace_address+i*0x170u,stats=record+0xa4u;
        unsigned id=(variant&(1u<<i)) ? ids[(variant/8+i)%8] : 0x7f;
        write_at(0x800d2d24u+i,1,id);write_at(0x800c3eb6u+i*0x1cu,1,id);
        write_at(record+0x56u,1,ids[(variant/3+i)%8]);
        write_at(record+0x4cu,2,hp[(variant+i)%10]);write_at(record+0x4eu,2,hp[(variant+i+3)%10]);
        write_at(record+0x50u,2,hp[(variant+i+5)%10]);write_at(record+0x52u,2,hp[(variant+i+7)%10]);
        write_at(record+0x30u,1,scales[(variant/10+i)%6]);write_at(record+0x31u,1,scales[(variant/6+i)%6]);
        write_at(record+0x32u,2,((variant&1)?0x100u:0)|((variant&2)?0x4000u:0));
        write_at(record+0x8au,2,(variant&4)?0x2000:0);write_at(record+0x82u,2,(variant&8)?0x2000:0);
        write_at(record+0x62u,1,variant%4==0 ? 49:variant%4==1 ? 50:255);
        write_at(record+0xa0u,1,special[(variant/4+i)%4]);
        write_at(stats+0x64u,4,gear_hp[(variant/2+i)%8]);
        write_at(stats+0x4fu,1,(variant+i)%3==0 ? 0:(variant+i)%3==1 ? 1:255);
        write_at(stats+0x7eu,2,0x8200);
    }
    write_at(0x8006d634u+0x1930u,2,variant%3==0 ? 0xe6:variant%3==1 ? 0xe7:0xffff);
    write_at(0x8006d634u+0x2355u,1,variant&16 ? 0xb5:0x35);
}

static int party_case(uint32_t entry)
{
    PcPortMipsCpu oracle,native;running_entry=entry;
    party_setup(&oracle,entry);candidate=compare=0;
    if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_HALTED) {
        fail(oracle.error,oracle.pc);return 0;
    }
    unsigned expected_calls=calls;
    if(!oracle.steps) fail("original stat body did not execute",entry);
    memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));memcpy(expected_shared,shared,sizeof(shared));
    memcpy(expected_pad,g_PsxScratchpad,sizeof(expected_pad));memcpy(expected_host,host_workspace,sizeof(expected_host));
    party_setup(&native,entry);candidate=compare=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,100000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    if(rc!=PC_PORT_MIPS_HALTED) { fail(native.error,native.pc);return 0; }
    if(native.steps || rt.bridge_cpu || calls!=expected_calls ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("native context/calls/instructions",calls);
    for(unsigned r=16;r<32;r++) if(native.gpr[r]!=oracle.gpr[r]) fail("preserved register",r);
    if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
        fail("return/HI/LO",native.gpr[2]);
        fprintf(stderr,"native %08x %08x %08x %08x expected %08x %08x %08x %08x\n",
                native.gpr[2],native.gpr[3],native.hi,native.lo,oracle.gpr[2],oracle.gpr[3],oracle.hi,oracle.lo);
    }
    if(memcmp(g_PsxRam,expected_ram,sizeof(g_PsxRam)) || memcmp(shared,expected_shared,sizeof(shared)) ||
       memcmp(g_PsxScratchpad,expected_pad,sizeof(expected_pad)) || memcmp(host_workspace,expected_host,sizeof(expected_host))) {
        fail("final memory",workspace_address);
        unsigned shown=0;
        for(unsigned i=0;i<sizeof(g_PsxRam) && shown<12;i++) if(g_PsxRam[i]!=expected_ram[i]) {
            fprintf(stderr,"RAM %08x native=%02x expected=%02x\n",0x80000000u+i,g_PsxRam[i],expected_ram[i]);shown++;
        }
        for(unsigned i=0;i<sizeof(host_workspace) && shown<12;i++) if(host_workspace[i]!=expected_host[i]) {
            fprintf(stderr,"host +%x native=%02x expected=%02x\n",i,host_workspace[i],expected_host[i]);shown++;
        }
    }
    return !failed;
}

static int party_controls(void)
{
    PcPortMipsCpu cpu;variant=7;candidate=1;compare=0;
    const uint32_t addresses[]={BATTLE_PARTY_STATS_ADDRESS,BATTLE_PARTY_SPECIAL_ADDRESS};
    const unsigned lengths[]={BATTLE_PARTY_STATS_BYTES,BATTLE_PARTY_SPECIAL_BYTES};
    for(unsigned i=0;i<2;i++) {
        party_setup(&cpu,addresses[i]);rt.bridge_cpu=&cpu;
        memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));
        const unsigned offsets[]={0,lengths[i]/2,lengths[i]-1};
        for(unsigned j=0;j<3;j++) {
            uint8_t *p=PSX_ADDR(addresses[i]+offsets[j]);*p^=1;
            if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat identity accepted mutation",j);
            *p^=1;
        }
        cpu.load_valid=1;cpu.load_reg=4;
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat pending entry load",i);
        cpu.load_valid=0;rt.force_interpret=1;
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat forced reference",i);
        rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_PARTY_STATS","0",1)==0);
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat reference switch",i);
        assert(unsetenv("XENO_BATTLE_NATIVE_PARTY_STATS")==0);g_ActiveBattleRuntime=NULL;
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat inactive runtime",i);
        g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat wrong CPU",i);
        rt.bridge_cpu=&cpu;rt.initialized=0;
        if(battle_party_stats_try_host(&rt,&cpu,addresses[i])) fail("stat uninitialized runtime",i);
        rt.initialized=1;rt.bridge_cpu=NULL;
        if(memcmp(g_PsxRam,expected_ram,sizeof(expected_ram)) || calls) fail("stat rejected call effects",i);
    }
    /* Identity failure in a real composed call must refuse before any decode. */
    party_setup(&cpu,BATTLE_MODULE_DISPATCH_ADDRESS);
    *(uint8_t *)PSX_ADDR(BATTLE_PARTY_STATS_ADDRESS)^=1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    if(PcPortMipsRun(&cpu,BATTLE_MODULE_DISPATCH_ADDRESS,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_UNRESOLVED_CALL ||
       cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().disabled_refusals!=before.disabled_refusals+1 ||
       PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("stat identity fallback/refusal",0);
    if(read_at(0x800d3338u,1)!=0) fail("earlier stage writes lost",0);
    party_setup(&cpu,BATTLE_PARTY_STATS_ADDRESS);write_at(0x800c34b0u,4,0x1f800ff0u);
    before=PcPortMipsExecutionStatsGet();
    if(PcPortMipsRun(&cpu,BATTLE_PARTY_STATS_ADDRESS,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_UNRESOLVED_CALL ||
       cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
        fail("stat invalid data range",0);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
    rt.bridge_cpu=NULL;
    return !failed;
}

int main(int argc,char **argv)
{
    assert(argc==2);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
    FILE *image=fopen("disc/battle.bin","rb");assert(image && fread(battle_image,1,sizeof(battle_image),image)==sizeof(battle_image));
    assert(fclose(image)==0);image=fopen(argv[1],"rb");assert(image && fread(module,1,sizeof(module),image)==sizeof(module));
    assert(fclose(image)==0);assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    for(variant=0;variant<192;variant++) if(!party_case(BATTLE_PARTY_STATS_ADDRESS)) return 1;
    puts("BATTLE PARTY STATS PASS 192 original-body differential cases");
    for(variant=0;variant<48;variant++) if(!party_case(BATTLE_PARTY_SPECIAL_ADDRESS)) return 1;
    puts("BATTLE PARTY SPECIAL PASS 48 original-body differential cases");
    for(variant=0;variant<32;variant++) if(!party_case(BATTLE_STAGE1_MODES_ADDRESS)) return 1;
    for(variant=0;variant<32;variant++) if(!party_case(BATTLE_MODULE_DISPATCH_ADDRESS)) return 1;
    if(!party_controls()) return 1;
    puts("BATTLE PARTY STATS PASS 304 differential cases: 192 stats, 48 special, 32 actor modes, 32 full stage-1; real libc/mask, no modeled callees; full memory, HI/LO, aliases, identity/refusal; native steps=0");
    return 0;
}
