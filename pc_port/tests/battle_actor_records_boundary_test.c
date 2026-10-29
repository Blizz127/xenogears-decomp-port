/* Rejected native adoption must not fall through to a cached legacy body. */
#define main actor_records_fixture_main
#include "battle_actor_records_host_test.c"
#undef main
int main(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fseek(f,0x3abf0,SEEK_SET)==0);
 assert(fread(body,1,sizeof(body),f)==sizeof(body)&&fclose(f)==0);
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 unsigned cases=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned gate=0;gate<4;gate++){
  count=0;PcPortMipsCpu cpu;setup(&cpu);observing=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){0x800aa6e0u,legacy_probe,"func_800AA6E0"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x800aa6e0u);slot->target=0x800aa6e0u;slot->state=1;slot->entry=rt.functions[0];}
  if(gate<3){unsigned offset=gate==0?0:gate==1?64:127;uint8_t *p=PSX_ADDR(0x800aa6e0u+offset);*p^=1;}
  else assert(setenv("XENO_BATTLE_NATIVE_ACTOR_RECORDS","0",1)==0);
  snapshot();PcPortMipsCpu before=cpu;PcPortMipsExecutionStats previous=PcPortMipsExecutionStatsGet();legacy_calls=0;
  int rc=PcPortMipsRun(&cpu,0x800aa6e0u,BATTLE_HALT_PC,100);
  assert(unsetenv("XENO_BATTLE_NATIVE_ACTOR_RECORDS")==0);
  if(rc>=0||legacy_calls||cpu.steps||rt.bridge_cpu||!memory_equal()||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||cpu.hi!=before.hi||cpu.lo!=before.lo||PcPortMipsExecutionStatsGet().interpreted_instructions!=previous.interpreted_instructions){
   fprintf(stderr,"ACTOR RECORD BOUNDARY FAIL gate=%u cached=%u rc=%d legacy_calls=%u\n",gate,cached,rc,legacy_calls);return 1;
  }
  cases++;
 }
 printf("ACTOR RECORD BOUNDARY PASS %u rejected-adoption cases; cached/resolved legacy bodies cannot bypass three identity mismatches or opt-out; zero interpreted instructions and unchanged data/registers\n",cases);
 return 0;
}
