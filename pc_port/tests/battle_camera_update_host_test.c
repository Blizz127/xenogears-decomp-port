/* Complete original battle camera update and view wrapper comparisons.
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
#include "psx/gtereg.h"
static GTERegisters want_gte;
static int candidate;
static unsigned oracle_gte_commands,native_gte_commands;
extern int color_fade_backend_command(int);
int doCOP2(int op)
{if(candidate)native_gte_commands++;else oracle_gte_commands++;return color_fade_backend_command(op);}

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
static const uint32_t targets[]={0x800bbab8u,0x800bb9d4u};
static const unsigned sizes[]={1064,228};
#define NUM_BODIES 2u
#define NEW_BODIES 2u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,mutation,mutation_seen,vector_case;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int compare,observing;
static uint32_t owner,camera_mode,smooth_flag=1,factor=4096,radius=12345,angle=1024,skip_effects;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[64];
static PcPortMipsCpu pending_cpu;
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE CAMERA UPDATE HOST FAIL kind=%u seed=%u stack=%u domain=%u mode=%08x smooth=%08x factor=%08x radius=%08x angle=%08x vector=%u skip=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,seed,stack_kind,task_domain,camera_mode,smooth_flag,factor,radius,angle,vector_case,skip_effects,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<NUM_BODIES;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static const uint8_t *original_body(unsigned i)
{return battle+(targets[i]-0x8006faf0u);}
static void fixture_store(uint32_t a,unsigned w,uint32_t v)
{if(model_test_raw_write(&rt,shard_sdk_address(a),w,v)!=0)fail("fixture write",a);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){assert(!candidate&&width==4);*value=load_le(original_body((unsigned)bi)+(address-targets[bi]),4);return 0;}
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access capacity",accesses);
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
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<64);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
 row->target=target;row->hi=cpu->hi;row->lo=cpu->lo;row->gte=gteRegs;
 memcpy(row->gpr,cpu->gpr,sizeof(row->gpr));memcpy(row->cp0,cpu->cp0,sizeof(row->cp0));
 memcpy(row->ram,g_PsxRam,sizeof(g_PsxRam));memcpy(row->shared,shared,sizeof(shared));
 memcpy(row->pad,g_PsxScratchpad,sizeof(g_PsxScratchpad));memcpy(row->host,host,sizeof(host));
 if(compare && (call_count>=expected_calls||memcmp(row,&calls[call_count],sizeof(*row)))){
  if(call_count<expected_calls)for(unsigned i=0;i<32;i++)if(row->gpr[i]!=calls[call_count].gpr[i])fprintf(stderr,"call register %u native=%08x oracle=%08x\n",i,row->gpr[i],calls[call_count].gpr[i]);
  fail("callee registers/full memory",target);
 }
 call_count++;
}

/* External callees are models. Complete original/native caller registers,
 * ordered accesses and all memory compare independently at every call. */
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 static const uint32_t allowed[]={0x800bc460u,0x8003f8ccu,0x8003f8b0u,0x8004a414u,0x80048c4cu,0x8004b32cu,0x800bb844u,0x800a4654u};
 unsigned found=0;for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);i++)if(target==allowed[i])found=1;
 /* Camera stores can overwrite this deliberately aliased saved return. */
 if(!found){if(stack_kind==5)return -1;fail("unexpected modeled callee",target);}
 uint32_t result=0x43210000u+call_count,site=cpu->gpr[31]-8u;
 if(target==0x8003f8ccu)result=seed&1u?0xfffff801u:0x00001801u;
 if(target==0x8003f8b0u)result=seed&2u?0x80001234u:0xfffff123u;
 if(target==0x80048c4cu)result=(cpu->gpr[4]>>2)^0x12345678u;
 if(target==0x8004b32cu)result=(cpu->gpr[4]*3u)^(cpu->gpr[5]*5u)^0x23456789u;
 if(target==0x8004a414u){
  uint32_t input[3];for(unsigned i=0;i<3;i++)if(runtime_read(runtime,cpu->gpr[4]+i*4u,4,&input[i]))return -1;
  for(unsigned i=0;i<3;i++){int32_t v=(int16_t)input[i];if(runtime_write(runtime,cpu->gpr[5]+i*4u,4,(uint32_t)(v*v)))return -1;}
  result=cpu->gpr[5];
 }
 if(target==0x800bb844u)for(unsigned i=0;i<3;i++)if(runtime_write(runtime,cpu->gpr[4]+0x14u+i*4u,4,0x7fff8000u+i*0x12345u+seed))return -1;
 if(runtime_write(runtime,0x80059488u,4,target^call_count))return -1;
 if(mutation==1&&target==0x800bc460u){fixture_store(0x800c3cbcu,4,1);fixture_store(0x800c3674u,4,0);fixture_store(0x800d30a0u,2,0xff01u);mutation_seen++;}
 if(mutation==2&&target==0x8003f8ccu){fixture_store(0x800c373cu,2,0x8123u);fixture_store(0x800c3738u,4,0x12345678u);mutation_seen++;}
 if(mutation==3&&target==0x8003f8b0u){fixture_store(0x800c3738u,4,0xfedcba98u);fixture_store(owner+6u,2,0x7654u);mutation_seen++;}
 if(mutation==4&&target==0x8004a414u){fixture_store(cpu->gpr[29]+0x44u,4,0x80000001u);mutation_seen++;}
 if(mutation==5&&site==0x800bbe6cu){fixture_store(cpu->gpr[29]+0x64u,4,0x89abcdefu);mutation_seen++;}
 if(mutation==6&&target==0x800bb844u){fixture_store(0x800c354cu,2,0x8001u);fixture_store(0x800c372cu,1,0);fixture_store(0x800ccb04u,4,0xa0123450u);fixture_store(0x800ccb34u,4,0x80000001u);mutation_seen++;}
 if(mutation==7&&site==0x800bbea0u){fixture_store(cpu->gpr[29]+0x44u,4,0x76543210u);mutation_seen++;}
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=result;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;
 gteRegs.CP2D.r[9]=target^call_count;
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
 if(pending_return&&call_count==pending_return&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return rc;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};
 rt.data[1]=(ResolvedData){0x8006f99cu,16u,shared+0x1f020u,"real-eye-owner"};
 rt.data[2]=(ResolvedData){0x8006f9acu,16u,shared+0x1f040u,"real-target-owner"};
 rt.data[3]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=4;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 owner=task_domain==0?0x80100000u:task_domain==1?0xa0100000u:task_domain==2?0x80050000u:
  task_domain==3?(uint32_t)(uintptr_t)host:task_domain==4?0x00100000u:task_domain==5?0x80300000u:
  task_domain==6?0x1f800000u:task_domain==7?0x1e000000u:task_domain==8?0:0x80100001u;
 static const int16_t vectors[][6]={{100,-200,300,-400,500,-600},{0,0,0,0,0,0},{-32768,32767,-1,32767,-32768,1},{1,-1,1,2,-2,2},{300,0,-300,300,1,-300},{32760,-32760,123,-32760,32760,-321}};
 const int16_t *v=vectors[vector_case%6u];
 for(unsigned i=0;i<3;i++){
  if(task_domain<7)fixture_store(owner+i*4u,4,((uint32_t)(uint16_t)v[i]<<16)|(0xfaceu+i));
  fixture_store(0x8006f99cu+i*4u,4,((uint32_t)(uint16_t)v[i]<<16)|(0x1234u+i));
  fixture_store(0x8006f9acu+i*4u,4,((uint32_t)(uint16_t)v[i+3u]<<16)|(0x5678u+i));
  fixture_store(0x800d30a0u+i*2u,2,(uint16_t)v[i]);fixture_store(0x800d30a8u+i*2u,2,(uint16_t)v[i+3u]);
  fixture_store(0x800d3354u+i*2u,2,(uint16_t)(vector_case==3?v[i]-1:v[i+3u]));
  fixture_store(0x800d335cu+i*2u,2,(uint16_t)(vector_case==3?v[i+3u]+1:v[i]));
  fixture_store(0x800c354cu+i*2u,2,(uint16_t)(v[i]+seed));
 }
 fixture_store(0x800d39ecu,4,owner);fixture_store(0x800c3cc0u,4,camera_mode);fixture_store(0x800c3cbcu,4,smooth_flag);
 fixture_store(0x800c3674u,4,factor);fixture_store(0x800c373cu,2,angle);fixture_store(0x800c3738u,4,radius);
 fixture_store(0x800c3678u,4,0x4321fedcu);fixture_store(0x800c372cu,1,skip_effects);
 fixture_store(0x800ccb04u,4,0x800c4a90u);fixture_store(0x800ccb34u,4,seed&1u);
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800f80u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;break;
 case 4:cpu->gpr[29]=owner+0x40u;break;case 5:cpu->gpr[29]=0x800d30b0u;break;
 case 6:cpu->gpr[29]++;break;case 7:cpu->gpr[29]=0x1e000000u;break;case 8:cpu->gpr[29]=0;break;
 }
 accesses=call_count=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=0;observing=1;
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
 if(!fault_at&&!stop_at&&stack_kind<4&&task_domain<4&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||native_gte_commands||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen&&!fault_at&&!stop_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static int controls(void)
{
 for(kind=0;kind<NEW_BODIES;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  for(unsigned i=0;i<3;i++){
   uint8_t *code=PSX_ADDR(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_camera_update_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_camera_update_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_camera_update_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_camera_update_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_camera_update_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_camera_update_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_camera_update_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_CAMERA_UPDATE","0",1)==0);if(battle_camera_update_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_CAMERA_UPDATE")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("BATTLE CAMERA UPDATE HOST PASS full identities, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<NEW_BODIES;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"timer_task"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_CAMERA_UPDATE","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_CAMERA_UPDATE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1&&kind<3){want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("BATTLE CAMERA UPDATE HOST PASS 24 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}

static void defaults(void)
{
 kind=seed=stack_kind=task_domain=mutation=vector_case=skip_effects=0;
 stop_at=stop_mode=pending_return=fault_at=0;camera_mode=0;smooth_flag=1;factor=4096;radius=12345;angle=1024;
}
static int faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<8;path++){
  defaults();kind=path>=6;camera_mode=path%4u;vector_case=path%6u;seed=path;smooth_flag=path!=0;
  if(path==4)factor=0;
  if(path==5){camera_mode=3;task_domain=3;stack_kind=3;}
  if(path==7){skip_effects=1;mutation=6;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("BATTLE CAMERA UPDATE HOST PASS %u ordered access faults with full CPU/memory state\n",total);return 1;
}
static int pending(void)
{
 for(unsigned path=0;path<4;path++){
  defaults();kind=path&1u;camera_mode=3;PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=path<2?1:2;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=pending_return||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal())fail("pending return refusal/partial state",(uint32_t)rc);
 }
 defaults();puts("BATTLE CAMERA UPDATE HOST PASS 4 pending returns refuse with completed callee state");return 1;
}
static int graphics(void)
{
 static const uint32_t words[]={0,1,0xfff,0x1000,0x7fff,0x8000,0xffff,0x12340001u,0x7fff8000u,0xffff8000u,0x80017fffu,0xffff0001u};
 unsigned total=0;
 for(unsigned a=0;a<12;a++)for(unsigned b=0;b<12;b++)for(unsigned c=0;c<3;c++){
  uint32_t delta[3]={words[b],words[(b+3u+c)%12u],words[(b+7u+c)%12u]};
  memset(&gteRegs,(int)(0xa5u+a+b),sizeof(gteRegs));MTC2(words[a],8);
  /* Distinct FIFO entries make either missing shift observable. */
  MTC2(0x12345678u+a,20);MTC2(0x89abcdefu+b,21);MTC2(0x76543210u+c,22);
  for(unsigned i=0;i<3;i++)MTC2(delta[i],(int)(9u+i));
  GTERegisters start=gteRegs;assert(color_fade_backend_command(0x4b98003d)==1);GTERegisters expected=gteRegs;
  gteRegs=start;battle_camera_update_product(words[a],delta);
  if(memcmp(&gteRegs,&expected,sizeof(gteRegs)))fail("fixed GPF12 complete registers",a*100u+b);
  total++;
 }
 printf("BATTLE CAMERA UPDATE HOST PASS %u fixed GPF12 full graphics-state cases\n",total);return 1;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 for(unsigned i=0;i<NEW_BODIES;i++){
  uint8_t digest[32];assert(SHA256(original_body(i),sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,battle_camera_update_bodies[i].sha)==0);
 }
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--orbit")==0){camera_mode=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--graphics")==0)return graphics()?0:1;
  return 2;
 }
 unsigned total=0;static const uint32_t words[]={0,1,0xfff,0x1000,0x7fff,0x8000,0xffff,0xffffffffu,0x80000000u,0x7fffffffu,0x12348001u,0xffff7fffu};
 for(kind=0;kind<2;kind++)for(seed=0;seed<12;seed++)for(task_domain=0;task_domain<7;task_domain++)for(unsigned option=0;option<2;option++){
  camera_mode=seed%4u;smooth_flag=option;skip_effects=option;stack_kind=seed%4;vector_case=seed%6u;
  factor=words[seed];radius=words[(seed+2u)%12u];angle=words[(seed+5u)%12u];run_case();total++;
 }
 defaults();
 for(unsigned i=0;i<12;i++){camera_mode=words[i];smooth_flag=words[(i+1u)%12u];run_case();total++;}
 defaults();
 for(kind=0;kind<2;kind++){
  camera_mode=3;
  for(stack_kind=4;stack_kind<9;stack_kind++){run_case();total++;}stack_kind=0;
  for(task_domain=7;task_domain<10;task_domain++){run_case();total++;}task_domain=0;
 }
 defaults();
 for(mutation=1;mutation<=7;mutation++){kind=mutation==6;camera_mode=mutation==1?1:3;smooth_flag=mutation==1?0:1;skip_effects=mutation==6;run_case();total++;}
 defaults();
 printf("BATTLE CAMERA UPDATE HOST PASS %u original/native comparisons, modes, vectors, pointer domains and live changes\n",total);fflush(stdout);
 unsigned refusals=0;
 for(unsigned path=0;path<6;path++){
  defaults();kind=path>=4;camera_mode=path%4u;vector_case=path%6u;skip_effects=path==5;
  run_case();unsigned n=expected_calls;
  for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();refusals++;}
 }
 defaults();printf("BATTLE CAMERA UPDATE HOST PASS %u callee refusals with exact partial state\n",refusals);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries()||!graphics())return 1;
 puts("BATTLE CAMERA UPDATE HOST ALL PASS native interpreted instructions=0 and graphics commands=0; external services modeled");return 0;
}
