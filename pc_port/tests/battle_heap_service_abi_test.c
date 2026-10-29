/* Concrete heap call ABI and pointer-domain proof. Heap policy is modeled;
 * real heap integration is exercised by battle_text_allocation_host_test.c. */
#define main heap_stage1_fixture_main
#include "battle_module_stage1_host_test.c"
#undef main
static uint8_t name_table_storage[64] __attribute__((aligned(8)));
static uint32_t seen_size,seen_flags,seen_user;
static char **seen_names;
static void *next_result;
static unsigned invoked;
static void *heap_probe_allocate(uint32_t size,uint32_t flags)
{seen_size=size;seen_flags=flags;invoked++;return next_result;}
static void heap_probe_user(uint32_t user,char **names)
{seen_user=user;seen_names=names;invoked++;}
static void heap_probe_setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.functions[0]=(ResolvedFunction){0x80031bdcu,heap_probe_allocate,"HeapAlloc"};
 rt.functions[1]=(ResolvedFunction){0x80032498u,heap_probe_user,"HeapChangeCurrentUser"};rt.function_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)name_table_storage,(uintptr_t)name_table_storage+sizeof(name_table_storage),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x89abcdefu;cpu->gpr[3]=0x76543210u;
 invoked=0;seen_size=seen_flags=seen_user=0;seen_names=NULL;
}
static int heap_probe_run(PcPortMipsCpu *cpu,uint32_t target,int wanted)
{
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,target,BATTLE_HALT_PC,100);
 if(rc!=wanted || cpu->steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) {
  fprintf(stderr,"HEAP SERVICE ABI FAIL status target=%08x rc=%d wanted=%d\n",target,rc,wanted);return 0;
 }
 return 1;
}
int main(void)
{
 assert((uintptr_t)name_table_storage<=UINT32_MAX);
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 const uint32_t values[]={0,1,0xffffffffu,0x7fffffffu,0x80000000u,0x80100000u,0xa0100000u,0x1f800100u};
 const uint32_t guests[]={0,0x80000000u,0x80000004u,0x80100000u,0x801fffffu};
 unsigned cases=0;
 for(unsigned v=0;v<8;v++) for(unsigned f=0;f<8;f++) for(unsigned p=0;p<5;p++) {
  PcPortMipsCpu cpu;heap_probe_setup(&cpu);cpu.gpr[4]=values[v];cpu.gpr[5]=values[f];
  next_result=guests[p] ? PSX_ADDR(guests[p]):NULL;
  if(!heap_probe_run(&cpu,0x80031bdcu,PC_PORT_MIPS_HALTED)) return 1;
  if(invoked!=1 || seen_size!=values[v] || seen_flags!=values[f] || cpu.gpr[2]!=guests[p] || cpu.gpr[3]) {
   fprintf(stderr,"HEAP SERVICE ABI FAIL allocation size=%08x/%08x flags=%08x/%08x return=%08x/%08x\n",seen_size,values[v],seen_flags,values[f],cpu.gpr[2],guests[p]);return 1;
  }
  cases++;
 }
 const uint32_t names[]={0,0x80180000u,0xa0180000u,0x00180000u,(uint32_t)(uintptr_t)name_table_storage};
 for(unsigned user=0;user<10;user++) for(unsigned n=0;n<5;n++) {
  PcPortMipsCpu cpu;heap_probe_setup(&cpu);cpu.gpr[4]=user;cpu.gpr[5]=names[n];
  char **expected=names[n] ? module_memory_pointer(&rt,names[n],1,0):NULL;
  if(!heap_probe_run(&cpu,0x80032498u,PC_PORT_MIPS_HALTED)) return 1;
  if(invoked!=1 || seen_user!=user || seen_names!=expected || cpu.gpr[2]!=0x89abcdefu || cpu.gpr[3]!=0x76543210u) {
   fputs("HEAP SERVICE ABI FAIL user/pointer/void return\n",stderr);return 1;
  }
  cases++;
 }
 PcPortMipsCpu cpu;heap_probe_setup(&cpu);cpu.gpr[4]=2;cpu.gpr[5]=0x1e000000u;
 if(!heap_probe_run(&cpu,0x80032498u,PC_PORT_MIPS_UNRESOLVED_CALL) || invoked) return 1;
 for(unsigned i=0;i<2;i++) {
  heap_probe_setup(&cpu);cpu.load_valid=1;cpu.load_reg=4;
  if(!heap_probe_run(&cpu,i ? 0x80032498u:0x80031bdcu,PC_PORT_MIPS_UNRESOLVED_CALL) || invoked) return 1;
 }
 heap_probe_setup(&cpu);next_result=(void *)(uintptr_t)0x100000000ull;
 if(!heap_probe_run(&cpu,0x80031bdcu,PC_PORT_MIPS_UNRESOLVED_CALL) || invoked!=1 || cpu.gpr[2]!=0x89abcdefu || cpu.gpr[3]!=0x76543210u) return 1;
 printf("HEAP SERVICE ABI PASS %u scalar/pointer/void-return cases and 4 refusal cases; pointee layouts/heap policy excluded; native steps=0\n",cases);
 return 0;
}
