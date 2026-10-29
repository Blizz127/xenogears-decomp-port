/* Cached legacy functions cannot bypass unconverted packet dependencies. */
#define main model_packets_fixture_main
#include "battle_model_packets_host_test.c"
#undef main
static unsigned legacy_calls;
static uintptr_t legacy_dependency_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,
 uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{
 (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;
 legacy_calls++;return 0;
}
int main(void)
{
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 const uint32_t targets[]={0x8002db84u,0x8004a19cu,0x8004a1b8u,0x8004a260u,0x8004a280u,0x8002d354u};
 for(unsigned index=0;index<6;index++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned cached=0;cached<2;cached++){
  observing=0;memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;
  ResolvedFunction legacy={targets[index],legacy_dependency_probe,"packet_dependency_probe"};
  rt.functions[0]=legacy;rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,legacy.address);slot->target=legacy.address;slot->state=1;slot->entry=legacy;}
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.gpr[31]=index==5?0x8002ca9cu:0x8002ce60u;legacy_calls=0;
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  else assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  snapshot();PcPortMipsCpu before=cpu;PcPortMipsExecutionStats previous=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,legacy.address,cpu.gpr[31],100);
  PcPortMipsExecutionStats after=PcPortMipsExecutionStatsGet();
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||legacy_calls||cpu.steps||rt.bridge_cpu||!memory_equal()||memcmp(before.gpr,cpu.gpr,sizeof(cpu.gpr))||after.interpreted_instructions!=previous.interpreted_instructions||after.unresolved_bridges!=previous.unresolved_bridges+1){
   fprintf(stderr,"MODEL PACKET BOUNDARY FAIL target=%08x disabled=%u cached=%u rc=%d legacy_calls=%u\n",targets[index],disabled,cached,rc,legacy_calls);return 1;
  }
 }
 /* A cached legacy dispatcher must yield to the converted native body. */
 for(unsigned disabled=0;disabled<2;disabled++)for(unsigned cached=0;cached<2;cached++){
  kind=0;groups=0;mode=variant=flags=alias_kind=live_change=0;
  PcPortMipsCpu cpu;setup(&cpu);observing=0;cpu.bus.bridge=runtime_bridge;
  ResolvedFunction legacy={0x8002c8ccu,legacy_dependency_probe,"func_8002C8CC"};
  rt.functions[0]=legacy;rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,legacy.address);slot->target=legacy.address;slot->state=1;slot->entry=legacy;}
  legacy_calls=0;
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);else assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,legacy.address,BATTLE_HALT_PC,100);
  if(rc!=PC_PORT_MIPS_HALTED||legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions){
   fprintf(stderr,"MODEL PACKET BOUNDARY FAIL native dispatcher disabled=%u cached=%u rc=%d legacy_calls=%u\n",disabled,cached,rc,legacy_calls);return 1;
  }
 }
 puts("MODEL PACKET BOUNDARY PASS five lighting dependencies and unsupported indirect callback, resolved and cached legacy bodies, interpreter allowed/disabled, unchanged refused registers/memory, native dispatcher precedence and zero instructions");return 0;
}
