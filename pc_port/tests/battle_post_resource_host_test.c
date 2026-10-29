/* Complete original post-resource setup and local reset comparisons.
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
static uint8_t battle[343936],module[0x3914],vector_body[16];
static const uint32_t targets[]={0x800b81bcu,0x800b88c4u,0x800b8840u,0x801e62e0u,0x80021b04u,0x800bed30u,0x800be108u,0x800bcd8cu,0x800b7c28u,0x800b89f4u,0x800bb7f8u,0x800bc2f0u},sizes[]={200,304,132,52,16,28,20,12,12,8,76,264};
#define NUM_BODIES 12u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,seed,variant,stack_kind,mutation,domain_kind,presence=255,camera_mode;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,mutation_seen,stop_at,stop_mode,pending_return,legacy_calls;
static unsigned source_reads,count_reads;
static int candidate,compare,observing;
static uint32_t source_address;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[8192];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[32];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"POST RESOURCE FAIL kind=%u seed=%u variant=%u presence=%x stack=%u mutation=%u domain=%u mode=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,seed,variant,presence,stack_kind,mutation,domain_kind,camera_mode,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<NUM_BODIES;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static const uint8_t *original_body(unsigned i)
{return i==3?module+0x22e0:i==4?vector_body:battle+(targets[i]-0x8006faf0u);}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,a,w,v)!=0)fail("fixture write",a);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){assert(!candidate&&width==4);*value=load_le(original_body((unsigned)bi)+(address-targets[bi]),4);return 0;}
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access capacity",accesses);
 if(!write&&address==0x800658c8u){source_reads++;if(mutation==1){fixture_store(address,4,source_address+source_reads*0x1000u);mutation_seen++;}}
 if(!write&&address==0x800c3d58u){count_reads++;if(mutation==2&&count_reads==3){fixture_store(address,4,0x80000001u);mutation_seen++;}}
 if(!write&&address==0x800c3684u&&mutation==3){fixture_store(address,4,0x80160200u);mutation_seen++;}
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
 assert(call_count<32);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
 row->target=target;row->hi=cpu->hi;row->lo=cpu->lo;
 memcpy(row->gpr,cpu->gpr,sizeof(row->gpr));memcpy(row->cp0,cpu->cp0,sizeof(row->cp0));
 memcpy(row->ram,g_PsxRam,sizeof(g_PsxRam));memcpy(row->shared,shared,sizeof(shared));
 memcpy(row->pad,g_PsxScratchpad,sizeof(g_PsxScratchpad));memcpy(row->host,host,sizeof(host));
 if(compare && (call_count>=expected_calls||memcmp(row,&calls[call_count],sizeof(*row))))fail("callee registers/full memory",target);
 call_count++;
}
/* External services are explicit models except the real four-byte OT
 * compatibility bridge. Both oracle and native legs use the same boundary.
 * The startup callee and dynamic camera destructors are not adopted here. */
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x80044ad8u)return bridge_clear_otag_r(runtime,cpu);
 uint32_t a0=cpu->gpr[4];
 if(target==0x8001c944u){
  if(runtime_write(runtime,0x8005958cu,4,0)||runtime_write(runtime,0x80059594u,4,0)||runtime_write(runtime,0x80059188u,4,0)||runtime_write(runtime,0x8005918cu,4,0)||runtime_write(runtime,0x80059428u,4,0))return -1;
 } else if(target==0x80024f64u){
  if(a0!=0x5000u||cpu->gpr[5])fail("work buffer arguments",a0);
  if(runtime_write(runtime,0x800592fcu,4,a0)||runtime_write(runtime,0x800594b4u,4,0x80180000u)||runtime_write(runtime,0x800594b8u,4,0x80185000u))return -1;
 } else if(target==0x801e7098u){
  if(runtime_write(runtime,0x80059480u,4,a0^0xfedcba98u))return -1;
 } else if(target==0x80038310u){
  if(a0!=0x80150100u)fail("sound token",a0);
  if(runtime_write(runtime,0x80150100u,4,0))return -1;
 } else if(target==0x80044534u){
  if(a0!=1)fail("display argument",a0);
  if(runtime_write(runtime,0x80059484u,4,a0))return -1;
 } else if(target==0x80110000u||target==0x80110010u){
  if(runtime_write(runtime,a0+4u,4,0xabcddcbau))return -1;
 } else fail("unexpected service",target);
 /* Deliberately clobber all volatile registers and HI/LO/CP0 to expose
  * callers that accidentally retain stale values across a service. */
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;
 return 1;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==targets[kind]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);if(bi>=0&&target!=targets[bi])return 0;
 record_call(cpu,target);
 if(stop_at&&call_count==stop_at){
  if(stop_mode==2){fixture_store(0x80059488u,4,0x76543210u);cpu->gpr[2]=0x87654321u;cpu->gpr[6]=0x13572468u;cpu->hi=0x12345678u;cpu->lo=0xabcdef01u;cpu->cp0[3]=0x23456789u;}
  return -1;
 }
 int rc=bi>=0?(candidate?runtime_bridge(opaque,cpu,target):0):service(opaque,cpu,target);
 if(pending_return&&call_count==pending_return&&rc==1){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return rc;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"real-lower-shared"};
 rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800fe0u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;break;
 case 4:cpu->gpr[29]=0x800d30a0u+0x20u;break;case 5:cpu->gpr[29]=0x800658c8u+0x20u;break;
 case 6:cpu->gpr[29]=0x800c3d58u+0x20u;break;case 7:cpu->gpr[29]++;break;
 case 8:cpu->gpr[29]=0x1e000000u;break;case 9:cpu->gpr[29]=0;break;case 10:cpu->gpr[29]=4;break;
 }
 source_address=domain_kind==1?0xa0140000u:domain_kind==2?0x80062000u:domain_kind==3?(uint32_t)(uintptr_t)host:domain_kind==4?0x1f800100u:domain_kind==5?0x800d2c1eu:0x80140000u;
 for(unsigned n=0;n<(mutation==1?5u:1u);n++)for(unsigned j=0;j<6;j++)fixture_store(source_address+n*0x1000u+0x47cu+j*2u,2,(seed*0x7349u+j*0x7351u+n*0x5789u)^0x8765u);
 fixture_store(0x800658c8u,4,source_address);fixture_store(0x800595acu,4,0x80150100u);
 fixture_store(0x800c3680u,4,variant&1u?0x80160100u:0);fixture_store(0x800c3684u,4,variant&2u?0x80160200u:0);
 fixture_store(0x8016010cu,4,0x80110000u);fixture_store(0x8016020cu,4,0x80110010u);
 fixture_store(0x800ccb00u,4,variant&4u?0x800c4a20u:0x800c8a90u);
 for(unsigned i=3;i<11;i++){
  fixture_store(0x800c3eb6u+i*0x1cu,1,presence&(1u<<(i-3u))?(seed%2u?16:0):(seed%2u?17:255));
  fixture_store(0x800c3eb8u+i*0x1cu,1,variant&8u?(i%2u?0:0xffu):0x80u);
 }
 cpu->gpr[4]=kind==4?source_address:kind==11?camera_mode:0x13572468u;
 cpu->gpr[5]=0x00008001u+seed;cpu->gpr[6]=0xffff7fffu-seed;cpu->gpr[7]=0x1234ffffu;
 if(kind==11&&camera_mode==2)for(unsigned j=0;j<7;j++)fixture_store(0x800d30a0u+j*2u,2,0x8000u+j*0x7531u+seed);
 accesses=call_count=mutation_seen=legacy_calls=source_reads=count_reads=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,targets[kind],BATTLE_HALT_PC,200000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,targets[kind],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_at&&stack_kind<4&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
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
 for(kind=0;kind<NUM_BODIES;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  for(unsigned i=0;i<3;i++){
   uint8_t *code=kind==4?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_post_resource_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  if(kind==3)for(unsigned m=0;m<2;m++){
   uint8_t *marker=PSX_ADDR(m?0x801e5840u:0x801e4000u);marker[m?227:31]^=1;
   if(battle_post_resource_try_host(&rt,&cpu,target))fail("startup identity accepted",m);
   marker[m?227:31]^=1;
  }
  if(kind==4){XenoRetailFileSlot saved=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_post_resource_try_host(&rt,&cpu,target))fail("missing vector asset",0);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+15u;
   if(battle_post_resource_try_host(&rt,&cpu,target))fail("short vector asset",0);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_post_resource_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_post_resource_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_post_resource_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_post_resource_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_post_resource_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_post_resource_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_POST_RESOURCE","0",1)==0);
  if(battle_post_resource_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_POST_RESOURCE")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("POST RESOURCE PASS full original bodies, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<NUM_BODIES;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"post_resource"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=kind==4?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_POST_RESOURCE","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_POST_RESOURCE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1&&kind!=4){want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("POST RESOURCE PASS 144 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}
static int faults(void)
{
 unsigned total=0;seed=stack_kind=mutation=stop_at=0;presence=255;variant=15;
 for(kind=0;kind<NUM_BODIES;kind++)for(unsigned mode=0;mode<(kind==11?3u:1u);mode++){
  camera_mode=mode==1?2:mode==2?4:0;fault_at=0;run_case();unsigned n=expected_accesses;
  Access reference[8192];memcpy(reference,trace,n*sizeof(Access));
  for(unsigned f=1;f<=n;f++){
   /* ClearOTagR writes 4096 equivalent entries; sample first/interior/last
    * faults there and exhaust every access in all newly adopted bodies. */
   uint32_t a=reference[f-1].address;
   if(reference[f-1].write&&a>=0x800c8b00u&&a<0x800ccb00u&&a!=0x800c8b00u&&a!=0x800cab00u&&a!=0x800ccafcu)continue;
   if(reference[f-1].write&&a>=0x800c4a90u&&a<0x800c8a90u&&a!=0x800c4a90u&&a!=0x800c6a90u&&a!=0x800c8a8cu)continue;
   fault_at=f;run_case();total++;
  }
  fault_at=0;
 }
 kind=variant=camera_mode=0;printf("POST RESOURCE PASS %u ordered access faults (OT bridge interior sampled)\n",total);return 1;
}
static int pending(void)
{
 kind=3;variant=stack_kind=mutation=stop_at=0;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 uint32_t flag=0;assert(model_test_raw_read(&rt,0x800591afu,1,&flag)==0);
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||flag!=1||cpu.gpr[31]!=0x801e62fcu||cpu.hi!=0xa7650001u)fail("pending return refusal/partial state",(uint32_t)rc);
 puts("POST RESOURCE PASS pending return preserves completed callee effects and leaves startup flag set");return 1;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");assert(f&&fread(module,1,sizeof(module),f)==sizeof(module)&&fclose(f)==0);
 f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x12304,SEEK_SET)==0&&fread(vector_body,1,16,f)==16&&fclose(f)==0);
 assert(XenoRetailData_File(XENO_RD_SLUS));
 for(unsigned i=0;i<NUM_BODIES;i++){
  uint8_t digest[32];assert(SHA256(original_body(i),sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,battle_post_resource_bodies[i].sha)==0);
 }
}
int main(int argc,char **argv)
{
 load_inputs();
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--renderer")==0){kind=1;variant=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--reset")==0){kind=2;variant=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--camera")==0){kind=11;camera_mode=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--callbacks")==0){kind=11;variant=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  return 2;
 }
 unsigned total=0;
 for(kind=0;kind<NUM_BODIES;kind++)for(variant=0;variant<16;variant++){seed=variant%4;stack_kind=variant%4;run_case();total++;}
 kind=1;stack_kind=0;
 for(presence=0;presence<256;presence++)for(seed=0;seed<2;seed++){variant=seed?12:0;run_case();total++;}
 kind=11;presence=255;
 const unsigned modes[]={0,1,2,3,4,5,0xffffffffu};
 for(unsigned m=0;m<7;m++)for(variant=0;variant<4;variant++){camera_mode=modes[m];run_case();total++;}
 kind=variant=camera_mode=0;
 for(domain_kind=0;domain_kind<6;domain_kind++)for(seed=0;seed<4;seed++){run_case();total++;}
 domain_kind=seed=0;
 printf("POST RESOURCE PASS %u original/native comparisons: complete chain, 256 enemy masks, camera modes, pointer domains\n",total);fflush(stdout);
 for(mutation=1;mutation<=3;mutation++){kind=mutation==1?0:mutation==2?1:11;variant=3;run_case();}mutation=0;
 for(kind=0;kind<NUM_BODIES;kind++)for(stack_kind=4;stack_kind<11;stack_kind++){variant=0;run_case();}
 stack_kind=kind=variant=0;run_case();unsigned n=expected_calls;
 for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++)run_case();
 stop_at=stop_mode=0;
 printf("POST RESOURCE PASS 3 live mutations, 84 stack aliases/malformed frames and %u callee refusals\n",n*2);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("POST RESOURCE ALL PASS native interpreted instructions=0; external work/graphics/audio/startup/callback services modeled");return 0;
}
