/* Complete original motion queue/restart comparisons.
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
static uint8_t body[236];
static uint8_t shared[0x10000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned count,actor_domain,source_domain,table_domain,stack_kind,seed,mutation,alias_kind;
static uint32_t slot,id_argument,actor,source,table_entry,node,selected;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,count_reads,mutation_seen;
static unsigned stop_mode,pending_return,legacy_calls;
static int candidate,compare,failed,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[128];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[2];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"MOTION QUEUE FAIL count=%u actor=%u source=%u table=%u stack=%u seed=%u slot=%08x mutation=%u alias=%u stop=%u pending=%u fault=%u access=%u call=%u %s %08x\n",count,actor_domain,source_domain,table_domain,stack_kind,seed,slot,mutation,alias_kind,stop_mode,pending_return,fault_at,accesses,call_count,what,value);
 failed=1;exit(1);
}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,a,w,v)!=0)fail("fixture write",a);}
static uint32_t fixture_load(uint32_t a,unsigned w)
{uint32_t v=0;if(model_test_raw_read(&rt,a,w,&v)!=0)fail("fixture read",a);return v;}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&address-0x800aa934u<sizeof(body)){
  assert(!candidate&&width==4);*value=load_le(body+(address-0x800aa934u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 if(!write&&address==actor+0x2bu&&width==1){
  count_reads++;
  if(count_reads==2&&mutation>=1&&mutation<=3){fixture_store(address,1,mutation==1?0:mutation==2?255:3);mutation_seen++;}
  if(count_reads==3&&mutation==4){fixture_store(address,1,0);mutation_seen++;}
  if(count_reads==4&&mutation==5){fixture_store(address,1,1);mutation_seen++;}
 }
 if(!write&&address==source+0x20u&&width==1&&mutation==6){fixture_store(actor+0x2bu,1,4);mutation_seen++;}
 if(write&&address==actor+0x10u&&width==4&&mutation==7){fixture_store(0x800c3e30u,2,0xbabe);mutation_seen++;}
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
static uint32_t domain(unsigned which,unsigned off)
{
 switch(which){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x80050000u+off;default:return 0x80100000u+off;}
}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<2);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("callee registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x800aa934u&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x800aa934u<sizeof(body))return 0;
 if(target!=0x800aad54u){fail("unexpected callee",target);return -1;}
 record_call(cpu,target);
 if(cpu->gpr[4]!=actor||cpu->gpr[5]!=id_argument||cpu->gpr[6]!=UINT32_MAX||cpu->gpr[7]!=1||cpu->gpr[31]!=0x800aaa10u||fixture_load(cpu->gpr[29]+0x10u,4)!=0)fail("motion updater ABI",cpu->gpr[4]);
 if(stop_mode==1)return -1;
 /* Deliberately visible callee effects; this fixture validates the complete
  * parent body, not the updater's independent motion-script semantics. */
 fixture_store(actor+0x114u,4,0x87654321u);fixture_store(0x800c3e30u,2,0x1357);
 cpu->gpr[2]=0x89abcdefu;cpu->gpr[3]=0x92345678u;
 for(unsigned i=4;i<16;i++)cpu->gpr[i]=0x76540000u+seed*32u+i;
 cpu->gpr[24]=0x87654321u;cpu->gpr[25]=0x34567890u;
 cpu->hi=0x12345678u;cpu->lo=0x89abcdefu;cpu->cp0[3]=0x35671289u;
 if(mutation==8){cpu->gpr[29]+=0x80u;fixture_store(cpu->gpr[29]+0x18u,4,BATTLE_HALT_PC);mutation_seen++;}
 if(stop_mode==2)return -1;
 if(pending_return){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x800aa934u),body,sizeof(body));
 rt.data[0]=(ResolvedData){0x80050000u,sizeof(shared),shared,"motion-shared"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(stack_kind==1)cpu->gpr[29]=0x1f800fe0u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 actor=domain(actor_domain,0x100u);source=domain(source_domain,0x400u);node=domain(table_domain,0x700u);table_entry=domain(table_domain,0x800u);
 id_argument=seed&1?0x800c3d0cu:0xfedc1234u;selected=0x80123400u+seed*4u;
 if(alias_kind==1)source=actor;
 if(alias_kind==2)source=actor+0xbu;
 if(alias_kind==3)actor=0x800c3e30u-0x40u;
 if(alias_kind==4)actor=0x800c3e30u-0x50u;
 if(alias_kind==5){actor=cpu->gpr[29]-0x18u;selected=BATTLE_HALT_PC;}
 if(alias_kind==6)table_entry=actor+0x10u;
 if(alias_kind==7)table_entry=actor+0x50u;
 if(alias_kind==8)node=actor+8u;
 fixture_store(source+8u,4,table_entry-(slot<<2));fixture_store(source+0xcu,4,node);
 fixture_store(node+4u,4,table_entry-(slot<<2)+0x138u);fixture_store(table_entry,4,selected);
 fixture_store(source+0x20u,1,0x90u+(seed&0x7fu));fixture_store(actor+0x2bu,1,count);
 fixture_store(0x800c3e30u,2,0xbeefu+seed);
 cpu->gpr[4]=actor;cpu->gpr[5]=source;cpu->gpr[6]=id_argument;cpu->gpr[7]=slot;
 if(alias_kind==9)cpu->gpr[4]=0;
 if(alias_kind==10)cpu->gpr[5]=0;
 if(alias_kind==11)cpu->gpr[4]=0x1e000000u;
 if(alias_kind==12)cpu->gpr[5]=0x1e000000u;
 if(alias_kind==13)cpu->gpr[29]=0x1e000000u;
 if(alias_kind==14)cpu->gpr[29]++;
 if(alias_kind==15){fixture_store(actor+0x2cu,1,count);cpu->gpr[4]++;}
 if(alias_kind==16)fixture_store(source+8u,4,table_entry-(slot<<2)+1u);
 if(alias_kind==17)fixture_store(source+8u,4,0x1f801000u-(slot<<2));
 if(alias_kind==18)fixture_store(source+8u,4,0x00100100u-(slot<<2));
 if(alias_kind==19)fixture_store(source+0xcu,4,0);
 if(alias_kind==20)fixture_store(source+0xcu,4,node+1u);
 accesses=call_count=count_reads=mutation_seen=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,0x800aa934u,BATTLE_HALT_PC,100000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,0x800aa934u,BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_mode&&!alias_kind&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
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
  unsigned off=i==0?0:i==1?116:235;uint8_t *p=PSX_ADDR(0x800aa934u+off);*p^=1;
  if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("identity accepted",off);
  *p^=1;
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("pending entry",0);cpu=original;
 rt.force_interpret=1;if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("forced entry",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("uninitialized entry",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("wrong context",0);rt.bridge_cpu=&cpu;
 if(battle_motion_queue_try_host(&rt,&cpu,0x800aa938u))fail("wrong target",0);
 assert(setenv("XENO_BATTLE_NATIVE_MOTION_QUEUE","0",1)==0);
 if(battle_motion_queue_try_host(&rt,&cpu,0x800aa934u))fail("opt out",0);
 assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_QUEUE")==0);
 if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 puts("MOTION QUEUE PASS full identity, context, pending entry and opt-out gates");return !failed;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 count=4;actor_domain=source_domain=table_domain=stack_kind=alias_kind=mutation=stop_mode=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
  rt.functions[0]=(ResolvedFunction){0x800aa934u,legacy_probe,"func_800AA934"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *p=bridge_cache_slot(&rt,0x800aa934u);p->target=0x800aa934u;p->state=1;p->entry=rt.functions[0];}
  if(reject==1)((uint8_t *)PSX_ADDR(0x800aa934u))[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MOTION_QUEUE","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,0x800aa934u):PcPortMipsRun(&cpu,0x800aa934u,BATTLE_HALT_PC,100000);
  if(reject==1){((uint8_t *)PSX_ADDR(0x800aa934u))[0]^=1;want_ram[0xaa934u]^=1;}
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_QUEUE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject&&((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected dispatch falls through",(uint32_t)rc);
 }
 puts("MOTION QUEUE PASS 12 resolved/cached native-precedence and rejected-dispatch checks");return !failed;
}
static int pending(void)
{
 count=slot=alias_kind=mutation=stop_mode=0;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,0x800aa934u,BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||accesses!=14)fail("pending return refusal",accesses);
 if(fixture_load(actor+0x114u,4)!=0x87654321u||cpu.gpr[2]!=0x89abcdefu||cpu.gpr[4]!=0x76540004u+seed*32u||cpu.gpr[31]!=0x800aaa10u)fail("pending partial state",cpu.gpr[2]);
 puts("MOTION QUEUE PASS pending updater return retains completed effects and refuses epilogue");return !failed;
}
static int faults(void)
{
 unsigned total=0;actor_domain=source_domain=table_domain=stack_kind=alias_kind=mutation=stop_mode=0;
 for(unsigned mode=0;mode<6;mode++){
  const unsigned counts[]={0,0,1,4,5,255};count=counts[mode];slot=mode==1?80:0;
  fault_at=0;if(!run_case())return 0;unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){if(!run_case())return 0;total++;}fault_at=0;
 }
 printf("MOTION QUEUE PASS %u injected access faults, delayed loads and JAL delay-slot store\n",total);return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fseek(f,0x3ae44,SEEK_SET)==0&&fread(body,1,sizeof(body),f)==sizeof(body)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(body,sizeof(body),digest));char text[65];
 for(unsigned i=0;i<32;i++)snprintf(text+i*2,3,"%02x",digest[i]);
 assert(strcmp(text,"5e3abe8f581f1199f35b02f61b60ba6681c7c152a2475df8d2a4e632dfeb4053")==0);
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--queue")==0){count=4;slot=0x12345678u;return run_case()?0:1;}
  if(strcmp(argv[1],"--signed-slot")==0){slot=UINT32_MAX;return run_case()?0:1;}
  if(strcmp(argv[1],"--indirect")==0){slot=80;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){count=2;mutation=6;return run_case()?0:1;}
  if(strcmp(argv[1],"--clock-alias")==0){alias_kind=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--stack-return")==0){mutation=8;return run_case()?0:1;}
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--partial")==0){stop_mode=2;return run_case()?0:1;}
  return 2;
 }
 unsigned total=0;
 for(actor_domain=0;actor_domain<6;actor_domain++)for(count=0;count<256;count++){
  source_domain=(actor_domain+1)%6;table_domain=(actor_domain+2)%6;seed=count;stack_kind=count%4;slot=count*0x10203u;
  if(!run_case())return 1;
  total++;
 }
 count=0;
 const uint32_t slots[]={0,79,80,81,UINT32_MAX,0x80000000u,0x7fffffffu,0x40000000u};
 for(actor_domain=0;actor_domain<6;actor_domain++)for(source_domain=0;source_domain<6;source_domain++)for(stack_kind=0;stack_kind<4;stack_kind++)for(unsigned n=0;n<8;n++){
  table_domain=(actor_domain+source_domain+n)%6;slot=slots[n];seed=n+stack_kind;
  if(!run_case())return 1;
  total++;
 }
 printf("MOTION QUEUE PASS %u complete original-body comparisons, all 256 counts, signed/wrapped slots, six data domains and four stack placements\n",total);fflush(stdout);
 actor_domain=source_domain=table_domain=stack_kind=seed=0;unsigned extras=0;
 for(mutation=1;mutation<=8;mutation++){count=mutation<=6?2:0;slot=0;if(!run_case())return 1;extras++;}mutation=0;
 for(alias_kind=1;alias_kind<=20;alias_kind++)for(unsigned arm=0;arm<3;arm++){
  count=arm==2?4:0;slot=arm==1?80:0;
  if(alias_kind==5&&count)continue;
  if(!run_case())return 1;
  extras++;
 }
 alias_kind=0;count=slot=0;
 for(stop_mode=1;stop_mode<=2;stop_mode++){if(!run_case())return 1;extras++;}stop_mode=0;
 printf("MOTION QUEUE PASS %u live-state, alias/malformed and callee-refusal comparisons\n",extras);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("MOTION QUEUE ALL PASS native interpreted instructions=0");return 0;
}
