/* Real packed timer allocation and pump through the production battle
 * dispatcher. Archive/heap/graphics and three later startup children are
 * explicit boundary models; no original CPU instructions execute here. */
#include <stdint.h>
#include "battle_mips_adapter.h"
static int runtime_bridge_call(void *,PcPortMipsCpu *,uint32_t);
#define main timer_caller_fixture_main
#include "battle_timer_task_host_test.c"
#undef main
s32 D_80059190,g_NumTimerWorkListEntries,g_NumWorkListEntries,g_WorkListCurTimer;
struct WorkListEntry *D_800594C0,*g_TimerWorkList,*D_80059590,*g_WorkList;
short D_80059494;
extern s32 D_80059184,D_80059464;
extern u8 D_800591AC,D_800591AF;
extern struct WorkListEntry *TimerWorkListAllocateTask(void *,int32_t);
extern void TimerWorkListDeleteTask(struct WorkListEntry *);
extern void TimerWorkListUpdate(void);
extern void TimerWorkListSetTaskCallback(struct WorkListEntry *,void (*)(void *));
static unsigned abi_calls,allocation_calls,free_calls,child_calls,sync_calls,query_calls;
static unsigned live_payload;
static int integrated;
static uint32_t abi_size,allocated_size[2],allocated_flags[2],free_tokens[2],children[3],child_args[3],child_stacks[3];
static void *abi_owner;static struct WorkListEntry *abi_result;
static int32_t query_result;
static uint32_t original_outer_sp;
static struct WorkListEntry *probe_allocate(void *owner,int32_t size)
{abi_calls++;abi_owner=owner;abi_size=(uint32_t)size;return abi_result;}
static int probe_query(void){abi_calls++;return query_result;}
void *HeapAlloc(u_int size,u_int flags)
{
 assert(allocation_calls<2);allocated_size[allocation_calls]=size;allocated_flags[allocation_calls]=flags;
 void *result=PSX_ADDR(allocation_calls?0x80150000u:0x80140000u);allocation_calls++;return result;
}
u_int HeapFree(void *pointer)
{assert(free_calls<2);free_tokens[free_calls++]=(uint32_t)(uintptr_t)pointer;return 0x12340000u+free_calls;}
static int draw_sync(int mode){assert(!mode);sync_calls++;return 0;}
static int archive_sync(void){query_calls++;return query_result;}
static int runtime_bridge_call(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(integrated&&(target==0x801e6314u||target==0x801e6710u||target==0x801e693cu)){
  assert(child_calls<3);children[child_calls]=target;child_args[child_calls]=cpu->gpr[4];child_stacks[child_calls]=cpu->gpr[29];child_calls++;
  if(live_payload&&target==0x801e6314u)fixture_store(0x80140020u,4,0x80170200u);
  if(live_payload&&target==0x801e6710u)fixture_store(0x80140020u,4,0x80170300u);
  cpu->gpr[2]=0xc1000000u+child_calls;cpu->gpr[3]=0xd2000000u+child_calls;cpu->hi=0xaabbccddu;cpu->lo=0xddccbbaau;
  return 1;
 }
 return timer_raw_bridge_call(opaque,cpu,target);
}
static void abi_setup(PcPortMipsCpu *cpu,uint32_t target,void *fn,int cached)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;observing=integrated=0;
 rt.functions[0]=(ResolvedFunction){target,fn,"timer-service-probe"};rt.function_count=1;
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x13579bdfu;cpu->gpr[3]=0x2468ace0u;cpu->gpr[29]=0x1e000000u;
 abi_calls=0;abi_owner=NULL;abi_size=0;
}
static void abi_run(PcPortMipsCpu *cpu,uint32_t target,int success)
{
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,target,BATTLE_HALT_PC,100);
 assert(rc==(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL));
 assert(!cpu->steps&&!rt.bridge_cpu&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 if(!success)assert(cpu->gpr[2]==0x13579bdfu&&cpu->gpr[3]==0x2468ace0u);
}
static unsigned interfaces(void)
{
 unsigned total=0;PcPortMipsCpu cpu;
 const uint32_t words[]={0,1,0x78,0xffffffe4u,0xffffffffu,0x80000000u,0xa0100000u,0x7fffffffu};
 const uint32_t owners[]={0,0x80100100,0xa0100100,0x80300100,0x100100,0x80050100,0x1f800100,(uint32_t)(uintptr_t)host+0x100};
 for(int cached=0;cached<2;cached++){
  for(unsigned o=0;o<8;o++)for(unsigned n=0;n<8;n++){
   abi_setup(&cpu,0x8001cd08u,probe_allocate,cached);cpu.gpr[4]=owners[o];cpu.gpr[5]=words[n];
   void *expected=o?module_memory_pointer(&rt,owners[o],0x1c,0):NULL;
   abi_result=(struct WorkListEntry *)(n&1?(void *)PSX_ADDR(0x80120000u):(void *)(host+0x200));
   abi_run(&cpu,0x8001cd08u,1);assert(abi_calls==1&&abi_owner==expected&&abi_size==words[n]&&cpu.gpr[2]==(uint32_t)(uintptr_t)abi_result&&!cpu.gpr[3]);total++;
  }
  for(unsigned n=0;n<8;n++){
   abi_setup(&cpu,0x800286ccu,probe_query,cached);query_result=(int32_t)words[n];abi_run(&cpu,0x800286ccu,1);assert(abi_calls==1&&cpu.gpr[2]==words[n]&&!cpu.gpr[3]);total++;
  }
  const uint32_t bad[]={0x1e000000u,0x80100101u,0x801ffff0u,0x1f800ff0u,(uint32_t)(uintptr_t)(host+sizeof(host)-16u)};
  for(unsigned b=0;b<5;b++){
   abi_setup(&cpu,0x8001cd08u,probe_allocate,cached);cpu.gpr[4]=bad[b];abi_run(&cpu,0x8001cd08u,0);assert(!abi_calls);total++;
  }
  abi_setup(&cpu,0x8001cd08u,probe_allocate,cached);abi_result=(struct WorkListEntry *)(uintptr_t)UINT64_C(0x100000000);abi_run(&cpu,0x8001cd08u,0);assert(abi_calls==1);total++;
  abi_setup(&cpu,0x8001cd08u,probe_allocate,cached);abi_result=NULL;abi_run(&cpu,0x8001cd08u,1);assert(abi_calls==1&&!cpu.gpr[2]&&!cpu.gpr[3]);total++;
  for(unsigned t=0;t<2;t++){
   uint32_t target=t?0x800286ccu:0x8001cd08u;abi_setup(&cpu,target,t?(void *)probe_query:(void *)probe_allocate,cached);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,target,0);assert(!abi_calls);total++;
  }
 }
 return total;
}
static uint32_t load_word(uint32_t address)
{uint32_t value;assert(model_test_raw_read(&rt,address,4,&value)==0);return value;}
static void composed(unsigned flags,int busy_value,unsigned live,unsigned outer)
{
 kind=0;seed=stack_kind=domain_kind=task_domain=mutation=stop_at=fault_at=0;PcPortMipsCpu cpu;setup(&cpu);observing=0;integrated=1;
 rt.data_count=0;
 rt.host_ranges[1]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+sizeof(g_PsxRam),1};rt.host_range_count=2;
 rt.functions[0]=(ResolvedFunction){0x8001cd08u,TimerWorkListAllocateTask,"TimerWorkListAllocateTask"};
 rt.functions[1]=(ResolvedFunction){0x800286ccu,archive_sync,"ArchiveDataSync"};
 rt.functions[2]=(ResolvedFunction){0x80031bdcu,HeapAlloc,"HeapAlloc"};
 rt.functions[3]=(ResolvedFunction){0x800320e8u,HeapFree,"HeapFree"};
 rt.functions[4]=(ResolvedFunction){0x800445d0u,draw_sync,"DrawSync"};rt.function_count=5;
 g_TimerWorkList=g_WorkList=D_800594C0=D_80059590=NULL;g_NumTimerWorkListEntries=g_NumWorkListEntries=g_WorkListCurTimer=0;
 D_80059184=(s32)0xffffffffu;D_80059464=7;D_800591AC=flags&1u;D_800591AF=flags;
 fixture_store(0x80000010u,4,0xf56789abu);query_result=busy_value;live_payload=live;
 allocation_calls=free_calls=child_calls=sync_calls=query_calls=0;
 cpu.bus.bridge=runtime_bridge;cpu.gpr[4]=0x80170100u;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(PcPortMipsRun(&cpu,0x801e7098u,BATTLE_HALT_PC,1000)==PC_PORT_MIPS_HALTED);
 uint32_t ptr=(uint32_t)(uintptr_t)PSX_ADDR(0x80140000u);
 assert(!cpu.steps&&allocation_calls==1&&allocated_size[0]==0x94u&&allocated_flags[0]==flags);
 assert((uint32_t)(uintptr_t)g_TimerWorkList==ptr&&g_NumTimerWorkListEntries==1&&D_80059184==0);
 assert(load_word(ptr)==0&&load_word(ptr+4u)==0&&load_word(ptr+8u)==0x801e6fecu&&load_word(ptr+0xcu)==(uint32_t)(uintptr_t)TimerWorkListDeleteTask);
 assert((load_word(ptr+0x10u)&0x1fffffffu)==0x1fffffffu&&(load_word(ptr+0x14u)&0x1fffffffu)==0x156789abu);
 assert((load_word(ptr+0x14u)>>31)==(flags&1u)&&D_80059464==(int)(7+(flags&1u))&&load_word(ptr+0x20u)==0x80170100u);
 PcPortMipsCpu outer_cpu;initialize_cpu(&outer_cpu,&rt);outer_cpu.gpr[29]=outer?(uint32_t)(uintptr_t)(host+0x70000u):0x801c0000u;
 original_outer_sp=outer_cpu.gpr[29];PcPortMipsCpu saved_outer=outer_cpu;rt.bridge_cpu=&outer_cpu;
 TimerWorkListUpdate();assert(rt.bridge_cpu==&outer_cpu&&!memcmp(&outer_cpu,&saved_outer,sizeof(outer_cpu)));rt.bridge_cpu=NULL;
 assert(query_calls==1&&g_TimerWorkList==(struct WorkListEntry *)(uintptr_t)ptr&&D_80059590==NULL);
 if(busy_value){assert(allocation_calls==1&&!free_calls&&!child_calls&&!sync_calls&&load_word(ptr+8u)==0x801e6fecu);}
 else {
  assert(allocation_calls==2&&allocated_size[1]==0x4000u&&allocated_flags[1]==1&&free_calls==2&&child_calls==3&&sync_calls==1);
  assert(children[0]==0x801e6314u&&children[1]==0x801e6710u&&children[2]==0x801e693cu);
  assert(child_args[0]==0x80170100u&&child_args[1]==(live?0x80170200u:0x80170100u)&&child_args[2]==ptr+0x30u);
  assert(child_stacks[0]==0x80153ff8u&&child_stacks[1]==0x80153ff8u&&child_stacks[2]==original_outer_sp-0x20u);
  assert(load_word(0x80153ffcu)==original_outer_sp-0x20u&&load_word(ptr+8u)==0x801e6f00u);
  assert(free_tokens[0]==(uint32_t)(uintptr_t)PSX_ADDR(0x80150000u)&&free_tokens[1]==(uint32_t)(uintptr_t)PSX_ADDR(live?0x80170300u:0x80170100u));
 }
 assert(PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 integrated=0;
}
int main(void)
{
 load_inputs();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 printf("TIMER SERVICE ABI PASS %u typed allocation/query signatures, aliases, scalar words, cached dispatch and refusals\n",interfaces());
 unsigned total=0;for(unsigned flags=0;flags<256;flags++)for(unsigned mode=0;mode<4;mode++){
  composed(flags,mode==1?1:mode==2?-1:0,mode==3,flags&1);total++;
 }
 printf("TIMER PUMP INTEGRATION PASS %u real packed allocation/register/pump/callback cases, native interpreted instructions=0; heap/archive/GPU/startup-child services modeled\n",total);return 0;
}
