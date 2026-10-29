/* Complete caller models and original/native matrix SDK composition. */
#define main effect_models_model_main
#include "battle_effect_models_host_test.c"
#undef main
#include <inline_c.h>
static unsigned abi_calls,abi_wrong_result;
static MATRIX *abi_pointers[3];
static MATRIX *probe_compose(MATRIX *a,MATRIX *b,MATRIX *out)
{abi_calls++;abi_pointers[0]=a;abi_pointers[1]=b;abi_pointers[2]=out;return abi_wrong_result?a:out;}
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static uint32_t domain_base(unsigned i){return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *domain_pointer(unsigned i)
{return i<4?g_PsxRam+0x120000u:i==4?shared+0x100u:i==5?g_PsxScratchpad+0x100u:host+0x100u;}
static void abi_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;cpu->bus.bridge=runtime_bridge;
 rt.functions[0]=(ResolvedFunction){0x8004931cu,probe_compose,"CompMatrix"};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8004931cu);slot->target=0x8004931cu;slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[29]=0x1e000000u;cpu->gpr[31]=0x800a497cu;
 for(unsigned i=0;i<3;i++)cpu->gpr[4+i]=domain_base(domain)+i*32u;
 abi_calls=abi_wrong_result=0;memset(abi_pointers,0,sizeof(abi_pointers));
}
static void abi_run(PcPortMipsCpu *cpu,int success,unsigned calls_expected)
{
 PcPortMipsCpu before=*cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,0x8004931cu,cpu->gpr[31],100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("compose ABI status",(uint32_t)rc);
 for(unsigned i=0;i<32;i++)if(cpu->gpr[i]!=(success&&i==2?before.gpr[6]:before.gpr[i]))fail("compose ABI register",i);
 if(abi_calls!=calls_expected||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("compose ABI state/pending/span",0);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned domain=0;domain<7;domain++){
   abi_setup(&cpu,cached,domain);abi_run(&cpu,1,1);
   for(unsigned i=0;i<3;i++)if((uint8_t *)abi_pointers[i]!=domain_pointer(domain)+i*32u)fail("compose ABI pointer",i);
   total++;
  }
  for(unsigned arg=0;arg<3;arg++)for(unsigned bad=0;bad<5;bad++){
   abi_setup(&cpu,cached,6);
   /* Keep the other arguments independently valid, so a later rejection
    * cannot hide a missing check on the argument under test. */
   for(unsigned other=0;other<3;other++)if(other!=arg)cpu.gpr[4+other]=0x80130000u+other*32u;
   if(bad==0)cpu.gpr[4+arg]+=2u;
   if(bad==1)cpu.gpr[4+arg]=0x1e000000u;
   if(bad==2)rt.host_ranges[0].end=(uintptr_t)host+0x100u+arg*32u+31u;
   if(bad==3)cpu.gpr[4+arg]=0x801ffff0u;
   if(bad==4)cpu.gpr[4+arg]=0x1f800ff0u;
   abi_run(&cpu,0,0);total++;
  }
  abi_setup(&cpu,cached,6);rt.host_ranges[0].writable=0;abi_run(&cpu,0,0);total++;
  abi_setup(&cpu,cached,6);rt.host_ranges[0].writable=0;cpu.gpr[6]=0x80130000u;abi_run(&cpu,1,1);total++;
  abi_setup(&cpu,cached,6);rt.host_ranges[0].end=(uintptr_t)host+0x160u;abi_run(&cpu,1,1);total++;
  abi_setup(&cpu,cached,0);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0,0);total++;
  abi_setup(&cpu,cached,0);abi_wrong_result=1;abi_run(&cpu,0,1);total++;
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);return total;
}
static unsigned in_sdk,real_count,want_real_count;
static uint64_t sdk_steps;
static int real_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==targets[kind]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);if(bi>=0&&target!=targets[bi])return 0;
 if(candidate&&compare&&call_count<expected_calls&&!hash_call_snapshots){
  for(unsigned i=0;i<32;i++){
   if(gteRegs.CP2D.r[i]!=calls[call_count].gte.CP2D.r[i])fprintf(stderr,"SDK GTE data %u native=%08x oracle=%08x target=%08x\n",i,gteRegs.CP2D.r[i],calls[call_count].gte.CP2D.r[i],target);
   if(gteRegs.CP2C.r[i]!=calls[call_count].gte.CP2C.r[i])fprintf(stderr,"SDK GTE control %u native=%08x oracle=%08x target=%08x\n",i,gteRegs.CP2C.r[i],calls[call_count].gte.CP2C.r[i],target);
  }
 }
 record_call(cpu,target);
 if(target==0x8002c700u)return service(opaque,cpu,target);
 if(target!=0x8004931cu&&target!=0x80049efcu&&target!=0x80049f8cu)return -1;
 real_count++;int old_observing=observing;observing=0;
 if(candidate){int rc=runtime_bridge(opaque,cpu,target);observing=old_observing;return rc;}
 PcPortMipsCpu original=*cpu;in_sdk=1;uint64_t before=original.steps;
 int rc=PcPortMipsRun(&original,target,cpu->gpr[31],50000);in_sdk=0;sdk_steps+=original.steps-before;observing=old_observing;
 if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("original matrix SDK preserved register",i);
 if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("original matrix SDK frame/return",target);
 /* SDK volatile CPU clobbers are normalized to its C ABI. All memory and
  * graphics registers retain their actual original results. */
 if(target==0x8004931cu)cpu->gpr[2]=original.gpr[2];
 return 1;
}
static unsigned translation_variant;
static void matrix_fill(uint32_t address,unsigned value)
{
 for(unsigned i=0;i<9;i++)fixture_store(address+i*2u,2,(uint16_t)(i%4u==0?4096:(int)(value*37u+i*53u)%1024-512));
 fixture_store(address+18u,2,0x8531u+value);
 for(unsigned i=0;i<3;i++){
  uint32_t translation=(uint32_t)((int)(value*117u+i*983u)-2048);
  if(translation_variant==1)translation=0xffff8000u+value*13u+i*57u;
  if(translation_variant==2)translation=0x12348000u+value*13u+i*57u;
  if(translation_variant==3)translation=0x89ab0000u+value*13u+i*57u;
  fixture_store(address+20u+i*4u,4,translation);
 }
}
static void prepare_real(PcPortMipsCpu *cpu,unsigned domain,unsigned alias)
{
 setup(cpu);observing=0;cpu->bus.bridge=real_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);
 const uint32_t sdk[]={0x8004931cu,0x80049efcu,0x80049f8cu};const unsigned length[]={352,48,32};
 for(unsigned i=0;i<3;i++)memcpy(PSX_ADDR(sdk[i]),exe->bytes+XENO_RD_SLUS_OFF(sdk[i]),length[i]);
 uint32_t input=domain_base(domain);matrix_fill(input,seed+1u);
 for(unsigned i=0;i<record_count-1u;i++)matrix_fill(header+0x7cu*(i+1u)+0x2cu,seed+7u+i);
 if(alias==1)input=0x1f800040u;
 if(alias==2)input=header+0x7cu+0x2cu;
 if(alias==3)input=0x1f800044u;
 if(alias==1||alias==3)matrix_fill(input,seed+1u);
 cpu->gpr[6]=input;
 rt.functions[0]=(ResolvedFunction){sdk[0],CompMatrix,"CompMatrix"};rt.functions[1]=(ResolvedFunction){sdk[1],SetRotMatrix,"SetRotMatrix"};rt.functions[2]=(ResolvedFunction){sdk[2],SetTransMatrix,"SetTransMatrix"};rt.function_count=3;
 real_count=in_sdk=0;sdk_steps=0;observing=1;
}
static void integration_case(unsigned domain,unsigned alias)
{
 PcPortMipsCpu oracle,native;prepare_real(&oracle,domain,alias);candidate=compare=0;
 int rc=PcPortMipsRun(&oracle,targets[0],BATTLE_HALT_PC,200000);if(rc!=PC_PORT_MIPS_HALTED)fail(oracle.error,oracle.pc);
 expected_calls=call_count;expected_accesses=accesses;want_real_count=real_count;unsigned want_commands=oracle_gte_commands;uint64_t steps=sdk_steps;snapshot();
 prepare_real(&native,domain,alias);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||sdk_steps||!steps||rt.bridge_cpu||native_gte_commands!=want_commands||real_count!=want_real_count||call_count!=expected_calls||accesses!=expected_accesses||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual matrix SDK execution/counts",native.pc);
 if(memcmp(native.gpr,oracle.gpr,sizeof(native.gpr))||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||native.hi!=oracle.hi||native.lo!=oracle.lo||!memory_equal())fail("actual matrix SDK caller/graphics/memory",alias);
}
static unsigned child_gates(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;cpu.gpr[31]=0x800a4aecu;
  rt.functions[0]=(ResolvedFunction){0x8002c700u,(void *)legacy_probe,"disabled_model_dispatch"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8002c700u);slot->target=0x8002c700u;slot->state=1;slot->entry=rt.functions[0];}
  /* Packet caller adoption has its own full-body suite. Its opt-out must
   * still refuse here even when a legacy generic entry is cached. */
  assert(setenv("XENO_BATTLE_NATIVE_PACKET_RENDER","0",1)==0);
  PcPortMipsCpu old=cpu;snapshot();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,0x8002c700u,cpu.gpr[31],100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  assert(unsetenv("XENO_BATTLE_NATIVE_PACKET_RENDER")==0);
  if(rc>=0||legacy_calls||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("disabled model dispatcher adoption",0);
  total++;
 }
 defaults();return total;
}
static unsigned sdk_compositions(void)
{
 unsigned total=0;
 for(translation_variant=0;translation_variant<4;translation_variant++)
  for(unsigned domain=0;domain<7;domain++)if(domain!=3)for(unsigned alias=0;alias<4;alias++)for(unsigned type=0;type<8;type++){
   defaults();record_count=2;model_kind=type;seed=type+domain;integration_case(domain,alias);total++;
  }
 translation_variant=0;return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--interfaces")){printf("EFFECT MODELS ABI PASS %u cases\n",interfaces());return 0;}
  if(!strcmp(argv[1],"--integration-basic")){record_count=2;integration_case(0,0);return 0;}
  if(!strcmp(argv[1],"--sdk-translations")){printf("EFFECT MODELS ACTUAL SDK PASS %u compositions\n",sdk_compositions());return 0;}
  if(!strcmp(argv[1],"--child-gates")){printf("EFFECT MODELS CHILD GATES PASS %u cases\n",child_gates());return 0;}
  return effect_models_model_main(argc,argv);
 }
 if(effect_models_model_main(argc,argv))return 1;
 printf("EFFECT MODELS ABI PASS %u cases\n",interfaces());fflush(stdout);
 printf("EFFECT MODELS ACTUAL SDK PASS %u compositions\n",sdk_compositions());
 printf("EFFECT MODELS CHILD GATES PASS %u cases\n",child_gates());
 puts("EFFECT MODELS INTEGRATION PASS; packet dispatcher is modeled in these caller tests; shared graphics backend is not a hardware oracle");return 0;
}
