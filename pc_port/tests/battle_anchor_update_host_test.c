/* Complete original two-anchor updater comparisons.
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
static const uint32_t targets[]={0x800a44c0u};
static const unsigned sizes[]={404};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned seed,stack_kind,task_domain,mutation,mutation_seen,active_mask,bad_kind,alias_case;
static int32_t slots[2],bones[2];
static uint32_t models,model[2],bodies[2];
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static unsigned events,expected_events;
static int compare,observing;
static PcPortMipsCpu *oracle_cpu,pending_cpu;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[512];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[8];
typedef struct Event {uint32_t command,gpr[32],cp0[32],hi,lo;GTERegisters before,after;} Event;
static Event event_trace[2];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE ANCHOR UPDATE HOST FAIL seed=%u stack=%u domain=%u active=%u slots=%d/%d bones=%d/%d alias=%u bad=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u event=%u %s %08x\n",seed,stack_kind,task_domain,active_mask,slots[0],slots[1],bones[0],bones[1],alias_case,bad_kind,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,events,what,value);exit(1);
}
int doCOP2(int op)
{
 if(candidate)native_gte_commands++;else oracle_gte_commands++;
 if(!observing)return color_fade_backend_command(op);
 if(events>=2)fail("GTE trace capacity",events);
 PcPortMipsCpu *cpu=candidate?rt.bridge_cpu:oracle_cpu;if(!cpu)fail("GTE CPU context",0);
 Event row;memset(&row,0,sizeof(row));row.command=(uint32_t)op;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));row.hi=cpu->hi;row.lo=cpu->lo;row.before=gteRegs;
 int rc=color_fade_backend_command(op);row.after=gteRegs;
 if(compare){if(events>=expected_events||memcmp(&row,&event_trace[events],sizeof(row)))fail("GTE command and complete boundary state",(uint32_t)op);}
 else event_trace[events]=row;
 events++;return rc;
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
 assert(call_count<8);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
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
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x8004931cu&&target!=0x80049efcu&&target!=0x80049f8cu)fail("unexpected modeled callee",target);
 if(target==0x8004931cu){
  uint32_t matrix[8]={4096,0,4096,0,4096,11u+seed*17u,0xffffffe9u-seed*13u,37u+seed*19u};
  for(unsigned i=0;i<8;i++)if(runtime_write(runtime,cpu->gpr[6]+4u*i,4,matrix[i]))return -1;
  if(mutation==1){fixture_store(0x800d330cu,4,0x80007fffu);fixture_store(0x800d331eu,2,0);mutation_seen++;}
  if(mutation==4){fixture_store(models+4u,4,0);mutation_seen++;}
 }else{
  unsigned start=target==0x80049efcu?0:5,end=target==0x80049efcu?5:8;
  for(unsigned i=start;i<end;i++){uint32_t word;if(runtime_read(runtime,cpu->gpr[4]+4u*i,4,&word))return -1;CTC2(word,i);}
  if(target==0x80049efcu&&mutation==2){fixture_store(0x800d3328u,2,0xffffu);mutation_seen++;}
  if(target==0x80049efcu&&mutation==3){fixture_store(0x1f800014u,4,0xffffffb9u);mutation_seen++;}
  if(target==0x80049f8cu&&mutation==5){fixture_store(cpu->gpr[29]+0x10u,4,0x87654321u);fixture_store(0x800d3304u,2,0x1234u);mutation_seen++;}
 }
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
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
{
 seed=stack_kind=task_domain=mutation=bad_kind=alias_case=0;active_mask=3;
 slots[0]=0;slots[1]=1;bones[0]=0;bones[1]=1;
 stop_at=stop_mode=pending_return=fault_at=0;
}
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
 models=task_domain==0?0x80150000u:task_domain==1?0xa0150000u:task_domain==2?0x80350000u:task_domain==3?0x80060000u:task_domain==4?0x1f800100u:(uint32_t)(uintptr_t)host;
 model[0]=0x80170000u;model[1]=0xa0172000u;bodies[0]=0x80160000u;bodies[1]=0xa0168000u;
 if(task_domain==5){for(unsigned i=0;i<2;i++){model[i]=(uint32_t)(uintptr_t)host+0x10000u+i*0x2000u;bodies[i]=(uint32_t)(uintptr_t)host+0x20000u+i*0x8000u;}}
 if(stack_kind==1)cpu->gpr[29]=0x1f800e00u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 if(alias_case==1)models=cpu->gpr[29]-0x30u;
 if(alias_case==2)models=0x800d3300u;
 if(alias_case==3)cpu->gpr[29]=0x800d3334u;
 if(alias_case==4)models=0x1f800000u;
 if(alias_case==5)model[0]=0x800d3308u;
 if(alias_case==6)models=0x800d3318u;
 for(unsigned i=0;i<2;i++){
  uint32_t a=0x800d3304u+i*0x14u;
  fixture_store(a,4,0x76543210u);fixture_store(a+4u,2,0x9876u);
  fixture_store(a+6u,2,(active_mask&(1u<<i))?(seed&1u?0x8000u:1u):0);
  fixture_store(a+8u,2,5u+i*7u+seed*431u);fixture_store(a+10u,2,0xfffffff9u-i*3u-seed*713u);fixture_store(a+12u,2,9u+i*11u+seed*2311u);fixture_store(a+14u,2,0xabcd);
  fixture_store(a+16u,2,(uint32_t)slots[i]);fixture_store(a+18u,2,(uint32_t)bones[i]);
  fixture_store(model[i]+4u,4,bodies[i]);
 }
 for(unsigned i=0;i<2;i++)if(slots[i]>=0&&slots[i]<64)fixture_store(models+(uint32_t)slots[i]*4u,4,model[i]);
 if(bad_kind==1)models++;
 if(bad_kind==2)models=0x1e000000u;
 if(bad_kind==3)fixture_store(models,4,0);
 if(bad_kind==4)fixture_store(models,4,model[0]+1u);
 if(bad_kind==5)fixture_store(models,4,0x1e000000u);
 if(bad_kind==6)fixture_store(model[0]+4u,4,0);
 if(bad_kind==7)fixture_store(model[0]+4u,4,0xfffffff4u);
 if(bad_kind==8){models=(uint32_t)(uintptr_t)host+sizeof(host)-4u;fixture_store(models,4,model[0]);}
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 cpu->gpr[4]=models;
 accesses=call_count=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=events=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 int original=PcPortMipsRun(&oracle,targets[0],BATTLE_HALT_PC,20000);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;unsigned expected_commands=oracle_gte_commands;snapshot();
 setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int actual=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,20000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET||(original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("status",native.pc);
 if(native.steps||native_gte_commands!=expected_commands||rt.bridge_cpu||events!=expected_events||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x original=%08x original_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",native.hi);
 if(original==PC_PORT_MIPS_HALTED&&(native.load_valid||oracle.load_valid))fail("terminal pending load",native.load_reg);
 if(mutation&&!mutation_seen&&!fault_at&&!stop_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<10;path++){
  defaults();seed=path;task_domain=path%6u;stack_kind=path%4u;
  if(path<4)active_mask=path;
  if(path==4)slots[0]=-1;
  if(path==5)bad_kind=3;
  if(path==6)mutation=3;
  if(path>=7)alias_case=path-6;
  run_case();unsigned count=expected_accesses;
  for(fault_at=1;fault_at<=count;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned mask=0;mask<4;mask++)for(unsigned copy=0;copy<2;copy++){
  defaults();active_mask=mask;if(copy)slots[0]=slots[1]=-1;run_case();
  if(expected_calls!=(copy?0u:3u*((mask&1u)+((mask>>1)&1u)))||expected_events!=(copy?0u:(mask&1u)+((mask>>1)&1u)))fail("independent path counts",mask);
  for(unsigned i=0;i<2;i++){
   uint32_t a=0x800d3304u+i*0x14u;const int32_t local[3]={5+(int)i*7,-7-(int)i*3,9+(int)i*11},translation[3]={11,-23,37};
   for(unsigned j=0;j<3;j++){
    uint16_t expected=(mask&(1u<<i))?(uint16_t)(local[j]+(copy?0:translation[j])):(uint16_t[]){0x3210,0x7654,0x9876}[j];
    if(peek(a+2u*j,2)!=expected)fail("independent transformed/copied/untouched anchor",i*3u+j);
   }
  }
  total++;
 }
 defaults();return total;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();uint32_t target=targets[0];uint8_t *code=PSX_ADDR(target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?202:403;code[off]^=1;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[200],code[201]};
 assert(SHA256(code,404,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[200]=(uint8_t)value;code[201]=(uint8_t)(value>>8);
  if(code[200]==saved[0]&&code[201]==saved[1])continue;
  assert(SHA256(code,404,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_anchor_update_try_host(&rt,&cpu,target))fail("digest-prefix collision",0);
 code[200]=saved[0];code[201]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_anchor_update_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_anchor_update_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_ANCHOR_UPDATE","0",1)==0);if(battle_anchor_update_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_ANCHOR_UPDATE")==0);total++;
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
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"anchor_update"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ANCHOR_UPDATE","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ANCHOR_UPDATE")==0);
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
static unsigned refusals(void)
{
 unsigned total=0;
 for(unsigned path=0;path<3;path++){
  defaults();active_mask=path+1u;run_case();unsigned n=expected_calls;
  for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();total++;}
  stop_at=stop_mode=0;
  for(pending_return=1;pending_return<=n;pending_return++){
   PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
   PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,targets[0],BATTLE_HALT_PC,200000);
   assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
   if(rc>=0||cpu.steps||rt.bridge_cpu||call_count!=pending_return||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("pending child refusal",pending_return);
   total++;
  }
 }
 defaults();return total;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(original_body(0),sizes[0],digest));char value[65];
 for(unsigned i=0;i<32;i++)snprintf(value+i*2,3,"%02x",digest[i]);
 assert(!strcmp(value,"fdecae56f92c1249ed1d16967fb2af6e52e160794b451ae36547d80f1443c8cf"));
}

int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--faults")){printf("ANCHOR UPDATE FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("ANCHOR UPDATE SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("ANCHOR UPDATE GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("ANCHOR UPDATE CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 for(unsigned mask=0;mask<4;mask++)for(unsigned s=0;s<8;s++)for(unsigned d=0;d<6;d++){
  defaults();active_mask=mask;seed=s;task_domain=d;stack_kind=s%4u;run_case();total++;
 }
 const int32_t indices[]={-32768,-2,-1,0,1,2,63,32767};
 for(unsigned s=0;s<8;s++)for(unsigned b=0;b<8;b++){
  defaults();slots[0]=indices[s];bones[0]=indices[b];bones[1]=indices[7-b];run_case();total++;
 }
 for(unsigned alias=1;alias<=6;alias++)for(unsigned s=0;s<4;s++){defaults();alias_case=alias;seed=s;run_case();total++;}
 for(unsigned bad=1;bad<=8;bad++){defaults();bad_kind=bad;run_case();total++;}
 for(unsigned s=4;s<7;s++){defaults();stack_kind=s;run_case();total++;}
 for(unsigned m=1;m<=5;m++){defaults();mutation=m;run_case();total++;}
 printf("ANCHOR UPDATE BODY PASS %u full architecture/memory/GTE comparisons\n",total);fflush(stdout);
 printf("ANCHOR UPDATE SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("ANCHOR UPDATE FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("ANCHOR UPDATE CHILD PASS %u refusals/pending returns\n",refusals());fflush(stdout);
 printf("ANCHOR UPDATE GATES PASS %u\n",controls());printf("ANCHOR UPDATE CACHE PASS %u\n",boundaries());
 puts("ANCHOR UPDATE HOST ALL PASS; native interpreted instructions=0; three external matrix services modeled");return 0;
}
