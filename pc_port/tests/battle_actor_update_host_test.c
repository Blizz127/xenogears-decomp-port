/* Complete original actor update/render coordinator comparisons.
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
static const uint32_t targets[]={0x800a9a50u};
static const unsigned sizes[]={1348};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,mutation,mutation_seen,arg_case,scenario;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int compare,observing;
static unsigned hash_call_snapshots;
static uint32_t owner,record_base,records_count=2,light_on,models_on,ot_count=0x1000u;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[64];
/* Large-count paths keep full-state hashes at each call and retain the usual
 * byte-exact final-memory comparison, without 900 MiB of call snapshots. */
typedef struct HashedCall {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[32],shared[32],pad[32],host[32];} HashedCall;
static HashedCall hashed_calls[264];
static PcPortMipsCpu pending_cpu;
static uint32_t actor_base,setup_base=0x80150000u;
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE ACTOR UPDATE HOST FAIL seed=%u stack=%u domain=%u records=%u light=%u models=%u OT=%08x args=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",seed,stack_kind,task_domain,records_count,light_on,models_on,ot_count,arg_case,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
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
 if(hash_call_snapshots){
  assert(call_count<264);HashedCall row;memset(&row,0,sizeof(row));
  row.target=target;row.hi=cpu->hi;row.lo=cpu->lo;row.gte=gteRegs;
  memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
  assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
  assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
  if(compare){if(call_count>=expected_calls||memcmp(&row,&hashed_calls[call_count],sizeof(row)))fail("large-count callee state hashes",target);}
  else hashed_calls[call_count]=row;
  call_count++;return;
 }
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

static uint32_t peek(uint32_t a,unsigned w)
{uint32_t v;if(model_test_raw_read(&rt,a,w,&v))fail("fixture read",a);return v;}
static void actor_record(unsigned i)
{
 uint32_t actor=actor_base+i*0x80u,body=actor_base+(records_count==32?0x3000u:0x800u)+i*0x80u;
 fixture_store(actor+4u,4,body);fixture_store(actor+0x4au,2,scenario==2&&i==0?32u:i&1u?64u:0);
 fixture_store(actor+0x34u,1,scenario==6?0:1);fixture_store(actor+0x5cu,1,scenario==14?254u:scenario==7?0:i&1u?255u:0);
 fixture_store(body+0x60u,4,scenario==12?(i?0x80000000u:0x7fffffffu):100u+i*20u);
 fixture_store(body+0x5cu,4,0x80123456u+i*123u);fixture_store(body+0x64u,4,0xfedcba98u-i*456u);
}
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 static const uint32_t allowed[]={0x8003f8ccu,0x800aa514u,0x800aa600u,0x800aa650u,0x800b10ecu,0x800a4cf8u,0x800aaa20u,0x800b026cu,0x800aab34u,0x800a44c0u,0x80049f5cu,0x8004a0ecu,0x8009f844u,0x800a2fd8u};
 unsigned found=0;for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);i++)if(target==allowed[i])found=1;
 if(!found)fail("unexpected modeled callee",target);
 uint32_t a=cpu->gpr[4],value=0x43210000u+call_count;
 if(runtime_write(runtime,0x80059488u,4,target^call_count))return -1;
 if(target==0x8003f8ccu){static const uint32_t wave[]={0xfffff000u,0xfffff001u,0,4096,0x80000000u,0x7fffffffu,0xffffefe0u,0xffffffffu};value=wave[(seed+call_count-1u)%8u];}
 if(target==0x8003f8ccu&&scenario==13)value=0xffffefffu;
 if(target==0x800aa514u)value=(31u+call_count*67u)&255u;
 if(target==0x800aa600u)value=scenario==10?0:scenario==11&&a==0?0:80u;
 if(target==0x800aa650u)value=scenario==12?0x7fffffffu:40u+a;
 if(mutation==1&&target==0x800aa514u&&!mutation_seen){fixture_store(0x800658c8u,4,setup_base+0x800u);fixture_store(0x800c3b80u,2,0xff81u);mutation_seen++;}
 if(mutation==2&&target==0x800aa600u&&!mutation_seen){fixture_store(0x800c3d40u,2,30);mutation_seen++;}
 if(mutation==3&&target==0x800aa600u&&!mutation_seen){fixture_store(0x800d336cu,4,actor_base+4u*0x80u);mutation_seen++;}
 if(mutation==4&&target==0x800aaa20u&&!mutation_seen){fixture_store(0x800ccc5cu,4,0xabcdef01u);mutation_seen++;}
 if(mutation==5&&target==0x800aaa20u&&!mutation_seen){fixture_store(0x800d3370u,4,actor_base+4u*0x80u);mutation_seen++;}
 if(mutation==6&&target==0x800aaa20u&&!mutation_seen){fixture_store(0x800c3df8u,1,0);mutation_seen++;}
 if(mutation==7&&target==0x800a44c0u){fixture_store(0x800c3b74u,1,0);mutation_seen++;}
 if(mutation==8&&target==0x800a44c0u){fixture_store(0x800d2fc0u,4,0xa0160400u);mutation_seen++;}
 if(mutation==9&&target==0x8004a0ecu&&!mutation_seen){fixture_store(0x800d3368u,4,actor_base+4u*0x80u);mutation_seen++;}
 if(mutation==10&&target==0x800aa600u&&a==0&&!mutation_seen){fixture_store(actor_base+0x80u+4u,4,actor_base+0x800u+4u*0x80u);mutation_seen++;}
 if(mutation==11&&target==0x8009f844u&&!mutation_seen){fixture_store(cpu->gpr[29]+0x20u,1,0xef);fixture_store(setup_base+0x475u,1,0xd1);mutation_seen++;}
 if(mutation==12&&target==0x800b10ecu&&!mutation_seen){fixture_store(0x800c3d40u,2,30);mutation_seen++;}
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=value;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;
 gteRegs.CP2D.r[9]=target^call_count;return 1;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==targets[0]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);if(bi>=0&&target!=targets[bi])return 0;
 record_call(cpu,target);
 if(stop_at&&call_count==stop_at){
  if(stop_mode==2){fixture_store(0x80059488u,4,0x76543210u);cpu->gpr[2]=0x87654321u;cpu->gpr[6]=0x13572468u;cpu->hi=0x12345678u;cpu->lo=0xabcdef01u;cpu->cp0[3]=0x23456789u;}
  return -1;
 }
 int rc=service(opaque,cpu,target);
 if(pending_return&&call_count==pending_return&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return rc;
}
static void defaults(void)
{kind=seed=stack_kind=task_domain=mutation=arg_case=hash_call_snapshots=0;stop_at=stop_mode=pending_return=fault_at=0;records_count=4;light_on=models_on=0;ot_count=0x1000u;scenario=1;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 owner=task_domain==0?0x80100000u:task_domain==1?0xa0100000u:task_domain==2?0x80300000u:task_domain==3?0x80060000u:task_domain==4?0x1f800000u:(uint32_t)(uintptr_t)host;
 actor_base=owner+0x100u;record_base=actor_base+0x800u;
 for(unsigned i=0;i<32;i++)fixture_store(0x800d3368u+i*4u,4,0);
 unsigned count=scenario==0?0:records_count;
 for(unsigned i=0;i<count;i++){
  actor_record(i);unsigned slot=records_count==32?i:i<2?i:i+28u;
  fixture_store(0x800d3368u+slot*4u,4,actor_base+i*0x80u);
 }
 if(records_count!=32)actor_record(4);
 if(scenario==3)fixture_store(0x800d3368u,4,0);
 fixture_store(0x800c3d40u,2,scenario==4?1:0);fixture_store(0x800c3e30u,2,scenario==15?2u:scenario==5?0xffff:0);
 static const uint32_t cadence[]={0,1,5,6,0xffffffffu,0x7fffffffu,0x80000000u,0xfffffff9u};
 static const uint32_t rate[]={0,1,2,0xffffffffu,6,0x80000000u,0x7fffffffu,17};
 fixture_store(0x800c3e88u,4,cadence[seed%8u]);fixture_store(0x800ccc5cu,4,rate[(seed+arg_case)%8u]);
 fixture_store(0x800c3cf0u,2,0x7ff0u+seed*128u);fixture_store(0x800c3b7cu,2,0xff80u+seed*63u);
 fixture_store(0x800c3df8u,1,scenario==9?0:1);fixture_store(0x800c3b74u,1,scenario==8?0:1);fixture_store(0x800c3d14u,2,0x5555u);
 fixture_store(0x800658c8u,4,setup_base);fixture_store(0x800d2fc0u,4,0x80160000u);
 for(unsigned i=0;i<3;i++){fixture_store(setup_base+0x474u+i,1,30u+i*99u);fixture_store(setup_base+0xc74u+i,1,171u-i*57u);}
 cpu->gpr[4]=0xa0161000u;cpu->gpr[5]=0x80162000u;cpu->gpr[6]=0x80163000u;cpu->gpr[7]=0x80000001u+seed;
 if(stack_kind==1)cpu->gpr[29]=0x1f800f80u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 accesses=call_count=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;
 int original=PcPortMipsRun(&oracle,targets[0],BATTLE_HALT_PC,200000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int actual=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET||(original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("status",native.pc);
 if(!fault_at&&!stop_at&&stack_kind<4&&original!=PC_PORT_MIPS_HALTED)fail("unexpected original fault",oracle.pc);
 if(native.steps||native_gte_commands||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x original=%08x original_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",native.hi);
 if(original==PC_PORT_MIPS_HALTED&&(native.load_valid||oracle.load_valid))fail("terminal pending load",native.load_reg);
 if(mutation&&!mutation_seen&&!fault_at&&!stop_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();uint32_t target=targets[0];uint8_t *code=PSX_ADDR(target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?674:1347;code[off]^=1;if(battle_actor_update_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 cpu.load_valid=1;cpu.load_reg=4;if(battle_actor_update_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_actor_update_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_actor_update_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_actor_update_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_actor_update_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_actor_update_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_ACTOR_UPDATE","0",1)==0);if(battle_actor_update_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_ACTOR_UPDATE")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 return total;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[0];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"actor_update"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ACTOR_UPDATE","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ACTOR_UPDATE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("canonical cache routing",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   if(reject==1)want_ram[target&0x1fffffu]^=1;
   if(rc!=(disabled?PC_PORT_MIPS_INTERPRETER_DISABLED:0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected cache routing",(uint32_t)rc);
  }
  total++;
 }
 return total;
}
static unsigned faults(void)
{
 static const unsigned paths[]={0,1,2,5,8,10,11,12};unsigned total=0;
 for(unsigned i=0;i<8;i++){
  defaults();scenario=paths[i];task_domain=i%6u;stack_kind=i%4u;seed=i;
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static unsigned refusals(void)
{
 defaults();run_case();unsigned n=expected_calls,total=0;
 for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();total++;}
 defaults();
 for(pending_return=1;pending_return<=n;pending_return++){
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,targets[0],BATTLE_HALT_PC,200000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||call_count!=pending_return||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("pending child refusal",pending_return);
  total++;
 }
 defaults();return total;
}
static unsigned semantics(void)
{
 defaults();run_case();unsigned updates=0,prepares=0,draws=0,back_colors=0;
 for(unsigned i=0;i<expected_calls;i++){
  if(calls[i].target==0x800aaa20u)updates++;
  if(calls[i].target==0x800aab34u)prepares++;
  if(calls[i].target==0x8004a0ecu)back_colors++;
  if(calls[i].target==0x8009f844u){draws++;if(calls[i].gpr[4]==actor_base+3u*0x80u)fail("slot 31 rendered",i);}
 }
 if(updates!=4||prepares!=2||draws!=3||back_colors!=3||peek(0x80050104u,4))fail("loop bounds / draw flag",updates);
 unsigned total=1;
 for(unsigned s=0;s<8;s++)for(unsigned a=0;a<8;a++){
  defaults();scenario=0;seed=s;arg_case=a;run_case();
  static const uint32_t cadence[]={0,1,5,6,0xffffffffu,0x7fffffffu,0x80000000u,0xfffffff9u};
  static const uint32_t rate[]={0,1,2,0xffffffffu,6,0x80000000u,0x7fffffffu,17};
  uint32_t v=cadence[s]+1u+rate[(s+a)%8u];if((int32_t)v>=7)v=6;
  unsigned ticks=0;while((int32_t)v>=2){v-=2;ticks++;}
  if(peek(0x800c3e88u,4)!=v||peek(0x800c3cf0u,2)!=(uint16_t)(0x7ff0u+s*128u+ticks*56u))fail("signed cadence witness",s);
  total++;
 }
 defaults();return total;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(original_body(0),sizes[0],digest));char value[65];
 for(unsigned i=0;i<32;i++)snprintf(value+i*2,3,"%02x",digest[i]);
 assert(!strcmp(value,"44c84dd358551240285c22dec8b58eeecb1a4869f13092b8b283ff6f2a60f3f2"));
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--faults")){printf("ACTOR UPDATE FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("ACTOR UPDATE SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("ACTOR UPDATE GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("ACTOR UPDATE CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 for(seed=0;seed<8;seed++)for(task_domain=0;task_domain<6;task_domain++){scenario=seed%13u;stack_kind=seed%4u;run_case();total++;}
 for(unsigned s=0;s<16;s++){defaults();scenario=s;run_case();total++;}
 for(unsigned m=1;m<=12;m++){defaults();mutation=m;run_case();total++;}
 for(unsigned s=4;s<7;s++){defaults();stack_kind=s;run_case();total++;}
 defaults();records_count=32;hash_call_snapshots=1;run_case();total++;defaults();
 printf("ACTOR UPDATE BODY PASS %u full comparisons, packed actors, live mutations and dense slot arrays\n",total);fflush(stdout);
 printf("ACTOR UPDATE SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("ACTOR UPDATE FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("ACTOR UPDATE CHILD PASS %u refusals and pending returns\n",refusals());
 printf("ACTOR UPDATE GATES PASS %u\n",controls());printf("ACTOR UPDATE CACHE PASS %u\n",boundaries());
 puts("ACTOR UPDATE HOST ALL PASS; native interpreted instructions=0; all external children modeled");return 0;
}

