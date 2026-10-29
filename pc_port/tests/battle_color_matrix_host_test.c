/* Exact matrix service extents and original/native SDK results. The existing
 * fixture supplies memory domains, real runtime dispatch and graphics backend. */
#define main color_matrix_model_main
#include "battle_effects_render_host_test.c"
#undef main
static const uint32_t matrix_targets[]={0x80049f2cu,0x80049f5cu};
static const char *matrix_names[]={"SetLightMatrix","SetColorMatrix"};
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static unsigned setter,abi_calls;
static MATRIX *abi_pointer;
static void probe_matrix(MATRIX *input){abi_calls++;abi_pointer=input;}
static uint32_t domain_base(unsigned i)
{return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *domain_pointer(unsigned i)
{return i<4?g_PsxRam+0x120000u:i==4?shared+0x100u:i==5?g_PsxScratchpad+0x100u:host+0x100u;}
static void matrix_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain,int real)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;
 cpu->bus.bridge=runtime_bridge;cpu->gpr[29]=0x1e000000u;
 void *function=real?(setter?(void *)SetColorMatrix:(void *)SetLightMatrix):(void *)probe_matrix;
 rt.functions[0]=(ResolvedFunction){matrix_targets[setter],function,matrix_names[setter]};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,matrix_targets[setter]);slot->target=matrix_targets[setter];slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[4]=domain_base(domain);abi_calls=0;abi_pointer=NULL;
}
static void abi_run(PcPortMipsCpu *cpu,int success)
{
 PcPortMipsCpu before=*cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,matrix_targets[setter],BATTLE_HALT_PC,100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("matrix ABI result",(uint32_t)rc);
 if(abi_calls!=(unsigned)success||memcmp(cpu->gpr,before.gpr,sizeof(cpu->gpr))||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("matrix ABI pointer/void/pending/span",matrix_targets[setter]);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(setter=0;setter<2;setter++)for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned domain=0;domain<7;domain++){
   matrix_setup(&cpu,cached,domain,0);abi_run(&cpu,1);assert((uint8_t *)abi_pointer==domain_pointer(domain));total++;
  }
  for(unsigned bad=0;bad<5;bad++){
   matrix_setup(&cpu,cached,0,0);
   if(bad==0)cpu.gpr[4]+=2u;
   else if(bad==1)cpu.gpr[4]=0x1e000000u;
   else if(bad==2){cpu.gpr[4]=domain_base(6);rt.host_ranges[0].end=(uintptr_t)host+0x113u;}
   else if(bad==3)cpu.gpr[4]=0x801ffff0u;
   else cpu.gpr[4]=0x1f800ff0u;
   abi_run(&cpu,0);total++;
  }
  matrix_setup(&cpu,cached,6,0);rt.host_ranges[0].end=(uintptr_t)host+0x114u;rt.host_ranges[0].writable=0;abi_run(&cpu,1);total++;
  matrix_setup(&cpu,cached,0,0);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0);total++;
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);return total;
}
/* The oracle fetches only these private SDK bytes. Native RAM carries no
 * executable SDK code. Physical and mirrored data tokens use the same resolver. */
static int original_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 uint32_t start=matrix_targets[setter];
 if(address-start<48u){const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);*value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;}
 return model_test_raw_read(opaque,shard_sdk_address(address),width,value);
}
static void sdk_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain,unsigned pattern)
{
 matrix_setup(cpu,cached,domain,1);
 static const uint32_t words[]={0,0xffffffffu,0x7fff8000u,0x80007fffu,0xdeadbeefu,0x0000ffffu,0xffff0000u,0x10203040u};
 for(unsigned i=0;i<5;i++)fixture_store(domain_base(domain)+4u*i,4,words[(pattern+i)%8u]);
 if(domain==6){rt.host_ranges[0].end=(uintptr_t)host+0x114u;rt.host_ranges[0].writable=0;}
 memset(PSX_ADDR(0x80049f2cu),0,96);
}
static void sdk_case(unsigned cached,unsigned domain,unsigned pattern)
{
 PcPortMipsCpu original,native;sdk_setup(&original,cached,domain,pattern);
 original.bus.read=original_read;original.bus.bridge=NULL;candidate=0;
 PcPortMipsCpu before=original;
 int rc=PcPortMipsRun(&original,matrix_targets[setter],BATTLE_HALT_PC,100);
 if(rc!=PC_PORT_MIPS_HALTED||original.steps!=12||original.load_valid)fail("original matrix SDK",original.pc);
 /* Only T0..T4 are SDK scratch registers. The native void C ABI preserves
  * every caller register; graphics and all memory are compared without normalization. */
 for(unsigned i=0;i<32;i++)if((i<8||i>12)&&original.gpr[i]!=before.gpr[i])fail("original matrix ABI",i);
 snapshot();sdk_setup(&native,cached,domain,pattern);before=native;
 PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 rc=PcPortMipsRun(&native,matrix_targets[setter],BATTLE_HALT_PC,100);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||native.load_valid||rt.bridge_cpu||memcmp(before.gpr,native.gpr,sizeof(native.gpr))||memcmp(before.cp0,native.cp0,sizeof(native.cp0))||before.hi!=native.hi||before.lo!=native.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual matrix SDK state",matrix_targets[setter]);
}
int main(void)
{
 load_inputs();defaults();printf("MATRIX ABI PASS %u cases: seven domains, exact read-only extent, alignment, unmapped/crossing/short spans, pending entry, cached/resolved, void GPRs\n",interfaces());fflush(stdout);
 unsigned total=0;
 for(setter=0;setter<2;setter++)for(unsigned cached=0;cached<2;cached++)for(unsigned domain=0;domain<7;domain++)for(unsigned pattern=0;pattern<8;pattern++){
  sdk_case(cached,domain,pattern);total++;
 }
 printf("MATRIX ACTUAL SDK PASS %u original/native comparisons, all graphics registers and memory, native interpreted instructions=0; original volatile T0..T4 normalized to void C ABI\n",total);return 0;
}
