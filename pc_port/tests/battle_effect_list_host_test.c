/* Complete original effect-list update and release comparisons.
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
static const uint32_t targets[]={0x800a2fd8u,0x800a2f94u};
static const unsigned sizes[]={768,68};
#define NUM_BODIES 2u
#define NEW_BODIES 2u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,mutation,mutation_seen,bad_kind,alias_case;
static int32_t count,step,age,duration,minimum;
static unsigned projected,buffer,shift;
static uint32_t owner,records,matrix,ot,release_pointer;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static unsigned events,expected_events;
static int compare,observing;
static PcPortMipsCpu *oracle_cpu,pending_cpu;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[4096];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[16];
typedef struct Event {uint32_t command,gpr[32],cp0[32],hi,lo;GTERegisters before,after;} Event;
static Event event_trace[32];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE EFFECT LIST HOST FAIL kind=%u seed=%u stack=%u domain=%u count=%d step=%d age=%d duration=%d projected=%u buffer=%u alias=%u bad=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u event=%u %s %08x\n",kind,seed,stack_kind,task_domain,count,step,age,duration,projected,buffer,alias_case,bad_kind,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,events,what,value);exit(1);
}
int doCOP2(int op)
{
 if(candidate)native_gte_commands++;else oracle_gte_commands++;
 if(!observing)return color_fade_backend_command(op);
 if(events>=32)fail("GTE trace capacity",events);
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
 assert(call_count<16);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
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
 if(target!=0x80049efcu&&target!=0x80049f8cu) return -1;
 unsigned first=target==0x80049efcu?0:5,last=target==0x80049efcu?5:8;
 for(unsigned i=first;i<last;i++){uint32_t word;if(runtime_read(runtime,cpu->gpr[4]+4u*i,4,&word))return -1;CTC2(word,i);}
 if(target==0x80049f8cu){
  if(mutation==1){fixture_store(owner+4u,2,1);mutation_seen++;}
  if(mutation==2){fixture_store(owner,4,records+0x7cu);mutation_seen++;}
  if(mutation==3){fixture_store(cpu->gpr[29]+0x10u,4,0xfedcba98u);fixture_store(owner+4u,2,0);mutation_seen++;}
 }
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
 if(target==0x800a2f94u&&mutation==4&&!mutation_seen){fixture_store(owner+4u,2,1);mutation_seen++;}
 int rc=bi>=0?(candidate?runtime_bridge(opaque,cpu,target):0):service(opaque,cpu,target);
 if(pending_return&&call_count==pending_return&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return rc;
}
static void defaults(void)
{
 kind=seed=stack_kind=task_domain=mutation=bad_kind=alias_case=projected=buffer=0;
 count=3;step=1;age=0;duration=20;minimum=7;shift=3;
 stop_at=stop_mode=pending_return=fault_at=0;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(&gteRegs,0,sizeof(gteRegs));memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 CTC2(160u<<16,24);CTC2(120u<<16,25);CTC2(256,26);
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 owner=0x80120000u;matrix=0x80130000u;records=0x80140000u;ot=0x80180000u;
 if(task_domain==1){owner|=0x20000000u;matrix|=0x20000000u;records|=0x20000000u;}
 if(task_domain==2){owner+=0x200000u;matrix+=0x200000u;records+=0x200000u;}
 if(task_domain==3){owner=0x80054000u;matrix=0x80055000u;records=0x80056000u;}
 if(task_domain==4){owner=0x1f800100u;matrix=0x1f800200u;records=0x1f800400u;}
 if(task_domain==5){owner=(uint32_t)(uintptr_t)host;matrix=owner+0x10000u;records=owner+0x20000u;}
 if(task_domain==6){owner&=0x1fffffffu;matrix&=0x1fffffffu;records&=0x1fffffffu;}
 if(stack_kind==1)cpu->gpr[29]=0x1f800e00u;
 if(stack_kind==2)cpu->gpr[29]|=0x20000000u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;
 if(alias_case==1)ot=records+0x2cu;
 if(alias_case==2)ot=records+0x20u;
 if(alias_case==3)matrix=cpu->gpr[29]-0x28u;
 if(alias_case==4)owner=cpu->gpr[29]-0x38u;
 if(alias_case==5)records=cpu->gpr[29]-0x200u;
 if(alias_case==6)ot=owner;
 if(alias_case==7)matrix=records+0x20u;
 for(unsigned i=0;i<0x4000u;i++)fixture_store(0x80180000u+i*4u,4,0x8d123456u+i);
 for(unsigned i=0;i<6;i++){
  uint32_t a=records+0x7cu*i;
  for(unsigned j=0;j<4;j++){
   fixture_store(a+j*8u,2,8u+j*8u+i*2u+seed*15u);
   fixture_store(a+j*8u+2u,2,8u+j*2u+i*2u-seed*23u);
   fixture_store(a+j*8u+4u,2,512u+seed*97u);fixture_store(a+j*8u+6u,2,0x1234);
  }
  fixture_store(a+0xeu,2,projected?1:0);fixture_store(a+0x16u,2,(uint32_t)age);fixture_store(a+0x1eu,2,(uint32_t)duration);
  for(unsigned j=0;j<3;j++){
   fixture_store(a+0x20u+j*2u,2,0x3201u+j*0x1100u+seed*3917u);
   fixture_store(a+0x26u+j*2u,2,2u+j*3u-seed*431u);
  }
  fixture_store(a+0x2cu,4,0x09111111u+i);fixture_store(a+0x54u,4,0x09222222u+i);
 }
 for(unsigned i=0;i<8;i++)fixture_store(matrix+i*4u,4,i==0||i==2||i==4?4096:0);
 fixture_store(owner,4,records);fixture_store(owner+4u,2,(uint32_t)count);fixture_store(owner+6u,2,(uint32_t)minimum);
 fixture_store(0x80050100u,4,shift);fixture_store(cpu->gpr[29]+0x10u,4,buffer);
 release_pointer=records+(seed%6u)*0x7cu;
 if(bad_kind==1)owner++;
 if(bad_kind==2)owner=0x1e000000u;
 if(bad_kind==3)fixture_store(owner,4,0x1e000000u);
 if(bad_kind==4)matrix++;
 if(bad_kind==5)matrix=0x1e000000u;
 if(bad_kind==6)ot++;
 if(bad_kind==7)ot=0x1e000000u;
 if(bad_kind==8)release_pointer++;
 if(bad_kind==9)release_pointer=0xffffffeau;
 if(bad_kind==10)fixture_store(owner,4,0);
 if(stack_kind==4)cpu->gpr[29]++;
 if(stack_kind==5)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==6)cpu->gpr[29]=0;
 cpu->gpr[4]=owner;cpu->gpr[5]=kind?release_pointer:matrix;cpu->gpr[6]=(uint32_t)step;cpu->gpr[7]=ot;
 accesses=call_count=mutation_seen=legacy_calls=oracle_gte_commands=native_gte_commands=events=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 int original=PcPortMipsRun(&oracle,targets[kind],BATTLE_HALT_PC,20000);
 expected_accesses=accesses;expected_calls=call_count;expected_events=events;unsigned expected_commands=oracle_gte_commands;snapshot();
 setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int actual=PcPortMipsRun(&native,targets[kind],BATTLE_HALT_PC,20000);
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
  defaults();seed=path%3u;projected=path&1u;task_domain=path%6u;stack_kind=path%4u;
  if(path==2)age=-1;
  if(path==3)age=duration;
  if(path==4)count=0;
  if(path==5)alias_case=1;
  if(path==6)alias_case=4;
  if(path==7)kind=1;
  if(path==8){kind=1;minimum=-1;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned mode=0;mode<2;mode++)for(unsigned buf=0;buf<2;buf++){
  defaults();count=2;projected=mode;buffer=buf;run_case();
  uint32_t depth=mode?16u:0u,previous=0x123456u+depth;
  for(unsigned i=0;i<2;i++){
   uint32_t record=records+i*0x7cu,packet=record+0x2cu+buf*40u;
   if(peek(packet,4)!=(0x09000000u|previous))fail("independent packet link",i);
   previous=packet&0xffffffu;
   for(unsigned j=0;j<4;j++){
    int x=8+(int)j*8+(int)i*2,y=8+(int)j*2+(int)i*2;
    if(mode){x=160+x/2;y=120+y/2;}
    if(peek(packet+8u+j*8u,2)!=(uint16_t)x||peek(packet+10u+j*8u,2)!=(uint16_t)y)fail("independent screen/projected vertex",i*4u+j);
   }
   if(peek(record+0x16u,2)!=1)fail("independent age",i);
   for(unsigned j=0;j<3;j++){
    uint32_t color=0x3201u+j*0x1100u,velocity=2u+j*3u;
    if(peek(packet+4u+j,1)!=((color>>6)&255u)||peek(record+0x20u+j*2u,2)!=(uint16_t)(color-velocity))fail("independent color",j);
   }
  }
  if(peek(ot+depth*4u,4)!=(0x8d000000u|previous))fail("independent OT head",depth);
  total++;
 }
 for(unsigned i=0;i<6;i++)for(int m=-1;m<8;m+=2){
  defaults();kind=1;seed=i;minimum=m;run_case();
  if(peek(release_pointer+0x16u,2)!=0xffffu||peek(owner+6u,2)!=(uint16_t)(m<(int)i?m:(int)i))fail("independent release minimum/index",i);
  total++;
 }
 defaults();return total;
}
static unsigned controls(void)
{
 PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();uint32_t target=targets[kind];uint8_t *code=PSX_ADDR(target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;if(battle_effect_list_try_host(&rt,&cpu,target))fail("full identity",off);code[off]^=1;total++;}
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[20],code[21]};
 assert(SHA256(code,sizes[kind],original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[20]=(uint8_t)value;code[21]=(uint8_t)(value>>8);
  if(code[20]==saved[0]&&code[21]==saved[1])continue;
  assert(SHA256(code,sizes[kind],changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_effect_list_try_host(&rt,&cpu,target))fail("digest-prefix collision",0);
 code[20]=saved[0];code[21]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_effect_list_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_effect_list_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_effect_list_try_host(&rt,&cpu,target))fail("initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_effect_list_try_host(&rt,&cpu,target))fail("active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_effect_list_try_host(&rt,&cpu,target))fail("bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_effect_list_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_EFFECT_LIST","0",1)==0);if(battle_effect_list_try_host(&rt,&cpu,target))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_LIST")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 return total;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"effect_list"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_EFFECT_LIST","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,200000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_LIST")==0);
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
  defaults();if(path==1)age=duration;if(path==2)projected=1;
  run_case();unsigned n=expected_calls;
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
 const char *hashes[]={"54616eb908fdbcf974ed8fe21dc2d4b74c938283af12d788af2a24b27b23181d","91e3293a33d45b2086a530876a9c0605a3bde08c2245f2ab7368cd874bad50ce"};
 for(unsigned k=0;k<NUM_BODIES;k++){
  uint8_t digest[32];assert(SHA256(original_body(k),sizes[k],digest));char value[65];
  for(unsigned i=0;i<32;i++)snprintf(value+i*2,3,"%02x",digest[i]);
  assert(!strcmp(value,hashes[k]));
 }
}

int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic"))return run_case()?0:1;
  if(!strcmp(argv[1],"--faults")){printf("EFFECT LIST FAULT PASS %u\n",faults());return 0;}
  if(!strcmp(argv[1],"--semantics")){printf("EFFECT LIST SEMANTICS PASS %u\n",semantics());return 0;}
  if(!strcmp(argv[1],"--gates")){printf("EFFECT LIST GATES PASS %u\n",controls());return 0;}
  if(!strcmp(argv[1],"--boundary")){printf("EFFECT LIST CACHE PASS %u\n",boundaries());return 0;}
  return 2;
 }
 unsigned total=0;
 for(unsigned k=0;k<2;k++)for(unsigned mode=0;mode<2;mode++)for(unsigned d=0;d<7;d++)for(unsigned s=0;s<8;s++){
  defaults();kind=k;projected=mode;task_domain=d;seed=s;stack_kind=s%4u;buffer=s&1u;run_case();total++;
 }
 const int32_t signed_values[]={-32768,-1,0,1,32767};
 for(unsigned a=0;a<5;a++)for(unsigned d=0;d<5;d++)for(unsigned mode=0;mode<2;mode++){
  defaults();age=signed_values[a];duration=signed_values[d];step=signed_values[(a+d)%5];projected=mode;run_case();total++;
 }
 for(unsigned alias=1;alias<=7;alias++)for(unsigned mode=0;mode<2;mode++){defaults();alias_case=alias;projected=mode;run_case();total++;}
 for(unsigned k=0;k<2;k++)for(unsigned bad=1;bad<=10;bad++){defaults();kind=k;bad_kind=bad;run_case();total++;}
 for(unsigned s=4;s<7;s++){defaults();stack_kind=s;run_case();total++;}
 for(unsigned m=1;m<=4;m++){defaults();mutation=m;if(m==4)age=duration;run_case();total++;}
 for(unsigned i=0;i<5;i++){defaults();count=signed_values[i];if(count>6)mutation=1;run_case();total++;}
 const uint32_t shifts[]={0,1,2,31,32,33,0xffffffffu};
 for(unsigned i=0;i<7;i++){defaults();projected=1;shift=shifts[i];run_case();total++;}
 const uint32_t steps[]={0x80000000u,0x7fffffffu,0xffffffffu};
 for(unsigned i=0;i<3;i++){defaults();step=(int32_t)steps[i];run_case();total++;}
 printf("EFFECT LIST BODY PASS %u full architecture/memory/GTE comparisons\n",total);fflush(stdout);
 printf("EFFECT LIST SEMANTICS PASS %u\n",semantics());fflush(stdout);
 printf("EFFECT LIST FAULT PASS %u ordered failures\n",faults());fflush(stdout);
 printf("EFFECT LIST CHILD PASS %u refusals/pending returns\n",refusals());fflush(stdout);
 unsigned gates=0,cache=0;
 for(unsigned k=0;k<2;k++){defaults();kind=k;gates+=controls();cache+=boundaries();}
 printf("EFFECT LIST GATES PASS %u\n",gates);printf("EFFECT LIST CACHE PASS %u\n",cache);
 puts("EFFECT LIST HOST ALL PASS; native interpreted instructions=0; actual release child, two external matrix services modeled");return 0;
}
