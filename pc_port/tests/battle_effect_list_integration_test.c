/* Compose original/native anchor and matrix bodies. SDK volatile CPU state
 * is normalized to its typed ABI; full memory/GTE state is compared. */
#define main effect_list_model_main
#include "battle_effect_list_host_test.c"
#undef main
static unsigned real_calls,expected_real_calls,in_sdk,matrix_mode;
static uint64_t sdk_steps;
static int sdk_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 static const struct {uint32_t start,length;} spans[]={{0x80049efcu,48},{0x80049f8cu,32}};
 for(unsigned i=0;i<2;i++)if(address-spans[i].start<spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 return model_test_raw_read(opaque,shard_sdk_address(address),width,value);
}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==targets[kind]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);if(bi>=0&&target!=targets[bi])return 0;
 record_call(cpu,target);
 if(bi>=0)return candidate?runtime_bridge(opaque,cpu,target):0;
 real_calls++;int saved=observing;observing=0;int rc;
 if(candidate)rc=runtime_bridge(opaque,cpu,target);
 else{
  if(target!=0x80049efcu&&target!=0x80049f8cu)fail("unexpected actual SDK",target);
  PcPortMipsCpu original=*cpu;original.bus.read=sdk_read;uint64_t before=original.steps;in_sdk=1;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);in_sdk=0;sdk_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED)fail("original SDK fault",original.pc);
  for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("SDK preserved register",i);
  if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("SDK frame",target);
  /* Typed native services preserve caller volatile CPU registers; normalize
   * only those SDK register effects while retaining all memory/GTE effects. */
  rc=1;
 }
 observing=saved;return rc;
}
static uint32_t matrix_domain(unsigned d)
{
 static const uint32_t tokens[]={0x80160000u,0xa0160000u,0x80360000u,0x00160000u,0x80061000u,0x1f800900u,0,0};
 return d==6?(uint32_t)(uintptr_t)host+0x60000u:tokens[d];
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;matrix=matrix_domain(domain);cpu->gpr[5]=matrix;
 const int16_t identity[9]={4096,0,0,0,4096,0,0,0,4096};
 const int16_t quarter[9]={0,4096,0,-4096,0,0,0,0,4096};
 for(unsigned j=0;j<9;j++){
  uint16_t value=(uint16_t)(matrix_mode==1?quarter[j]:identity[j]);
  if(matrix_mode==2)value=(uint16_t)(seed*173u+j*917u);
  if(matrix_mode==3)value=(uint16_t)(0x7fff8000u+(j&1u));
  fixture_store(matrix+2u*j,2,value);
 }
 fixture_store(matrix+18u,2,0xb7a9u);
 for(unsigned j=0;j<3;j++)fixture_store(matrix+20u+4u*j,4,matrix_mode==3?0x7fffff00u+j*0x200u:matrix_mode==2?0u-21u*j*seed:0);
 rt.functions[0]=(ResolvedFunction){0x80049efcu,(void *)SetRotMatrix,"SetRotMatrix"};
 rt.functions[1]=(ResolvedFunction){0x80049f8cu,(void *)SetTransMatrix,"SetTransMatrix"};rt.function_count=2;
 if(cached)for(unsigned i=0;i<2;i++){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];}
 real_calls=in_sdk=0;sdk_steps=0;observing=1;
}
static void actual_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,domain,cached);candidate=compare=0;oracle_cpu=&original;
 int original_rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,20000);
 if(original_rc!=PC_PORT_MIPS_HALTED)fail("original actual composition",original.pc);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;expected_real_calls=real_calls;unsigned expected_commands=oracle_gte_commands;snapshot();
 actual_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,20000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||sdk_steps||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||events!=expected_events||real_calls!=expected_real_calls||native_gte_commands!=expected_commands||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual SDK composition counts",native.pc);
 if(memcmp(native.gpr,original.gpr,sizeof(native.gpr))||memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("actual SDK complete state",domain);
 if(matrix_mode<2&&projected&&seed==0&&age==0&&count==3){
  for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++){
   int32_t x=8+(int)i*2+(int)j*8,y=8+(int)i*2+(int)j*2;
   int32_t sx=160+(matrix_mode?y:x)/2,sy=120+(matrix_mode?-x:y)/2;
   uint32_t p=records+0x7cu*i+0x2cu+40u*buffer+8u+8u*j;
   if(peek(p,2)!=(uint16_t)sx||peek(p+2u,2)!=(uint16_t)sy)fail("independent actual matrix projection",i*4u+j);
  }
 }
}
static unsigned probe_calls;
static MATRIX *probe_args[3];
static void matrix_probe(MATRIX *a){probe_calls++;probe_args[0]=a;}
static unsigned interfaces(void)
{
 unsigned total=0;
 for(unsigned target=0;target<2;target++)for(unsigned cached=0;cached<2;cached++)for(unsigned shape=0;shape<12;shape++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;observing=1;cpu.bus.bridge=runtime_bridge;
  uint32_t address=(const uint32_t[]){0x80049efcu,0x80049f8cu}[target];
  cpu.gpr[31]=(const uint32_t[]){0x800a3020u,0x800a3028u}[target];cpu.gpr[29]=0x1e000000u;
  cpu.gpr[4]=0x80160000u;cpu.gpr[5]=0xa0160100u;cpu.gpr[6]=0x1f800000u;
  if(shape==1)cpu.gpr[4]=0;
  if(shape==2)cpu.gpr[4]=0x00160000u;
  if(shape==3)cpu.gpr[4]=0x80061000u;
  if(shape==4)cpu.gpr[4]=(uint32_t)(uintptr_t)host+0x1000u;
  if(shape==5){cpu.gpr[4]=(uint32_t)(uintptr_t)host+0x1000u;rt.host_ranges[0].writable=0;}
  if(shape==6)cpu.gpr[4]++;
  if(shape==7)cpu.gpr[4]=0x1e000000u;
  if(shape==8)cpu.gpr[4]=0x1ffff0u;
  if(shape==9){cpu.load_valid=1;cpu.load_reg=4;}
  if(shape==10)cpu.gpr[4]=(uint32_t)(uintptr_t)host+sizeof(host)-16u;
  if(shape==11){cpu.gpr[6]=(uint32_t)(uintptr_t)host+0x1000u;rt.host_ranges[0].writable=0;}
  rt.functions[0]=(ResolvedFunction){address,(void *)matrix_probe,target==0?"SetRotMatrix":"SetTransMatrix"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,address);slot->target=address;slot->state=1;slot->entry=rt.functions[0];}
  probe_calls=0;memset(probe_args,0,sizeof(probe_args));PcPortMipsCpu before=cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  int rejected=(shape>=6&&shape<=10);
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,address,cpu.gpr[31],100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if((rc==PC_PORT_MIPS_HALTED)==rejected||probe_calls!=(unsigned)!rejected||accesses||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||cpu.hi!=before.hi||cpu.lo!=before.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("concrete matrix ABI/no unused stack",target*16u+shape);
  if(!rejected)for(unsigned i=0;i<1u;i++)if(probe_args[i]!=(MATRIX *)module_memory_pointer(&rt,before.gpr[4+i],32,i==2))fail("typed matrix argument",i);
  total++;
 }
 defaults();return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--sdk-interfaces")){printf("EFFECT LIST SDK ABI PASS %u\n",interfaces());return 0;}
  if(!strcmp(argv[1],"--integration-basic")){matrix_mode=0;actual_case(0,0);return 0;}
  if(!strcmp(argv[1],"--integration"))goto integration;
  return effect_list_model_main(argc,argv);
 }
 if(effect_list_model_main(argc,argv))return 1;
 integration:
 printf("EFFECT LIST SDK ABI PASS %u pointer domains, read-only inputs, pending/shape refusal and no unused stack\n",interfaces());fflush(stdout);
 unsigned total=0;
 for(unsigned mode=0;mode<4;mode++)for(unsigned d=0;d<8;d++)for(unsigned scenario=0;scenario<6;scenario++)for(unsigned cached=0;cached<2;cached++){
  defaults();matrix_mode=mode;seed=scenario%3u;task_domain=scenario;stack_kind=scenario%4u;projected=scenario&1u;buffer=scenario&1u;
  if(scenario==4)age=duration;
  if(scenario==5)age=-1;
  actual_case(d,cached);total++;
 }
 for(unsigned mode=0;mode<2;mode++)for(unsigned d=0;d<8;d++){
  defaults();projected=1;matrix_mode=mode;actual_case(d,0);total++;
 }
 defaults();printf("EFFECT LIST ACTUAL SDK PASS %u complete original/native compositions; original/native matrix services, actual release child, independent identity/rotation/projection witnesses, native interpreted instructions=0\n",total);return 0;
}
