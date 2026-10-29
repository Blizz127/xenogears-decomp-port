/* Actual actor, anchor and effect-list bodies. Eight other overlay children
 * remain modeled; original SDK CPU volatile state uses the typed C ABI. */
#define ANCHOR_ACTOR_MAIN effect_list_previous_actor_main
#define composed_bridge anchor_previous_composed_bridge
#include "battle_anchor_actor_host_test.c"
#undef composed_bridge
#undef ANCHOR_ACTOR_MAIN
static unsigned in_list,list_sdk,list_calls,list_matrix_calls,list_refusals;
static uint64_t list_steps,list_matrix_steps;
static int effect_actor_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 if(address-0x800a2f94u<836u){assert(width==4);*value=load_le(battle+(address-0x8006faf0u),4);return 0;}
 return composed_read(opaque,address,width,value);
}
static int effect_actor_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(list_sdk)return 0;
 if(in_list){
  if(target==0x800a2fd8u)return candidate?runtime_bridge(opaque,cpu,target):0;
  if(target-0x800a2f94u<836u){
   if(target!=0x800a2f94u)return 0;
   record_call(cpu,target);return candidate?runtime_bridge(opaque,cpu,target):0;
  }
  if(target!=0x80049efcu&&target!=0x80049f8cu)fail("effect-list matrix target",target);
  record_call(cpu,target);list_matrix_calls++;
  if(candidate)return runtime_bridge(opaque,cpu,target);
  PcPortMipsCpu original=*cpu;original.bus.read=effect_actor_read;uint64_t before=original.steps;list_sdk=1;
  int rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);list_sdk=0;list_matrix_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED||original.load_valid)fail("effect-list original SDK",original.pc);
  for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("effect-list SDK preserved register",i);
  if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31])fail("effect-list SDK frame",target);
  return 1;
 }
 if(target!=0x800a2fd8u)return anchor_previous_composed_bridge(opaque,cpu,target);
 record_call(cpu,target);list_calls++;int saved=observing;observing=0;in_list=1;int rc;
 if(candidate)rc=runtime_bridge(opaque,cpu,target);
 else{
  PcPortMipsCpu original=*cpu;original.bus.read=effect_actor_read;uint64_t before=original.steps;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],20000);list_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED||original.load_valid)fail("original full effect list",original.pc);
  memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));cpu->hi=original.hi;cpu->lo=original.lo;rc=1;
 }
 in_list=0;observing=saved;return rc;
}
static void effect_actor_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 composed_setup(cpu,domain,cached);observing=0;cpu->bus.bridge=effect_actor_bridge;
 const uint32_t domains[]={0x801c0000u,0xa01c0000u,0x803c0000u,0x80064000u,0x1f800a00u,(uint32_t)(uintptr_t)(host+0x80000u)};
 uint32_t records=domains[seed%6u],view=cpu->gpr[4],ot=0x801a0000u;
 cpu->gpr[6]=ot;cpu->gpr[7]=seed&1u;
 for(unsigned i=0;i<8;i++)fixture_store(view+4u*i,4,i==0||i==2||i==4?4096u:0u);
 for(unsigned i=0;i<0x4000u;i++)fixture_store(ot+4u*i,4,0x8d123456u+i);
 CTC2(160u<<16,24);CTC2(120u<<16,25);CTC2(256,26);CTC2(0,27);CTC2(0,28);
 fixture_store(0x80050100u,4,3);
 fixture_store(0x800c3d04u,4,records);fixture_store(0x800c3d08u,2,seed==4?0u:3u);fixture_store(0x800c3d0au,2,7);
 for(unsigned i=0;i<3;i++){
  uint32_t a=records+0x7cu*i;
  for(unsigned j=0;j<4;j++){
   fixture_store(a+8u*j,2,8u+i*2u+j*8u);fixture_store(a+8u*j+2u,2,8u+i*2u+j*2u);
   fixture_store(a+8u*j+4u,2,512u);fixture_store(a+8u*j+6u,2,0);
  }
  fixture_store(a+0xeu,2,seed&1u);fixture_store(a+0x16u,2,seed%4u==3?20u:seed%4u==2?0xffffu:0u);fixture_store(a+0x1eu,2,20);
  for(unsigned j=0;j<3;j++){fixture_store(a+0x20u+2u*j,2,0x3201u+j*0x1100u);fixture_store(a+0x26u+2u*j,2,seed*711u-j*317u);}
  fixture_store(a+0x2cu,4,0x09111111u);fixture_store(a+0x54u,4,0x09222222u);
 }
 in_list=list_sdk=list_calls=list_matrix_calls=0;list_steps=list_matrix_steps=0;observing=1;
}
static void effect_actor_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;effect_actor_setup(&original,domain,cached);candidate=compare=0;
 int original_rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);
 if(original_rc!=PC_PORT_MIPS_HALTED){
  if(domain!=3||!strstr(original.error,"read32 fault"))fail("original actor with effect list",original.pc);
  list_refusals++;
 }
 expected_calls=call_count;expected_accesses=accesses;expected_real_calls=real_calls;
 unsigned want_anchor=anchor_calls,want_matrix=matrix_calls,want_list=list_calls,want_list_matrix=list_matrix_calls,want_commands=oracle_gte_commands;
 uint64_t original_list_steps=list_steps,original_matrix_steps=list_matrix_steps;snapshot();
 effect_actor_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if((rc==PC_PORT_MIPS_HALTED)!=(original_rc==PC_PORT_MIPS_HALTED)||native.steps||sdk_steps||anchor_steps||matrix_steps||list_steps||list_matrix_steps||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||real_calls!=expected_real_calls||anchor_calls!=want_anchor||matrix_calls!=want_matrix||list_calls!=want_list||list_matrix_calls!=want_list_matrix||native_gte_commands!=want_commands||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actor/anchor/effect-list execution and counts",native.pc);
 if(original_rc==PC_PORT_MIPS_HALTED&&(!want_list||!original_list_steps||!want_list_matrix||!original_matrix_steps))fail("actual effect-list/SDK not exercised",want_list);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal())fail("actor/anchor/effect-list full final state",domain);
}
int main(int argc,char **argv)
{
 if(argc>1&&strcmp(argv[1],"--effect-list-actor"))return effect_list_previous_actor_main(argc,argv);
 load_inputs();unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();seed=s;task_domain=s%6u;stack_kind=s%4u;scenario=s%4u;records_count=2;anchor_mask=s%4u;anchor_mode=s&1u;
  effect_actor_case(d,cached);total++;
 }
 printf("EFFECT LIST ACTOR COMPOSITION PASS %u cases (%u completed, %u parent physical-address refusals), original/native coordinator, anchor, effect-list and release; actual original/native SDK and AA514; eight overlay children modeled; native interpreted instructions=0\n",total,total-list_refusals,list_refusals);return 0;
}
