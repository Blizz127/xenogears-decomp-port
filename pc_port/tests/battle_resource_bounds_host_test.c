/* Complete original resource bounds and archive relocation comparisons.
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
static uint8_t module[0x3914],relocate[72];
static const uint32_t targets[]={0x801e70e8u,0x8003342cu},sizes[]={296,72};
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,count,domain_kind,record_domain,stack_kind,seed,pattern,mutation,alias_kind;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,count_reads,mutation_seen,tag_reads;
static unsigned stop_mode,pending_return,legacy_calls;
static int candidate,compare,failed,observing;
static uint32_t table,records,alternate;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[4];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"RESOURCE BOUNDS FAIL kind=%u count=%u domain=%u records=%u stack=%u seed=%u pattern=%u mutation=%u alias=%u stop=%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,count,domain_kind,record_domain,stack_kind,seed,pattern,mutation,alias_kind,stop_mode,pending_return,fault_at,accesses,call_count,what,value);failed=1;
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<2;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,a,w,v)!=0){fail("fixture setup write",a);abort();}}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){
  assert(!candidate&&width==4);const uint8_t *body=bi?relocate:module+0x30e8;
  *value=load_le(body+(address-targets[bi]),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 if(!write&&address==table&&width==4){
  count_reads++;
  if((mutation==1||mutation==2)&&count_reads==2){fixture_store(address,4,mutation==1?0:5);mutation_seen++;}
  if(mutation==3&&count_reads==3){fixture_store(address,4,1);mutation_seen++;}
  if(mutation==4&&count_reads==3){fixture_store(address,4,2);mutation_seen++;}
  if((mutation==5||mutation==8)&&count_reads==count+3u){fixture_store(address,4,mutation==5?UINT32_MAX:0);mutation_seen++;}
 }
 if(!write&&address-records<0x200u&&width==2&&(address-records)%0x20u==0){
  tag_reads++;
  if(tag_reads==1&&mutation==6){fixture_store(table,4,0);mutation_seen++;}
  if(tag_reads==1&&mutation==7){fixture_store(table+8u,4,alternate);mutation_seen++;}
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
 switch(which){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x80050000u+off;default:return 0x80100000u+off;}
}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<4);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("callee registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==targets[kind]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);
 if(bi>=0){
  if(target!=targets[bi])return 0;
  record_call(cpu,target);
  if(target!=0x8003342cu||cpu->gpr[31]!=0x801e70fcu||cpu->gpr[16]!=cpu->gpr[4])fail("relocator ABI",target);
  if(stop_mode==1)return -1;
  if(stop_mode==2){fixture_store(table+4u,4,0x12345678u);cpu->gpr[2]=0x87654321u;cpu->gpr[6]=0x13572468u;cpu->hi=0x12345678u;cpu->lo=0xabcdef01u;cpu->cp0[3]=0x23456789u;return -1;}
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
 memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 /* Main instruction RAM remains poisoned; native identity uses retail cache. */
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"bounds-shared"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(stack_kind==1)cpu->gpr[29]=0x1f800fe0u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 table=domain(domain_kind,0x100u);records=domain(record_domain,0x400u);alternate=domain(record_domain,0x900u);
 if(alias_kind==1)table=0x800d2d2cu;
 if(alias_kind==2)table=cpu->gpr[29]+0x10u;
 if(alias_kind==3)records=cpu->gpr[29]-0x20u;
 if(alias_kind==4)records=cpu->gpr[29]-0x10u;
 const uint16_t values[]={0,1,2047,2048,32767,32768,65534,65535};
 for(unsigned i=0;i<16;i++){
  uint32_t tag=pattern==0?0x1101u:pattern==1?0x1100u:pattern==2?(i&1u?0x1100u:0x1101u):(i%3?0xffffu:0x1101u);
  fixture_store(records+i*0x20u,2,tag);
  for(unsigned j=0;j<6;j++)fixture_store(records+i*0x20u+4u+j*2u,2,values[(seed+i+j)%8]);
 }
 fixture_store(alternate,2,0x1101u);for(unsigned j=0;j<6;j++)fixture_store(alternate+4u+j*2u,2,1u+j*17u);
 fixture_store(table,4,count);
 for(unsigned i=0;i<16;i++)fixture_store(table+4u+i*4u,4,records+i*0x20u-table);
 cpu->gpr[4]=table;
 if(alias_kind==5)cpu->gpr[4]=0;
 if(alias_kind==6)fixture_store(table+4u,4,0u-table);
 if(alias_kind==7)cpu->gpr[4]++;
 if(alias_kind==8)cpu->gpr[4]=0x1e000000u;
 if(alias_kind==9)cpu->gpr[29]=0x1e000000u;
 if(alias_kind==10)cpu->gpr[29]++;
 if(alias_kind==11)fixture_store(table+4u,4,0x1e000000u-table);
 if(alias_kind==12)fixture_store(table+4u,4,records+1u-table);
 if(alias_kind==13){table=0x1f800ffcu;fixture_store(table,4,count);cpu->gpr[4]=table;}
 if(alias_kind==14){fixture_store(table+4u,4,0x1f800ff2u-table);fixture_store(0x1f800ff2u,2,0x1101u);}
 if(alias_kind==15)cpu->gpr[4]=0x00100100u;
 if(alias_kind==16)fixture_store(table+4u,4,0x00100400u-table);
 accesses=call_count=count_reads=mutation_seen=tag_reads=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,targets[kind],BATTLE_HALT_PC,100000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,targets[kind],BATTLE_HALT_PC,100000);
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
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  const uint32_t marker_addresses[]={target,0x801e4000u,0x801e5840u},marker_sizes[]={sizes[kind],32,228};
  for(unsigned m=0;m<(kind?1u:3u);m++)for(unsigned i=0;i<3;i++){
   uint8_t *code=kind?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(marker_addresses[m]);
   unsigned off=i==0?0:i==1?marker_sizes[m]/2:marker_sizes[m]-1;code[off]^=1;
   if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("identity accepted",marker_addresses[m]+off);
   code[off]^=1;
  }
  if(kind){
   XenoRetailFileSlot saved=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("missing asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+sizes[kind]-1u;
   if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("short asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_resource_bounds_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_RESOURCE_BOUNDS","0",1)==0);
  if(battle_resource_bounds_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_RESOURCE_BOUNDS")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("RESOURCE BOUNDS PASS full bodies/module markers, asset-cache bounds, context, pending entry and opt-out gates");return !failed;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static unsigned int legacy_relocate(uint32_t *p)
{(void)p;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<2;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,kind?(void *)legacy_relocate:(void *)legacy_probe,kind?"ResolveArchiveEntryPointers":"func_801E70E8"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=kind?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_RESOURCE_BOUNDS","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_RESOURCE_BOUNDS")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1){if(kind==0)want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("RESOURCE BOUNDS PASS 48 resolved/cached native-precedence and rejected-dispatch checks");return !failed;
}
static int pending(void)
{
 kind=0;count=3;pattern=mutation=alias_kind=stop_mode=0;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||count_reads!=5)fail("pending return refusal",(uint32_t)rc);
 uint32_t pointer=0,output=0;assert(model_test_raw_read(&rt,table+4u,4,&pointer)==0);assert(model_test_raw_read(&rt,0x800d2d30u,2,&output)==0);
 if(pointer!=records||output!=0xa5a5u||cpu.gpr[2]!=count||cpu.gpr[6]!=table||cpu.gpr[11]!=0x1234000bu+seed*256u)fail("pending partial state",pointer);
 puts("RESOURCE BOUNDS PASS pending relocator return retains completed relocation and refuses bounds continuation");return !failed;
}
static int faults(void)
{
 unsigned total=0;domain_kind=record_domain=stack_kind=alias_kind=mutation=stop_mode=0;
 for(kind=0;kind<2;kind++)for(unsigned mode=0;mode<4;mode++){
  const unsigned counts[]={0,1,3,8};count=counts[mode];pattern=mode==3?1:0;
  fault_at=0;if(!run_case())return 0;unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){if(!run_case())return 0;total++;}fault_at=0;
 }
 printf("RESOURCE BOUNDS PASS %u injected access faults, nested relocation and delayed loads/multiply effects\n",total);return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");assert(f&&fread(module,1,sizeof(module),f)==sizeof(module)&&fclose(f)==0);
 f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x23c2c,SEEK_SET)==0&&fread(relocate,1,sizeof(relocate),f)==sizeof(relocate)&&fclose(f)==0);
 const char *expected[]={"435abb1d6779bd86e0c96676cdac5d11caea54b2724c256c9f4735e5e07b9a16","5032ef7b832871473c3654f6ddb73d0031aacf51119de54d9d81f3c9a071fac6"};
 for(unsigned i=0;i<2;i++){
  uint8_t digest[32];assert(SHA256(i?relocate:module+0x30e8,sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,expected[i])==0);
 }
 assert(XenoRetailData_File(XENO_RD_SLUS));count=3;record_domain=2;
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--zero")==0){count=0;return run_case()?0:1;}
  if(strcmp(argv[1],"--relocate")==0){kind=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--partial")==0){stop_mode=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=6;return run_case()?0:1;}
  if(strcmp(argv[1],"--count")==0){mutation=2;count=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--unsigned")==0){kind=1;mutation=4;count=UINT32_MAX;return run_case()?0:1;}
  if(strcmp(argv[1],"--negative-count")==0){mutation=5;return run_case()?0:1;}
  return 2;
 }
 unsigned total=0;const unsigned counts[]={0,1,3,8,16};
 kind=0;
 for(domain_kind=0;domain_kind<6;domain_kind++)for(record_domain=0;record_domain<6;record_domain++)for(unsigned c=0;c<4;c++)for(pattern=0;pattern<4;pattern++){
  count=counts[c];seed=(domain_kind+record_domain+c+pattern)%8;stack_kind=(c+pattern)%4;
  if(!run_case())return 1;
  total++;
 }
 kind=1;
 for(domain_kind=0;domain_kind<6;domain_kind++)for(unsigned c=0;c<5;c++)for(seed=0;seed<4;seed++){
  count=counts[c];record_domain=seed;stack_kind=seed;pattern=0;
  if(!run_case())return 1;
  total++;
 }
 printf("RESOURCE BOUNDS PASS %u complete original-body comparisons, six data domains including actual shared bindings and four stack placements\n",total);
 kind=domain_kind=record_domain=stack_kind=seed=pattern=0;unsigned extras=0;
 for(mutation=1;mutation<=8;mutation++){
  count=mutation==4?UINT32_MAX:mutation==2?1:3;
  if(!run_case())return 1;
  extras++;
 }
 mutation=4;count=0x80000000u;if(!run_case())return 1;extras++;mutation=0;count=3;
 for(kind=0;kind<2;kind++)for(alias_kind=1;alias_kind<=16;alias_kind++){
  if(!run_case())return 1;
  extras++;
 }
 alias_kind=0;kind=0;
 for(stop_mode=1;stop_mode<=2;stop_mode++){if(!run_case())return 1;extras++;}stop_mode=0;
 printf("RESOURCE BOUNDS PASS %u live-state, alias/malformed, signed/unsigned count and callee-refusal comparisons\n",extras);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("RESOURCE BOUNDS ALL PASS native interpreted instructions=0");return failed?1:0;
}
