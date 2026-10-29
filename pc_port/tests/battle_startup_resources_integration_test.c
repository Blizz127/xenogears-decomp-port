/* Original/native memcpy and word-reader compositions, plus the complete
 * timer callback's alternate-stack path. Other nested services stay modeled. */
#define main startup_resources_model_main
#include "battle_startup_resources_host_test.c"
#undef main
static unsigned timer_composition,in_service,actual_calls,expected_actual_calls;
static uint64_t service_steps;
static uint32_t timer_task=0x80190000u,timer_stack=0x801a0000u,outer_sp;
static const uint32_t service_targets[]={0x8003f968u,0x80022a00u,0x8001cd6cu};
static const uint32_t service_returns[]={0x801e63dcu,0x801e664cu,0x801e7078u};
static const unsigned service_sizes[]={52,12,8};
static int original_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 for(unsigned i=0;i<3;i++)if(address-service_targets[i]<service_sizes[i]){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 if(address-0x801e6fecu<172u){assert(width==4);*value=load_le(module+(address-0x801e4000u),4);return 0;}
 return runtime_read(opaque,address,width,value);
}
static int original_service(PcPortMipsCpu *cpu,uint32_t target)
{
 PcPortMipsCpu original=*cpu;original.bus.read=original_read;uint64_t before=original.steps;
 in_service=1;int rc=PcPortMipsRun(&original,target,cpu->gpr[31],200000);in_service=0;service_steps+=original.steps-before;
 memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));
 cpu->hi=original.hi;cpu->lo=original.lo;cpu->steps=original.steps;
 cpu->load_valid=original.load_valid;cpu->load_reg=original.load_reg;cpu->load_value=original.load_value;cpu->load_mask=original.load_mask;
 if(rc==PC_PORT_MIPS_BUDGET)fail("original service budget",target);
 return rc==PC_PORT_MIPS_HALTED?1:-1;
}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_service)return 0;
 if((target==targets[0]&&(cpu->gpr[31]==BATTLE_HALT_PC||cpu->gpr[31]==0x801e7038u))||
    (target==0x801e6fecu&&cpu->gpr[31]==BATTLE_HALT_PC))return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0||target-0x801e6fecu<172u)return 0;
 record_call(cpu,target);
 if(stop_at&&call_count==stop_at)return -1;
 if(battle_startup_resources_service_target(target,cpu->gpr[31])||target==0x8001cd6cu){
  actual_calls++;
  return candidate?runtime_bridge(opaque,cpu,target):original_service(cpu,target);
 }
 if(timer_composition){
  uint32_t result=0;
  if(target==0x80031bdcu&&cpu->gpr[31]==0x801e7014u){
   if(cpu->gpr[4]!=0x4000u||cpu->gpr[5]!=1)fail("actual timer scratch args",0);
   result=timer_stack;
  }else if(target==0x800286ccu)result=0;
  else if(target==0x800320e8u){if(runtime_write(opaque,0x80059100u+call_count*4u,4,cpu->gpr[4]))return -1;}
  else if(target==0x800445d0u){if(cpu->gpr[4])fail("actual timer DrawSync",0);}
  else if(target==0x801e6710u){if(runtime_write(opaque,cpu->gpr[29]-0x10u,4,cpu->gpr[4]))return -1;}
  else if(target==0x801e693cu){if(runtime_write(opaque,cpu->gpr[4],4,0x789abcdeu))return -1;}
  else return service(opaque,cpu,target);
  for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x51000000u+call_count*256u+i;
  cpu->gpr[2]=result;cpu->hi=0x61420000u+call_count;cpu->lo=0x72530000u+call_count;cpu->cp0[3]=0x83640000u+call_count;
  gteRegs.CP2D.r[9]=target^call_count;return 1;
 }
 return service(opaque,cpu,target);
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;cpu->bus.read=original_read;outer_sp=cpu->gpr[29];
 if(timer_composition){
  timer_stack=task_domain==5?(uint32_t)(uintptr_t)host+0x40000u:task_domain==1?0xa01a0000u:0x801a0000u;
  frame=timer_stack+0x3ff8u-0xc8u;initial_sp=timer_stack+0x3ff8u;
  fixture_store(timer_task+0x20u,4,blob);fixture_store(timer_task+8u,4,0x11223344u);cpu->gpr[4]=timer_task;
 }
 for(unsigned i=0;i<3;i++){
  rt.functions[i]=(ResolvedFunction){service_targets[i],(void *)legacy_probe,"unusable_startup_probe"};
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,service_targets[i]);slot->target=service_targets[i];slot->state=1;slot->entry=rt.functions[i];}
 }
 rt.function_count=3;in_service=actual_calls=0;service_steps=0;observing=1;
}
static void actual_case(unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,cached);candidate=compare=0;oracle_cpu=&original;
 uint32_t target=timer_composition?0x801e6fecu:targets[0];
 int original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,200000);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;expected_actual_calls=actual_calls;snapshot();
 actual_setup(&native,cached);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,target,BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("actual service status",native.pc);
 if(native.steps||service_steps||legacy_calls||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||events!=expected_events||actual_calls!=expected_actual_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual service counts",native.pc);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=original.gpr[i]){fprintf(stderr,"native=%08x original=%08x original_pc=%08x\n",native.gpr[i],original.gpr[i],original.pc);fail("actual service register",i);}
 if(memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("actual service complete state",cached);
 if(fault_at&&accesses!=fault_at)fail("actual fault not reached",accesses);
 if(timer_composition&&original_rc==PC_PORT_MIPS_HALTED&&!fault_at&&!stop_at&&!bad_kind&&!alias_case){
  if(native.gpr[29]!=outer_sp||peek(timer_stack+0x3ffcu,4)!=outer_sp-0x20u||peek(timer_stack+0x3ff8u,4)!=blob||peek(timer_task+8u,4)!=0x801e6f00u||peek(timer_task+0x30u,4)!=0x789abcdeu)fail("independent alternate-stack and timer finish",native.gpr[29]);
 }
}
static unsigned actual_compositions(void)
{
 unsigned total=0;
 for(timer_composition=0;timer_composition<2;timer_composition++)for(unsigned sh=0;sh<9;sh++)for(unsigned d=0;d<7;d++){
  defaults();shape=sh;task_domain=d;seed=d*39u;stack_kind=d%4u;actual_case(d%2u);total++;
 }
 timer_composition=0;
 for(unsigned alias=2;alias<=6;alias++){defaults();alias_case=alias;actual_case(1);total++;}
 for(unsigned bad=1;bad<=8;bad++){defaults();bad_kind=bad;task_domain=bad==8?5:0;actual_case(1);total++;}
 for(unsigned m=1;m<=6;m++){
  if(m==2)continue; /* Model-only mutation lives inside the replaced memcpy. */
  defaults();mutation=m;if(m==5||m==6)shape=3;actual_case(1);total++;
 }
 defaults();return total;
}
static unsigned actual_faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<6;path++){
  defaults();timer_composition=path%2u;shape=path<2?0:path<4?2:4;
  actual_case(0);unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){actual_case(1);total++;}
 }
 defaults();timer_composition=0;return total;
}
static unsigned service_kind,service_shape;
static uint32_t copy_length=17;
static void service_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 actual_setup(cpu,cached);observing=0;
 cpu->gpr[31]=service_returns[service_kind];cpu->gpr[4]=allocation;cpu->gpr[5]=blob;cpu->gpr[6]=copy_length;
 if(service_kind==1)cpu->gpr[4]=blob+0x400u;
 if(service_shape==1)cpu->gpr[4]=0;
 if(service_shape==2)cpu->gpr[4]=blob+1u;
 if(service_shape==3)cpu->gpr[4]=blob-1u;
 if(service_shape==4)cpu->gpr[4]=0x1e000000u;
 if(service_shape==5)cpu->gpr[5]=0x1e000000u;
 if(service_shape==6)cpu->gpr[29]=0x1e000000u;
 if(service_shape==7)cpu->gpr[4]=0x801ffffcu;
 accesses=call_count=actual_calls=legacy_calls=0;observing=1;
}
static void service_case(unsigned cached)
{
 PcPortMipsCpu original,native;service_setup(&original,cached);candidate=compare=0;original.bus.bridge=NULL;
 int original_rc=PcPortMipsRun(&original,service_targets[service_kind],service_returns[service_kind],200000);
 expected_accesses=accesses;snapshot();
 service_setup(&native,cached);candidate=compare=1;native.bus.bridge=runtime_bridge;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPortMipsRun(&native,service_targets[service_kind],service_returns[service_kind],200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("isolated service status",service_kind);
 if(native.steps||legacy_calls||rt.bridge_cpu||accesses!=expected_accesses||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("isolated service counts",service_kind);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=original.gpr[i])fail("isolated service register",i);
 if(memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("isolated service complete state",service_kind);
}
static unsigned services(void)
{
 unsigned total=0;const uint32_t lengths[]={0,1,17,0x7fffffffu,0x80000000u,0xffffffffu};
 for(service_kind=0;service_kind<2;service_kind++)for(unsigned d=0;d<7;d++)for(service_shape=0;service_shape<8;service_shape++){
  defaults();task_domain=d;copy_length=17;service_case(service_shape%2u);total++;
 }
 service_kind=0;
 for(unsigned n=0;n<6;n++)for(unsigned sh=0;sh<3;sh++){
  defaults();copy_length=lengths[n];service_shape=sh;
  if(n==3&&sh!=1)continue; /* Positive huge length tested through first fault. */
  service_case(1);total++;
 }
 defaults();copy_length=17;return total;
}
static unsigned service_faults(void)
{
 unsigned total=0;
 for(service_kind=0;service_kind<2;service_kind++)for(service_shape=0;service_shape<4;service_shape++){
  defaults();copy_length=17;service_case(0);unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){service_case(1);total++;}
 }
 defaults();return total;
}
static unsigned service_gates(void)
{
 unsigned total=0;timer_composition=0;
 for(service_kind=0;service_kind<2;service_kind++){
  defaults();service_shape=0;PcPortMipsCpu cpu;service_setup(&cpu,1);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t target=service_targets[service_kind];
  XenoRetailFileSlot saved=g_XenoRetailFiles[XENO_RD_SLUS];size_t off=XENO_RD_SLUS_OFF(target);uint8_t *code=saved.bytes+off;
  for(unsigned i=0;i<3;i++){
   unsigned pos=i==0?0:i==1?service_sizes[service_kind]/2u:service_sizes[service_kind]-1u;code[pos]^=1;
   if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service full identity",pos);
   code[pos]^=1;total++;
  }
  g_XenoRetailFiles[XENO_RD_SLUS].state=-1;
  if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service missing executable",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=saved;total++;
  g_XenoRetailFiles[XENO_RD_SLUS].size=off+service_sizes[service_kind]-1u;
  if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service short executable",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=saved;total++;
  cpu.load_valid=1;cpu.load_reg=4;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service pending entry",0);cpu=old;total++;
  rt.force_interpret=1;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service forced entry",0);rt.force_interpret=0;total++;
  rt.initialized=0;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service initialization",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service active runtime",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service bridge context",0);rt.bridge_cpu=&cpu;total++;
  cpu.gpr[31]+=4u;if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service wrong caller",0);cpu=old;total++;
  if(battle_startup_resources_service_try_host(&rt,&cpu,target+4u))fail("service wrong target",0);
  total++;
  assert(setenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES","0",1)==0);if(battle_startup_resources_service_try_host(&rt,&cpu,target))fail("service opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES")==0);total++;
  if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("service gate side effect",0);
  for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
   service_setup(&cpu,cached);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;old=cpu;snapshot();
   if(reject==1)code[0]^=1;
   if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES","0",1)==0);
   if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
   PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
   int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,service_returns[service_kind],200000);
   if(reject==1)code[0]^=1;
   assert(unsetenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES")==0);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
   if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("service canonical cache routing",cached);
   if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("service native precedence",(uint32_t)rc);
   if(reject&&(rc!=(disabled?PC_PORT_MIPS_INTERPRETER_DISABLED:0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("service rejected cache routing",(uint32_t)rc);
   total++;
  }
 }
 for(unsigned which=0;which<2;which++)for(unsigned cached=0;cached<2;cached++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
  uint32_t target=which?0x800bb350u:0x80022a70u;cpu.gpr[31]=which?0x801e65e0u:0x801e6620u;
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"unconverted_startup_child"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  cpu.bus.bridge=runtime_bridge;PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,target,cpu.gpr[31],1000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_INTERPRETER_DISABLED||legacy_calls||cpu.steps||memcmp(old.gpr,cpu.gpr,sizeof(cpu.gpr))||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("unconverted child cache refusal",target);
  total++;
 }
 defaults();return total;
}
int main(int argc,char **argv)
{
 if(argc>1){
  load_inputs();defaults();
  if(!strcmp(argv[1],"--integration-basic")){actual_case(1);return 0;}
  if(!strcmp(argv[1],"--integration")){printf("COMPOSITION PASS %u\n",actual_compositions());return 0;}
  if(!strcmp(argv[1],"--integration-faults")){printf("ACTUAL FAULT PASS %u\n",actual_faults());return 0;}
  if(!strcmp(argv[1],"--services")){printf("SERVICES PASS %u\n",services());return 0;}
  if(!strcmp(argv[1],"--service-faults")){printf("SERVICE FAULT PASS %u\n",service_faults());return 0;}
  if(!strcmp(argv[1],"--service-gates")){printf("SERVICE GATES PASS %u\n",service_gates());return 0;}
  return startup_resources_model_main(argc,argv);
 }
 if(startup_resources_model_main(argc,argv))return 1;
 printf("STARTUP RESOURCES COMPOSITION PASS %u\n",actual_compositions());fflush(stdout);
 printf("STARTUP RESOURCES ACTUAL FAULT PASS %u\n",actual_faults());fflush(stdout);
 printf("STARTUP RESOURCES SERVICES PASS %u\n",services());fflush(stdout);
 printf("STARTUP RESOURCES SERVICE FAULT PASS %u\n",service_faults());fflush(stdout);
 printf("STARTUP RESOURCES SERVICE GATES PASS %u\n",service_gates());fflush(stdout);
 puts("STARTUP RESOURCES INTEGRATION ALL PASS; no ABI normalization; remaining actor/CLUT graphs modeled, not adopted");return 0;
}
