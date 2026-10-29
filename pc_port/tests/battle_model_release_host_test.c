/* Complete original model-list and secondary-buffer release comparisons.
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
typedef struct Body {uint32_t address,size;const char *sha;uint8_t bytes[176];} Body;
static Body bodies[]={
 {0x8009f794u,176,"e5993739ad3629bfbd3c86a337b5e52df6444b5ed0b841ac62608d62fbd3ce2d",{0}},
 {0x8002cbbcu,84,"068f185d0c08afc9cc57a091b7fc33cb587688f62a3fc4d159daa4b350b25ee0",{0}}};
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,count,release,domain_kind,stack_kind,seed,pattern,flags,mutation,alias_kind;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,count_reads,mutation_seen,frees;
static unsigned stop_mode,pending_return,legacy_calls;
static int candidate,compare,failed,observing;
static uint32_t list,items,alternate,models,other_models,model;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[64];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"MODEL RELEASE FAIL kind=%u count=%u release=%08x domain=%u stack=%u seed=%u pattern=%u flags=%x mutation=%u alias=%u stop=%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,count,release,domain_kind,stack_kind,seed,pattern,flags,mutation,alias_kind,stop_mode,pending_return,fault_at,accesses,call_count,what,value);
 failed=1;
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<2;i++)if(address-bodies[i].address<bodies[i].size)return (int)i;return -1;}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{assert(model_test_raw_write(&rt,a,w,v)==0);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){
  assert(!candidate&&width==4);*value=load_le(bodies[bi].bytes+(address-bodies[bi].address),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 if(!write&&address==list+4u){
  count_reads++;
  if((mutation==7||mutation==8)&&count_reads==2){fixture_store(address,4,mutation==7?1:5);mutation_seen++;}
  if(mutation==11&&count_reads==3){fixture_store(address,4,2);mutation_seen++;}
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
static uint32_t domain(unsigned which,unsigned off)
{
 switch(which){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x800c0000u+off;default:return 0x80100000u+off;}
}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<64);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("callee registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==bodies[kind].address&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);
 if(bi>=0){
  if(target!=bodies[bi].address)return 0;
  record_call(cpu,target);
  if(target!=0x8002cbbcu||cpu->gpr[31]!=0x8009f7f8u)fail("nested helper ABI",target);
  if(stop_mode==3)return -1;
  int rc=candidate?runtime_bridge(opaque,cpu,target):0;
  if(pending_return==2&&rc==1){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
  return rc;
 }
 if(target!=0x800320e8u){fail("unexpected callee",target);return -1;}
 record_call(cpu,target);frees++;
 int secondary=cpu->gpr[31]==0x8002cbecu;
 if(!secondary&&cpu->gpr[31]!=0x8009f824u)fail("free return address",cpu->gpr[31]);
 if(stop_mode==1)return -1;
 if(frees==1){
  uint32_t current=cpu->gpr[16];
  if(mutation==1||mutation==9){assert(secondary);fixture_store(current,2,0xa5a7u);mutation_seen++;}
  if(mutation==9)fixture_store(current+0x18u,4,0x87654321u);
  if(mutation==2||mutation==3){fixture_store(list+4u,4,mutation==2?0:5);mutation_seen++;}
  if(mutation==4||mutation==5){fixture_store(list,4,mutation==4?alternate:0);mutation_seen++;}
  if(mutation==6){fixture_store(items+4u,4,models);mutation_seen++;}
 }
 if(mutation==10&&!secondary){fixture_store(list,4,0x12345678u);mutation_seen++;}
 /* Heap policy is the only modeled service. Preserve its caller-visible
  * volatile register effects and partial writes in both executions. */
 cpu->gpr[2]=0x97650000u+frees;cpu->gpr[3]=0x92345678u;
 for(unsigned i=4;i<16;i++)cpu->gpr[i]=0x76540000u+seed*32u+i;
 cpu->gpr[24]=0x87654321u;cpu->gpr[25]=0x34567890u;
 cpu->hi=0x12345678u;cpu->lo=0x89abcdefu;cpu->cp0[3]=0x35671289u;
 if(stop_mode==2){fixture_store(list+8u,4,0x45678901u);return -1;}
 if(pending_return==1){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(bodies[0].address),bodies[0].bytes,bodies[0].size);
 /* Main-executable .text stays poisoned: only the verified asset cache
  * supplies the native helper identity; the oracle fetches its private body. */
 rt.data[0]=(ResolvedData){0x800c0000u,sizeof(shared),shared,"release"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(stack_kind==1)cpu->gpr[29]=0x1f800fe0u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 list=domain(domain_kind,0x80u);items=domain((domain_kind+1)%6,0x100u);alternate=domain((domain_kind+1)%6,0x180u);
 models=domain((domain_kind+2)%6,0x400u);other_models=domain((domain_kind+2)%6,0x600u);model=models;
 fixture_store(list,4,pattern==1?0:items);fixture_store(list+4u,4,count);
 for(unsigned i=0;i<8;i++){
  fixture_store(items+i*4u,4,pattern==2&&(i%2)?0:models+i*0x40u);
  fixture_store(alternate+i*4u,4,other_models+i*0x40u);
  fixture_store(models+i*0x40u,2,flags);fixture_store(other_models+i*0x40u,2,flags^0x40u);
  fixture_store(models+i*0x40u+0x18u,4,domain((domain_kind+3)%6,0xa00u+i*0x20u));
  fixture_store(other_models+i*0x40u+0x18u,4,domain((domain_kind+3)%6,0xb00u+i*0x20u));
 }
 cpu->gpr[4]=kind?model:pattern==3?0:list;cpu->gpr[5]=release;
 if(alias_kind==1)fixture_store(list,4,list);
 if(alias_kind==2)fixture_store(list,4,cpu->gpr[29]-0x10u);
 if(alias_kind==3)cpu->gpr[4]=cpu->gpr[29]-8u;
 if(alias_kind==4)fixture_store(items,4,list);
 if(alias_kind==5)fixture_store(model+0x18u,4,0);
 if(alias_kind==6)cpu->gpr[4]=0;
 if(alias_kind==7)cpu->gpr[4]++;
 if(alias_kind==8)cpu->gpr[4]=0x1e000000u;
 if(alias_kind==9)cpu->gpr[29]=0x1e000000u;
 if(alias_kind==10)cpu->gpr[29]++;
 if(alias_kind==11)fixture_store(items,4,model+1u);
 if(alias_kind==12)fixture_store(list,4,0x1f800ffcu);
 if(alias_kind==13)fixture_store(list,4,items+1u);
 if(alias_kind==14)fixture_store(items,4,0x1e000000u);
 accesses=call_count=count_reads=mutation_seen=frees=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,bodies[kind].address,BATTLE_HALT_PC,100000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,bodies[kind].address,BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET){fail("oracle budget",oracle.pc);return 0;}
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_mode&&!alias_kind&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("register",i);fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen&&!fault_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return !failed;
}
static int controls(void)
{
 for(kind=0;kind<2;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=bodies[kind].address;
  uint8_t *code=kind?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);
  for(unsigned i=0;i<3;i++){
   unsigned off=i==0?0:i==1?bodies[kind].size/2:bodies[kind].size-1;code[off]^=1;
   if(battle_model_release_try_host(&rt,&cpu,target))fail("identity accepted",off);
   code[off]^=1;
  }
  if(kind){
   XenoRetailFileSlot saved=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_release_try_host(&rt,&cpu,target))fail("missing asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+bodies[kind].size-1u;
   if(battle_model_release_try_host(&rt,&cpu,target))fail("short asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_model_release_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_model_release_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_model_release_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_model_release_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_model_release_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_model_release_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_MODEL_RELEASE","0",1)==0);
  if(battle_model_release_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_RELEASE")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("MODEL RELEASE PASS both full identities, asset-cache bounds, context, pending entry and opt-out gates");return !failed;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<2;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=bodies[kind].address;
  rt.functions[0]=(ResolvedFunction){target,legacy_probe,kind?"func_8002CBBC":"func_8009F794"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=kind?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MODEL_RELEASE","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_RELEASE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1){if(kind==0)want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("MODEL RELEASE PASS 48 resolved/cached native-precedence and rejected-dispatch checks");return !failed;
}
static int pending(void)
{
 kind=0;count=3;release=1;flags=1;
 for(pending_return=1;pending_return<=2;pending_return++){
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,bodies[kind].address,BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=2||count_reads!=1)fail("pending return refusal",(uint32_t)rc);
  uint32_t pointer=0,flag=0;assert(model_test_raw_read(&rt,list,4,&pointer)==0);assert(model_test_raw_read(&rt,model,2,&flag)==0);
  if(pointer!=items||flag!=(pending_return==1?1u:0u)||cpu.gpr[4]!=0x76540004u+seed*32u||cpu.gpr[2]==0xfeedbeefu)fail("pending partial state",pointer);
 }
 pending_return=0;puts("MODEL RELEASE PASS pending HeapFree/helper returns preserve partial effects and refuse caller continuation");return !failed;
}
static int faults(void)
{
 unsigned total=0;domain_kind=stack_kind=alias_kind=mutation=stop_mode=pattern=0;flags=0xa5a7u;
 for(kind=0;kind<2;kind++)for(unsigned mode=0;mode<3;mode++){
  count=mode?3:0;release=mode==1?0:1;fault_at=0;if(!run_case())return 0;unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){if(!run_case())return 0;total++;}fault_at=0;
 }
 printf("MODEL RELEASE PASS %u injected access faults across nested helper and delayed epilogues\n",total);return !failed;
}
int main(int argc,char **argv)
{
 for(unsigned i=0;i<2;i++){
  FILE *f=fopen(i?"disc/SLUS_006.64":"disc/battle.bin","rb");assert(f);
  unsigned offset=i?XENO_RD_SLUS_OFF(bodies[i].address):bodies[i].address-0x8006faf0u;
  assert(fseek(f,offset,SEEK_SET)==0&&fread(bodies[i].bytes,1,bodies[i].size,f)==bodies[i].size);assert(fclose(f)==0);
  uint8_t digest[32];assert(SHA256(bodies[i].bytes,bodies[i].size,digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,bodies[i].sha)==0);
 }
 assert(XenoRetailData_File(XENO_RD_SLUS));count=3;release=1;flags=0xa5a7u;
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--partial")==0){stop_mode=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--flags")==0){mutation=9;return run_case()?0:1;}
  if(strcmp(argv[1],"--count")==0){mutation=8;count=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--unsigned")==0){mutation=11;count=UINT32_MAX;return run_case()?0:1;}
  if(strcmp(argv[1],"--no-release")==0){release=0;return run_case()?0:1;}
  if(strcmp(argv[1],"--null-buffer")==0){kind=1;alias_kind=5;return run_case()?0:1;}
  return 2;
 }
 unsigned total=0;const unsigned counts[]={0,1,3,8},releases[]={0,1,UINT32_MAX},flag_values[]={0,1,0xfffe,0xffff};
 for(kind=0;kind<2;kind++)for(domain_kind=0;domain_kind<6;domain_kind++)for(unsigned c=0;c<4;c++)for(unsigned r=0;r<3;r++)for(pattern=0;pattern<4;pattern++){
  count=counts[c];release=releases[r];flags=flag_values[(c+r)%4];stack_kind=(c+r+pattern)%4;seed=c*3+r;
  if(!run_case())return 1;
  total++;
 }
 printf("MODEL RELEASE PASS %u complete original-body comparisons, six data domains and four stack placements\n",total);
 domain_kind=stack_kind=seed=pattern=0;count=3;release=1;flags=0xa5a7u;
 unsigned extras=0;kind=0;
 for(mutation=1;mutation<=11;mutation++){
  count=(mutation==7||mutation==11)?UINT32_MAX:mutation==8?1:3;if(!run_case())return 1;extras++;
 }
 mutation=7;count=0x80000000u;if(!run_case())return 1;extras++;
 mutation=11;if(!run_case())return 1;extras++;mutation=0;count=3;
 for(kind=0;kind<2;kind++)for(alias_kind=1;alias_kind<=14;alias_kind++){
  if(alias_kind==3&&kind==0)continue;
  if(!run_case())return 1;
  extras++;
 }
 alias_kind=0;kind=0;flags=1;release=1;
 for(stop_mode=1;stop_mode<=3;stop_mode++){if(!run_case())return 1;extras++;}stop_mode=0;
 printf("MODEL RELEASE PASS %u live-state, alias/malformed, high-count and callee-refusal comparisons\n",extras);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("MODEL RELEASE ALL PASS native interpreted instructions=0");return failed?1:0;
}
