/* Caller models plus concrete SDK spans and original/native memcpy composition. */
#define main palette_update_model_main
#include "battle_palette_update_host_test.c"
#undef main
static unsigned abi_calls;
static RECT16 *abi_rect;
static u_long *abi_pixels;
static int probe_upload(RECT16 *rect,u_long *pixels)
{abi_calls++;abi_rect=rect;abi_pixels=pixels;return (int)0xfacebeefu;}
static void *probe_copy(void *d,const void *s,size_t n)
{(void)d;(void)s;(void)n;fail("legacy memcpy called",0);return NULL;}
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static uint32_t domain_base(unsigned i){return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *domain_pointer(unsigned i,int expected)
{return i<4?(expected?want_ram:g_PsxRam)+0x120000u:i==4?(expected?want_shared:shared)+0x100u:i==5?(expected?want_pad:g_PsxScratchpad)+0x100u:(expected?want_host:host)+0x100u;}
static void abi_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain,int upload)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;cpu->bus.bridge=runtime_bridge;
 uint32_t target=upload?0x80044894u:0x8003f968u;
 rt.functions[0]=(ResolvedFunction){target,upload?(void *)probe_upload:(void *)probe_copy,upload?"LoadImage":"memcpy"};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[29]=0x1e000000u; /* No speculative stack-argument reads. */
 cpu->gpr[31]=upload?0x800a6f64u:0x800a6b78u;
 cpu->gpr[4]=domain_base(domain);cpu->gpr[5]=domain_base(domain)+0x100u;cpu->gpr[6]=17;
 uint8_t *p=domain_pointer(domain,0);for(unsigned i=0;i<0x200u;i++)p[i]=(uint8_t)(i*53u+7u);
 if(upload){fixture_store(cpu->gpr[4],2,0x3ffu);fixture_store(cpu->gpr[4]+2u,2,511);fixture_store(cpu->gpr[4]+4u,2,4);fixture_store(cpu->gpr[4]+6u,2,2);}
 abi_calls=0;abi_rect=NULL;abi_pixels=NULL;
}
static void abi_run(PcPortMipsCpu *cpu,int success,int upload)
{
 PcPortMipsCpu before=*cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,upload?0x80044894u:0x8003f968u,cpu->gpr[31],100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("SDK ABI status",(uint32_t)rc);
 for(unsigned i=0;i<32;i++){
  uint32_t expected=before.gpr[i];
  if(success&&i==2)expected=upload?0xfacebeefu:before.gpr[4];
  if(success&&!upload&&i==3)expected=0;
  if(cpu->gpr[i]!=expected)fail("SDK ABI register",i);
 }
 if(abi_calls!=(unsigned)(success&&upload)||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("SDK ABI state/span/pending",0);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  static const uint32_t lengths[]={0,1,17,136,0xffffffffu,0x80000000u};
  for(unsigned domain=0;domain<7;domain++)for(unsigned mode=0;mode<9;mode++){
   abi_setup(&cpu,cached,domain,0);
   unsigned src=0x100u,dst=0;
   cpu.gpr[6]=mode<6?lengths[mode]:32;
   if(mode>=6){src=0;dst=mode==6?1:mode==7?0:0x100u;cpu.gpr[5]=domain_base(domain)+src;cpu.gpr[4]=domain_base(domain)+dst;}
   snapshot();uint8_t *p=domain_pointer(domain,1);
   if((int32_t)cpu.gpr[6]>0)for(uint32_t i=0;i<cpu.gpr[6];i++)p[dst+i]=p[src+i];
   abi_run(&cpu,1,0);total++;
  }
  for(unsigned bad=0;bad<8;bad++){
   abi_setup(&cpu,cached,6,0);
   if(bad==0)cpu.gpr[4]=0x1e000000u;
   if(bad==1)cpu.gpr[5]=0x1e000000u;
   if(bad==2)rt.host_ranges[0].writable=0;
   if(bad==3)rt.host_ranges[0].end=(uintptr_t)host+0x210u;
   if(bad==4)cpu.gpr[4]=0x801ffff8u;
   if(bad==5)cpu.gpr[5]=0x1f800ff8u;
   if(bad==6)cpu.gpr[6]=0x7fffffffu;
   if(bad==7){cpu.load_valid=1;cpu.load_reg=4;}
   snapshot();abi_run(&cpu,0,0);total++;
  }
  abi_setup(&cpu,cached,0,0);cpu.gpr[4]=0;cpu.gpr[5]=0x1e000000u;cpu.gpr[6]=0x7fffffffu;snapshot();abi_run(&cpu,1,0);total++;
  for(unsigned domain=0;domain<7;domain++){
   abi_setup(&cpu,cached,domain,1);snapshot();abi_run(&cpu,1,1);
   if((uint8_t *)abi_rect!=domain_pointer(domain,0)||(uint8_t *)abi_pixels!=domain_pointer(domain,0)+0x100u)fail("upload pointer",domain);
   total++;
  }
  for(unsigned bad=0;bad<12;bad++){
   abi_setup(&cpu,cached,6,1);
   if(bad==0)cpu.gpr[4]++;
   if(bad==1)cpu.gpr[5]++;
   if(bad==2)cpu.gpr[4]=0x1e000000u;
   if(bad==3)cpu.gpr[5]=0x1e000000u;
   if(bad==4)rt.host_ranges[0].end=(uintptr_t)host+0x107u;
   if(bad==5)rt.host_ranges[0].end=(uintptr_t)host+0x20fu;
   if(bad==6)cpu.gpr[5]=0x801ffff8u;
   if(bad==7)cpu.gpr[5]=0x1f800ff8u;
   if(bad==8)fixture_store(cpu.gpr[4]+4u,2,0xffffu);
   if(bad==9)fixture_store(cpu.gpr[4]+6u,2,0);
   if(bad==10){fixture_store(cpu.gpr[4]+4u,2,32767);fixture_store(cpu.gpr[4]+6u,2,32767);}
   if(bad==11){cpu.load_valid=1;cpu.load_reg=4;}
   snapshot();abi_run(&cpu,0,1);total++;
  }
  abi_setup(&cpu,cached,6,1);rt.host_ranges[0].end=(uintptr_t)host+0x210u;rt.host_ranges[0].writable=0;snapshot();abi_run(&cpu,1,1);total++;
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
 if(target!=0x8003f968u)return service(opaque,cpu,target);
 real_count++;int saved_observing=observing;observing=0;
 if(candidate){int rc=runtime_bridge(opaque,cpu,target);observing=saved_observing;return rc;}
 PcPortMipsCpu original=*cpu;in_sdk=1;uint64_t before=original.steps;
 int rc=PcPortMipsRun(&original,target,cpu->gpr[31],50000);in_sdk=0;sdk_steps+=original.steps-before;observing=saved_observing;
 if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("original memcpy preserved register",i);
 if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("original memcpy frame/return",target);
 /* Volatile registers follow the established C ABI. Returned token and all
  * copied bytes are the actual original result, including forward overlap. */
 cpu->gpr[2]=original.gpr[2];cpu->gpr[3]=0;return 1;
}
static void prepare_real(PcPortMipsCpu *cpu,unsigned domain,unsigned mode)
{
 setup(cpu);observing=0;cpu->bus.bridge=real_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);
 memcpy(PSX_ADDR(0x8003f968u),exe->bytes+XENO_RD_SLUS_OFF(0x8003f968u),52);
 uint32_t destination=domain_base(domain),source=destination+0x100u;
 if(mode==1)destination=source+1u;
 if(mode==2)destination=source-1u;
 if(mode==3)destination=source;
 fixture_store(0x800d3604u,4,source);fixture_store(0x800d3608u,4,destination);
 for(unsigned i=0;i<64;i++)fixture_store(source+i,1,i*37u+19u);
 rt.functions[0]=(ResolvedFunction){0x8003f968u,probe_copy,"memcpy"};rt.function_count=1;
 real_count=in_sdk=0;sdk_steps=0;observing=1;
}
static unsigned integration(void)
{
 unsigned total=0;
 for(unsigned domain=0;domain<7;domain++)if(domain!=3)for(unsigned mode=0;mode<5;mode++){
  defaults();active_mask=0;stack_kind=1;if(mode==4)dim_x=-1;
  PcPortMipsCpu oracle,native;prepare_real(&oracle,domain,mode);candidate=compare=0;
  int rc=PcPortMipsRun(&oracle,targets[0],BATTLE_HALT_PC,100000);
  if(rc!=PC_PORT_MIPS_HALTED||!sdk_steps||real_count!=1)fail("original memcpy composition",(uint32_t)rc);
  expected_calls=call_count;expected_accesses=accesses;want_real_count=real_count;snapshot();
  prepare_real(&native,domain,mode);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc!=PC_PORT_MIPS_HALTED||memcmp(native.gpr,oracle.gpr,sizeof(native.gpr))||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||native.hi!=oracle.hi||native.lo!=oracle.lo||native.steps||sdk_steps||native_gte_commands||rt.bridge_cpu||real_count!=want_real_count||call_count!=expected_calls||accesses!=expected_accesses||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native memcpy composition",(uint32_t)rc);
  total++;
 }
 defaults();return total;
}
static unsigned child_gates(void)
{
 unsigned total=0;const uint32_t child[]={0x80025d4cu,0x80026f44u,0x800a6884u};
 const uint32_t returns[]={0x800a6d04u,0x800a6d44u,0x800a6d84u};
 for(unsigned i=0;i<3;i++)for(unsigned cached=0;cached<2;cached++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;cpu.gpr[31]=returns[i];
  rt.functions[0]=(ResolvedFunction){child[i],(void *)legacy_probe,"unproven_palette_child"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,child[i]);slot->target=child[i];slot->state=1;slot->entry=rt.functions[0];}
  PcPortMipsCpu old=cpu;snapshot();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,child[i],returns[i],100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc>=0||legacy_calls||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("unproven child adoption",child[i]);
  total++;
 }
 defaults();return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--interfaces")){printf("PALETTE SDK ABI PASS %u cases\n",interfaces());return 0;}
  if(!strcmp(argv[1],"--composition")){printf("PALETTE MEMCPY COMPOSITION PASS %u cases\n",integration());return 0;}
  if(!strcmp(argv[1],"--child-gates")){printf("PALETTE CHILD GATES PASS %u cases\n",child_gates());return 0;}
  return palette_update_model_main(argc,argv);
 }
 if(palette_update_model_main(argc,argv))return 1;
 printf("PALETTE SDK ABI PASS %u cases\n",interfaces());
 printf("PALETTE MEMCPY COMPOSITION PASS %u cases\n",integration());
 printf("PALETTE CHILD GATES PASS %u cases\n",child_gates());
 puts("PALETTE INTEGRATION PASS; LoadImage backend and color children remain outside original-body proof");return 0;
}
