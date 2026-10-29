/* Compose original/native anchor and matrix bodies. SDK volatile CPU state
 * is normalized to its typed ABI; full memory/GTE state is compared. */
#define main anchor_update_model_main
#include "battle_anchor_update_host_test.c"
#undef main
static unsigned real_calls,expected_real_calls,in_sdk,overflow_refusals;
static unsigned matrix_mode,last_overflow;
static uint64_t sdk_steps;
static int sdk_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 static const struct {uint32_t start,length;} spans[]={{0x8004931cu,352},{0x80049efcu,48},{0x80049f8cu,32}};
 for(unsigned i=0;i<3;i++)if(address-spans[i].start<spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 return model_test_raw_read(opaque,shard_sdk_address(address),width,value);
}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==targets[0]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0)return 0;
 record_call(cpu,target);real_calls++;int saved=observing;observing=0;int rc;
 if(candidate)rc=runtime_bridge(opaque,cpu,target);
 else{
  if(target!=0x8004931cu&&target!=0x80049efcu&&target!=0x80049f8cu)fail("unexpected actual SDK",target);
  PcPortMipsCpu original=*cpu;original.bus.read=sdk_read;uint64_t before=original.steps;in_sdk=1;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);in_sdk=0;sdk_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED){
   if(target!=0x8004931cu||!strstr(original.error,"add overflow"))fail("original SDK fault",original.pc);
   overflow_refusals++;rc=-1;
  }else{
   for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("SDK preserved register",i);
   if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("SDK frame",target);
   if(target==0x8004931cu)cpu->gpr[2]=original.gpr[2];
   rc=1;
  }
  /* SDK volatile CPU state is normalized to its typed C ABI on success or
   * refusal. Actual memory/GTE prefix effects are retained byte for byte. */
 }
 observing=saved;return rc;
}
static uint32_t matrix_domain(unsigned d,unsigned i)
{
 static const uint32_t tokens[]={0x80160000u,0xa0160000u,0x80360000u,0x00160000u,0x80061000u,0x1f800500u,0,0};
 return d==6?(uint32_t)(uintptr_t)host+0x60000u+i*0x200u:tokens[d]+i*0x200u;
}
static void fill_matrix(uint32_t address,unsigned i,int bone)
{
 const int16_t identity[9]={4096,0,0,0,4096,0,0,0,4096};
 const int16_t quarter[9]={0,4096,0,-4096,0,0,0,0,4096};
 for(unsigned j=0;j<9;j++){
  uint16_t value=(uint16_t)(matrix_mode==1&&!bone?quarter[j]:identity[j]);
  if(matrix_mode==2)value=(uint16_t)(seed*173u+j*917u+i*711u+bone*1193u);
  if(matrix_mode==3)value=(uint16_t)(0x7fff8000u+(j&1u));
  fixture_store(address+2u*j,2,value);
 }
 fixture_store(address+18u,2,0xb7a9u);
 int32_t translation[3]={bone?10:100+(int)i*13,bone?-20:-200,bone?30:300};
 for(unsigned j=0;j<3;j++){
  uint32_t value=matrix_mode==3?0x7fffff00u+j*0x200u:(uint32_t)translation[j];
  if(matrix_mode>=4){
   unsigned axis=(matrix_mode-4u)/2u,negative=(matrix_mode-4u)%2u;
   value=j!=axis?0u:bone?(negative?0xffffffffu:1u):negative?0x80000000u+seed:0x7fffffffu-seed;
  }
  fixture_store(address+20u+4u*j,4,value);
 }
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;
 for(unsigned i=0;i<2;i++){
  bodies[i]=matrix_domain(domain,i);fixture_store(model[i]+4u,4,bodies[i]);
  fill_matrix(bodies[i]+12u,i,0);fill_matrix(bodies[i]+0xa8u+(uint32_t)bones[i]*0x7cu,i,1);
 }
 rt.functions[0]=(ResolvedFunction){0x8004931cu,(void *)CompMatrix,"CompMatrix"};
 rt.functions[1]=(ResolvedFunction){0x80049efcu,(void *)SetRotMatrix,"SetRotMatrix"};
 rt.functions[2]=(ResolvedFunction){0x80049f8cu,(void *)SetTransMatrix,"SetTransMatrix"};rt.function_count=3;
 if(cached)for(unsigned i=0;i<3;i++){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];}
 real_calls=in_sdk=overflow_refusals=0;sdk_steps=0;observing=1;
}
static void actual_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,domain,cached);candidate=compare=0;oracle_cpu=&original;
 int original_rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,20000);
 if(original_rc!=PC_PORT_MIPS_HALTED&&!overflow_refusals)fail("original actual composition",original.pc);
 unsigned expected_overflows=overflow_refusals;last_overflow=expected_overflows;
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;expected_real_calls=real_calls;unsigned expected_commands=oracle_gte_commands;snapshot();
 actual_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,20000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if((rc==PC_PORT_MIPS_HALTED)!=(original_rc==PC_PORT_MIPS_HALTED)||native.steps||sdk_steps||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||events!=expected_events||real_calls!=expected_real_calls||native_gte_commands!=expected_commands||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual SDK composition counts",native.pc);
 if(memcmp(native.gpr,original.gpr,sizeof(native.gpr))||memcmp(native.cp0,original.cp0,sizeof(native.cp0))||native.hi!=original.hi||native.lo!=original.lo||!memory_equal())fail("actual SDK complete state",domain);
 if(expected_overflows&&rc>=0)fail("native overflow refusal",domain);
 if(matrix_mode<2&&!alias_case&&!bad_kind&&!mutation){
  for(unsigned i=0;i<2;i++)if(active_mask&(1u<<i)){
   int32_t x=(int16_t)(5u+i*7u+seed*431u),y=(int16_t)(0xfffffff9u-i*3u-seed*713u),z=(int16_t)(9u+i*11u+seed*2311u);
   int32_t expected[3]={x+110+(int)i*13,y-220,z+330};
   if(matrix_mode==1){expected[0]=y+80+(int)i*13;expected[1]=-x-210;}
   if(slots[i]<0){expected[0]=x;expected[1]=y;expected[2]=z;}
   for(unsigned j=0;j<3;j++)if(peek(0x800d3304u+i*0x14u+2u*j,2)!=(uint16_t)expected[j])fail("independent actual matrix/point witness",i*3u+j);
  }
 }
}
static unsigned probe_calls,probe_wrong_return;
static MATRIX *probe_args[3];
static MATRIX *compose_probe(MATRIX *a,MATRIX *b,MATRIX *c)
{probe_calls++;probe_args[0]=a;probe_args[1]=b;probe_args[2]=c;return probe_wrong_return?a:c;}
static void matrix_probe(MATRIX *a){probe_calls++;probe_args[0]=a;}
static unsigned interfaces(void)
{
 unsigned total=0;
 for(unsigned target=0;target<3;target++)for(unsigned cached=0;cached<2;cached++)for(unsigned shape=0;shape<12;shape++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;observing=1;cpu.bus.bridge=runtime_bridge;
  uint32_t address=(const uint32_t[]){0x8004931cu,0x80049efcu,0x80049f8cu}[target];
  cpu.gpr[31]=(const uint32_t[]){0x800a456cu,0x800a4574u,0x800a457cu}[target];cpu.gpr[29]=0x1e000000u;
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
  rt.functions[0]=(ResolvedFunction){address,target==0?(void *)compose_probe:(void *)matrix_probe,target==0?"CompMatrix":target==1?"SetRotMatrix":"SetTransMatrix"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,address);slot->target=address;slot->state=1;slot->entry=rt.functions[0];}
  probe_calls=probe_wrong_return=0;memset(probe_args,0,sizeof(probe_args));PcPortMipsCpu before=cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  int rejected=(shape>=6&&shape<=10)||(shape==11&&target==0);
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,address,cpu.gpr[31],100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(!rejected&&target==0)before.gpr[2]=before.gpr[6];
  if((rc==PC_PORT_MIPS_HALTED)==rejected||probe_calls!=(unsigned)!rejected||accesses||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||cpu.hi!=before.hi||cpu.lo!=before.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("concrete matrix ABI/no unused stack",target*16u+shape);
  if(!rejected)for(unsigned i=0;i<(target==0?3u:1u);i++)if(probe_args[i]!=(MATRIX *)module_memory_pointer(&rt,before.gpr[4+i],32,i==2))fail("typed matrix argument",i);
  total++;
 }
 defaults();return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--sdk-interfaces")){printf("ANCHOR SDK ABI PASS %u\n",interfaces());return 0;}
  if(!strcmp(argv[1],"--integration-basic")){matrix_mode=0;actual_case(0,0);return 0;}
  if(!strcmp(argv[1],"--integration"))goto integration;
  return anchor_update_model_main(argc,argv);
 }
 if(anchor_update_model_main(argc,argv))return 1;
 integration:
 printf("ANCHOR SDK ABI PASS %u pointer domains, read-only inputs, pending/shape refusal and no unused stack\n",interfaces());fflush(stdout);
 unsigned total=0;
 for(unsigned mode=0;mode<4;mode++)for(unsigned d=0;d<8;d++)for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();matrix_mode=mode;seed=s;task_domain=s%6u;stack_kind=s%4u;active_mask=s%4u;
  bones[0]=(int)(s%4u)-2;bones[1]=(int)(s%3u)-1;if(s==6)slots[0]=-1;
  if(d==7&&bones[0]<-1)bones[0]=-1;
  actual_case(d,cached);total++;
 }
 unsigned edges=0;
 for(unsigned mode=4;mode<10;mode++)for(unsigned s=0;s<3;s++)for(unsigned d=0;d<2;d++)for(unsigned cached=0;cached<2;cached++){
  defaults();matrix_mode=mode;seed=s;active_mask=3;actual_case(d?6:0,cached);
  if(last_overflow!=(unsigned)(s==0))fail("independent signed ADD overflow boundary",mode);
  edges++;
 }
 printf("ANCHOR SDK SIGNED LIMIT PASS %u positive/negative axis limits, exact non-overflow edges and retained fault effects\n",edges);
 defaults();printf("ANCHOR ACTUAL SDK PASS %u complete original/native compositions; original/native matrix SDK bodies and overflow refusal, independent identity/rotation/translation witnesses, native interpreted instructions=0\n",total);return 0;
}
