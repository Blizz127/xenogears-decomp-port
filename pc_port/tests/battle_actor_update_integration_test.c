/* Execute the new color helper and existing SDK with the complete actor
 * coordinator. Ten remaining overlay descendants are explicit models. */
#define main actor_update_model_main
#include "battle_actor_update_host_test.c"
#undef main
/* The game shim swaps source names; bind the conventional exported cosine,
 * matching initialize_runtime's dlsym("rcos") for retail 8003F8CC. */
#undef rcos
#undef rsin
static unsigned in_sdk,real_calls,expected_real_calls;
static uint64_t sdk_steps;
static int actual_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 static const struct {uint32_t start,length;} spans[]={{0x8003f8ccu,28},{0x80049f5cu,48},{0x8004a0ecu,32}};
 for(unsigned i=0;i<3;i++)if(address-spans[i].start<spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&width==4);
  *value=load_le(exe->bytes+XENO_RD_SLUS_OFF(address),4);return 0;
 }
 if(address-0x800aa514u<80u){assert(width==4);*value=load_le(battle+(address-0x8006faf0u),4);return 0;}
 return model_test_raw_read(opaque,shard_sdk_address(address),width,value);
}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==targets[0]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0)return 0;
 record_call(cpu,target);
 int actual=target==0x800aa514u||target==0x8003f8ccu||target==0x80049f5cu||target==0x8004a0ecu;
 if(!actual)return service(opaque,cpu,target);
 real_calls++;int saved=observing;observing=0;int rc;
 if(candidate)rc=runtime_bridge(opaque,cpu,target);
 else{
  PcPortMipsCpu original=*cpu;original.bus.read=actual_read;uint64_t before=original.steps;in_sdk=1;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);in_sdk=0;sdk_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED||original.load_valid)fail("actual original helper/SDK",original.pc);
  if(target==0x800aa514u){memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));cpu->hi=original.hi;cpu->lo=original.lo;}
  else{
   for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("SDK callee-saved register",i);
   if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31])fail("SDK frame/return",target);
   /* Existing typed SDKs preserve caller CPU state, returning only V0 from
    * cosine. Actual GTE and memory effects are retained without normalization. */
   if(target==0x8003f8ccu)cpu->gpr[2]=original.gpr[2];
  }
  rc=1;
 }
 observing=saved;return rc;
}
static uint32_t matrix_token(unsigned d)
{
 static const uint32_t tokens[]={0x80160000u,0xa0160000u,0x80360000u,0x00160000u,0x80061000u,0x1f800c00u,0};
 return d==6?(uint32_t)(uintptr_t)(host+0x60000u):tokens[d];
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);
 /* Restore private trig data into its established lower-data owner. SDK
  * executable text remains absent; the oracle fetches it through actual_read. */
 for(unsigned off=0;off<16384u;off+=4u)fixture_store(0x800523f0u+off,4,load_le(exe->bytes+XENO_RD_SLUS_OFF(0x800523f0u)+off,4));
 uint32_t matrix=matrix_token(domain);fixture_store(0x800d2fc0u,4,matrix);
 for(unsigned i=0;i<8;i++)fixture_store(matrix+4u*i,4,0x721b9801u+seed*0x9875u+i*0x21754u);
 rt.functions[0]=(ResolvedFunction){0x8003f8ccu,(void *)rcos,"rsin"};
 rt.functions[1]=(ResolvedFunction){0x80049f5cu,(void *)SetColorMatrix,"SetColorMatrix"};
 rt.functions[2]=(ResolvedFunction){0x8004a0ecu,(void *)SetBackColor,"SetBackColor"};rt.function_count=3;
 if(cached)for(unsigned i=0;i<3;i++){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];}
 for(unsigned i=0;i<3;i++)memset(PSX_ADDR(rt.functions[i].address),0,i==0?28:i==1?48:32);
 in_sdk=real_calls=0;sdk_steps=0;observing=1;
}
static void actual_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,domain,cached);candidate=compare=0;
 int rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);
 if(rc!=PC_PORT_MIPS_HALTED)fail("original composition",original.pc);
 expected_calls=call_count;expected_accesses=accesses;expected_real_calls=real_calls;uint64_t original_sdk_steps=sdk_steps;snapshot();
 actual_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||native_gte_commands||rt.bridge_cpu||sdk_steps||!original_sdk_steps||real_calls!=expected_real_calls||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual helper/SDK composition",native.pc);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal())fail("actual composition full state",domain);
}
static unsigned back_calls;
static uint32_t back_values[3];
static void back_probe(int r,int g,int b)
{back_calls++;back_values[0]=(uint32_t)r;back_values[1]=(uint32_t)g;back_values[2]=(uint32_t)b;}
static unsigned back_interfaces(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned value=0;value<9;value++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);cpu.bus.bridge=runtime_bridge;observing=1;candidate=1;compare=0;
  rt.functions[0]=(ResolvedFunction){0x8004a0ecu,(void *)back_probe,"SetBackColor"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8004a0ecu);slot->target=0x8004a0ecu;slot->state=1;slot->entry=rt.functions[0];}
  cpu.gpr[29]=0x1e000000u;cpu.gpr[4]=value&1u?255:0;cpu.gpr[5]=value&2u?255:0;cpu.gpr[6]=value&4u?255:0;
  if(value==8){cpu.load_valid=1;cpu.load_reg=4;}
  back_calls=0;PcPortMipsCpu before=cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,0x8004a0ecu,BATTLE_HALT_PC,100);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc!=(value==8?PC_PORT_MIPS_UNRESOLVED_CALL:PC_PORT_MIPS_HALTED)||back_calls!=(value!=8)||accesses||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||cpu.hi!=before.hi||cpu.lo!=before.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("background color concrete ABI",value);
  if(value!=8&&memcmp(back_values,&before.gpr[4],sizeof(back_values)))fail("background color scalar arguments",value);
  total++;
 }
 defaults();return total;
}
#ifndef ACTOR_INTEGRATION_MAIN
#define ACTOR_INTEGRATION_MAIN main
#endif
int ACTOR_INTEGRATION_MAIN(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--integration-basic")){actual_case(0,0);return 0;}
  if(!strcmp(argv[1],"--back-interface")){printf("ACTOR BACK COLOR ABI PASS %u\n",back_interfaces());return 0;}
  if(!strcmp(argv[1],"--integration"))goto integration;
  return actor_update_model_main(argc,argv);
 }
 if(actor_update_model_main(argc,argv))return 1;
 integration:
 printf("ACTOR BACK COLOR ABI PASS %u byte-RGB corners and pending entries, cached/resolved, no stack reads, void return\n",back_interfaces());fflush(stdout);
 unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();seed=s;task_domain=s%6u;stack_kind=s%4u;scenario=s%4u;actual_case(d,cached);total++;
 }
 printf("ACTOR UPDATE ACTUAL SDK PASS %u complete compositions; AA514 full architecture, cosine/color matrix/background actual original/native SDK, ten overlay descendants modeled; native interpreted instructions=0\n",total);return 0;
}
