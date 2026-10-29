/* Body models, concrete light-matrix interface and SDK composition. */
#define main effects_render_model_main
#include "battle_effects_render_host_test.c"
#undef main
#include <inline_c.h>
static unsigned abi_calls;
static MATRIX *abi_pointer;
static void probe_light(MATRIX *input){abi_calls++;abi_pointer=input;}
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static uint32_t domain_base(unsigned i){return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *domain_pointer(unsigned i)
{return i<4?g_PsxRam+0x120000u:i==4?shared+0x100u:i==5?g_PsxScratchpad+0x100u:host+0x100u;}
static void abi_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;cpu->bus.bridge=runtime_bridge;cpu->gpr[29]=0x1e000000u;
 rt.functions[0]=(ResolvedFunction){0x80049f2cu,probe_light,"SetLightMatrix"};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x80049f2cu);slot->target=0x80049f2cu;slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[4]=domain_base(domain);abi_calls=0;abi_pointer=NULL;
}
static void abi_run(PcPortMipsCpu *cpu,int success)
{
 PcPortMipsCpu before=*cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,0x80049f2cu,BATTLE_HALT_PC,100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("light ABI result",(uint32_t)rc);
 if(abi_calls!=(unsigned)success||memcmp(cpu->gpr,before.gpr,sizeof(cpu->gpr))||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("light ABI pointer/void/pending/span",0);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned domain=0;domain<7;domain++){
   abi_setup(&cpu,cached,domain);abi_run(&cpu,1);assert((uint8_t *)abi_pointer==domain_pointer(domain));total++;
  }
  for(unsigned bad=0;bad<5;bad++){
   abi_setup(&cpu,cached,0);
   if(bad==0)cpu.gpr[4]+=2u;
   else if(bad==1)cpu.gpr[4]=0x1e000000u;
   else if(bad==2){cpu.gpr[4]=domain_base(6);rt.host_ranges[0].end=(uintptr_t)host+0x113u;}
   else if(bad==3)cpu.gpr[4]=0x801ffff0u;
   else cpu.gpr[4]=0x1f800ff0u;
   abi_run(&cpu,0);total++;
  }
  abi_setup(&cpu,cached,6);rt.host_ranges[0].end=(uintptr_t)host+0x114u;rt.host_ranges[0].writable=0;abi_run(&cpu,1);total++;
  abi_setup(&cpu,cached,0);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0);total++;
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
 record_call(cpu,target);
 if(target!=0x80049f2cu)return service(opaque,cpu,target);
 real_count++;int was_observing=observing;observing=0;
 if(candidate){int rc=runtime_bridge(opaque,cpu,target);observing=was_observing;return rc;}
 PcPortMipsCpu original=*cpu;in_sdk=1;uint64_t before=original.steps;
 int rc=PcPortMipsRun(&original,target,cpu->gpr[31],50000);in_sdk=0;sdk_steps+=original.steps-before;observing=was_observing;
 if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("original light SDK callee-saved",i);
 if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("original light SDK frame/return",target);
 /* Void C ABI keeps caller CPU state; SDK T0..T4 are deliberately normalized.
  * All graphics registers and memory remain the actual original result. */
 return 1;
}
static void prepare_real(PcPortMipsCpu *cpu,unsigned domain)
{
 setup(cpu);observing=0;cpu->bus.bridge=real_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);
 memcpy(PSX_ADDR(0x80049f2cu),exe->bytes+XENO_RD_SLUS_OFF(0x80049f2cu),48);
 for(unsigned i=0;i<5;i++)fixture_store(domain_base(domain)+i*4u,4,0x713da905u+seed*0x34217u+i*0x1753u);
 cpu->gpr[5]=light_on?domain_base(domain):0;
 rt.functions[0]=(ResolvedFunction){0x80049f2cu,SetLightMatrix,"SetLightMatrix"};rt.function_count=1;
 real_count=in_sdk=0;sdk_steps=0;observing=1;
}
static void integration_case(unsigned domain)
{
 PcPortMipsCpu original,native;prepare_real(&original,domain);candidate=compare=0;
 int rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 expected_calls=call_count;expected_accesses=accesses;want_real_count=real_count;uint64_t original_sdk_steps=sdk_steps;snapshot();
 prepare_real(&native,domain);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||native_gte_commands||rt.bridge_cpu||sdk_steps||real_count!=want_real_count||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual light SDK integration",native.pc);
 if((light_on&&!original_sdk_steps)||want_real_count!=light_on)fail("actual light SDK count",real_count);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal())fail("actual light SDK caller/graphics/memory",seed);
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--interfaces")){printf("EFFECTS RENDER ABI PASS %u concrete light-matrix cases\n",interfaces());return 0;}
  if(!strcmp(argv[1],"--integration-basic")){light_on=1;integration_case(0);return 0;}
  return effects_render_model_main(argc,argv);
 }
 if(effects_render_model_main(argc,argv))return 1;
 printf("EFFECTS RENDER ABI PASS %u concrete light-matrix cases\n",interfaces());fflush(stdout);
 unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned value=0;value<8;value++)for(unsigned gate=0;gate<2;gate++){
  /* Physical tokens are ABI-tested separately; the original CPU test bus requires guest segments. */
  if(d==3)continue;
  defaults();seed=value;task_domain=d<4?d:d==4?5:d==5?6:0;stack_kind=value%4u;records_count=value%3u;light_on=gate;models_on=value&1u;
  /* Scratch input words overlap the owner count at +10E. Keep that valid
   * alias and use full-state hashes for its potentially 255-record loop. */
  if(d==5)hash_call_snapshots=1;
  integration_case(d);total++;
 }
 printf("EFFECTS RENDER ACTUAL SDK PASS %u original/native compositions; SetLightMatrix executes, other children modeled, SDK CPU clobbers normalized, native instructions=0\n",total);return 0;
}
