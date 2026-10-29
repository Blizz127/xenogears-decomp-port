/* Complete original color-fade draw callback comparisons.
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
static const uint32_t targets[]={0x800b3878u};
static const unsigned sizes[]={260};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned seed,stack_kind,task_domain,mutation,mutation_seen,bad_kind,alias_case;
static int32_t gap;
static uint32_t task,owner,buffer,ot,end;
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
 fprintf(stderr,"BATTLE FADE DRAW HOST FAIL seed=%u stack=%u domain=%u gap=%d alias=%u bad=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",seed,stack_kind,task_domain,gap,alias_case,bad_kind,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
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
 uint32_t a=cpu->gpr[4],b=cpu->gpr[5],x,y,result=0;
 if(target==0x80043c9cu){
  if(runtime_read(runtime,a,4,&x)||runtime_write(runtime,a,4,(x&0xffffffu)|0x05000000u)||runtime_write(runtime,a+7u,1,0x28))return -1;
  if(mutation==1){fixture_store(task+4u,4,owner+0x100u);mutation_seen++;}
 }else if(target==0x80043bfcu){
  if(runtime_read(runtime,a+7u,1,&x)||runtime_write(runtime,a+7u,1,(x&0xfdu)|(b?2u:0u)))return -1;
  if(mutation==2){fixture_store(owner+0x48u,1,0x31);fixture_store(owner+0x49u,1,0x57);fixture_store(owner+0x4au,1,0x9b);mutation_seen++;}
 }else if(target==0x80043a1cu){result=0xabcd0000u|((b&3u)<<5);
 }else if(target==0x800454dcu){
  if(runtime_read(runtime,cpu->gpr[29]+0x10u,4,&x))return -1;
  if(x)fail("modeled texture window must be null",x);
  if(runtime_read(runtime,a,4,&x)||runtime_write(runtime,a,4,(x&0xffffffu)|0x02000000u)||runtime_write(runtime,a+4u,4,0xe1000000u|(cpu->gpr[7]&0x9ffu))||runtime_write(runtime,a+8u,4,0xe2000000u))return -1;
  if(mutation==4){fixture_store(0x80059580u,4,buffer+0x200u);mutation_seen++;}
 }else if(target==0x80043b48u){
  if(runtime_read(runtime,a,4,&x)||runtime_read(runtime,b,4,&y)||runtime_write(runtime,b,4,(y&0xff000000u)|(x&0xffffffu))||runtime_read(runtime,a,4,&x)||runtime_write(runtime,a,4,(x&0xff000000u)|(b&0xffffffu)))return -1;
  if(mutation==3&&call_count==5){fixture_store(0x8005956cu,4,ot+0x100u);mutation_seen++;}
 }else return -1;
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;
 if(target==0x80043a1cu)cpu->gpr[2]=result;
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
 seed=stack_kind=task_domain=mutation=bad_kind=alias_case=0;gap=0x100;
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
 task=0x80140000u;owner=0x80150000u;buffer=0x80160000u;ot=0x80170000u;
 if(task_domain==1){task|=0x20000000u;owner|=0x20000000u;buffer|=0x20000000u;ot|=0x20000000u;}
 if(task_domain==2){task+=0x200000u;owner+=0x200000u;buffer+=0x200000u;ot+=0x200000u;}
 if(task_domain==3){task=0x80060000u;owner=0x80061000u;buffer=0x80062000u;ot=0x80063000u;}
 if(task_domain==4){task=0x1f800100u;owner=0x1f800200u;buffer=0x1f800400u;ot=0x1f800900u;}
 if(task_domain==5){task=(uint32_t)(uintptr_t)host;owner=task+0x10000u;buffer=task+0x20000u;ot=task+0x30000u;}
 if(task_domain==6)task&=0x1fffffffu;
 if(stack_kind==1)cpu->gpr[29]=0x1f800e00u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 if(alias_case==1)owner=buffer-0x44u;
 if(alias_case==2)task=cpu->gpr[29]-8u;
 if(alias_case==3)buffer=cpu->gpr[29]-0x18u;
 if(alias_case==4)ot=buffer;
 if(alias_case==5)ot=0x80059578u;
 if(alias_case==6)owner=cpu->gpr[29]-0x68u;
 fixture_store(buffer,4,0x09123456u);fixture_store(buffer+0x18u,4,0x09876543u);
 fixture_store(ot+8u,4,0x8d112233u);fixture_store(ot+0x108u,4,0x8e445566u);
 fixture_store(task+4u,4,owner);
 for(unsigned i=0;i<3;i++){fixture_store(owner+0x48u+i,1,seed+i*71u);fixture_store(owner+0x148u+i,1,0xc1u+i);}
 fixture_store(owner+0x41u,1,seed);fixture_store(owner+0x141u,1,3);
 if(bad_kind==1)task++;
 if(bad_kind==2)task=0x1e000000u;
 if(bad_kind==3)fixture_store(task+4u,4,0x1e000000u);
 if(bad_kind==4)buffer++;
 if(bad_kind==5)buffer=0xfffffff0u;
 if(bad_kind==6)ot++;
 if(bad_kind==7)ot=0x1e000000u;
 if(bad_kind==8)buffer=0x801fffe0u;
 if(bad_kind==9)buffer=0x1f800ff0u;
 if(bad_kind==10)buffer=(uint32_t)(uintptr_t)host+sizeof(host)-16u;
 end=buffer+(uint32_t)gap;
 fixture_store(0x80059580u,4,buffer);fixture_store(0x80059534u,4,end);fixture_store(0x8005956cu,4,ot);
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 cpu->gpr[4]=task;
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
  if(path==1)gap=0x50;
  if(path==2)gap=0x4f;
  if(path==3)mutation=2;
  if(path==4)mutation=3;
  if(path>=5)alias_case=path-4;
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned value=0;value<256;value++){
  defaults();seed=value;run_case();
  if(expected_calls!=6||expected_events||peek(0x80059580u,4)!=buffer+0x24u)fail("independent allocation and calls",value);
  if(peek(buffer,4)!=(0x05000000u|0x112233u)||peek(buffer+0x18u,4)!=(0x02000000u|(buffer&0xffffffu))||peek(ot+8u,4)!=(0x8d000000u|((buffer+0x18u)&0xffffffu)))fail("independent packet tags/order",value);
  if(peek(buffer+7u,1)!=0x2a||peek(buffer+0x1cu,4)!=(0xe1000000u|((value&3u)<<5))||peek(buffer+0x20u,4)!=0xe2000000u)fail("independent opcode/blend/window",value);
  for(unsigned i=0;i<3;i++)if(peek(buffer+4u+i,1)!=(uint8_t)(value+i*71u))fail("independent color byte",i);
  const int16_t xy[]={-32,-32,320,-32,-32,240,320,240};
  for(unsigned i=0;i<8;i++)if(peek(buffer+8u+2u*i,2)!=(uint16_t)xy[i])fail("independent full-screen corner",i);
  total++;
 }
 const int32_t spaces[]={-1,0,0x4f,0x50,0x51};
 for(unsigned i=0;i<5;i++){
  defaults();gap=spaces[i];run_case();
  if(expected_calls!=(gap>0x50?6u:0u)||peek(0x80059580u,4)!=(gap>0x50?buffer+0x24u:buffer))fail("strict space boundary",i);
  total++;
 }
 defaults();return total;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();uint32_t target=targets[0];uint8_t *code=PSX_ADDR(target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?130:259;code[off]^=1;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[200],code[201]};
 assert(SHA256(code,260,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[200]=(uint8_t)value;code[201]=(uint8_t)(value>>8);
  if(code[200]==saved[0]&&code[201]==saved[1])continue;
  assert(SHA256(code,260,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_fade_draw_try_host(&rt,&cpu,target))fail("digest-prefix collision",0);
 code[200]=saved[0];code[201]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_fade_draw_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_fade_draw_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_FADE_DRAW","0",1)==0);if(battle_fade_draw_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_FADE_DRAW")==0);total++;
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
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"fade_draw"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_FADE_DRAW","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_FADE_DRAW")==0);
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
  defaults();if(path)mutation=path+1u;run_case();unsigned n=expected_calls;
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
 assert(!strcmp(value,"0b87d853cdc6d76d6101a044ee6cfec3adb5d536aa61a65c3dfb1626d640ac8a"));
}

int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--faults")){printf("FADE DRAW FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("FADE DRAW SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("FADE DRAW GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("FADE DRAW CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 const int32_t spaces[]={-1,0,0x4f,0x50,0x51,0x100};
 for(unsigned d=0;d<7;d++)for(unsigned v=0;v<8;v++)for(unsigned g=0;g<6;g++){
  defaults();task_domain=d;seed=v*37u;gap=spaces[g];stack_kind=v%4u;run_case();total++;
 }
 for(unsigned alias=1;alias<=6;alias++)for(unsigned g=0;g<2;g++){defaults();alias_case=alias;gap=g?0x100:0x50;run_case();total++;}
 for(unsigned bad=1;bad<=10;bad++){defaults();bad_kind=bad;run_case();total++;}
 for(unsigned st=4;st<7;st++){defaults();stack_kind=st;run_case();total++;}
 for(unsigned m=1;m<=4;m++){defaults();mutation=m;run_case();total++;}
 printf("FADE DRAW BODY PASS %u full architecture/memory/GTE comparisons\n",total);fflush(stdout);
 printf("FADE DRAW SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("FADE DRAW FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("FADE DRAW CHILD PASS %u refusals/pending returns\n",refusals());fflush(stdout);
 printf("FADE DRAW GATES PASS %u\n",controls());printf("FADE DRAW CACHE PASS %u\n",boundaries());
 puts("FADE DRAW HOST ALL PASS; native interpreted instructions=0; five external GPU SDK functions modeled");return 0;
}
