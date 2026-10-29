/* Real native paired records, timer updates and cleanup. Rendering through
 * the separate work-list callback 800B3878 is outside this fixture. */
#define main fade_comparison_main
#include "battle_color_fade_host_test.c"
#undef main
s32 D_80059190,g_NumTimerWorkListEntries,g_NumWorkListEntries,g_WorkListCurTimer;
struct WorkListEntry *D_800594C0,*g_TimerWorkList,*D_80059590,*g_WorkList;
short D_80059494;
extern s32 D_80059184,D_80059464;
extern u8 D_800591AC,D_800591AF;
extern void TimerWorkListAddTask(struct WorkListEntry *,struct WorkListEntry *);
extern void WorkListAddTask(struct WorkListEntry *,struct WorkListEntry *);
extern void TimerWorkListRemoveTask(struct WorkListEntry *);
extern void WorkListRemoveTask(struct WorkListEntry *);
extern void TimerWorkListUpdate(void);
void *HeapAlloc(u_int size,u_int flags){(void)size;(void)flags;abort();}
u_int HeapFree(void *p){(void)p;abort();}
static unsigned abi_calls;
static struct WorkListEntry *abi_first,*abi_second;
static void probe_add(struct WorkListEntry *a,struct WorkListEntry *b)
{abi_calls++;abi_first=a;abi_second=b;}
static void probe_remove(struct WorkListEntry *a)
{abi_calls++;abi_first=a;}
static void abi_setup(PcPortMipsCpu *cpu,uint32_t target,int cached)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;observing=0;candidate=1;
 rt.functions[0]=(ResolvedFunction){target,target==0x8001ca58u||target==0x8001cc18u?(void *)probe_add:(void *)probe_remove,"list-abi"};rt.function_count=1;
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x13579bdfu;cpu->gpr[3]=0x2468ace0u;cpu->gpr[29]=0x1e000000u;
 abi_calls=0;abi_first=abi_second=NULL;
}
static void abi_run(PcPortMipsCpu *cpu,uint32_t target,int success)
{
 uint32_t before[32];memcpy(before,cpu->gpr,sizeof(before));PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,target,BATTLE_HALT_PC,100);
 assert(rc==(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL));
 assert(!cpu->steps&&!rt.bridge_cpu&&PcPortMipsExecutionStatsGet().interpreted_instructions==stats.interpreted_instructions);
 assert(!memcmp(before,cpu->gpr,sizeof(before))&&abi_calls==(unsigned)success);
}
static unsigned interfaces(void)
{
 const uint32_t addresses[]={0x80140100u,0xa0140100u,0x80340100u,0x00140100u,0x80050100u,0x1f800100u,(uint32_t)(uintptr_t)(host+0x100)};
 const uint32_t bad[]={0,0x80100101u,0x1e000000u,0x801ffff0u,0x1f800ff0u,(uint32_t)(uintptr_t)(host+sizeof(host)-16u)};
 const uint32_t targets[]={0x8001cc18u,0x8001ca58u,0x8001cb48u,0x8001cd94u};unsigned total=0;
 for(unsigned t=0;t<4;t++)for(int cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned a=0;a<(t<2?8u:7u);a++)for(unsigned b=0;b<(t<2?7u:1u);b++){
   abi_setup(&cpu,targets[t],cached);cpu.gpr[4]=a<7?addresses[a]:0;cpu.gpr[5]=addresses[b];
   void *expected_a=cpu.gpr[4]?module_memory_pointer(&rt,cpu.gpr[4],0x1c,t>=2):NULL;
   void *expected_b=module_memory_pointer(&rt,cpu.gpr[5],0x1c,1);
   abi_run(&cpu,targets[t],1);assert(abi_first==expected_a&&(t>=2||abi_second==expected_b));total++;
  }
  for(unsigned a=0;a<6;a++){
   abi_setup(&cpu,targets[t],cached);cpu.gpr[4]=t<2?addresses[0]:bad[a];cpu.gpr[5]=bad[a];abi_run(&cpu,targets[t],0);total++;
   if(t<2&&a){abi_setup(&cpu,targets[t],cached);cpu.gpr[4]=bad[a];cpu.gpr[5]=addresses[0];abi_run(&cpu,targets[t],0);total++;}
  }
  abi_setup(&cpu,targets[t],cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=addresses[1];cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,targets[t],0);total++;
  abi_setup(&cpu,targets[t],cached);rt.host_ranges[0].writable=0;cpu.gpr[4]=t<2?addresses[0]:addresses[6];cpu.gpr[5]=addresses[6];abi_run(&cpu,targets[t],0);total++;
 }
 return total;
}
static uint32_t load_word(uint32_t address)
{uint32_t value;assert(model_test_raw_read(&rt,address,4,&value)==0);return value;}
static void start_environment(unsigned flag)
{
 kind=seed=stack_kind=task_domain=selection=mutation=zero_color=stop_at=fault_at=0;ticks=20;remaining=39;
 PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
 rt.data[0]=(ResolvedData){0x800591acu,1,&D_800591AC,"counted-flag"};
 rt.data[1]=(ResolvedData){0x80059464u,4,(uint8_t *)&D_80059464,"counted-total"};rt.data_count=2;
 rt.host_ranges[1]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+sizeof(g_PsxRam),1};rt.host_range_count=2;
 rt.functions[0]=(ResolvedFunction){0x8001ca58u,WorkListAddTask,"WorkListAddTask"};
 rt.functions[1]=(ResolvedFunction){0x8001cb48u,WorkListRemoveTask,"WorkListRemoveTask"};
 rt.functions[2]=(ResolvedFunction){0x8001cc18u,TimerWorkListAddTask,"TimerWorkListAddTask"};
 rt.functions[3]=(ResolvedFunction){0x8001cd94u,TimerWorkListRemoveTask,"TimerWorkListRemoveTask"};rt.function_count=4;
 g_TimerWorkList=g_WorkList=D_800594C0=D_80059590=NULL;g_NumTimerWorkListEntries=g_NumWorkListEntries=g_WorkListCurTimer=0;
 D_80059184=-1;D_80059464=7;D_800591AC=flag;D_800591AF=0;
 fixture_store(0x80000010u,4,0xf56789abu);fixture_store(0x800c3554u,4,0);fixture_store(0x800c3558u,4,0);fixture_store(0x800d3638u,1,0);
}
static void setup_fade(unsigned duration,unsigned alternate,unsigned color)
{
 fixture_store(0x800c355cu,1,alternate);
 PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.gpr[4]=duration;cpu.gpr[5]=2;cpu.gpr[6]=cpu.gpr[7]=color;
 fixture_store(cpu.gpr[29]+0x10u,4,color);
 assert(PcPortMipsRun(&cpu,0x800b39c0u,BATTLE_HALT_PC,10000)==PC_PORT_MIPS_HALTED&&!cpu.steps);
 assert(D_80059464==7);
}
static void verify_record(uint32_t address)
{
 uint32_t native=(uint32_t)(uintptr_t)PSX_ADDR(address);
 assert(load_word(address+8u)==0x800b36bcu&&load_word(address+0xcu)==0x800b383cu);
 assert(load_word(address+0x24u)==0x800b3878u&&load_word(address+0x28u)==(uint32_t)(uintptr_t)WorkListRemoveTask);
 assert(load_word(address+4u)==address&&load_word(address+0x20u)==address);
 assert(load_word(address+0x1cu)==native&&!(load_word(address+0x14u)&0x80000000u));
}
static void pump(PcPortMipsCpu *outer)
{
 PcPortMipsCpu saved=*outer;rt.bridge_cpu=outer;TimerWorkListUpdate();assert(rt.bridge_cpu==outer&&!memcmp(outer,&saved,sizeof(saved)));rt.bridge_cpu=NULL;
}
static void run_integration(unsigned flag,unsigned scenario)
{
 start_environment(flag);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 PcPortMipsCpu outer;initialize_cpu(&outer,&rt);outer.gpr[29]=flag&1u?(uint32_t)(uintptr_t)(host+0x70000u):0x801c0000u;
 if(scenario==2){
  setup_fade(2,1,255);verify_record(0x800c3c00u);
  setup_fade(0,0,255);verify_record(0x800c3c50u);
  assert(g_NumTimerWorkListEntries==2&&g_NumWorkListEntries==2);
  setup_fade(2,1,127);assert(g_NumTimerWorkListEntries==2&&g_NumWorkListEntries==2);
  for(unsigned i=0;i<4;i++)pump(&outer);
  assert(PcPort_BattleMipsDispatchCallback(0x800b383cu,PSX_ADDR(0x800c3c00u))==1);
  assert(g_NumTimerWorkListEntries==1&&g_NumWorkListEntries==1&&g_TimerWorkList==(struct WorkListEntry *)PSX_ADDR(0x800c3c50u));
  assert(PcPort_BattleMipsDispatchCallback(0x800b383cu,PSX_ADDR(0x800c3c50u))==1);
 } else if(scenario==3)setup_fade(0,0,0);
 else {
  setup_fade(scenario==0?0:2,0,scenario==0?255:0);verify_record(0x800c3c50u);
  assert(g_TimerWorkList==(struct WorkListEntry *)PSX_ADDR(0x800c3c50u)&&g_WorkList==(struct WorkListEntry *)PSX_ADDR(0x800c3c6cu));
  assert((load_word(0x800c3c60u)&0x1fffffffu)==0x1fffffffu&&(load_word(0x800c3c64u)&0x1fffffffu)==0x156789abu&&D_80059184==1);
  if(scenario==0)setup_fade(3,0,0);
  unsigned n=scenario==0?6:4;
  for(unsigned i=0;i<n;i++){assert(g_NumTimerWorkListEntries==1&&g_NumWorkListEntries==1);pump(&outer);}
 }
 assert(!g_TimerWorkList&&!g_WorkList&&!g_NumTimerWorkListEntries&&!g_NumWorkListEntries&&D_80059464==7&&!load_word(0x800c3558u));
 assert(!native_gte_commands&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
}
#ifndef COLOR_FADE_LIST_MAIN
#define COLOR_FADE_LIST_MAIN main
#endif
int COLOR_FADE_LIST_MAIN(void)
{
 load_inputs();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 printf("COLOR FADE LIST ABI PASS %u concrete void signatures, pointer domains, cached dispatch, pending and malformed refusals\n",interfaces());fflush(stdout);
 unsigned total=0;for(unsigned flag=0;flag<256;flag++)for(unsigned scenario=0;scenario<4;scenario++){run_integration(flag,scenario);total++;}
 printf("COLOR FADE INTEGRATION PASS %u actual packed add/remove/timer-pump/native fade cases; interpreted instructions=0, graphics commands=0; renderer callback not invoked\n",total);return 0;
}
