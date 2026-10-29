/* Complete original fade setup, update, cleanup and setter comparisons.
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
static uint8_t battle[343936],setters[24];
static const uint32_t targets[]={0x800b39c0u,0x800b36bcu,0x800b383cu,0x8001cd64u,0x8001cd74u,0x8001cd6cu};
static const unsigned sizes[]={428,384,60,8,8,8};
#define NUM_BODIES 6u

static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,selection,mutation,mutation_seen,zero_color;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int candidate,compare,observing;
static uint32_t ticks=20,remaining=39,task;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[512];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[16];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"COLOR FADE FAIL kind=%u seed=%u stack=%u domain=%u selection=%u ticks=%08x remaining=%08x zero=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",kind,seed,stack_kind,task_domain,selection,ticks,remaining,zero_color,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<NUM_BODIES;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static const uint8_t *original_body(unsigned i)
{return i>=3?setters+(targets[i]-0x8001cd64u):battle+(targets[i]-0x8006faf0u);}
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
 if(compare && (call_count>=expected_calls||memcmp(row,&calls[call_count],sizeof(*row))))fail("callee registers/full memory",target);
 call_count++;
}
/* Packed list operations are explicit models here. The setup, update,
 * cleanup and all three raw setters execute complete original/native bodies. */
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x8001cc18u||target==0x8001ca58u){
  uint32_t record=cpu->gpr[5];
  if(runtime_write(runtime,record,4,cpu->gpr[4])||runtime_write(runtime,record+0x14u,4,0xf1234567u))return -1;
  if(mutation&&target==0x8001cc18u){fixture_store(0x800591acu,1,mutation==1?255:0);fixture_store(0x80059464u,4,0x80000000u);mutation_seen++;}
  if(mutation==2&&target==0x8001ca58u){fixture_store(cpu->gpr[29]+0x1cu,4,0x11223344u);mutation_seen++;}
 } else if(target==0x8001cb48u||target==0x8001cd94u){
  if(runtime_write(runtime,0x80059080u+call_count*4u,4,cpu->gpr[4]))return -1;
 } else return -1;
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=0x43210000u+call_count;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
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
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"real-lower-shared"};
 rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 task=task_domain==0?0x80140000u:task_domain==1?0xa0140000u:task_domain==2?0x80060100u:task_domain==3?0x1f800100u:task_domain==4?(uint32_t)(uintptr_t)host:task_domain==5?0x00140000u:0x80340000u;
 if(kind==0&&(selection==0||selection==2))task=selection==0?0x800c3c50u:0x800c3c00u;
 fixture_store(0x800c3554u,4,task);fixture_store(0x800c3558u,4,kind==0&&(selection==0||selection==2)?0:task);
 fixture_store(0x800c355cu,1,selection==2||selection==3?255:0);fixture_store(0x800d3638u,1,selection==4?255:0);
 fixture_store(0x800591acu,1,seed&1u?255:0);fixture_store(0x80059464u,4,seed==7?0x80000000u:seed);
 fixture_store(task+0xcu,4,0x800b383cu);fixture_store(task+0x38u,4,ticks);fixture_store(task+0x3cu,4,remaining);
 for(unsigned i=0;i<3;i++){
  fixture_store(task+0x42u+i,1,zero_color?0:(0xe1u+seed*37u+i*53u)&255u);
  fixture_store(task+0x45u+i,1,(0x18u+seed*31u+i*67u)&255u);
  fixture_store(task+0x48u+i,1,(0x72u+seed*23u+i*41u)&255u);
 }
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800fe0u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;break;
 case 4:cpu->gpr[29]=task+(kind==1?0x48u:0x58u);break;
 case 5:cpu->gpr[29]++;break;case 6:cpu->gpr[29]=0x1e000000u;break;case 7:cpu->gpr[29]=0;break;
 }
 cpu->gpr[4]=kind?task:ticks;cpu->gpr[5]=kind>=3?0xdeadbeefu:0xabcd0002u;
 cpu->gpr[6]=zero_color?0:0x1234f100u+seed*13u+0x17u;cpu->gpr[7]=zero_color?0:0x5678e200u+seed*31u+0x2bu;
 (void)model_test_raw_write(&rt,cpu->gpr[29]+0x10u,4,zero_color?0:0x99770135u+seed*59u);
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
 if(!fault_at&&!stop_at&&stack_kind<4&&task>=0x200000u&&!(kind==0&&selection==2&&(ticks<<1)==0&&zero_color)&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
 if(native.steps||native_gte_commands||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(mutation&&!mutation_seen&&!fault_at)fail("live mutation not exercised",mutation);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static int controls(void)
{
 for(kind=0;kind<5;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  PcPortMipsCpu original=cpu;uint32_t target=targets[kind];
  for(unsigned i=0;i<3;i++){
   uint8_t *code=kind>=3?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_color_fade_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  if(kind>=3){XenoRetailFileSlot saved=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_color_fade_try_host(&rt,&cpu,target))fail("missing asset",0);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+7u;
   if(battle_color_fade_try_host(&rt,&cpu,target))fail("short asset",0);
   g_XenoRetailFiles[XENO_RD_SLUS]=saved;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_color_fade_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_color_fade_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_color_fade_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_color_fade_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_color_fade_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_color_fade_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_COLOR_FADE","0",1)==0);if(battle_color_fade_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_COLOR_FADE")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("COLOR FADE PASS full identities, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<5;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"timer_task"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=kind>=3?g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target):PSX_ADDR(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_COLOR_FADE","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_COLOR_FADE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Identity is outside the data-state assertion after restoring its byte. */
   if(reject==1&&kind<3){want_ram[(target&0x1fffffu)]^=1;}
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("COLOR FADE PASS 60 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}
static int faults(void)
{
 unsigned total=0;seed=stack_kind=mutation=stop_at=task_domain=0;
 for(kind=0;kind<6;kind++)for(unsigned path=0;path<(kind<2?4u:1u);path++){
  selection=kind==0?path:0;ticks=path&1u?0:40;remaining=path&1u?0:79;zero_color=path==3;
  fault_at=0;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}fault_at=0;
 }
 kind=selection=zero_color=0;ticks=20;remaining=39;
 printf("COLOR FADE PASS %u ordered access faults with full CPU/memory/GTE state\n",total);return 1;
}
static int pending(void)
{
 kind=selection=zero_color=stack_kind=mutation=stop_at=task_domain=0;ticks=20;remaining=39;
 PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_return=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;pending_return=0;
 if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=1||cpu.gpr[2]!=0x43210001u||cpu.gpr[31]!=0x800b3a78u||cpu.hi!=0xa7650001u)fail("pending return refusal/partial state",(uint32_t)rc);
 puts("COLOR FADE PASS pending return refuses with completed callee state");return 1;
}
static unsigned products(void)
{
 const uint32_t factors[]={0,1,2,0x7fff,0x8000,0x8001,0xffff,0x12348000};
 const int inputs[]={-32768,-255,-1,0,1,255,32767};unsigned total=0;
 for(unsigned f=0;f<8;f++)for(unsigned r=0;r<7;r++)for(unsigned g=0;g<7;g++)for(unsigned b=0;b<7;b++){
  uint32_t delta[]={0x4e5f0000u|(uint16_t)inputs[r],0xa9bc0000u|(uint16_t)inputs[g],0x13570000u|(uint16_t)inputs[b]};
  memset(&gteRegs,(int)(0x38u+f),sizeof(gteRegs));MTC2(factors[f],8);for(unsigned i=0;i<3;i++)MTC2(delta[i],(int)(9u+i));GTERegisters initial=gteRegs;
  color_fade_backend_command(0x4b90003d);GTERegisters expected=gteRegs;gteRegs=initial;
  battle_color_fade_product(factors[f],delta);
  if(memcmp(&expected,&gteRegs,sizeof(gteRegs)))fail("fixed product full GTE state",f);
  total++;
 }
 printf("COLOR FADE PRODUCT PASS %u independent native arithmetic/backend register comparisons\n",total);return total;
}
static int indirect(void)
{
 kind=1;remaining=zero_color=0;selection=stack_kind=task_domain=0;
 PcPortMipsCpu cpu;setup(&cpu);observing=0;cpu.gpr[31]=0x800b371cu;
 rt.functions[0]=(ResolvedFunction){0x8001cd08u,(void *)legacy_probe,"unapproved-fade-callback"};rt.function_count=1;
 BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8001cd08u);slot->target=0x8001cd08u;slot->state=1;slot->entry=rt.functions[0];
 if(runtime_bridge(&rt,&cpu,0x8001cd08u)!=-1||legacy_calls)fail("unapproved callback",0);
 puts("COLOR FADE PASS unapproved indirect callback refused before cached dispatch");return 1;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0xd564,SEEK_SET)==0&&fread(setters,1,24,f)==24&&fclose(f)==0);
 assert(XenoRetailData_File(XENO_RD_SLUS));
 const char *hashes[]={"01ca07dd594ac760407b9f6af4e15392cd704e880f45b2e702d252eeadb22c27","1de6b281e156a3477dc5cfb8e402b0f8a03e8a39fb228bf09a05c3dc335c503f","eafac70a3a3d04c69ac4806429b1960ac03d4ca162386888805c00237e208041","6b5d5f1a0f2b7c721b6d3106c6530c0974e5b76ba7599b0e84edc41c452f5385","80cd297c0a26df8ec5f387cc5cb1e8039897493be7456a05b23f5b596b2181aa","6b5d5f1a0f2b7c721b6d3106c6530c0974e5b76ba7599b0e84edc41c452f5385"};
 for(unsigned i=0;i<NUM_BODIES;i++){
  uint8_t digest[32];assert(SHA256(original_body(i),sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,hashes[i])==0);
 }
}
int main(int argc,char **argv)
{
 load_inputs();
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--zero")==0){ticks=0;return run_case()?0:1;}
  if(strcmp(argv[1],"--cleanup")==0){ticks=zero_color=0;zero_color=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--reuse")==0){selection=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--alternate")==0){selection=2;return run_case()?0:1;}
  if(strcmp(argv[1],"--disabled")==0){selection=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--live")==0){mutation=1;return run_case()?0:1;}
  if(strcmp(argv[1],"--stack")==0){kind=1;stack_kind=4;return run_case()?0:1;}
  if(strcmp(argv[1],"--product")==0)return products()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--indirect")==0)return indirect()?0:1;
  return 2;
 }
 unsigned total=0;const uint32_t words[]={0,1,20,0xffffffffu,0x80000000u,0x7fffffffu,0x10000u,0x10001u};
 for(kind=0;kind<6;kind++)for(seed=0;seed<8;seed++)for(task_domain=0;task_domain<7;task_domain++){
  stack_kind=seed%4;selection=kind==0?seed%5:0;ticks=words[seed];remaining=words[(seed+2)%8];zero_color=seed&1u;run_case();total++;
 }
 task_domain=stack_kind=seed=0;
 for(kind=0;kind<2;kind++)for(selection=0;selection<(kind==0?5u:1u);selection++)for(unsigned t=0;t<8;t++)for(zero_color=0;zero_color<2;zero_color++){
  ticks=words[t];remaining=words[t];run_case();total++;
 }
 selection=zero_color=0;ticks=20;remaining=39;
 for(kind=0;kind<6;kind++)for(stack_kind=4;stack_kind<8;stack_kind++){run_case();total++;}
 stack_kind=kind=0;for(mutation=1;mutation<=2;mutation++){run_case();total++;}mutation=0;
 printf("COLOR FADE PASS %u original/native comparisons, selection/duration/color states, aliases and live changes\n",total);fflush(stdout);
 unsigned refusals=0;
 for(kind=0;kind<3;kind++){
  ticks=0;remaining=0;zero_color=1;run_case();unsigned n=expected_calls;
  for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();refusals++;}
  stop_at=stop_mode=0;
 }
 printf("COLOR FADE PASS %u callee refusals with exact partial state\n",refusals);fflush(stdout);
 ticks=20;remaining=39;zero_color=selection=kind=0;
 if(!products()||!faults()||!controls()||!pending()||!boundaries()||!indirect())return 1;
 puts("COLOR FADE ALL PASS native interpreted instructions=0 and graphics commands=0; list services modeled");return 0;
}
