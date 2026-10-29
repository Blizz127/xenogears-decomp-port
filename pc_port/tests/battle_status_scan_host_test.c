/* Complete original status scan and mask helper comparisons.
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
static uint8_t battle[343936];
static const uint32_t targets[]={0x8007252cu,0x80089c08u,0x80089c48u,0x80089c9cu},sizes[]={908,28,36,48};
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,seed,variant,present=2047,stack_kind,mutation,index_value;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,table_reads,mutation_seen,stop_mode,pending_return,legacy_calls;
static int candidate,compare,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[2048];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[64];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"STATUS SCAN FAIL kind=%u seed=%u variant=%u present=%x stack=%u mutation=%u index=%u stop=%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,seed,variant,present,stack_kind,mutation,index_value,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<4;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,a,w,v)!=0)fail("fixture write",a);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&body_index(address)>=0){
  assert(!candidate&&width==4);*value=load_le(battle+(address-0x8006faf0u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access capacity",accesses);
 if(!write&&width==2&&address-0x800c3448u<512u){
  table_reads++;
  if(table_reads==1&&mutation==1){fixture_store(address,2,0x7315u);mutation_seen++;}
  if(table_reads==1&&mutation==2){fixture_store(0x800d39dcu,2,0x8840u);mutation_seen++;}
  if(table_reads==15&&mutation==5){fixture_store(0x800d39dcu,2,0x0040u);mutation_seen++;}
 }
 if(!write&&address==0x800d2dcfu&&mutation==3){fixture_store(0x800c3eb8u+3u*0x1cu,1,0xffu);fixture_store(0x800cce08u+3u*0x170u,2,0xc000u);mutation_seen++;}
 if(!write&&address==0x800c48eau&&mutation==4){fixture_store(address,1,0);mutation_seen++;}
 if(!write&&address==0x800d2dd6u&&mutation==6){fixture_store(0x800c3608u,2,0x0400u);mutation_seen++;}
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
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<64);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
 row->target=target;row->hi=cpu->hi;row->lo=cpu->lo;
 memcpy(row->gpr,cpu->gpr,sizeof(row->gpr));memcpy(row->cp0,cpu->cp0,sizeof(row->cp0));
 memcpy(row->ram,g_PsxRam,sizeof(g_PsxRam));memcpy(row->shared,shared,sizeof(shared));
 memcpy(row->pad,g_PsxScratchpad,sizeof(g_PsxScratchpad));memcpy(row->host,host,sizeof(host));
 if(compare && (call_count>=expected_calls||memcmp(row,&calls[call_count],sizeof(*row))))fail("callee registers/full memory",target);
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==targets[kind]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);
 if(bi>=0){
  if(target!=targets[bi])return 0;
  record_call(cpu,target);
  if(stop_mode==1)return -1;
  if(stop_mode==2){fixture_store(0x800d39dcu,2,0x7654u);cpu->gpr[2]=0x87654321u;cpu->gpr[6]=0x13572468u;cpu->hi=0x12345678u;cpu->lo=0xabcdef01u;cpu->cp0[3]=0x23456789u;return -1;}
  int rc=candidate?runtime_bridge(opaque,cpu,target):0;
  if(pending_return&&rc==1){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
  return rc;
 }
 fail("unexpected callee",target);return -1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x800c0000u,0x20000u,shared,"unused-upper-shadow"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800fe0u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;break;
 case 4:cpu->gpr[29]=0x800c3448u+0x20u;break;case 5:cpu->gpr[29]=0x800d39dcu+0x20u;break;
 case 6:cpu->gpr[29]=0x800c48e8u+0x20u;break;case 7:cpu->gpr[29]++;break;
 case 8:cpu->gpr[29]=0x1e000000u;break;case 9:cpu->gpr[29]=0;break;case 10:cpu->gpr[29]=4;break;
 }
 for(unsigned i=0;i<256;i++)fixture_store(0x800c3448u+i*2u,2,variant==5||kind?((i*0x7359u+seed*0x9157u)^0xa591u):(1u<<(i&15u)));
 fixture_store(0x800c3608u,2,variant==2?0x2a8u:variant==7?0xffffu:0);
 fixture_store(0x800c48eau,1,variant==6?0x81u:variant==8?1u:variant==9?0x55u:0);
 fixture_store(0x800d39dcu,2,0xdead);
 const uint16_t status[]={0,0x4000u,0x8000u,0xc000u};
 for(unsigned i=0;i<11;i++){
  uint32_t a=0x800ccd34u+i*0x170u,g=0x800c3eb4u+i*0x1cu;
  fixture_store(0x800d2dccu+i,1,present&(1u<<i)?(i%3u?0x80u:0xffu):0);
  fixture_store(0x800d2de4u+i,1,0x70u+i);
  fixture_store(a,2,0x2345u+i);fixture_store(a+0xb8u,4,0xa5001234u+i);
  fixture_store(a+0x30u,2,(variant==2||variant==7||variant>=10)?status[(i+seed)&3u]:0);
  fixture_store(a+0xd4u,2,(variant==2||variant==7||variant>=10)?status[(i+seed+1)&3u]:0);
  fixture_store(a+0x34u,2,0x8641u|((variant==1||variant==5||(variant==4&&i%3u))?0x1000u:0));
  fixture_store(g+3u,1,(variant==3||variant>=10)?((i+seed)%3u==0?0xffu:0):0);
  fixture_store(g+4u,1,(variant==2||variant==7||variant>=10)?((i+seed)%2u?0xffu:0):0);
  if(i>=3)fixture_store(0x800c3d1bu+(i-3u)*4u,1,(variant==3||variant>=10)?((i+seed)%4u==0?0x80u:0):0);
 }
 cpu->gpr[4]=kind==3?(0x8fed0321u^(seed*0x7531u)):(index_value|((seed&3u)*0x51430000u));
 cpu->gpr[5]=index_value|((seed&3u)*0x63410000u);
 accesses=call_count=table_reads=mutation_seen=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,targets[kind],BATTLE_HALT_PC,100000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_mode&&stack_kind<7&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen&&!fault_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static int controls(void)
{
 for(kind=0;kind<4;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  for(unsigned i=0;i<3;i++){
   uint8_t *code=PSX_ADDR(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_status_scan_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_status_scan_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_status_scan_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_status_scan_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_status_scan_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_status_scan_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_status_scan_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_STATUS_SCAN","0",1)==0);
  if(battle_status_scan_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_STATUS_SCAN")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("STATUS SCAN PASS full original bodies, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<4;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"status_scan"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_STATUS_SCAN","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_STATUS_SCAN")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1){want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("STATUS SCAN PASS 48 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}
static int faults(void)
{
 unsigned total=0;kind=stack_kind=mutation=stop_mode=0;present=2047;
 for(variant=0;variant<4;variant++){
  fault_at=0;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}fault_at=0;
 }
 variant=0;
 for(kind=1;kind<4;kind++){index_value=3;fault_at=1;run_case();total++;}fault_at=0;kind=0;
 printf("STATUS SCAN PASS %u injected access faults including helper reads and delayed epilogue loads\n",total);return 1;
}
static int pending(void)
{
 kind=variant=stack_kind=mutation=stop_mode=0;present=2047;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 uint32_t mask=0;assert(model_test_raw_read(&rt,0x800d39dcu,2,&mask)==0);
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||mask||cpu.gpr[2]!=1||cpu.gpr[4]!=0||cpu.gpr[31]!=0x800725e4u)fail("pending return refusal/partial state",(uint32_t)rc);
 puts("STATUS SCAN PASS pending helper return preserves completed helper effects and refuses continuation");return 1;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 const char *hashes[]={"00dee0919b05786d384471d7a10c6255872165986ab10bcaf1d53491e702a41c","74c04258a3cf89081e9e82cef4ed802aff2c03b830a9fc7bb204291532cb45c0","26147e029cf1e4b28352108e9b27c29001c27d922d5ddb9e813d0a0fab0e9bcf","33fdf72ffb138a88c0e99fa9bf1805a931dd890167a8d071be06ff6eab525690"};
 for(unsigned i=0;i<4;i++){
  uint8_t digest[32];assert(SHA256(battle+(targets[i]-0x8006faf0u),sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,hashes[i])==0);
 }
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--sleep")==0){variant=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--blocked")==0){variant=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--stale")==0){variant=9;return run_case()?0:1;}
  if(strcmp(argv[1],"--empty")==0){present=0;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--mask")==0){kind=1;index_value=255;seed=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--complement")==0){kind=2;index_value=255;seed=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--mask-test")==0){kind=3;index_value=15;seed=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--mask-limit")==0){kind=3;index_value=16;seed=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--partial")==0){stop_mode=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  return 2;
 }
 unsigned total=0;
 for(present=0;present<2048;present++){seed=present%4;stack_kind=present%4;run_case();total++;if((present&255u)==255u){printf("STATUS SCAN PROGRESS presence cases=%u\n",present+1u);fflush(stdout);}}
 present=2047;
 for(variant=0;variant<16;variant++)for(seed=0;seed<8;seed++){stack_kind=seed%4;run_case();total++;}
 printf("STATUS SCAN PASS %u complete parent + real helper comparisons, every 11-slot presence mask\n",total);fflush(stdout);
 total=0;variant=stack_kind=0;
 for(kind=1;kind<4;kind++)for(index_value=0;index_value<256;index_value++)for(seed=0;seed<4;seed++){run_case();total++;}
 printf("STATUS SCAN PASS %u complete mask-helper comparisons, every byte index and four upper-bit patterns\n",total);fflush(stdout);
 kind=seed=index_value=0;unsigned extras=0;
 for(mutation=1;mutation<=6;mutation++){variant=mutation==5?1:mutation==6?7:0;run_case();extras++;}mutation=0;
 for(stack_kind=4;stack_kind<11;stack_kind++)for(variant=0;variant<4;variant++){run_case();extras++;}
 stack_kind=variant=0;for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();extras++;}stop_mode=0;
 printf("STATUS SCAN PASS %u live-state, frame alias, malformed-stack and callee refusal comparisons\n",extras);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("STATUS SCAN ALL PASS native interpreted instructions=0");return 0;
}
