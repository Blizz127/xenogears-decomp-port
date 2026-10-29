/* Complete original actor-record initializer comparisons.
 * The runner only renames the production bus definitions so these wrappers
 * can observe accesses and inject faults; production bodies stay unchanged. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the test runner to produce the bus-observed runtime snapshot"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
int g_GPUDisabledState;unsigned g_PcPortPresentedFrames;
static uint32_t cop_data[32],cop_control[32];
unsigned int MFC2(int r){return cop_data[r&31];}
unsigned int CFC2(int r){return cop_control[r&31];}
void MTC2(unsigned int v,int r){cop_data[r&31]=v;}
void CTC2(unsigned int v,int r){cop_control[r&31]=v;}
int doCOP2(int op){(void)op;abort();}
void ControllerPushState(void){}
void ControllerPoll(void){}
void PsyX_UpdateInput(void){}
int PsyX_Sys_GetVBlankCount(void){return 0;}
int PcPort_QuickCheckpointFieldIsActive(void){return 0;}
char PsyX_BeginScene(void){abort();}
void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();}
void ParsePrimitivesLinkedList(u_long *p,int s){(void)p;(void)s;abort();}
static BattleMipsRuntime rt;
static uint8_t body[128];
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned count,source_domain,output_domain,stack_kind,seed,mutation,alias_kind;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,count_reads,mutation_seen;
static unsigned stop_mode,pending_return;
static int candidate,compare,failed,observing;
static uint32_t actor,buffer;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[2];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"ACTOR RECORDS FAIL count=%u source=%u output=%u stack=%u seed=%u mutation=%u alias=%u stop=%u pending=%u fault=%u access=%u call=%u %s %08x\n",count,source_domain,output_domain,stack_kind,seed,mutation,alias_kind,stop_mode,pending_return,fault_at,accesses,call_count,what,value);
 failed=1;
}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{assert(model_test_raw_write(&rt,a,w,v)==0);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&address-0x800aa6e0u<sizeof(body)){
  assert(!candidate&&width==4);*value=load_le(body+(address-0x800aa6e0u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 if(!write&&address==actor+0x10cu){
  count_reads++;
  if(mutation==4&&count_reads==3){fixture_store(address,1,0);mutation_seen++;}
  if(mutation==5&&count_reads==3){fixture_store(address,1,5);mutation_seen++;}
  if(mutation==6&&count_reads==4){fixture_store(address,1,1);mutation_seen++;}
 }
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 Access row={address,width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{return observed_access(opaque,address,width,value,0);}
static int runtime_write(void *opaque,uint32_t address,unsigned width,uint32_t value)
{return observed_access(opaque,address,width,&value,1);}
static void snapshot(void)
{memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static uint32_t domain(unsigned kind,unsigned off)
{
 switch(kind){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x800c0000u+off;default:return 0x80100000u+off;}
}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<2);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("allocator registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x800aa6e0u&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x800aa6e0u<sizeof(body))return 0;
 if(target!=0x80031bdcu){fail("unexpected callee",target);return -1;}
 record_call(cpu,target);
 if(cpu->gpr[4]!=count*0x70u||cpu->gpr[5]||cpu->gpr[31]!=0x800aa710u)fail("allocator ABI",cpu->gpr[4]);
 if(stop_mode==1)return -1;
 if(mutation>=1&&mutation<=3){
  fixture_store(actor+0x10cu,1,mutation==1?0:mutation==2?255:1);mutation_seen++;
 }
 /* Model the service boundary, including caller-visible volatile state.
  * Actual heap allocation/free policy is independently validated. */
 cpu->gpr[2]=buffer;cpu->gpr[3]=0x92345678u;
 for(unsigned i=4;i<16;i++)cpu->gpr[i]=0x76540000u+seed*32u+i;
 cpu->gpr[24]=0x87654321u;cpu->gpr[25]=0x34567890u;
 cpu->hi=0x12345678u;cpu->lo=0x89abcdefu;cpu->cp0[3]=0x35671289u;
 if(stop_mode==2){fixture_store(actor+0x100u,4,0x45678901u);return -1;}
 if(pending_return){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x800aa6e0u),body,sizeof(body));
 rt.data[0]=(ResolvedData){0x800c0000u,sizeof(shared),shared,"records"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(stack_kind==1)cpu->gpr[29]=0x1f800fe0u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 actor=domain(source_domain,0x200u);buffer=domain(output_domain,0x800u);
 fixture_store(actor+0x10cu,1,count);fixture_store(actor+0x110u,4,0x12345678u);fixture_store(actor+0x114u,4,0xabcdef01u);
 cpu->gpr[4]=actor;
 if(alias_kind==1)buffer=actor+0x10cu;
 if(alias_kind==2)buffer=actor+0x108u;
 if(alias_kind==3)buffer=actor+0x104u;
 if(alias_kind==4)buffer=cpu->gpr[29]-8u;
 if(alias_kind==5)buffer=0;
 if(alias_kind==6)buffer++;
 if(alias_kind==7)buffer=0x1f800ffcu;
 if(alias_kind==8)cpu->gpr[4]=0x1e000000u;
 if(alias_kind==9)cpu->gpr[29]=0x1e000000u;
 if(alias_kind==10)cpu->gpr[29]++;
 accesses=call_count=count_reads=mutation_seen=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,0x800aa6e0u,BATTLE_HALT_PC,1000000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,0x800aa6e0u,BATTLE_HALT_PC,1000000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET){fail("oracle budget",oracle.pc);return 0;}
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_mode&&!alias_kind&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("register",i);fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return !failed;
}
static int controls(void)
{
 PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
 PcPortMipsCpu original=cpu;
 for(unsigned i=0;i<3;i++){
  unsigned off=i==0?0:i==1?64:127;uint8_t *p=PSX_ADDR(0x800aa6e0u+off);*p^=1;
  if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("identity accepted",off);
  *p^=1;
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("pending entry",0);cpu=original;
 rt.force_interpret=1;if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("forced entry",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("uninitialized entry",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("wrong context",0);rt.bridge_cpu=&cpu;
 if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e4u))fail("wrong target",0);
 assert(setenv("XENO_BATTLE_NATIVE_ACTOR_RECORDS","0",1)==0);
 if(battle_actor_records_try_host(&rt,&cpu,0x800aa6e0u))fail("opt out",0);
 assert(unsetenv("XENO_BATTLE_NATIVE_ACTOR_RECORDS")==0);
 if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 puts("ACTOR RECORDS PASS full identity, active context, pending entry and opt-out gates");return !failed;
}
static int pending(void)
{
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,0x800aa6e0u,BATTLE_HALT_PC,1000000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);pending_return=0;observing=0;
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||accesses!=3)fail("pending return refusal",(uint32_t)rc);
 uint32_t pointer=0;assert(model_test_raw_read(&rt,actor+0x110u,4,&pointer)==0);
 if(pointer!=0x12345678u||cpu.gpr[2]!=buffer||cpu.gpr[4]!=0x76540004u+seed*32u)fail("pending partial state",pointer);
 puts("ACTOR RECORDS PASS pending allocator return refuses before caller resumes");return !failed;
}
static int faults(void)
{
 unsigned total=0;const unsigned counts[]={0,1,3,17};
 source_domain=output_domain=stack_kind=alias_kind=mutation=stop_mode=0;
 for(unsigned c=0;c<4;c++){
  count=counts[c];fault_at=0;if(!run_case())return 0;unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){if(!run_case())return 0;total++;}
  fault_at=0;
 }
 printf("ACTOR RECORDS PASS %u injected access faults, including delayed epilogue loads\n",total);return !failed;
}
static unsigned legacy_calls;
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 count=0;source_domain=output_domain=stack_kind=alias_kind=mutation=stop_mode=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
  rt.functions[0]=(ResolvedFunction){0x800aa6e0u,legacy_probe,"func_800AA6E0"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x800aa6e0u);slot->target=0x800aa6e0u;slot->state=1;slot->entry=rt.functions[0];}
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x800aa6e0u,BATTLE_HALT_PC,1000000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_HALTED||legacy_calls||cpu.steps||cpu.gpr[2]!=0||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
 }
 puts("ACTOR RECORDS PASS native dispatch precedes resolved/cached legacy functions with CPU allowed/disabled");return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fseek(f,0x3abf0,SEEK_SET)==0);assert(fread(body,1,sizeof(body),f)==sizeof(body));assert(fclose(f)==0);
 uint8_t digest[32];assert(SHA256(body,sizeof(body),digest));char text[65];
 for(unsigned i=0;i<32;i++)snprintf(text+i*2,3,"%02x",digest[i]);
 assert(strcmp(text,"0764da3a5f31586321a67a62de035066a3c1694ce28e81ecec92308e09dc580d")==0);
 count=3;
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--partial")==0){stop_mode=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--alias")==0){alias_kind=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--zero")==0){count=0;return run_case()?0:1;}
  return 2;
 }
 unsigned total=0;output_domain=2;
 for(source_domain=0;source_domain<6;source_domain++)for(count=0;count<256;count++){
  seed=total++;if(!run_case())return 1;
 }
 const unsigned counts[]={0,1,2,3,7,17};
 for(source_domain=0;source_domain<6;source_domain++)for(output_domain=0;output_domain<6;output_domain++)for(stack_kind=0;stack_kind<4;stack_kind++)for(unsigned c=0;c<6;c++){
  count=counts[c];seed=total++;if(!run_case())return 1;
 }
 printf("ACTOR RECORDS PASS %u full comparisons, all 256 counts, six source/output domains and four stack domains\n",total);fflush(stdout);
 source_domain=output_domain=stack_kind=0;count=3;
 for(mutation=1;mutation<=6;mutation++)if(!run_case())return 1;
 mutation=0;
 for(alias_kind=1;alias_kind<=10;alias_kind++)if(!run_case())return 1;
 alias_kind=0;
 for(stop_mode=1;stop_mode<=2;stop_mode++)if(!run_case())return 1;
 stop_mode=0;
 puts("ACTOR RECORDS PASS six live count mutations, ten aliases/malformed pointers and two missing/partial allocator refusals");fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 return 0;
}
