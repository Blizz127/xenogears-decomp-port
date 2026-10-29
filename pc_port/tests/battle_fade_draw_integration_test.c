/* Full callback plus original/native GPU packet helpers. No ABI register or
 * guest-stack normalization: compare every observed access and memory byte. */
#define main fade_draw_model_main
#include "battle_fade_draw_host_test.c"
#undef main
static unsigned in_sdk,gpu_version,real_calls,expected_real_calls;
static uint64_t sdk_steps;
static const uint32_t sdk_targets[]={0x80043c9cu,0x80043bfcu,0x80043a1cu,0x800454dcu,0x80043b48u};
static const uint32_t sdk_returns[]={0x800b38bcu,0x800b38c8u,0x800b3924u,0x800b393cu,0x800b3950u};
static const struct {uint32_t start;unsigned length;} sdk_spans[]={
 {0x80043c9cu,20},{0x80043bfcu,40},{0x80043a1cu,60},{0x800454dcu,88},
 {0x80043b48u,60},{0x800459dcu,88},{0x80045c10u,132}};
static int sdk_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 for(unsigned i=0;i<7;i++)if(address-sdk_spans[i].start<sdk_spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 return runtime_read(opaque,address,width,value);
}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==targets[0]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0)return 0;
 /* Deliberate frame aliases can corrupt the callback return address. Stop
  * both sides at that unsupported control-flow boundary, retaining state. */
 if(!battle_fade_draw_sdk_target(target,cpu->gpr[31]))return -1;
 record_call(cpu,target);real_calls++;
 if(candidate)return runtime_bridge(opaque,cpu,target);
 PcPortMipsCpu original=*cpu;original.bus.read=sdk_read;uint64_t before=original.steps;
 in_sdk=1;int rc=PcPortMipsRun(&original,target,cpu->gpr[31],1000);in_sdk=0;sdk_steps+=original.steps-before;
 memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));
 cpu->hi=original.hi;cpu->lo=original.lo;cpu->steps=original.steps;
 cpu->load_valid=original.load_valid;cpu->load_reg=original.load_reg;cpu->load_value=original.load_value;cpu->load_mask=original.load_mask;
 if(rc==PC_PORT_MIPS_BUDGET)fail("original SDK budget",target);
 return rc==PC_PORT_MIPS_HALTED?1:-1;
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;
 fixture_store(0x800568d0u,1,gpu_version);
 for(unsigned i=0;i<5;i++){
  rt.functions[i]=(ResolvedFunction){sdk_targets[i],(void *)legacy_probe,"unusable_gpu_probe"};
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,sdk_targets[i]);slot->target=sdk_targets[i];slot->state=1;slot->entry=rt.functions[i];}
 }
 rt.function_count=5;in_sdk=real_calls=0;sdk_steps=0;observing=1;
}
static void actual_case(unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,cached);candidate=compare=0;oracle_cpu=&original;
 int original_rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,20000);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;expected_real_calls=real_calls;snapshot();
 actual_setup(&native,cached);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,20000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("actual SDK status",native.pc);
 if(native.steps||sdk_steps||legacy_calls||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||events!=expected_events||real_calls!=expected_real_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual SDK counts",native.pc);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=original.gpr[i]){
  fprintf(stderr,"actual register native=%08x original=%08x\n",native.gpr[i],original.gpr[i]);fail("actual SDK register",i);
 }
 if(memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("actual SDK complete state",cached);
 if(fault_at&&accesses!=fault_at)fail("actual fault not reached",accesses);
}
static unsigned actual_semantics(void)
{
 unsigned total=0;
 for(unsigned value=0;value<256;value++){
  defaults();seed=value;gpu_version=value%4u;actual_case(value%2u);
  if(expected_calls!=6||peek(0x80059580u,4)!=buffer+0x24u)fail("actual allocation/calls",value);
  if(peek(buffer,4)!=(0x05000000u|0x112233u)||peek(buffer+0x18u,4)!=(0x02000000u|(buffer&0xffffffu))||peek(ot+8u,4)!=(0x8d000000u|((buffer+0x18u)&0xffffffu)))fail("actual tags/order",value);
  if(peek(buffer+7u,1)!=0x2a||peek(buffer+0x1cu,4)!=(0xe1000000u|((value&3u)<<5))||peek(buffer+0x20u,4)!=0)fail("actual opcode/blend/null window",value);
  for(unsigned i=0;i<3;i++)if(peek(buffer+4u+i,1)!=(uint8_t)(value+i*71u))fail("actual color",i);
  const int16_t xy[]={-32,-32,320,-32,-32,240,320,240};
  for(unsigned i=0;i<8;i++)if(peek(buffer+8u+2u*i,2)!=(uint16_t)xy[i])fail("actual quad corner",i);
  total++;
 }
 return total;
}
static unsigned actual_faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<8;path++){
  defaults();seed=path;gpu_version=path%4u;task_domain=path%6u;stack_kind=path%4u;
  if(path==1)gap=0x50;
  if(path>=2)alias_case=path-1u;
  actual_case(path%2u);unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){actual_case(path%2u);total++;}
 }
 defaults();return total;
}
static unsigned sdk_kind,sdk_shape;
static void service_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 actual_setup(cpu,cached);observing=0;cpu->gpr[31]=sdk_returns[sdk_kind];
 cpu->gpr[4]=buffer;cpu->gpr[5]=sdk_kind==4?ot+8u:seed;
 cpu->gpr[6]=seed^0x12345678u;cpu->gpr[7]=seed^0x8765ffffu;
 if(sdk_kind==3){
  cpu->gpr[5]=seed&1u;cpu->gpr[6]=seed&2u;cpu->gpr[7]=seed*7919u;
  fixture_store(cpu->gpr[29]+0x10u,4,sdk_shape==1?owner:0);
  for(unsigned i=0;i<4;i++)fixture_store(owner+2u*i,2,seed*113u+i*7919u);
 }
 if(sdk_shape==2)cpu->gpr[4]++;
 if(sdk_shape==3)cpu->gpr[4]=0x1e000000u;
 if(sdk_shape==4)cpu->gpr[29]=0x1e000000u;
 if(sdk_shape==5)cpu->gpr[4]=cpu->gpr[29]-0x1cu;
 if(sdk_shape==6)cpu->gpr[4]=0x801ffffcu;
 if(sdk_shape==7)cpu->gpr[4]=cpu->gpr[5];
 if(sdk_shape==8){cpu->gpr[4]=(uint32_t)(uintptr_t)host+0x20000u;rt.host_ranges[0].writable=0;}
 accesses=call_count=real_calls=legacy_calls=0;observing=1;
}
static void service_case(unsigned cached)
{
 PcPortMipsCpu original,native;service_setup(&original,cached);candidate=compare=0;
 original.bus.read=sdk_read;original.bus.bridge=NULL;
 int original_rc=PcPortMipsRun(&original,sdk_targets[sdk_kind],sdk_returns[sdk_kind],1000);
 expected_accesses=accesses;snapshot();
 service_setup(&native,cached);candidate=compare=1;native.bus.bridge=runtime_bridge;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPortMipsRun(&native,sdk_targets[sdk_kind],sdk_returns[sdk_kind],1000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("isolated SDK status",sdk_kind*16u+sdk_shape);
 if(native.steps||legacy_calls||rt.bridge_cpu||accesses!=expected_accesses||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("isolated SDK counts",sdk_kind*16u+sdk_shape);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=original.gpr[i]){
  fprintf(stderr,"SDK %u shape %u register native=%08x original=%08x\n",sdk_kind,sdk_shape,native.gpr[i],original.gpr[i]);fail("isolated SDK register",i);
 }
 if(memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("isolated SDK complete state",sdk_kind*16u+sdk_shape);
}
static unsigned services(void)
{
 unsigned total=0;
 for(sdk_kind=0;sdk_kind<5;sdk_kind++)for(unsigned d=0;d<6;d++)for(unsigned v=0;v<8;v++){
  defaults();seed=v*0x21379u;gpu_version=v%4u;task_domain=d;stack_kind=v%4u;sdk_shape=v%2u;
  service_case(v%2u);total++;
 }
 for(sdk_kind=0;sdk_kind<5;sdk_kind++)for(sdk_shape=2;sdk_shape<9;sdk_shape++){
  defaults();gpu_version=seed=3;service_case(sdk_shape%2u);total++;
 }
 return total;
}
static unsigned service_faults(void)
{
 unsigned total=0;
 for(sdk_kind=0;sdk_kind<5;sdk_kind++)for(sdk_shape=0;sdk_shape<2;sdk_shape++){
  defaults();gpu_version=seed=3;service_case(0);unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){service_case(1);total++;}
 }
 defaults();return total;
}
static unsigned service_gates(void)
{
 unsigned total=0;XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);
 for(sdk_kind=0;sdk_kind<5;sdk_kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned gate=0;gate<11;gate++){
  defaults();sdk_shape=0;PcPortMipsCpu cpu;service_setup(&cpu,cached);observing=0;candidate=1;compare=0;
  rt.bridge_cpu=&cpu;PcPortMipsCpu before=cpu;uint32_t target=sdk_targets[sdk_kind];
  unsigned offset=XENO_RD_SLUS_OFF(target);uint8_t saved=exe->bytes[offset];
  if(gate==0){cpu.load_valid=1;cpu.load_reg=4;}
  if(gate==1)rt.force_interpret=1;
  if(gate==2)rt.initialized=0;
  if(gate==3)g_ActiveBattleRuntime=NULL;
  if(gate==4)rt.bridge_cpu=NULL;
  if(gate==5)cpu.gpr[31]++;
  if(gate==6)assert(setenv("XENO_BATTLE_NATIVE_FADE_DRAW","0",1)==0);
  if(gate>=7){
   unsigned span=0;while(sdk_spans[span].start!=target)span++;
   if(gate==8)offset+=sdk_spans[span].length/2u;
   if(gate==9)offset+=sdk_spans[span].length-1u;
   if(gate==10&&sdk_kind==3)offset=XENO_RD_SLUS_OFF(0x80045c10u)+131u;
   saved=exe->bytes[offset];exe->bytes[offset]^=1;
  }
  before=cpu;snapshot();
  if(battle_fade_draw_sdk_try_host(&rt,&cpu,target)!=0||memcmp(&cpu,&before,sizeof(cpu))||accesses||legacy_calls||!memory_equal())fail("scoped SDK gate side effect",gate);
  if(gate>=7)exe->bytes[offset]=saved;
  assert(unsetenv("XENO_BATTLE_NATIVE_FADE_DRAW")==0);total++;
 }
 /* Invalid identity and opt-out must never fall through a populated native
  * cache entry. Native-only execution refuses before the legacy probe. */
 for(sdk_kind=0;sdk_kind<5;sdk_kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned gate=0;gate<2;gate++){
  defaults();sdk_shape=0;PcPortMipsCpu cpu;service_setup(&cpu,cached);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  uint32_t target=sdk_targets[sdk_kind];unsigned offset=XENO_RD_SLUS_OFF(target);uint8_t saved=exe->bytes[offset];
  if(gate)assert(setenv("XENO_BATTLE_NATIVE_FADE_DRAW","0",1)==0);else exe->bytes[offset]^=1;
  PcPortMipsCpu before=cpu;snapshot();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,target,cpu.gpr[31],100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_FADE_DRAW")==0);exe->bytes[offset]=saved;
  if(rc!=PC_PORT_MIPS_INTERPRETER_DISABLED||cpu.steps||rt.bridge_cpu||legacy_calls||accesses||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||!memory_equal())fail("scoped SDK cache refusal",gate);
  total++;
 }
 defaults();return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();gpu_version=0;
 if(argc>1){
  if(!strcmp(argv[1],"--integration-basic")){actual_case(0);puts("FADE DRAW ACTUAL SDK BASIC PASS");return 0;}
  if(!strcmp(argv[1],"--integration-faults")){printf("FADE DRAW ACTUAL SDK FAULT PASS %u\n",actual_faults());return 0;}
  if(!strcmp(argv[1],"--sdk-services")){printf("FADE DRAW SDK SERVICES PASS %u, FAULTS %u\n",services(),service_faults());return 0;}
  if(!strcmp(argv[1],"--sdk-gates")){printf("FADE DRAW SDK GATES PASS %u\n",service_gates());return 0;}
  if(!strcmp(argv[1],"--integration"))goto integration;
  return fade_draw_model_main(argc,argv);
 }
 if(fade_draw_model_main(argc,argv))return 1;
integration:
 ;unsigned total=0;
 const int32_t spaces[]={-1,0,0x4f,0x50,0x51,0x100};
 for(unsigned d=0;d<7;d++)for(unsigned v=0;v<8;v++)for(unsigned g=0;g<6;g++){
  defaults();task_domain=d;seed=v*37u;gpu_version=v;gap=spaces[g];stack_kind=v%4u;actual_case(v%2u);total++;
 }
 for(unsigned alias=1;alias<=6;alias++){defaults();alias_case=alias;actual_case(alias%2u);total++;}
 for(unsigned bad=1;bad<=10;bad++){defaults();bad_kind=bad;actual_case(bad%2u);total++;}
 for(unsigned st=4;st<7;st++){defaults();stack_kind=st;actual_case(st%2u);total++;}
 printf("FADE DRAW ACTUAL SDK PASS %u original/native compositions, full CPU/GTE/memory and ordered accesses\n",total);fflush(stdout);
 printf("FADE DRAW ACTUAL SDK SEMANTICS PASS %u\n",actual_semantics());fflush(stdout);
 printf("FADE DRAW ACTUAL SDK FAULT PASS %u\n",actual_faults());fflush(stdout);
 printf("FADE DRAW SDK SERVICES PASS %u\n",services());fflush(stdout);
 printf("FADE DRAW SDK SERVICE FAULT PASS %u\n",service_faults());fflush(stdout);
 printf("FADE DRAW SDK GATES PASS %u\n",service_gates());fflush(stdout);
 puts("FADE DRAW INTEGRATION ALL PASS; native interpreted instructions=0");return 0;
}
