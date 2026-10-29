/* SDK packet/return contract against original SDK code. Original service
 * frames and scratch registers are intentionally outside this boundary. */
#define main stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main

static _Alignas(4) uint8_t packet_host[128];
static uint8_t sdk_code[0x2300];
static void sdk_gt4(POLY_GT4 *p) {SetPolyGT4(p);}
static void sdk_g4(POLY_G4 *p) {SetPolyG4(p);}
static void sdk_f4(POLY_F4 *p) {SetPolyF4(p);}
static void sdk_line(LINE_F2 *p) {SetLineF2(p);}
static void sdk_semi(void *p,int a) {SetSemiTrans(p,a);}
static void sdk_shade(void *p,int a) {SetShadeTex(p,a);}
static void sdk_mode(DR_MODE *p,int a,int b,int page,RECT16 *r) {SetDrawMode(p,a,b,page,r);}
static u_short sdk_clut_unused(int x,int y) {(void)x;(void)y;abort();}
static const ResolvedFunction packet_services[]={
    {0x80043a58u,sdk_clut_unused,"GetClut"},{0x80043cd8u,sdk_gt4,"SetPolyGT4"},
    {0x80043cc4u,sdk_g4,"SetPolyG4"},{0x80043c9cu,sdk_f4,"SetPolyF4"},
    {0x80043d78u,sdk_line,"SetLineF2"},{0x80043bfcu,sdk_semi,"SetSemiTrans"},
    {0x80043c24u,sdk_shade,"SetShadeTex"},{0x800454dcu,sdk_mode,"SetDrawMode"}};
static int packet_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    return candidate && target==running_entry ? runtime_bridge(opaque,cpu,target):0;
}
static uint32_t packet_setup(PcPortMipsCpu *cpu,unsigned service)
{
    setup(cpu);memset(packet_host,0x9b,sizeof(packet_host));assert((uintptr_t)packet_host<=UINT32_MAX);
    rt.host_ranges[0]=(HostRange){(uintptr_t)packet_host,(uintptr_t)packet_host+sizeof(packet_host),1};rt.host_range_count=1;
    memcpy(PSX_ADDR(0x80043a1cu),sdk_code,sizeof(sdk_code));
    memcpy(rt.functions,packet_services,sizeof(packet_services));rt.function_count=8;
    qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
    cpu->bus.bridge=packet_bridge;
    const uint32_t packets[]={0x80100000,0xa0100000,0x1f800800,(uint32_t)(uintptr_t)packet_host};
    uint32_t packet=packets[variant%4],rectangle=packet+64;
    for(unsigned i=0;i<96;i++) write_at(packet+i,1,i*17u+variant*29u);
    write_at(0x800568d0u,1,0); /* Graph type used by the native backend. */
    const uint32_t values[]={0,1,0x7fff,0x8000,0xffff,0xffffffff,0x80000000,0x7fffffff};
    cpu->gpr[4]=packet;cpu->gpr[5]=values[(variant/4)%8];cpu->gpr[6]=values[(variant/8)%8];cpu->gpr[7]=values[variant%8];
    if(service==0) {cpu->gpr[4]=values[variant%8];cpu->gpr[5]=values[(variant/8)%8];}
    if(service==7) {
        const uint32_t rectangles[]={0,rectangle,packet+4,packet+8};
        rectangle=rectangles[(variant/4)%4];
        write_at(cpu->gpr[29]+0x10u,4,rectangle);
        if(rectangle) for(unsigned i=0;i<4;i++) write_at(rectangle+i*2,2,values[(variant+i)%8]);
    }
    return packet;
}
int main(void)
{
    FILE *f=fopen("disc/SLUS_006.64","rb");assert(f && !fseek(f,0x80043a1cu-0x8000f800u,SEEK_SET));
    assert(fread(sdk_code,1,sizeof(sdk_code),f)==sizeof(sdk_code));assert(!fclose(f));
    assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
    for(unsigned service=0;service<8;service++) for(variant=0;variant<64;variant++) {
        PcPortMipsCpu oracle,native;uint8_t expected[96],actual[96];
        running_entry=packet_services[service].address;
        uint32_t packet=packet_setup(&oracle,service);candidate=0;
        if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,2000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 1;}
        for(unsigned i=0;i<sizeof(expected);i++) expected[i]=read_at(packet+i,1);
        packet=packet_setup(&native,service);candidate=1;
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
        PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
        int rc=PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,2000);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
        if(rc!=PC_PORT_MIPS_HALTED || native.steps || rt.bridge_cpu ||
           PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) {fail("SDK native dispatch",service);return 1;}
        for(unsigned i=0;i<sizeof(actual);i++) actual[i]=read_at(packet+i,1);
        if(memcmp(expected,actual,sizeof(expected)) || (service==0 && native.gpr[2]!=oracle.gpr[2])) {
            fail("stage3 SDK original packet/return",service);return 1;
        }
    }
    puts("BATTLE STAGE3 SDK PASS 512 original packet/return cases; graph type 0; KSEG/scratch/host and rectangle aliases; native steps=0; service CPU frames/volatile state excluded");
    return 0;
}
