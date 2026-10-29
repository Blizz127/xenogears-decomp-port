/* Complete original resource table binding comparisons.
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
static uint8_t module[0x3f4c];
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned domain_kind,stack_kind,seed,mutation,alias_kind;
static uint32_t count,table,alternate,resource;
static unsigned fault_at,accesses,expected_accesses,base_reads,count_reads,flag_writes,mutation_seen,legacy_calls;
static int candidate,compare,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[2048];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"RESOURCE BIND FAIL count=%u domain=%u stack=%u seed=%u mutation=%u alias=%u fault=%u access=%u %s %08x\n",count,domain_kind,stack_kind,seed,mutation,alias_kind,fault_at,accesses,what,value);
 exit(1);
}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,a,w,v)!=0)fail("fixture write",a);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&address-0x801e7ec4u<136u){
  assert(!candidate&&width==4);*value=load_le(module+(address-0x801e4000u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access trace capacity",accesses);
 if(!write&&address==0x800d39ccu&&width==4){
  base_reads++;
  if(mutation==5&&base_reads==2){fixture_store(address,4,alternate);mutation_seen++;}
  if(mutation==8&&base_reads==1){fixture_store(address,4,0x1e000000u);mutation_seen++;}
 }
 if(!write&&address==0x800d3348u&&width==4){
  count_reads++;
  if(count_reads==1&&mutation>=1&&mutation<=4){
   const uint32_t values[]={0,0,5,UINT32_MAX,0x80000000u};fixture_store(address,4,values[mutation]);mutation_seen++;
  }
  if(count_reads==1&&mutation==10){fixture_store(address,4,2);fixture_store(0x800d39ccu,4,alternate);mutation_seen++;}
 }
 if(write&&width==1&&accesses>=4){
  flag_writes++;
  if(flag_writes==1&&mutation>=6&&mutation<=7){fixture_store(mutation==6?0x800d39ccu:0x800d3348u,4,mutation==6?alternate:0);mutation_seen++;}
  if(flag_writes==1&&mutation==9){fixture_store(0x800d3348u,4,5);mutation_seen++;}
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
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x801e7ec4u&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x801e7ec4u<136u)return 0;
 fail("unexpected callee",target);return -1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"resource-bind-shared"};
 /* The upper mapping must remain untouched: this bus uses raw overlay RAM. */
 rt.data[1]=(ResolvedData){0x800d0000u,0x10000u,shared+0x10000u,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(stack_kind==1)cpu->gpr[29]=0x1f800fe0u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 if(stack_kind==4)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==5)cpu->gpr[29]++;
 if(stack_kind==6)cpu->gpr[29]=4;
 if(stack_kind==7)cpu->gpr[29]=0;
 table=domain(domain_kind,0x100u);alternate=domain(domain_kind,0x500u);
 const uint32_t raw[]={0,UINT32_MAX,0x800d3344u,0x1e000000u,0xa0100000u,0x00100100u,0x12345678u,0x80000000u};resource=raw[seed%8];
 if(alias_kind==1)table=0x800d3348u-13u;
 if(alias_kind==2)table=0x800d3348u-14u-13u;
 if(alias_kind==3)table=0x800d39ccu-13u;
 if(alias_kind==4)table=0x800d39ccu-14u-13u;
 if(alias_kind==5)table=0x800d3344u-13u;
 if(alias_kind==6)table=0x800d2f64u-13u;
 if(alias_kind==7)table=0x800d3348u+1u-13u;
 if(alias_kind==8)table=0x800d3348u+2u-13u;
 if(alias_kind==9)table=0x800d3348u+3u-13u;
 if(alias_kind==10)table=0;
 if(alias_kind==11)table=0x1e000000u;
 if(alias_kind==12)table=0x00100100u;
 if(alias_kind==13)table=0x1f800ff8u;
 if(alias_kind==14)table++;
 if(alias_kind==15)table=0xfffffff3u;
 if(alias_kind==16)table=cpu->gpr[29]-8u;
 if(alias_kind==17)table=0x800d39ccu+3u-13u;
 cpu->gpr[4]=resource;cpu->gpr[5]=table;cpu->gpr[6]=count;
 accesses=base_reads=count_reads=flag_writes=mutation_seen=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,0x801e7ec4u,BATTLE_HALT_PC,100000);
 expected_accesses=accesses;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,0x801e7ec4u,BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!alias_kind&&mutation!=8&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen&&!fault_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static int controls(void)
{
 PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();PcPortMipsCpu original=cpu;
 const uint32_t regions[]={0x801e7ec4u,0x801e4000u,0x801e5840u},sizes[]={136,32,228};
 for(unsigned r=0;r<3;r++)for(unsigned i=0;i<3;i++){
  unsigned off=i==0?0:i==1?sizes[r]/2:sizes[r]-1;uint8_t *p=PSX_ADDR(regions[r]+off);*p^=1;
  if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("identity accepted",regions[r]+off);
  *p^=1;
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("pending entry",0);cpu=original;
 rt.force_interpret=1;if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("forced entry",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("uninitialized entry",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("wrong context",0);rt.bridge_cpu=&cpu;
 if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec8u))fail("wrong target",0);
 assert(setenv("XENO_BATTLE_NATIVE_RESOURCE_BIND","0",1)==0);
 if(battle_resource_bind_try_host(&rt,&cpu,0x801e7ec4u))fail("opt out",0);
 assert(unsetenv("XENO_BATTLE_NATIVE_RESOURCE_BIND")==0);
 if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||accesses)fail("gate side effect",0);
 puts("RESOURCE BIND PASS full body/startup markers, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 count=3;domain_kind=stack_kind=alias_kind=mutation=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<5;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
  rt.functions[0]=(ResolvedFunction){0x801e7ec4u,legacy_probe,"resource_bind"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *p=bridge_cache_slot(&rt,0x801e7ec4u);p->target=0x801e7ec4u;p->state=1;p->entry=rt.functions[0];}
  const uint32_t addresses[]={0,0x801e7ec4u,0x801e4000u,0x801e5840u};
  if(reject>=1&&reject<=3)((uint8_t *)PSX_ADDR(addresses[reject]))[0]^=1;
  if(reject==4)assert(setenv("XENO_BATTLE_NATIVE_RESOURCE_BIND","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,0x801e7ec4u):PcPortMipsRun(&cpu,0x801e7ec4u,BATTLE_HALT_PC,100000);
  if(reject>=1&&reject<=3){((uint8_t *)PSX_ADDR(addresses[reject]))[0]^=1;want_ram[addresses[reject]&0x1fffffu]^=1;}
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_RESOURCE_BIND")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject&&((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected dispatch falls through",(uint32_t)rc);
 }
 puts("RESOURCE BIND PASS 20 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}
static int faults(void)
{
 unsigned total=0;domain_kind=stack_kind=alias_kind=mutation=0;
 for(unsigned mode=0;mode<8;mode++){
  const uint32_t counts[]={0,1,3,UINT32_MAX,0,3,3,3};count=counts[mode];alias_kind=mode==4||mode==5?10:mode==6?1:mode==7?3:0;
  fault_at=0;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}fault_at=0;
 }
 alias_kind=0;printf("RESOURCE BIND PASS %u injected access faults including pre-frame stores and delayed loop loads\n",total);return 1;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");assert(f&&fread(module,1,sizeof(module),f)==sizeof(module)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(module+0x3ec4,136,digest));char text[65];
 for(unsigned i=0;i<32;i++)snprintf(text+i*2,3,"%02x",digest[i]);
 assert(strcmp(text,"45a9f8e1446f80ddabfa3bdb465dd076f5f197913a13e41e5a661e590484c470")==0);
 count=3;
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--zero")==0){count=0;return run_case()?0:1;}
  if(strcmp(argv[1],"--negative")==0){count=UINT32_MAX;return run_case()?0:1;}
  if(strcmp(argv[1],"--null")==0){alias_kind=10;return run_case()?0:1;}
  if(strcmp(argv[1],"--live-count")==0){mutation=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--live-table")==0){mutation=6;return run_case()?0:1;}
  if(strcmp(argv[1],"--count-alias")==0){alias_kind=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--bad-stack")==0){stack_kind=5;return run_case()?0:1;}
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  return 2;
 }
 unsigned total=0;
 for(domain_kind=0;domain_kind<6;domain_kind++)for(count=0;count<256;count++){
  seed=count;stack_kind=count%8;run_case();total++;
 }
 const uint32_t counts[]={0,1,2,7,255,UINT32_MAX,0x80000000u,0xfffffff0u};
 for(domain_kind=0;domain_kind<6;domain_kind++)for(stack_kind=0;stack_kind<8;stack_kind++)for(unsigned i=0;i<8;i++){
  count=counts[i];seed=i+stack_kind;run_case();total++;
 }
 printf("RESOURCE BIND PASS %u complete original-body comparisons across six data domains and eight stack values\n",total);fflush(stdout);
 domain_kind=stack_kind=seed=0;unsigned extras=0;
 for(mutation=1;mutation<=10;mutation++){count=3;run_case();extras++;}mutation=0;
 for(alias_kind=1;alias_kind<=17;alias_kind++)for(unsigned i=0;i<3;i++){
  count=i==0?0:i==1?3:UINT32_MAX;
  if(i==1&&alias_kind>=7&&alias_kind<=9)count=(1u<<((alias_kind-6)*8u))|1u;
  run_case();extras++;
 }
 alias_kind=0;count=0x7fffffffu;mutation=1;run_case();extras++;mutation=0;
 printf("RESOURCE BIND PASS %u live-state, alias, malformed-pointer and signed-count comparisons\n",extras);fflush(stdout);
 if(!faults()||!controls()||!boundaries())return 1;
 puts("RESOURCE BIND ALL PASS native interpreted instructions=0");return 0;
}
