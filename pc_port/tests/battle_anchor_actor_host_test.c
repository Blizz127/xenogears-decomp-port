/* Full actor coordinator with the actual anchor and AA514 bodies. Original
 * matrix SDK instructions provide the oracle; nine overlay children remain
 * modeled. SDK volatile CPU registers follow their typed C ABI. */
#define ACTOR_INTEGRATION_MAIN actor_previous_integration_main
#define actual_bridge actor_previous_bridge
#include "battle_actor_update_integration_test.c"
#undef actual_bridge
#undef ACTOR_INTEGRATION_MAIN
static unsigned in_anchor,anchor_sdk,anchor_calls,matrix_calls;
static uint64_t anchor_steps,matrix_steps;
static unsigned anchor_mask,anchor_mode;
static unsigned composed_refusals;
static int composed_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 if(address-0x800a44c0u<404u){assert(width==4);*value=load_le(battle+(address-0x8006faf0u),4);return 0;}
 static const struct {uint32_t start,length;} spans[]={{0x8004931cu,352},{0x80049efcu,48},{0x80049f8cu,32}};
 for(unsigned i=0;i<3;i++)if(address-spans[i].start<spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 return actual_read(opaque,address,width,value);
}
static int composed_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(anchor_sdk)return 0;
 if(in_anchor){
  if(target-0x800a44c0u<404u)return candidate?runtime_bridge(opaque,cpu,target):0;
  if(target!=0x8004931cu&&target!=0x80049efcu&&target!=0x80049f8cu)fail("anchor matrix target",target);
  record_call(cpu,target);matrix_calls++;
  if(candidate)return runtime_bridge(opaque,cpu,target);
  PcPortMipsCpu original=*cpu;original.bus.read=composed_read;uint64_t before=original.steps;anchor_sdk=1;
  int rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);anchor_sdk=0;matrix_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED||original.load_valid)fail("anchor original SDK",original.pc);
  for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("anchor SDK preserved register",i);
  if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31])fail("anchor SDK frame",target);
  if(target==0x8004931cu)cpu->gpr[2]=original.gpr[2];
  return 1;
 }
 if(target!=0x800a44c0u)return actor_previous_bridge(opaque,cpu,target);
 record_call(cpu,target);anchor_calls++;int saved=observing;observing=0;in_anchor=1;int rc;
 if(candidate)rc=runtime_bridge(opaque,cpu,target);
 else{
  PcPortMipsCpu original=*cpu;original.bus.read=composed_read;uint64_t before=original.steps;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],20000);anchor_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED||original.load_valid)fail("original full anchor",original.pc);
  memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));cpu->hi=original.hi;cpu->lo=original.lo;rc=1;
 }
 in_anchor=0;observing=saved;return rc;
}
static uint32_t body_token(unsigned d,unsigned i)
{
 static const uint32_t tokens[]={0x80170000u,0xa0170000u,0x80370000u,0x00170000u,0x80062000u,0x1f800600u,0};
 return (d==6?(uint32_t)(uintptr_t)(host+0x70000u):tokens[d])+i*0x200u;
}
static void anchor_matrix(uint32_t address,unsigned i,int bone)
{
 static const int16_t identity[]={4096,0,0,0,4096,0,0,0,4096};
 static const int16_t rotated[]={0,4096,0,-4096,0,0,0,0,4096};
 for(unsigned j=0;j<9;j++)fixture_store(address+2u*j,2,(uint16_t)(anchor_mode&&!bone?rotated[j]:identity[j]));
 fixture_store(address+18u,2,0x8172u);
 for(unsigned j=0;j<3;j++)fixture_store(address+20u+4u*j,4,(bone?10u:100u)+i*13u+j*19u);
}
static void composed_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 actual_setup(cpu,domain,0);observing=0;cpu->bus.bridge=composed_bridge;
 for(unsigned i=0;i<2;i++){
  if(scenario==0)actor_record(i);
  uint32_t actor=actor_base+i*0x80u,old_body=peek(actor+4u,4),body=body_token(domain,i);
  for(unsigned j=0;j<0x80u;j+=4u)fixture_store(body+j,4,peek(old_body+j,4));
  fixture_store(actor+4u,4,body);
  int bone=(int)((seed+i)%3u)-1;
  anchor_matrix(body+12u,i,0);anchor_matrix(body+0xa8u+(uint32_t)bone*0x7cu,i,1);
  uint32_t record=0x800d3304u+i*0x14u;
  for(unsigned j=0;j<3;j++){fixture_store(record+2u*j,2,0xd100u+i*11u+j);fixture_store(record+8u+2u*j,2,seed*713u+i*13u+j*37u);}
  fixture_store(record+6u,2,anchor_mask&(1u<<i)?1u:0u);
  fixture_store(record+16u,2,seed==6&&i==0?0xffffu:i);fixture_store(record+18u,2,(uint16_t)bone);
 }
 rt.functions[0]=(ResolvedFunction){0x8003f8ccu,(void *)rcos,"rsin"};
 rt.functions[1]=(ResolvedFunction){0x8004931cu,(void *)CompMatrix,"CompMatrix"};
 rt.functions[2]=(ResolvedFunction){0x80049efcu,(void *)SetRotMatrix,"SetRotMatrix"};
 rt.functions[3]=(ResolvedFunction){0x80049f5cu,(void *)SetColorMatrix,"SetColorMatrix"};
 rt.functions[4]=(ResolvedFunction){0x80049f8cu,(void *)SetTransMatrix,"SetTransMatrix"};
 rt.functions[5]=(ResolvedFunction){0x8004a0ecu,(void *)SetBackColor,"SetBackColor"};rt.function_count=6;
 if(cached)for(unsigned i=0;i<6;i++){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];}
 in_anchor=anchor_sdk=anchor_calls=matrix_calls=0;anchor_steps=matrix_steps=0;observing=1;
}
static void composed_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;composed_setup(&original,domain,cached);candidate=compare=0;
 int original_rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);
 if(original_rc!=PC_PORT_MIPS_HALTED){
  if(domain!=3||!strstr(original.error,"read32 fault"))fail("original actor with anchor",original.pc);
  composed_refusals++;
 }
 expected_calls=call_count;expected_accesses=accesses;expected_real_calls=real_calls;
 unsigned expected_anchor=anchor_calls,expected_matrices=matrix_calls,want_commands=oracle_gte_commands;
 uint64_t original_anchor_steps=anchor_steps,original_matrix_steps=matrix_steps;snapshot();
 composed_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if((rc==PC_PORT_MIPS_HALTED)!=(original_rc==PC_PORT_MIPS_HALTED)||native.steps||sdk_steps||anchor_steps||matrix_steps||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||real_calls!=expected_real_calls||anchor_calls!=expected_anchor||matrix_calls!=expected_matrices||native_gte_commands!=want_commands||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actor/anchor execution and counts",native.pc);
 if(original_rc==PC_PORT_MIPS_HALTED&&(!expected_anchor||!original_anchor_steps||(expected_matrices&&!original_matrix_steps)))fail("actual anchor/SDK not exercised",expected_anchor);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal())fail("actor/anchor full final state",domain);
}
#ifndef ANCHOR_ACTOR_MAIN
#define ANCHOR_ACTOR_MAIN main
#endif
int ANCHOR_ACTOR_MAIN(int argc,char **argv)
{
 if(argc==1&&actor_previous_integration_main(argc,argv))return 1;
 if(argc>1&&strcmp(argv[1],"--anchor-actor"))return actor_previous_integration_main(argc,argv);
 load_inputs();unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();seed=s;task_domain=s%6u;stack_kind=s%4u;scenario=s%4u;records_count=2;anchor_mask=s%4u;anchor_mode=s&1u;
  composed_case(d,cached);total++;
 }
 printf("ANCHOR ACTOR COMPOSITION PASS %u cases (%u completed, %u parent physical-address refusals), original/native coordinator and anchor bodies, actual original/native matrix SDK and AA514/cosine/color/background, nine overlay descendants modeled, native interpreted instructions=0\n",total,total-composed_refusals,composed_refusals);return 0;
}
