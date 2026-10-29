/* Complete original VRAM backup and texture setup comparisons.
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
static const uint32_t targets[]={0x80077990u};
static const unsigned sizes[]={1816};
#define NUM_BODIES 1u

static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,selection,mutation,mutation_seen,zero_color;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int candidate,compare,observing;
static uint32_t ticks=20,remaining=39,task;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[512];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[32];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"VRAM SETUP FAIL kind=%u seed=%u stack=%u domain=%u selection=%u ticks=%08x remaining=%08x zero=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,seed,stack_kind,task_domain,selection,ticks,remaining,zero_color,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<NUM_BODIES;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static const uint8_t *original_body(unsigned i)
{return battle+(targets[i]-0x8006faf0u);}
static uint32_t owner,alternate_owner,state,alternate_state,table,alternate_table;
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
 if(compare && (call_count>=expected_calls||memcmp(row,&calls[call_count],sizeof(*row))))fail("callee registers/full memory",target);
 call_count++;
}
/* GPU and atlas services are explicit deterministic models here. Native
 * implementations, VRAM contents and concrete interfaces are checked separately. */
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 uint32_t result=0x43210000u+call_count;
 if(target==0x800448f8u){
  const uint8_t *r=module_memory_pointer(runtime,cpu->gpr[4],8,0);
  uint8_t *dst=module_memory_pointer(runtime,cpu->gpr[5],396,1);
  if(!r||!dst||load_le(r+4,2)!=198||load_le(r+6,2)!=1)return -1;
  uint32_t pattern=load_le(r,2)+load_le(r+2,2)*3u+seed*17u+call_count*31u;
  for(unsigned i=0;i<396;i++)dst[i]=(uint8_t)(pattern+i*23u);
 }else if(target==0x80026338u){
  uint32_t out[6]={cpu->gpr[6],cpu->gpr[7]},index=cpu->gpr[5];
  uint8_t *t=module_memory_pointer(runtime,cpu->gpr[4],4,0);if(!t)return -1;
  uint32_t salt=load_le(t,4);
  for(unsigned i=2;i<6;i++)if(runtime_read(runtime,cpu->gpr[29]+0x10u+(i-2u)*4u,4,&out[i]))return -1;
  for(unsigned i=0;i<6;i++)if(runtime_write(runtime,out[i],4,(salt+index*19u+i*97u+seed*13u)&0xffffu))return -1;
 }else if(target==0x80043a1cu){
  result=((cpu->gpr[4]<<7)|(cpu->gpr[5]<<5)|((cpu->gpr[7]&0x100u)>>4)|((cpu->gpr[6]&0x3ffu)>>6)|((cpu->gpr[7]&0x200u)<<2))&0xffffu;
 }else if(target==0x800454dcu){
  uint32_t rectangle,tag;
  if(runtime_read(runtime,cpu->gpr[29]+0x10u,4,&rectangle)||runtime_read(runtime,cpu->gpr[4],4,&tag))return -1;
  uint8_t *r=module_memory_pointer(runtime,rectangle,8,0);if(!r)return -1;
  uint32_t packed=load_le(r,4)^load_le(r+4,4);
  if(runtime_write(runtime,cpu->gpr[4],4,(tag&0xffffffu)|0x02000000u)||runtime_write(runtime,cpu->gpr[4]+4u,4,0xe1000000u|cpu->gpr[7])||runtime_write(runtime,cpu->gpr[4]+8u,4,packed))return -1;
 }else return -1;
 if(mutation==1&&call_count<=16){fixture_store(0x800c3ea4u,4,call_count&1u?alternate_owner:owner);mutation_seen++;}
 if(mutation==2&&target==0x80026338u){fixture_store(0x800c3ea4u,4,call_count&1u?alternate_owner:owner);fixture_store(0x800d2f5cu,4,call_count&1u?alternate_table:table);mutation_seen++;}
 if(mutation==3&&call_count>=20){fixture_store(0x800c3ea4u,4,call_count&1u?alternate_owner:owner);fixture_store(0x800d2d28u,4,call_count&1u?alternate_state:state);mutation_seen++;}
 if(mutation==4&&call_count==1){fixture_store(cpu->gpr[29]+0x20u,4,0x11223344u);fixture_store(cpu->gpr[29]+0x38u,4,0x55667788u);mutation_seen++;}
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=result;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
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
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};
 rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 owner=task_domain==0?0x80100000u:task_domain==1?0xa0100000u:task_domain==2?0x80050000u:task_domain==3?(uint32_t)(uintptr_t)host:task_domain==4?0x00100000u:task_domain==5?0x80300000u:0x1f800000u;
 alternate_owner=task_domain==2?owner+0xc000u:owner+0x20000u;
 const uint32_t states[]={0x80180000u,0xa0180000u,0x80060100u,0x1f800100u,(uint32_t)(uintptr_t)host+0x50000u,0x00180000u,0x80380000u};
 state=states[selection%7u];alternate_state=0x80182000u;table=0x80190000u;alternate_table=0x80192000u;task=owner;
 for(unsigned pass=0;pass<2;pass++){
  uint32_t base=pass?alternate_owner:owner;
  for(unsigned i=0;i<4;i++){
   (void)model_test_raw_write(&rt,shard_sdk_address(base+0x8950u+i*8u),2,1);
   (void)model_test_raw_write(&rt,shard_sdk_address(base+0x8952u+i*8u),2,40u+i+seed);
   (void)model_test_raw_write(&rt,shard_sdk_address(base+0x8954u+i*8u),2,198);
   (void)model_test_raw_write(&rt,shard_sdk_address(base+0x8956u+i*8u),2,1);
  }
  for(unsigned i=0;i<29;i++)(void)model_test_raw_write(&rt,shard_sdk_address(base+0xa238u+i*4u),4,0x80000000u+seed*167u+i*19u+pass*0x312u);
  (void)model_test_raw_write(&rt,shard_sdk_address(base+0xa240u),2,ticks);
 }
 fixture_store(table,4,0x10u+seed);fixture_store(alternate_table,4,0x35u+seed);
 fixture_store(0x800c3ea4u,4,owner);fixture_store(0x800d2d28u,4,state);fixture_store(0x800d2f5cu,4,table);
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800fe0u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;break;
 case 4:cpu->gpr[29]=owner+0xa288u;break;case 5:cpu->gpr[29]=owner+0x8990u;break;
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
 if(!fault_at&&!stop_at&&stack_kind<4&&task_domain<4&&selection<5&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
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
 for(kind=0;kind<1;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  for(unsigned i=0;i<3;i++){
   uint8_t *code=PSX_ADDR(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_vram_setup_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_vram_setup_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_vram_setup_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_vram_setup_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_vram_setup_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_vram_setup_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_vram_setup_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_VRAM_SETUP","0",1)==0);if(battle_vram_setup_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_VRAM_SETUP")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("VRAM SETUP PASS full identities, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<1;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"timer_task"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_VRAM_SETUP","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_VRAM_SETUP")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1&&kind<3){want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("VRAM SETUP PASS 12 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}
static int faults(void)
{
 unsigned total=0;seed=stack_kind=mutation=stop_at=task_domain=selection=0;ticks=240;
 for(unsigned path=0;path<3;path++){
  mutation=path==1?1:path==2?3:0;fault_at=0;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}fault_at=0;
 }
 mutation=0;printf("VRAM SETUP PASS %u ordered access faults with full CPU/memory state\n",total);return 1;
}
static int pending(void)
{
 kind=selection=zero_color=stack_kind=mutation=stop_at=task_domain=0;ticks=20;remaining=39;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||cpu.gpr[2]!=0x43210001u||cpu.gpr[31]!=0x80077af4u||cpu.hi!=0xa7650001u)fail("pending return refusal/partial state",(uint32_t)rc);
 puts("VRAM SETUP PASS pending return refuses with completed callee state");return 1;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 uint8_t digest[32];assert(SHA256(original_body(0),sizes[0],digest));char text[65];
 for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
 assert(strcmp(text,"8ecb274a1cfafa6c298546d68e180b82aa784dfd69f3899287d38ee8e4382a16")==0);
}
int main(int argc,char **argv)
{
 load_inputs();ticks=240;
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--live")==0){mutation=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--atlas")==0){mutation=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--packets")==0){mutation=3;return run_case()?0:1;}
  if(strcmp(argv[1],"--stack")==0){mutation=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  return 2;
 }
 unsigned total=0;const uint32_t words[]={0,1,240,0xffffffffu,0x80000000u,0x7fffffffu,0x8000u,0x10001u};
 for(seed=0;seed<8;seed++)for(task_domain=0;task_domain<7;task_domain++)for(selection=0;selection<7;selection++){
  stack_kind=seed%4;ticks=words[seed];run_case();total++;
 }
 task_domain=selection=seed=0;ticks=240;
 for(stack_kind=4;stack_kind<9;stack_kind++){run_case();total++;}
 stack_kind=0;for(mutation=1;mutation<=4;mutation++){run_case();total++;}mutation=0;
 printf("VRAM SETUP PASS %u original/native comparisons, pointer domains, narrowing, aliases and live changes\n",total);fflush(stdout);
 run_case();unsigned n=expected_calls,refusals=0;assert(n==28);
 for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();refusals++;}
 stop_at=stop_mode=0;printf("VRAM SETUP PASS %u callee refusals with exact partial state\n",refusals);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries())return 1;
 puts("VRAM SETUP ALL PASS native interpreted instructions=0 and graphics commands=0; external services modeled");return 0;
}
