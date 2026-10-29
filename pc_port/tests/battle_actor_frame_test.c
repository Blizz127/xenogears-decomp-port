/* Full frame tick -> actor coordinator -> native color helper and SDK.
 * Other frame services and the ten remaining actor descendants are models. */
#define ACTOR_INTEGRATION_MAIN actor_frame_integration_main
#include "battle_actor_update_integration_test.c"
static unsigned actor_entries;
static int frame_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 if(address-0x800be790u<884u){assert(!candidate&&width==4);*value=load_le(battle+(address-0x8006faf0u),4);return 0;}
 return runtime_read(opaque,address,width,value);
}
static int frame_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==0x800be790u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x800be790u<884u)return 0;
 if(target==targets[0]){
  record_call(cpu,target);actor_entries++;
  if(cpu->gpr[31]!=0x800be8a4u)fail("actual frame actor callsite",cpu->gpr[31]);
  return candidate?runtime_bridge(opaque,cpu,target):0;
 }
 static const uint32_t modeled[]={0x80019ca0u,0x8004b54cu,0x80044ad8u,0x800250e0u,0x800bbab8u,0x800bb9d4u,0x80024ff4u,0x80024fe4u,0x8001d468u,0x8001c9f8u,0x8001c964u,0x80076544u,0x8008a9c0u,0x800445d0u,0x80044e9cu,0x80044c44u,0x80025044u,0x80044bd0u,0x800beb04u};
 unsigned found=0;for(unsigned i=0;i<sizeof(modeled)/sizeof(modeled[0]);i++)if(target==modeled[i])found=1;
 if(!found)return actual_bridge(opaque,cpu,target);
 record_call(cpu,target);uint32_t value=0x43210000u+call_count,site=cpu->gpr[31]-8u;
 if(target==0x8004b54cu)value=site==0x800be7bcu?0x10020u:site==0x800be98cu?0x10023u:0x23456780u+call_count;
 if(runtime_write(opaque,0x80059488u,4,target^call_count))return -1;
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=value;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;gteRegs.CP2D.r[9]=target^call_count;return 1;
}
static void frame_setup(PcPortMipsCpu *cpu,unsigned domain,unsigned cached)
{
 actual_setup(cpu,domain,cached);observing=0;cpu->bus.bridge=frame_bridge;cpu->bus.read=frame_read;
 rt.data[2]=rt.data[1];rt.data[1]=rt.data[0];rt.data[0]=(ResolvedData){0x80010000u,4u,shared+0x1f100u,"frame-sentinel"};rt.data_count=3;
 fixture_store(0x80010000u,4,UINT32_MAX);fixture_store(0x800c37d0u,4,0);
 fixture_store(0x800ccb00u,4,seed&1u?0x800c4a20u:0x800c8a90u);fixture_store(0x800ccb04u,4,0x800c4a90u);fixture_store(0x800ccb34u,4,seed&1u);
 fixture_store(0x8005956cu,4,0x80140000u);fixture_store(0x80059494u,2,seed%4u);fixture_store(0x80059198u,4,seed%2u);
 fixture_store(0x800c3610u,4,0);fixture_store(0x800591b4u,2,0);fixture_store(0x800c37ccu,1,0);
 fixture_store(0x8005917cu,4,0x80180000u);fixture_store(0x80180000u,4,UINT32_MAX);fixture_store(0x80059180u,4,0x12345678u);
 actor_entries=0;observing=1;
}
static void frame_case(unsigned domain,unsigned cached)
{
 PcPortMipsCpu original,native;frame_setup(&original,domain,cached);candidate=compare=0;
 int rc=PcPortMipsRun(&original,0x800be790u,BATTLE_HALT_PC,200000);
 if(rc!=PC_PORT_MIPS_HALTED||actor_entries!=1)fail("original frame composition",original.pc);
 expected_calls=call_count;expected_accesses=accesses;expected_real_calls=real_calls;uint64_t original_sdk_steps=sdk_steps;snapshot();
 frame_setup(&native,domain,cached);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);rc=PcPortMipsRun(&native,0x800be790u,BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||native_gte_commands||rt.bridge_cpu||actor_entries!=1||sdk_steps||!original_sdk_steps||real_calls!=expected_real_calls||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("native frame composition",native.pc);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal()||original.load_valid||native.load_valid)fail("frame full architectural/memory/GTE state",domain);
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--frame-basic")){frame_case(0,0);return 0;}
  return actor_frame_integration_main(argc,argv);
 }
 for(unsigned s=13;s<16;s++){defaults();scenario=s;run_case();}
 puts("ACTOR UPDATE EDGE CASES PASS 3 appended cases: negative rounding, status254 and selective exclusion mask");fflush(stdout);
 unsigned total=0;
 for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();seed=s;stack_kind=s%4u;task_domain=s%6u;scenario=s%4u;frame_case(s%7u,cached);total++;
 }
 printf("ACTOR FRAME PASS %u complete original/native frame -> actor -> color helper/SDK compositions, actual callsite 800BE89C; native interpreted instructions=0; other frame services and ten actor descendants modeled\n",total);return 0;
}
