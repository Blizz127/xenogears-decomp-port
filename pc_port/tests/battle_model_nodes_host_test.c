/* Complete original model relocator and pointer-list caller comparisons.
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
typedef struct Body {uint32_t address,size;uint8_t bytes[752];} Body;
static Body bodies[]={{.address=0x8009ec4cu,.size=752},{.address=0x8009f708u,.size=140},
 {.address=0x8002cb54u,.size=104},{.address=0x8002cc10u,.size=68},{.address=0x8002cc74u,.size=56}};
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,domain_kind,variant,nodes,dummies,overrides,fail_alloc,alias_kind,live_change;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,allocs,frees;
static int candidate,compare,failed,observing;
static uint32_t stop_target,pending_target,list,pairs,items,model,set,buffer;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[65536];
typedef struct Call {uint32_t target,gpr[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[512];
static void fail(const char *what,uint32_t value)
{fprintf(stderr,"MODEL NODES FAIL kind=%u domain=%u variant=%u nodes=%u dummy=%u override=%u null=%u alias=%u live=%u fault=%u access=%u call=%u %s %08x\n",kind,domain_kind,variant,nodes,dummies,overrides,fail_alloc,alias_kind,live_change,fault_at,accesses,call_count,what,value);failed=1;}
static int body_index(uint32_t address)
{for(unsigned i=0;i<5;i++)if(address-bodies[i].address<bodies[i].size)return (int)i;return -1;}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){assert(!candidate&&width==4);*value=load_le(bodies[bi].bytes+address-bodies[bi].address,4);return 0;}
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 Access row={address,width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{return observed_access(opaque,address,width,value,0);}
static int runtime_write(void *opaque,uint32_t address,unsigned width,uint32_t value)
{return observed_access(opaque,address,width,&value,1);}
static void fixture_store(uint32_t a,unsigned w,uint32_t v){assert(model_test_raw_write(&rt,a,w,v)==0);}
static void put(uint32_t a,uint32_t v){fixture_store(a,4,v);}
static void snapshot(void){memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void){return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static uint32_t domain(unsigned off)
{switch(domain_kind){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x800c0000u+off;default:return 0x80100000u+off;}}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<512);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("call registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==bodies[kind].address&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0){
  if(target!=bodies[body_index(target)].address)return 0;
  record_call(cpu,target);if(target==stop_target)return -1;
  if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;return 1;}
  return candidate?runtime_bridge(opaque,cpu,target):0;
 }
 record_call(cpu,target);if(target==stop_target)return -1;
 uint32_t result=0x76543210u;
 switch(target){
 case 0x80032498u:if(cpu->gpr[4]!=4||cpu->gpr[5])fail("heap user arguments",cpu->gpr[4]);break;
 case 0x800324b8u:if(cpu->gpr[4]!=0x25u)fail("heap content arguments",cpu->gpr[4]);break;
 case 0x80031bdcu:
  allocs++;if(cpu->gpr[5])fail("allocation flags",cpu->gpr[5]);
  if(cpu->gpr[31]==0x8009ed04u){
   if(cpu->gpr[4]!=(nodes+1)*124u)fail("node allocation size",cpu->gpr[4]);
   result=set;
  }else{result=buffer+(allocs-(kind==0?2u:1u))*0x100u;}
  if(fail_alloc==allocs)result=0;
  break;
 case 0x800320e8u:
  frees++;
  if(live_change==3&&frees==1)fixture_store(set+0xau,2,1);
  break;
 case 0x80043a1cu:
  result=((cpu->gpr[4]<<7)|(cpu->gpr[5]<<5)|((cpu->gpr[7]&0x100u)>>4)|((cpu->gpr[6]&0x3ffu)>>6)|((cpu->gpr[7]&0x200u)<<2))&0xffffu;break;
 case 0x80043a58u:result=((cpu->gpr[5]<<6)|((cpu->gpr[4]>>4)&0x3fu))&0xffffu;break;
 case 0x8002c8ccu:
  /* Packet contents are deterministic external-service outputs here. This
   * proves the caller/copy boundary, not unconverted packet generation. */
  for(unsigned i=0;i<16;i++)fixture_store(cpu->gpr[5]+i,1,0x50u+i+(unsigned)variant);
  if(live_change==1){put(list,items+0x100u);for(unsigned i=0;i<nodes;i++)put(items+0x100u+i*4u,model+0x100u);put(model+0x134u,12);}
  if(live_change==2)put(list+4u,1);
  break;
 case 0x8003f968u:
  assert(cpu->gpr[6]<=0x80u);
  memmove(resolve_memory(&rt,cpu->gpr[4],cpu->gpr[6],1),resolve_memory(&rt,cpu->gpr[5],cpu->gpr[6],0),cpu->gpr[6]);result=cpu->gpr[4];break;
 default:fail("unexpected callee",target);return -1;
 }
 cpu->gpr[2]=result;cpu->gpr[3]=0xabc00000u+call_count;cpu->hi=0x65430000u+call_count;cpu->lo=0x34560000u+call_count;
 if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 for(unsigned i=0;i<2;i++)memcpy(PSX_ADDR(bodies[i].address),bodies[i].bytes,bodies[i].size);
 rt.data[0]=(ResolvedData){0x800c0000u,sizeof(shared),shared,"nodes"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&1u)cpu->gpr[29]=0x1f800fe0u;else if(variant&2u)cpu->gpr[29]|=0x20000000u;
 list=domain(0x20);pairs=domain(0x80);model=domain(0x200);items=domain(0x500);set=domain(0x800);buffer=0x80160000u;
 put(list,items);put(list+4u,nodes);
 for(unsigned i=0;i<nodes;i++){
  fixture_store(pairs+i*4u,2,dummies==1||((dummies==2)&&(i%2))?0xffffu:i);
  fixture_store(pairs+i*4u+2u,2,i%4==0?0xffffu:i%4==1?0:i%4==2?0xfffeu:1);
  put(items+i*4u,model);
 }
 fixture_store(pairs+nodes*4u,2,0xfffeu);fixture_store(pairs+nodes*4u+2u,2,0x8123u);put(model+0x34u,32);
 fixture_store(cpu->gpr[29]+0x10u,2,0x8001u+variant);fixture_store(cpu->gpr[29]+0x14u,2,0xfff0u+variant);
 fixture_store(cpu->gpr[29]+0x18u,2,0x8023u+variant);fixture_store(cpu->gpr[29]+0x1cu,2,0xffffu-variant);
 cpu->gpr[4]=list;cpu->gpr[5]=pairs;cpu->gpr[6]=variant%5;cpu->gpr[7]=overrides;
 if(kind==1){
  cpu->gpr[4]=alias_kind==1?0:set;fixture_store(set+0xau,2,nodes);
  for(unsigned i=0;i<nodes;i++){put(set+i*124u+0x68u,i%2?0:buffer+i*0x100u);put(set+i*124u+0x6cu,0x87654321u+i);}
 }else if(kind==2){
  cpu->gpr[4]=model;cpu->gpr[5]=set;cpu->gpr[6]=set+4u;
  if(alias_kind==1)cpu->gpr[5]=model+0x34u;
  if(alias_kind==2)cpu->gpr[6]=cpu->gpr[5];
  if(alias_kind==3)put(model+0x34u,0x80000001u);
  if(alias_kind==4)put(model+0x34u,0xffffffffu);
 }else if(kind>=3){cpu->gpr[4]=0x80000000u+variant*0x12345u;cpu->gpr[5]=0xffffffffu-variant*0x10321u;}
 if(kind==0){
  if(alias_kind==1)cpu->gpr[4]=0x1e000000u;
  if(alias_kind==2)cpu->gpr[5]=pairs+1;
  if(alias_kind==3)set=0x1f800ffcu;
  if(alias_kind==4)cpu->gpr[29]=0x1e000000u;
  if(alias_kind==5){fixture_store(pairs+2u,2,0xfffeu);}
 }
 accesses=call_count=allocs=frees=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;uint32_t target=bodies[kind].address;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,1000000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,target,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET){fail("oracle instruction budget",oracle.pc);return 0;}
 int expected_failure=fault_at||stop_target||(kind==0&&alias_kind>=1&&alias_kind<=4);
 if((original<0)!=!!expected_failure)fail("unexpected oracle completion/fault",(uint32_t)original);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("register",i);fprintf(stderr,"native=%08x oracle=%08x\n",native.gpr[i],oracle.gpr[i]);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo)fail("HI/LO",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 return !failed;
}
static int controls(void)
{
 for(kind=0;kind<5;kind++){
  PcPortMipsCpu cpu;domain_kind=dummies=alias_kind=fail_alloc=live_change=0;nodes=2;overrides=1;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  uint32_t target=bodies[kind].address;unsigned size=bodies[kind].size;PcPortMipsCpu original=cpu;
  uint8_t *code=kind<2?PSX_ADDR(target):g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);
  for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?size/2:size-1;uint8_t *p=code+off;*p^=1;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("identity accepted",off);*p^=1;}
  if(kind>=2){
   XenoRetailFileSlot original_slot=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("missing cached asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+size-1u;
   if(battle_model_nodes_try_host(&rt,&cpu,target))fail("short cached asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("pending entry",target);cpu=original;
  rt.force_interpret=1;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("forced entry",target);rt.force_interpret=0;
  rt.initialized=0;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("uninitialized entry",target);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("inactive entry",target);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_model_nodes_try_host(&rt,&cpu,target))fail("wrong context",target);rt.bridge_cpu=&cpu;
  if(battle_model_nodes_try_host(&rt,&cpu,target+4u))fail("wrong target",target);
  assert(setenv("XENO_BATTLE_NATIVE_MODEL_NODES","0",1)==0);if(battle_model_nodes_try_host(&rt,&cpu,target))fail("opt out",target);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_NODES")==0);
  if(memcmp(cpu.gpr,original.gpr,sizeof(cpu.gpr))||!memory_equal()||call_count||accesses)fail("gate side effect",target);
 }
 kind=0;nodes=2;const uint32_t targets[]={0x80032498u,0x80031bdcu,0x8002cb54u,0x800324b8u,0x8002cc10u,0x80043a1cu,0x8002cc74u,0x80043a58u,0x8002c8ccu,0x8003f968u,0x8009f708u,0x800320e8u};
 for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);i++){
  fail_alloc=i>=10?2:0;stop_target=targets[i];if(!run_case())return 0;stop_target=0;
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_target=targets[i];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,bodies[kind].address,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);pending_target=0;observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending callee refusal",targets[i]);
 }
 fail_alloc=0;
 /* Packet generation now has its own complete-body and cached-boundary
  * suite. Here it remains a modeled callee to isolate constructor semantics. */
 puts("MODEL NODES PASS five identity/context/opt-out gates, cached asset controls, twelve missing/pending callees");return !failed;
}
/* Export separate symbols with guard bytes; the bridge must bind each exact
 * four-byte global even when a neighboring inferred range covers it. */
uint32_t D_80050108[4],D_8005010C[4],D_80059310[4],D_80059314[4];
static unsigned tag_calls;static uint16_t seen_tag;
static void tag_probe(uint16_t value){tag_calls++;seen_tag=value;}
static int bindings(void)
{
 observing=0;memset(&rt,0,sizeof(rt));uint8_t neighbor[0x100];memset(neighbor,0xa7,sizeof(neighbor));
 rt.data[0]=(ResolvedData){0x80050104u,0x40u,neighbor,"neighbor"};
 rt.data[1]=(ResolvedData){0x800592fcu,0x1cu,neighbor+0x40u,"neighbor2"};rt.data_count=2;
 initialize_model_node_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 if(rt.data_count!=6)fail("exact binding count",(uint32_t)rt.data_count);
 uint32_t addresses[]={0x80050108u,0x8005010cu,0x80059310u,0x80059314u};
 uint32_t *globals[]={D_80050108,D_8005010C,D_80059310,D_80059314};
 for(unsigned i=0;i<4;i++){
  for(unsigned j=0;j<4;j++)globals[i][j]=0x12345678u+j;
  if(resolve_memory(&rt,addresses[i],4,1)!=(uint8_t *)globals[i])fail("exact global storage",addresses[i]);
  put(addresses[i],0x76543210u+i);
  if(globals[i][0]!=0x76543210u+i||globals[i][1]!=0x12345679u)fail("global guards",i);
 }
 for(unsigned i=0;i<sizeof(neighbor);i++)if(neighbor[i]!=0xa7u)fail("neighbor overwritten",i);
 const uint32_t tags[]={0,1,0x25,0xffff,0x10000,0x7fffffff,0x80000000,0x80100000,0xa0101234,0xffffffff};
 for(unsigned i=0;i<10;i++){
  PcPortMipsCpu cpu;memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;
  rt.functions[0]=(ResolvedFunction){0x800324b8u,tag_probe,"HeapSetCurrentContentType"};rt.function_count=1;
  initialize_cpu(&cpu,&rt);cpu.gpr[4]=tags[i];cpu.gpr[2]=0x12345678;cpu.gpr[3]=0xabcdef12;tag_calls=0;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,0x800324b8u,BATTLE_HALT_PC,100);
  if(rc!=PC_PORT_MIPS_HALTED||cpu.steps||tag_calls!=1||seen_tag!=(uint16_t)tags[i]||cpu.gpr[2]!=0x12345678||cpu.gpr[3]!=0xabcdef12)fail("content tag ABI",tags[i]);
  cpu.load_valid=1;cpu.load_reg=4;tag_calls=0;
  if(PcPortMipsRun(&cpu,0x800324b8u,BATTLE_HALT_PC,100)!=PC_PORT_MIPS_UNRESOLVED_CALL||tag_calls)fail("pending content tag",tags[i]);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 }
 puts("MODEL NODES PASS four real symbol bindings with neighbor guards; ten u16/void ABI values and pending-load refusals");return !failed;
}
static int faults(void)
{
 unsigned total=0;domain_kind=dummies=fail_alloc=alias_kind=live_change=0;nodes=2;overrides=1;
 for(kind=0;kind<5;kind++){
  fault_at=0;if(!run_case())return 0;unsigned count=accesses;
  for(fault_at=1;fault_at<=count;fault_at++){if(!run_case())return 0;total++;}fault_at=0;
 }
 printf("MODEL NODES PASS %u injected faults across every data access of five complete bodies\n",total);fflush(stdout);return !failed;
}
int main(int argc,char **argv)
{
 for(unsigned i=0;i<5;i++){
  FILE *f=fopen(i<2?"disc/battle.bin":"disc/SLUS_006.64","rb");assert(f);
  unsigned offset=i<2?bodies[i].address-0x8006faf0u:XENO_RD_SLUS_OFF(bodies[i].address);
  assert(fseek(f,offset,SEEK_SET)==0&&fread(bodies[i].bytes,1,bodies[i].size,f)==bodies[i].size);fclose(f);
 }
 assert(XenoRetailData_File(XENO_RD_SLUS));assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2){
  if(strcmp(argv[1],"--controls")==0)return controls()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--bindings")==0)return bindings()?0:1;
  if(strcmp(argv[1],"--buffers")==0)kind=2;
  else if(strcmp(argv[1],"--cleanup")==0)kind=1;
  else if(strcmp(argv[1],"--page")==0)kind=3;
  else if(strcmp(argv[1],"--clut")==0)kind=4;
  else if(strcmp(argv[1],"--live-list")==0)live_change=1;
  else if(strcmp(argv[1],"--null")==0)fail_alloc=2;
  else if(strcmp(argv[1],"--dummy")==0)dummies=1;
  else assert(strcmp(argv[1],"--nodes")==0);
  nodes=2;overrides=1;return run_case()?0:1;
 }
 unsigned total=0;const unsigned counts[]={0,1,2,6};
 for(kind=0;kind<5;kind++)for(domain_kind=0;domain_kind<6;domain_kind++)for(unsigned n=0;n<4;n++)for(overrides=0;overrides<2;overrides++){
  nodes=counts[n];dummies=variant%3;variant=total++;if(!run_case())return 1;
 }
 printf("MODEL NODES PASS %u full comparisons across six address domains\n",total);fflush(stdout);
 kind=0;domain_kind=dummies=overrides=0;nodes=3;
 for(fail_alloc=1;fail_alloc<=4;fail_alloc++)if(!run_case())return 1;
 fail_alloc=0;
 for(alias_kind=1;alias_kind<=5;alias_kind++)if(!run_case())return 1;
 alias_kind=0;
 for(live_change=1;live_change<=2;live_change++)if(!run_case())return 1;
 live_change=0;
 kind=1;live_change=3;if(!run_case())return 1;
 live_change=0;alias_kind=1;if(!run_case())return 1;
 alias_kind=0;
 kind=2;for(alias_kind=1;alias_kind<=4;alias_kind++)if(!run_case())return 1;
 alias_kind=0;fail_alloc=1;if(!run_case())return 1;
 fail_alloc=0;
 kind=0;nodes=64;dummies=1;if(!run_case())return 1;
 nodes=20;dummies=0;overrides=1;if(!run_case())return 1;
 puts("MODEL NODES PASS allocation/cleanup failures, live list/count reloads, aliases, malformed pointers, size wrapping and 20/64-node capacities");fflush(stdout);
 if(!faults()||!controls()||!bindings())return 1;
 return 0;
}
