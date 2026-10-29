/* Complete original startup resource-loader comparisons.
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
static uint8_t module[0x3914];
static const uint32_t targets[]={0x801e6314u};
static const unsigned sizes[]={1020};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned seed,stack_kind,task_domain,mutation,mutation_seen,bad_kind,alias_case;
static int32_t gap;
static unsigned shape,base_byte,record_count;
static uint32_t palette_count=1,blob,allocation,frame,initial_sp;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static unsigned events,expected_events;
static int compare,observing;
static PcPortMipsCpu *oracle_cpu,pending_cpu;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[8192];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[32];
typedef struct Event {uint32_t command,gpr[32],cp0[32],hi,lo;GTERegisters before,after;} Event;
static Event event_trace[2];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"STARTUP RESOURCES FAIL shape=%u seed=%u stack=%u domain=%u gap=%d alias=%u bad=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",shape,seed,stack_kind,task_domain,gap,alias_case,bad_kind,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
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
{return module+(targets[i]-0x801e4000u);}
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
 assert(call_count<32);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
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
/* These services deliberately clobber every volatile register and exercise
 * live memory changes. Only the parent body is under test in this fixture. */
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 uint32_t a=cpu->gpr[4],b=cpu->gpr[5],n=cpu->gpr[6],v,result=0;
 if(target==0x80031bdcu){
  if(b)fail("allocation flags",b);
  result=allocation;
  if(mutation==1){fixture_store(blob+4u,4,peek(blob+4u,4)+1u);mutation_seen++;}
  if(mutation==7){fixture_store(frame+0xc8u,4,blob+0x800u);mutation_seen++;}
 }else if(target==0x8003f968u){
  result=a;
  /* The model refuses a deliberately malformed huge copy, before access.
   * The production fixed body has no such clamp. */
  if((int32_t)n>256)return -1;
  if(a&&(int32_t)n>0)for(uint32_t i=0;i<n;i++)
   if(runtime_read(runtime,b+i,1,&v)||runtime_write(runtime,a+i,1,v))return -1;
  if(mutation==2){fixture_store(0x800d39c8u,4,allocation+0x100u);mutation_seen++;}
 }else if(target==0x80022a70u){
  if(n!=256)fail("CLUT y argument",n);
  if(runtime_read(runtime,a,4,&v))return -1;
  if(mutation==3){fixture_store(blob+8u,4,0x12345678u);fixture_store(blob+19u,1,0xf3);fixture_store(frame+0x88u,4,2);mutation_seen++;}
 }else if(target==0x80022a00u){
  if(runtime_read(runtime,a,4,&result))return -1;
  if(mutation==4){fixture_store(frame+0x9cu,4,0x800d0000u);fixture_store(frame+0x8cu,4,7);mutation_seen++;}
  if(mutation==8){fixture_store(frame+0x8cu,4,0xfffffffeu);for(unsigned i=3;i<6;i++)fixture_store(frame+0x48u+i*4u,4,0xfffffffcu);mutation_seen++;}
  if(mutation==9){fixture_store(frame+0x80u,4,1);mutation_seen++;}
 }else if(target==0x800a8bf0u){
  for(unsigned i=0;i<5;i++)if(runtime_read(runtime,cpu->gpr[29]+0x10u+i*4u,4,&v))return -1;
  if(runtime_write(runtime,0x80059200u+a*4u,4,b))return -1;
  if(mutation==5&&!mutation_seen){fixture_store(frame+0x48u+6u*4u,4,0);mutation_seen++;}
 }else if(target==0x800bb350u){
  if(runtime_write(runtime,0x80059300u+a*4u,4,a))return -1;
  if(mutation==6){fixture_store(frame+0x9cu,4,0x800d0000u);fixture_store(frame+0xa0u,4,0x98765432u);mutation_seen++;}
 }else return -1;
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=result;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
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
 seed=stack_kind=task_domain=mutation=bad_kind=alias_case=base_byte=0;gap=0;shape=2;palette_count=1;
 stop_at=stop_mode=pending_return=fault_at=0;
}
static void record(unsigned i,unsigned actor,uint32_t reference)
{
 uint32_t a=blob+8u+i*12u;
 fixture_store(a,4,8u+12u*record_count+i*8u);fixture_store(a+4u,4,reference);
 fixture_store(a+8u,1,actor);fixture_store(a+11u,1,seed+i*71u);
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 blob=0x80140000u;allocation=0x80160000u;
 if(task_domain==1){blob|=0x20000000u;allocation|=0x20000000u;}
 if(task_domain==2){blob+=0x200000u;allocation+=0x200000u;}
 if(task_domain==3){blob=0x80060000u;allocation=0x80062000u;}
 if(task_domain==4){blob=0x1f800100u;allocation=0x1f800c00u;}
 if(task_domain==5){blob=(uint32_t)(uintptr_t)host;allocation=blob+0x20000u;}
 if(task_domain==6)blob&=0x1fffffffu;
 if(stack_kind==1)cpu->gpr[29]=0x1f800f00u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 if(alias_case==1)blob=cpu->gpr[29];
 if(alias_case==2)blob=0x800ccbe8u;
 if(alias_case==3)allocation=blob+8u+12u*(shape==0?0u:shape==2?2u:1u)+1u;
 if(alias_case==4)allocation=blob+8u;
 if(alias_case==5)cpu->gpr[29]=0x800ccc20u;
 if(alias_case==6)allocation=0x800d39c8u;
 initial_sp=cpu->gpr[29];frame=initial_sp-0xc8u;
 record_count=shape==0?0:shape==2?2:shape==4||shape==6||shape==7||shape==8?3:shape==9?255:1;
 fixture_store(blob,1,record_count);fixture_store(blob+1u,1,base_byte);fixture_store(blob+4u,4,8u+12u*record_count+64u);
 for(unsigned i=0;i<record_count;i++)record(i,0,i?0:0x400u);
 for(unsigned i=0;i<11;i++)fixture_store(0x800c3eb6u+i*0x1cu,1,seed%3u?255u:7u);
 if(shape==3||shape==4||shape==5||shape==6||shape==9||shape==10||shape==11){
  record(0,1,(shape==6||shape==9)?0:0x400u);
  for(unsigned i=3;i<11;i++)fixture_store(0x800c3eb6u+i*0x1cu,1,shape==5?255u:shape==4?(i-3u)%3u:0u);
  if(shape==4){record(1,1,0);record(2,1,0);}
  if(shape==10||shape==11)for(unsigned i=4;i<11;i++)fixture_store(0x800c3eb6u+i*0x1cu,1,shape==11&&i==4?0:255);
 }
 if(shape==7){record(1,1,0x400u);for(unsigned i=3;i<6;i++)fixture_store(0x800c3eb6u+i*0x1cu,1,1);}
 if(shape==8)for(unsigned i=0;i<3;i++)record(i,0,0x400u+i*4u);
 if(shape==12)record(0,0,7);
 if(shape==13)record(0,0,8);
 for(unsigned i=0;i<3;i++)fixture_store(blob+0x400u+i*4u,4,palette_count+i);
 for(unsigned i=0;i<64;i++)fixture_store(blob+8u+record_count*12u+i,1,seed+i*13u);
 if(mutation==7){fixture_store(blob+0x800u,1,0);fixture_store(blob+0x804u,4,8u+record_count*12u+64u);}
 if(bad_kind==1)blob++;
 if(bad_kind==2)blob=0x1e000000u;
 if(bad_kind==3)fixture_store(blob+4u,4,0);
 if(bad_kind==4)allocation=0x1e000000u;
 if(bad_kind==5)allocation=0;
 if(bad_kind==6)allocation=0x801ffff0u;
 if(bad_kind==7)fixture_store(blob+12u,4,0xffffff00u);
 if(bad_kind==8)rt.host_ranges[0].writable=0;
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 cpu->gpr[4]=blob;
 accesses=call_count=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=events=0;observing=1;
}

static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 int original=PcPortMipsRun(&oracle,targets[0],BATTLE_HALT_PC,200000);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;unsigned expected_commands=oracle_gte_commands;snapshot();
 setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int actual=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
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
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();uint32_t target=targets[0];uint8_t *code=PSX_ADDR(target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?510:1019;code[off]^=1;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[200],code[201]};
 assert(SHA256(code,1020,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[200]=(uint8_t)value;code[201]=(uint8_t)(value>>8);
  if(code[200]==saved[0]&&code[201]==saved[1])continue;
  assert(SHA256(code,1020,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_startup_resources_try_host(&rt,&cpu,target))fail("digest-prefix collision",0);
 code[200]=saved[0];code[201]=saved[1];total++;
 for(unsigned m=0;m<2;m++){
  uint8_t *marker=PSX_ADDR(m?0x801e5840u:0x801e4000u);marker[m?227:31]^=1;
  if(battle_startup_resources_try_host(&rt,&cpu,target))fail("startup identity",m);
  marker[m?227:31]^=1;total++;
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_startup_resources_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_startup_resources_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES","0",1)==0);if(battle_startup_resources_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES")==0);total++;
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
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"startup_resources"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_STARTUP_RESOURCES")==0);
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
 for(unsigned path=0;path<4;path++){
  defaults();shape=path==0?0:path==1?2:path==2?3:4;run_case();unsigned n=expected_calls;
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
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned v=0;v<256;v++){
  defaults();seed=base_byte=v;run_case();
  uint32_t out=0x800ccbf8u,y=0x140u+(v<<6),header=8u+record_count*12u;
  if(expected_calls!=4||peek(0x800d39c8u,4)!=allocation||peek(initial_sp,4)!=blob||peek(frame+0x84u,4)!=2||peek(frame+0x8cu,4)!=5)fail("independent count/publication/stack",v);
  for(unsigned i=0;i<2;i++)if(peek(out+i*12u,4)!=allocation+i*8u||peek(out+i*12u+4u,2)!=(uint16_t)y||peek(out+i*12u+6u,2)!=256||peek(out+i*12u+8u,4)!=(uint8_t)(v+i*71u))fail("independent output records",i);
  for(unsigned i=0;i<64;i++)if(peek(allocation+i,1)!=peek(blob+header+i,1))fail("independent copied payload",i);
  if(calls[0].gpr[4]!=64||calls[0].gpr[5]||calls[1].gpr[4]!=allocation||calls[1].gpr[5]!=blob+header||calls[1].gpr[6]!=64)fail("independent allocation/copy arguments",v);
  total++;
 }
 for(unsigned sh=3;sh<=4;sh++){
  defaults();shape=sh;run_case();
  if(expected_calls!=18||peek(0x800ccbf8u,4)||peek(frame+0x8cu,4)!=4)fail("independent actor grouping/skipped records",sh);
  for(unsigned i=0;i<8;i++){
   const Call *c=&calls[2u+i*2u];uint32_t flags=0x80u|(i==0?2u:i==7?5u:7u);
   if(c->target!=0x800a8bf0u||c->gpr[4]!=i+3u||c->gpr[5]!=flags||c->gpr[6]!=allocation||c->gpr[7]!=blob+0x400u||calls[3u+i*2u].target!=0x800bb350u||calls[3u+i*2u].gpr[4]!=i+3u)fail("independent nine-argument actor routing",i);
   const uint8_t *stack=c->ram+(frame&0x1fffffu);
   if(load_le(stack+0x10,4)!=320||load_le(stack+0x14,4)!=256||load_le(stack+0x18,4)||load_le(stack+0x1c,4)!=451||load_le(stack+0x20,4))fail("independent actor stack arguments",i);
  }
  total++;
 }
 defaults();shape=8;palette_count=6;run_case();
 const uint32_t ys[]={320,704,0};
 for(unsigned i=0;i<3;i++)if(peek(0x800ccbfcu+i*12u,2)!=ys[i])fail("independent palette wrap boundary",i);
 total++;
 for(unsigned sh=10;sh<12;sh++){
  defaults();shape=sh;run_case();
  if(expected_calls!=2u+2u*(sh-9u)||calls[2].gpr[5]!=(sh==10?0x80u:0x82u)||(sh==11&&calls[4].gpr[5]!=0x85u))fail("independent one/two shared actors",sh);
  total++;
 }
 defaults();shape=9;run_case();
 if(expected_calls!=2||peek(frame+0x84u,4)!=255||peek(frame+0x8cu,4)!=3||peek(frame+0x9cu,4)!=0x800c3ed4u)fail("independent repeated skipped record",0);
 total++;defaults();return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<10;path++){
  defaults();shape=path<9?path:2;
  if(path==9){alias_case=6;mutation=2;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static void load_inputs(void)
{
 FILE *f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");assert(f&&fread(module,1,sizeof(module),f)==sizeof(module)&&fclose(f)==0);
 assert(XenoRetailData_File(XENO_RD_SLUS));
 uint8_t digest[32];assert(SHA256(original_body(0),sizes[0],digest));char value[65];
 for(unsigned i=0;i<32;i++)snprintf(value+i*2,3,"%02x",digest[i]);
 assert(!strcmp(value,"4581d128c6e3d82317b5bd330bb6439fe22208f572dfdcb74e165da57e120a5f"));
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--shape")){shape=(unsigned)atoi(argv[2]);return run_case()?0:1;}
  if(!strcmp(argv[1],"--faults")){printf("STARTUP RESOURCES FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("STARTUP RESOURCES SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("STARTUP RESOURCES GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("STARTUP RESOURCES CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned sh=0;sh<14;sh++)for(unsigned v=0;v<4;v++){
  if(sh==9)continue; /* The 255-record case has its separate witness. */
  defaults();task_domain=d;shape=sh;seed=v*71u;stack_kind=v;run_case();total++;
 }
 const uint32_t counts[]={0,1,6,7,0x1ffu,0x7fffffffu,0x80000000u,0xffffffffu};
 for(unsigned c=0;c<8;c++)for(unsigned b=0;b<4;b++){
  defaults();shape=8;palette_count=counts[c];base_byte=b==3?255:b*3u;run_case();total++;
 }
 for(unsigned alias=1;alias<=6;alias++){defaults();alias_case=alias;run_case();total++;}
 for(unsigned bad=1;bad<=8;bad++){defaults();bad_kind=bad;task_domain=bad==8?5:0;run_case();total++;}
 for(unsigned st=4;st<7;st++){defaults();stack_kind=st;run_case();total++;}
 for(unsigned m=1;m<=9;m++){defaults();mutation=m;if(m==5||m==6)shape=3;if(m>=8)shape=7;run_case();total++;}
 printf("STARTUP RESOURCES BODY PASS %u full architecture/memory/GTE comparisons\n",total);fflush(stdout);
 printf("STARTUP RESOURCES SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("STARTUP RESOURCES FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("STARTUP RESOURCES CHILD PASS %u refusals/pending returns\n",refusals());fflush(stdout);
 printf("STARTUP RESOURCES GATES PASS %u\n",controls());printf("STARTUP RESOURCES CACHE PASS %u\n",boundaries());
 puts("STARTUP RESOURCES HOST ALL PASS; native interpreted instructions=0; six external child functions modeled");return 0;
}
