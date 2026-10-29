/* Complete original effect-image constructor comparisons.
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
static const uint32_t targets[]={0x800a3640u};
static const unsigned sizes[]={2136};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned seed,stack_kind,task_domain,mutation,mutation_seen,scenario;
static uint32_t type_arg=1,flags_arg=0x722u,width_arg=5,height_arg=3,out_y=2;
static unsigned alias_case,allocation_failure,allocations;
static uint32_t effect,source,plane[3],parent_word,callback_word;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int compare,observing;

typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[8];
static PcPortMipsCpu pending_cpu;
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE EFFECT CONSTRUCTOR HOST FAIL seed=%u stack=%u domain=%u type=%08x flags=%08x width=%08x height=%08x y=%08x alias=%u mutation=%u allocation_failure=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",seed,stack_kind,task_domain,type_arg,flags_arg,width_arg,height_arg,out_y,alias_case,mutation,allocation_failure,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
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
 if(target!=0x80032498u&&target!=0x80031bdcu&&target!=0x800448f8u&&target!=0x800445d0u)fail("unexpected modeled callee",target);
 uint32_t value=0x43210000u+call_count;
 if(runtime_write(runtime,0x80059488u,4,target^call_count))return -1;
 if(target==0x80032498u){
  if(cpu->gpr[4]!=4||cpu->gpr[5])fail("heap user arguments",cpu->gpr[4]);
  if(mutation==1){
   fixture_store(cpu->gpr[29]+0x18u,4,0x00170204u);fixture_store(cpu->gpr[29]+0x30u,4,source+2u);
   fixture_store(cpu->gpr[29]+0x88u,4,0x1f800124u);mutation_seen++;
  }
  if(mutation==2){fixture_store(cpu->gpr[29]+0x20u,2,3);mutation_seen++;}
 }
 if(target==0x80031bdcu){
  if(cpu->gpr[5])fail("heap allocation flags",cpu->gpr[5]);
  allocations++;value=allocation_failure==allocations?0:plane[allocations-1u];
  if(mutation==3&&allocations==1){
   fixture_store(cpu->gpr[29]+0x38u,2,0x7e3fu);fixture_store(cpu->gpr[29]+0x40u,2,0xffe0u);
   fixture_store(cpu->gpr[29]+0x48u,2,0x803fu);fixture_store(cpu->gpr[29]+0x70u,2,0xfffeu);mutation_seen++;
  }
 }
 if(target==0x800448f8u){
  uint32_t r=cpu->gpr[4],x,y,w,h;
  if(runtime_read(runtime,r,2,&x)||runtime_read(runtime,r+2u,2,&y)||runtime_read(runtime,r+4u,2,&w)||runtime_read(runtime,r+6u,2,&h))return -1;
  if(runtime_write(runtime,cpu->gpr[5],2,(x*3u+y*5u+w*7u+h*11u)&0xffffu))return -1;
  if(mutation==4){fixture_store(cpu->gpr[29]+0x50u,2,0x4321u);fixture_store(effect+8u,4,plane[1]+16u);mutation_seen++;}
 }
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
{
 seed=stack_kind=task_domain=mutation=scenario=alias_case=allocation_failure=0;
 stop_at=stop_mode=pending_return=fault_at=0;type_arg=1;flags_arg=0x722u;width_arg=5;height_arg=3;out_y=2;
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
 effect=task_domain==0?0x80150000u:task_domain==1?0xa0150000u:task_domain==2?0x80350000u:task_domain==3?0x80060000u:task_domain==4?0x1f800100u:(uint32_t)(uintptr_t)host;
 source=0x80160080u;plane[0]=0x80130000u;plane[1]=0xa0132000u;plane[2]=0x80334000u;
 if(task_domain==5){source=(uint32_t)(uintptr_t)host+0x10000u;for(unsigned i=0;i<3;i++)plane[i]=(uint32_t)(uintptr_t)host+0x20000u+i*0x2000u;}
 if(stack_kind==1)cpu->gpr[29]=0x1f800e00u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 uint32_t frame=cpu->gpr[29]-0x108u;
 if(alias_case==1)plane[2]=effect+4u;
 if(alias_case==2)plane[2]=source+2u;
 if(alias_case==3)plane[2]=effect+8u;
 if(alias_case==4)plane[2]=frame+0x50u;
 if(alias_case==5)effect=frame+0x90u;
 parent_word=(const uint32_t[]){0,0xa0170000u,0x00170000u,0x1f800700u,0xdeadbeefu,0x80000000u,0xffffffffu,0x80370000u}[seed%8u];
 callback_word=(const uint32_t[]){0x800a3490u,0x800a3514u,0x800a3578u,0x800a35c8u,0,0xa00a3514u,0xffffffffu,0x1f800400u}[seed%8u];
 cpu->gpr[4]=effect;cpu->gpr[5]=parent_word;cpu->gpr[6]=type_arg;cpu->gpr[7]=flags_arg;
 const uint32_t arguments[]={source,3,5,7,11,13,17,0xffff8001u,out_y,width_arg,height_arg,0x80018002u,0xffff8003u,0x1234ffffu,callback_word};
 for(unsigned i=0;i<15;i++)fixture_store(cpu->gpr[29]+0x10u+i*4u,4,arguments[i]);
 for(unsigned i=0;i<256;i++)fixture_store(source-0x40u+i*2u,2,0x1234u+i*73u);
 fixture_store(effect+0x1au,2,scenario==1?0x8000u:0);fixture_store(effect+4u,4,plane[2]);fixture_store(effect+8u,4,plane[1]);fixture_store(effect+12u,4,plane[0]);
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 accesses=call_count=allocations=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=0;observing=1;
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
 if(!fault_at&&!stop_at&&stack_kind<4&&!alias_case&&!allocation_failure&&original!=PC_PORT_MIPS_HALTED)fail("unexpected original fault",oracle.pc);
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
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?1068:2135;code[off]^=1;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[1066],code[1067]};
 assert(SHA256(code,2136,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[1066]=(uint8_t)value;code[1067]=(uint8_t)(value>>8);
  if(code[1066]==saved[0]&&code[1067]==saved[1])continue;
  assert(SHA256(code,2136,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_effect_constructor_try_host(&rt,&cpu,target))fail("digest-prefix collision",0);
 code[1066]=saved[0];code[1067]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_effect_constructor_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_EFFECT_CONSTRUCTOR","0",1)==0);if(battle_effect_constructor_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_CONSTRUCTOR")==0);total++;
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
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"effect_constructor"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_EFFECT_CONSTRUCTOR","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_CONSTRUCTOR")==0);
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
 unsigned total=0;
 for(unsigned path=0;path<12;path++){
  defaults();task_domain=path%6u;stack_kind=path%4u;seed=path;
  if(path==0)scenario=1;
  if(path==1)type_arg=3;
  if(path==2)type_arg=0xffff8001u;
  if(path==3)flags_arg=0x711u;
  if(path==4)flags_arg=0x712u;
  if(path==5)flags_arg=0x721u;
  if(path>=6){type_arg=5;flags_arg=path&1u?0x611u:0x622u;out_y=0xfffffffeu;}
  if(path==8){flags_arg=0x22u;alias_case=4;}
  if(path==9){flags_arg=0x11u;alias_case=2;}
  if(path==10){width_arg=0xffffu;flags_arg=0x622u;}
  if(path==11){height_arg=0xffffu;flags_arg=0x611u;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static unsigned refusals(void)
{
 unsigned total=0;
 for(unsigned path=0;path<2;path++){
  defaults();type_arg=path?5:1;flags_arg=path?0x611u:0x711u;run_case();unsigned n=expected_calls;
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
 defaults();seed=3;run_case();
 if(peek(effect,4)!=parent_word||peek(effect+0x1cu,4)!=source||peek(effect+0x24u,4)!=callback_word||
    peek(effect+0x1au,2)!=1||peek(effect+0x14u,2)||peek(effect+0x18u,2)!=0xffffu||peek(effect+0x11u,1)||
    peek(effect+0x10u,1)!=1||peek(effect+0x16u,2)!=0x8002u||peek(effect+0x20u,2)!=0x8003u||
    peek(effect+0x22u,2)!=0xffffu||peek(effect+0x28u,2)!=0x8001u||peek(effect+0x2au,2)!=2||
    peek(effect+0x2cu,2)!=6||peek(effect+0x2eu,2)!=3||peek(effect+0x12u,2)!=18||allocations!=3)fail("record/19-word/narrow/rounded-width witness",effect);
 for(unsigned bank=0;bank<2;bank++){
  uint32_t address=peek(effect+4u+bank*4u,4),color=bank?((17u<<10)+(13u<<5))|11u:((7u<<10)+(5u<<5))|3u;
  for(unsigned i=0;i<18;i++)if(peek(address+i*2u,2)!=color)fail("packed color pixel witness",i);
 }
 unsigned total=1;
 for(unsigned s=0;s<8;s++){
  defaults();seed=s;type_arg=5;flags_arg=0x22u;out_y=0xfffffffeu;width_arg=2;height_arg=3;run_case();
  uint16_t incoming=(uint16_t)(0x12340011u+s*256u);
  uint32_t a=peek(effect+4u,4),b=peek(effect+8u,4);
  for(unsigned i=0;i<6;i++){
   if(peek(a+i*2u,2)!=(i<4?incoming:3u))fail("incoming S1 negative-row carry",i);
   if(peek(b+i*2u,2)!=(i<4?3u:11u))fail("cross-plane carry",i);
  }
  total++;
 }
 defaults();type_arg=5;flags_arg=0x11u;width_arg=2;height_arg=3;run_case();
 for(unsigned bank=0;bank<2;bank++){
  uint32_t address=peek(effect+4u+bank*4u,4),x=bank?11u:3u,y=bank?13u:5u;
  for(unsigned row=0;row<3;row++)for(unsigned col=0;col<2;col++)if(peek(address+(row*2u+col)*2u,2)!=peek(source+(y+row)*6u+(x+col)*2u,2))fail("six-byte source stride",row);
 }
 total++;defaults();return total;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(original_body(0),sizes[0],digest));char value[65];
 for(unsigned i=0;i<32;i++)snprintf(value+i*2,3,"%02x",digest[i]);
 assert(!strcmp(value,"c3673076fb507130a0d0f073437cb130c589ef92b40cebbb109c1257423a6426"));
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--faults")){printf("EFFECT CONSTRUCTOR FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("EFFECT CONSTRUCTOR SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("EFFECT CONSTRUCTOR GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("EFFECT CONSTRUCTOR CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 for(unsigned s=0;s<8;s++)for(unsigned d=0;d<6;d++){
  defaults();seed=s;task_domain=d;stack_kind=s%4u;type_arg=s%6u;flags_arg=(s&1u)?0x711u:0x722u;run_case();total++;
 }
 const uint32_t types[]={0,1,2,3,4,5,6,255,256,0x7fff,0x8000,0xffff,0x10001,0xffff0005u,0x80000000u};
 for(unsigned t=0;t<sizeof(types)/sizeof(types[0]);t++)for(unsigned modes=0;modes<16;modes++){
  defaults();type_arg=types[t];flags_arg=0x700u+(modes/4u)*16u+modes%4u;run_case();total++;
 }
 const uint32_t dimensions[]={0,1,2,3,0x7fff,0x8000,0x8001,0xfffe,0xffff,0x10000,0x10001,0x80000002u,0xffffffffu};
 for(unsigned t=0;t<2;t++)for(unsigned w=0;w<13;w++)for(unsigned h=0;h<13;h++){
  defaults();type_arg=t?5:1;flags_arg=0x700u;width_arg=dimensions[w];height_arg=dimensions[h];run_case();total++;
 }
 for(unsigned flags=0;flags<8;flags++){
  defaults();flags_arg=flags<<8;run_case();total++;
  defaults();type_arg=5;flags_arg=flags<<8;run_case();total++;
 }
 for(unsigned m=1;m<=4;m++){
  defaults();mutation=m;if(m==4)flags_arg=0x711u;run_case();total++;
 }
 for(unsigned a=1;a<=5;a++){
  defaults();type_arg=a==2?5:1;flags_arg=a==2?0x11u:0x22u;alias_case=a;width_arg=2;height_arg=2;run_case();total++;
 }
 for(unsigned a=1;a<=3;a++){
  defaults();allocation_failure=a;flags_arg=0x700u;run_case();total++;
  defaults();allocation_failure=a;run_case();total++;
 }
 for(unsigned s=4;s<7;s++){defaults();stack_kind=s;run_case();total++;}
 defaults();scenario=1;run_case();total++;defaults();
 printf("EFFECT CONSTRUCTOR BODY PASS %u full architecture/memory comparisons\n",total);fflush(stdout);
 printf("EFFECT CONSTRUCTOR SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("EFFECT CONSTRUCTOR FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("EFFECT CONSTRUCTOR CHILD PASS %u refusals and pending returns\n",refusals());
 printf("EFFECT CONSTRUCTOR GATES PASS %u\n",controls());printf("EFFECT CONSTRUCTOR CACHE PASS %u\n",boundaries());
 puts("EFFECT CONSTRUCTOR HOST ALL PASS; native interpreted instructions=0; four external services modeled");return 0;
}
