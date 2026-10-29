/* Complete original packet-render dispatcher comparisons.
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
int32_t D_80050104;
uint32_t D_800595C0;
uint16_t D_80059308,D_8005930C;
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
static uint8_t battle[343936],main_exe[303104];
static const uint32_t targets[]={0x8002c700u,0x80018980u};
static const unsigned sizes[]={460,24};
#define NUM_BODIES 1u
#define NEW_BODIES 1u
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x100000],want_host[sizeof(host)];
static unsigned kind,seed,stack_kind,task_domain,mutation,mutation_seen;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,stop_mode,pending_return,legacy_calls;
static int compare,observing;
static unsigned hash_call_snapshots;
static uint32_t header,commands,output=0x80180000u,ot=0x80190000u;
static unsigned groups=3,variant,primitive=3,command_domain,alias_case,cull_word,cull_result,header_count=0xffffu;
static uint32_t command_count=2,command_stride=8,forced_callback,stale_callback=0x80194000u;
static unsigned callback_mode;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[65536];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[PSX_RAM_SIZE],shared[sizeof(shared)],pad[4096],host[sizeof(host)];} Call;
static Call calls[160];
/* Large-count paths keep full-state hashes at each call and retain the usual
 * byte-exact final-memory comparison, without 900 MiB of call snapshots. */
typedef struct HashedCall {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[32],shared[32],pad[32],host[32];} HashedCall;
static HashedCall hashed_calls[1024];
static PcPortMipsCpu pending_cpu;
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"BATTLE PACKET RENDER HOST FAIL seed=%u groups=%u variant=%u primitive=%u cull=%u/%u count=%08x stride=%08x stack=%u domain=%u commands=%u alias=%u mutation=%u stop=%u/%u pending=%u fault=%u access=%u call=%u %s %08x\n",seed,groups,variant,primitive,cull_word,cull_result,command_count,command_stride,stack_kind,task_domain,command_domain,alias_case,mutation,stop_at,stop_mode,pending_return,fault_at,accesses,call_count,what,value);exit(1);
}
static int body_index(uint32_t address)
{for(unsigned i=0;i<NUM_BODIES;i++)if(address-targets[i]<sizes[i])return (int)i;return -1;}
static const uint8_t *original_body(unsigned i)
{return main_exe+(targets[i]-0x8000f800u);}
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
  assert(call_count<1024);HashedCall row;memset(&row,0,sizeof(row));
  row.target=target;row.hi=cpu->hi;row.lo=cpu->lo;row.gte=gteRegs;
  memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
  assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
  assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
  if(compare){if(call_count>=expected_calls||memcmp(&row,&hashed_calls[call_count],sizeof(row)))fail("large-count callee state hashes",target);}
  else hashed_calls[call_count]=row;
  call_count++;return;
 }
 assert(call_count<160);static Call scratch;Call *row=compare?&scratch:&calls[call_count];
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

/* Cull and every render callback are modeled; no successful empty draw is
 * substituted for an unconverted production child. */
static uint32_t callback_token(unsigned row,unsigned slot){return 0x80190000u+row*0x40u+slot*4u;}
static int service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 int cull=target==0x8003101cu;
 if(!cull&&(target<0x80190000u||target>0x80194000u))return -1;
 if(runtime_write(runtime,0x80059488u,4,target^call_count))return -1;
 if(mutation==1&&cull&&!mutation_seen){fixture_store(header+6u,2,2);fixture_store(header+0x10u,4,commands+0x100u);mutation_seen++;}
 if(mutation==2&&!cull&&!mutation_seen){fixture_store(header+6u,2,0);mutation_seen++;}
 if(mutation==3&&!cull&&!mutation_seen){fixture_store(cpu->gpr[17]+2u,2,0xffffu);fixture_store(cpu->gpr[16]+0x1cu,4,0xfffffff8u);mutation_seen++;}
 if(mutation==4&&!cull&&!mutation_seen){fixture_store(0x80059528u,4,commands+0x100u-16u);mutation_seen++;}
 if(mutation==5&&!cull&&!mutation_seen){fixture_store(cpu->gpr[16]+(variant%6u)*4u,4,callback_token(16,5));mutation_seen++;}
 if(mutation==6&&!mutation_seen){
  fixture_store(0x80018980u+(variant%6u)*4u,4,0x8002c848u);mutation_seen++;
 }
 if(mutation==7&&!cull&&!mutation_seen){cpu->gpr[18]=callback_token(16,4);mutation_seen++;}
 if(mutation==8&&!mutation_seen){
  uint32_t sp=cpu->gpr[29];for(unsigned i=0;i<0x30u;i+=4u){uint32_t v;assert(model_test_raw_read(runtime,sp+i,4,&v)==0);fixture_store(sp+0x400u+i,4,v);}
  cpu->gpr[29]+=0x400u;mutation_seen++;
 }
 if(mutation==9&&!cull&&!mutation_seen){fixture_store(0x80050104u,4,0xffffffffu);fixture_store(0x800595c0u,4,0xfacebeefu);mutation_seen++;}
 for(unsigned i=1;i<16;i++)cpu->gpr[i]=0x41000000u+call_count*256u+i;
 cpu->gpr[2]=cull?cull_result:0x43210000u+call_count;cpu->gpr[24]=0x12340000u+call_count;cpu->gpr[25]=0x56780000u+call_count;
 cpu->hi=0xa7650000u+call_count;cpu->lo=0xb2100000u+call_count;cpu->cp0[3]=0xc3450000u+call_count;
 gteRegs.CP2D.r[9]=target^call_count;return 1;
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
static uint32_t domain_address(unsigned domain,uint32_t normal,uint32_t shared_address,uint32_t scratch,uint32_t host_offset)
{
 switch(domain){case 0:return normal;case 1:return normal|0x20000000u;case 2:return shared_address;
 case 3:return (uint32_t)(uintptr_t)host+host_offset;case 4:return normal&0x1fffffu;case 5:return normal+0x200000u;
 case 6:return scratch;case 7:return 0x1e000000u;case 8:return 0;default:return normal+1u;}
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(&gteRegs,(int)(0xa5u+seed),sizeof(gteRegs));memcpy(PSX_ADDR(0x8000f800u),main_exe,sizeof(main_exe));memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 /* Match production's omitted executable text. The test-only oracle fetches
  * instructions from original_body(), independently of guest data RAM. */
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"real-lower-shared"};
 rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,host,"unused-upper-shadow"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0x12345600u+i;
 cpu->gpr[18]=stale_callback;cpu->gpr[30]=0x87650000u+seed;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 header=domain_address(task_domain,0x80100000u,0x80053000u,0x1f800200u,0x10000u);
 commands=domain_address(command_domain,0x80110000u,0x80054000u,0x1f800600u,0x50000u);
 uint32_t descriptor=0x8004fe50u+primitive*40u;
 if(alias_case==1)header=0x80059520u;
 if(alias_case==2)commands=0x80059528u;
 if(alias_case==3)header=descriptor;
 if(alias_case==4)commands=descriptor+0x1cu;
 for(unsigned i=0;i<6;i++)fixture_store(descriptor+4u*i,4,callback_mode?forced_callback:callback_token(primitive,i));
 fixture_store(descriptor+0x1cu,4,command_stride);
 if(task_domain<7||alias_case==1||alias_case==3){
  fixture_store(header+4u,2,header_count);fixture_store(header+6u,2,groups);
  fixture_store(header+8u,4,0x80121000u+seed);fixture_store(header+0xcu,4,0x80122000u+seed);
  fixture_store(header+0x10u,4,commands);fixture_store(header+0x18u,4,0x80123000u+seed);
 }
 if(command_domain<7||alias_case==2||alias_case==4){
  unsigned count=groups>=32768?1:groups;
  for(unsigned i=0;i<count;i++){
   uint32_t cursor=commands+i*(4u+(uint32_t)(int32_t)(int16_t)command_count*command_stride);
   fixture_store(cursor,1,primitive);fixture_store(cursor+1u,1,0x83u+seed);fixture_store(cursor+2u,2,command_count);
  }
  if(mutation==1||mutation==4)for(unsigned i=0;i<2;i++){
   uint32_t cursor=commands+0x100u+i*20u;fixture_store(cursor,1,primitive);fixture_store(cursor+2u,2,2);
  }
  if(mutation==3)for(unsigned i=1;i<3;i++){
   uint32_t cursor=commands+i*12u;fixture_store(cursor,1,primitive);fixture_store(cursor+2u,2,0xffffu);
  }
 }
 fixture_store(0x80050104u,4,cull_word);fixture_store(0x800595c0u,4,0xfffffff0u+seed);
 cpu->gpr[4]=header;cpu->gpr[5]=output;cpu->gpr[6]=ot;cpu->gpr[7]=variant;
 switch(stack_kind){
 case 1:cpu->gpr[29]=0x1f800f80u;break;case 2:cpu->gpr[29]|=0x20000000u;break;
 case 3:cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x90000u;break;case 4:cpu->gpr[29]=0x80058000u;break;
 case 5:cpu->gpr[29]=commands+0x30u;break;case 6:cpu->gpr[29]=header+0x30u;break;
 case 7:cpu->gpr[29]++;break;case 8:cpu->gpr[29]=0x1e000000u;break;case 9:cpu->gpr[29]=0;break;
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
 if(!fault_at&&!stop_at&&!alias_case&&stack_kind<5&&task_domain<4&&command_domain<4&&!callback_mode&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle fault",oracle.pc);
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
   uint8_t *code=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);unsigned off=i==0?0:i==1?sizes[kind]/2:sizes[kind]-1;code[off]^=1;
   if(battle_packet_render_try_host(&rt,&cpu,target))fail("identity accepted",target+off);
   code[off]^=1;
  }
  for(unsigned i=0;i<3;i++){
   unsigned off=i==0?0:i==1?12:23;uint8_t *table=PSX_ADDR(0x80018980u);table[off]^=1;
   if(battle_packet_render_try_host(&rt,&cpu,target))fail("table identity accepted",off);
   table[off]^=1;
  }
  XenoRetailFileSlot original_slot=g_XenoRetailFiles[XENO_RD_SLUS];
  g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_packet_render_try_host(&rt,&cpu,target))fail("missing image",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+sizes[kind]-1u;
  if(battle_packet_render_try_host(&rt,&cpu,target))fail("short image",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;
  cpu.load_valid=1;cpu.load_reg=4;if(battle_packet_render_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=original;
  rt.force_interpret=1;if(battle_packet_render_try_host(&rt,&cpu,target))fail("forced entry",0);rt.force_interpret=0;
  rt.initialized=0;if(battle_packet_render_try_host(&rt,&cpu,target))fail("uninitialized entry",0);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_packet_render_try_host(&rt,&cpu,target))fail("inactive entry",0);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_packet_render_try_host(&rt,&cpu,target))fail("wrong context",0);rt.bridge_cpu=&cpu;
  if(battle_packet_render_try_host(&rt,&cpu,target+4u))fail("wrong target",0);
  assert(setenv("XENO_BATTLE_NATIVE_PACKET_RENDER","0",1)==0);if(battle_packet_render_try_host(&rt,&cpu,target))fail("opt out",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_PACKET_RENDER")==0);
  if(memcmp(&cpu,&original,sizeof(cpu))||!memory_equal()||call_count||accesses)fail("gate side effect",0);
 }
 puts("BATTLE PACKET RENDER HOST PASS private-image/live-table identities, absent/short images, empty text RAM, context, pending entry and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{(void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;legacy_calls++;return 0xfacebeefu;}
static int boundaries(void)
{
 for(kind=0;kind<NEW_BODIES;kind++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;uint32_t target=targets[kind];
  rt.functions[0]=(ResolvedFunction){target,(void *)legacy_probe,"timer_task"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_PACKET_RENDER","0",1)==0);
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();snapshot();PcPortMipsCpu old=cpu;
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,100000);
  if(reject==1)code[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_PACKET_RENDER")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption bypass",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native precedence",(uint32_t)rc);
  if(reject){
   /* Private code identity is separate from all guest data-state snapshots. */
   if((disable?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal())fail("rejected dispatch falls through",(uint32_t)rc);
  }
 }
 puts("BATTLE PACKET RENDER HOST PASS 12 resolved/cached native-precedence and rejected-dispatch checks");return 1;
}

static void defaults(void)
{
 kind=seed=stack_kind=task_domain=mutation=hash_call_snapshots=0;
 stop_at=stop_mode=pending_return=fault_at=alias_case=command_domain=variant=cull_word=cull_result=callback_mode=0;
 groups=3;primitive=3;header_count=0xffffu;command_count=2;command_stride=8;
 output=0x80180000u;ot=0x80190000u;forced_callback=0;stale_callback=0x80194000u;
}
static int faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<9;path++){
  defaults();groups=path==0?0:1;variant=path%7u;
  if(path==1||path==2){cull_word=0x80000001u;cull_result=path==1?0x80000000u:0;}
  if(path==3){task_domain=3;command_domain=3;stack_kind=3;}
  if(path==4){command_count=0x8000u;command_stride=0x80000000u;}
  if(path==5)stack_kind=1;
  if(path==6)alias_case=1;
  if(path==7){mutation=6;cull_word=1;}
  if(path==8){primitive=255;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("BATTLE PACKET RENDER HOST PASS %u ordered access faults with complete architectural registers/memory\n",total);return 1;
}
static int pending(void)
{
 for(unsigned path=0;path<2;path++){
  defaults();cull_word=1;pending_return=path+1;PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,targets[kind],BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||call_count!=pending_return||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal())fail("pending return refusal/partial state",(uint32_t)rc);
 }
 defaults();puts("BATTLE PACKET RENDER HOST PASS 2 pending returns refuse with completed callee state");return 1;
}
static int high_counts(void)
{
 static const unsigned counts[]={64,128,257};
 for(unsigned i=0;i<3;i++){
  defaults();groups=counts[i];variant=i*2;hash_call_snapshots=1;run_case();
  if(expected_calls!=groups)fail("complete high-count calls",expected_calls);
 }
 defaults();puts("BATTLE PACKET RENDER HOST PASS 3 complete high-count loops with full-state call hashes and exact final memory");return 1;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 f=fopen("disc/SLUS_006.64","rb");assert(f&&fread(main_exe,1,sizeof(main_exe),f)==sizeof(main_exe)&&fclose(f)==0);
 assert(XenoRetailData_File(XENO_RD_SLUS));
 for(unsigned i=0;i<2;i++){
  uint8_t digest[32];assert(SHA256(original_body(i),sizes[i],digest));char text[65];
  for(unsigned j=0;j<32;j++)snprintf(text+j*2,3,"%02x",digest[j]);
  assert(strcmp(text,battle_packet_render_bodies[i].sha)==0);
 }
}
static unsigned child_gates(void)
{
 assert(setenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS","0",1)==0);
 assert(setenv("XENO_BATTLE_NATIVE_QUAD_WALKERS","0",1)==0);
 assert(setenv("XENO_BATTLE_NATIVE_TRIANGLE_MAX","0",1)==0);
 assert(setenv("XENO_BATTLE_NATIVE_QUAD_MAX","0",1)==0);
 uint32_t children[105]={0x8003101cu,0x8002cdccu,0x8002c700u};unsigned n=3,total=0;
 for(unsigned row=0;row<17;row++)for(unsigned slot=0;slot<6;slot++){
  uint32_t target=load_le(main_exe+(0x8004fe50u-0x8000f800u)+row*40u+slot*4u,4);
  unsigned i=0;while(i<n&&children[i]!=target)i++;if(i==n)children[n++]=target;
 }
 for(unsigned i=0;i<n;i++)for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  cpu.gpr[31]=i==0?0x8002c744u:0x8002c86cu;
  rt.functions[0]=(ResolvedFunction){children[i],(void *)legacy_probe,"unadopted_packet_child"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,children[i]);slot->target=children[i];slot->state=1;slot->entry=rt.functions[0];}
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=disabled?PcPortMipsRun(&cpu,children[i],cpu.gpr[31],100):runtime_bridge(&rt,&cpu,children[i]);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if((disabled?rc>=0:rc!=0)||legacy_calls||cpu.steps||rt.bridge_cpu||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,old.cp0,sizeof(cpu.cp0))||cpu.hi!=old.hi||cpu.lo!=old.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("unadopted child dispatch",children[i]);
  total++;
 }
 assert(unsetenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS")==0);
 assert(unsetenv("XENO_BATTLE_NATIVE_QUAD_WALKERS")==0);
 assert(unsetenv("XENO_BATTLE_NATIVE_TRIANGLE_MAX")==0);
 assert(unsetenv("XENO_BATTLE_NATIVE_QUAD_MAX")==0);
 defaults();printf("BATTLE PACKET RENDER HOST PASS %u child gates covering unadopted or disabled render callbacks, culler and converted builder/caller targets\n",total);return total;
}
static unsigned data_bindings(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;
 rt.data_count=5;
 rt.data[0]=(ResolvedData){0x80018970u,0x80u,host+0x100u,"jump-neighbor"};
 rt.data[1]=(ResolvedData){0x8004fe50u,1088u,host+0x400u,"LP64-descriptors"};
 rt.data[2]=(ResolvedData){0x80050104u,60u,host+0x1000u,"wide-cull-word"};
 rt.data[3]=(ResolvedData){0x80059564u,8u,host+0x1400u,"audio-list-head"};
 rt.data[4]=(ResolvedData){0x800595c0u,8u,host+0x1500u,"wide-counter"};
 memcpy(want_host,host,sizeof(host));
 initialize_model_packet_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 const struct {uint32_t address,size;uint8_t *owner;} owners[]={
  {0x80018980u,24u,PSX_ADDR(0x80018980u)},{0x8004fe50u,680u,PSX_ADDR(0x8004fe50u)},
  {0x80059424u,4u,PSX_ADDR(0x80059424u)},{0x80059528u,4u,PSX_ADDR(0x80059528u)},
  {0x80059538u,4u,PSX_ADDR(0x80059538u)},{0x8005952cu,4u,PSX_ADDR(0x8005952cu)},
  {0x8005953cu,4u,PSX_ADDR(0x8005953cu)},{0x80059498u,4u,PSX_ADDR(0x80059498u)},
  {0x80059308u,2u,(uint8_t *)&D_80059308},{0x8005930cu,2u,(uint8_t *)&D_8005930C},
  {0x800595c0u,4u,(uint8_t *)&D_800595C0},{0x80059568u,4u,PSX_ADDR(0x80059568u)},
  {0x80050104u,4u,(uint8_t *)&D_80050104}
 };
 size_t count=rt.data_count;initialize_model_packet_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 if(rt.data_count!=count)fail("binding idempotence",(uint32_t)rt.data_count);
 unsigned total=0;
 for(unsigned i=0;i<sizeof(owners)/sizeof(owners[0]);i++){
  ResolvedData *entry=find_data(&rt,owners[i].address,owners[i].size);
  if(!entry||entry->address!=owners[i].address||entry->size!=owners[i].size||entry->host!=owners[i].owner)fail("exact data owner",owners[i].address);
  unsigned width=owners[i].size==2?2:4;
  for(unsigned offset=0;offset<owners[i].size;offset+=width)for(unsigned alias=0;alias<3;alias++){
   uint32_t address=owners[i].address+offset;
   uint32_t token=alias==0?address:alias==1?address|0x20000000u:address+0x200000u;
   uint32_t value=(0x82345600u+i*16u+alias)&(width==2?0xffffu:0xffffffffu),read;
   /* The existing resolver shares KSEG0/KSEG1 globals but sends higher
    * RAM mirrors to raw RAM. Preserve and expose that coverage limit. */
   uint8_t *expected=alias==2?PSX_ADDR(address):owners[i].owner+offset;
   uint32_t previous=load_le(owners[i].owner+offset,width);
   if(runtime_write(&rt,token,width,value)||runtime_read(&rt,token,width,&read)||read!=value||load_le(expected,width)!=value)fail("aliased data owner",token);
   if(expected!=owners[i].owner+offset&&load_le(owners[i].owner+offset,width)!=previous)fail("raw mirror changed shared owner",token);
   total++;
  }
 }
 /* The original inferred ranges must not swallow adjacent guest words. */
 const uint32_t adjacent[]={0x80050108u,0x800595c4u,0x800500f8u};
 for(unsigned i=0;i<3;i++){
  uint32_t value=0x91234500u+i;if(runtime_write(&rt,adjacent[i],4,value)||load_le(PSX_ADDR(adjacent[i]),4)!=value)fail("binding width spill",adjacent[i]);
 }
 if(memcmp(want_host,host,sizeof(host)))fail("legacy descriptors/audio owner overwritten",0);
 uint32_t audio;if(runtime_read(&rt,0x80059564u,4,&audio)||audio!=0xb6b6b6b6u)fail("audio owner changed",audio);
 defaults();printf("BATTLE PACKET RENDER HOST PASS %u binding accesses, exact widths, KSEG aliases/raw mirrors, idempotence and untouched legacy/audio owners\n",total);return total;
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(strcmp(argv[1],"--basic")==0)return run_case()?0:1;
  if(strcmp(argv[1],"--high-count")==0)return high_counts()?0:1;
  if(strcmp(argv[1],"--faults")==0)return faults()?0:1;
  if(strcmp(argv[1],"--gates")==0)return controls()?0:1;
  if(strcmp(argv[1],"--pending")==0)return pending()?0:1;
  if(strcmp(argv[1],"--boundary")==0)return boundaries()?0:1;
  if(strcmp(argv[1],"--child-gates")==0)return child_gates()?0:1;
  if(strcmp(argv[1],"--data-bindings")==0)return data_bindings()?0:1;
  return 2;
 }
 unsigned total=0;
 static const uint32_t variants[]={0,1,2,3,4,5,6,7,0xffffffffu,0x80000000u};
 for(unsigned v=0;v<10;v++)for(unsigned c=0;c<3;c++)for(unsigned count=0;count<4;count++){
  defaults();variant=variants[v];cull_word=c?0x80000001u:0;cull_result=c==2?0x80000000u:0;groups=count;seed=v+c;run_case();total++;
 }
 for(unsigned prim=0;prim<17;prim++)for(unsigned v=0;v<6;v++){
  defaults();primitive=prim;variant=v;groups=1;seed=prim;run_case();total++;
 }
 for(unsigned domain=0;domain<10;domain++)for(unsigned command=0;command<10;command++){
  defaults();groups=1;task_domain=domain;command_domain=command;seed=domain;run_case();total++;
 }
 static const uint32_t values[]={0,1,0xffffffffu,0x80000000u,0x7fffffffu,0x8000u,0xffffu};
 for(unsigned i=0;i<7;i++)for(unsigned j=0;j<7;j++){
  defaults();groups=1;command_count=values[i];command_stride=values[j];ot=values[i];output=values[j];run_case();total++;
 }
 for(unsigned s=0;s<10;s++){defaults();stack_kind=s;run_case();total++;}
 for(unsigned a=1;a<5;a++){defaults();alias_case=a;groups=1;run_case();total++;}
 for(unsigned m=1;m<=9;m++){
  defaults();mutation=m;if(m==1||m==6)cull_word=1;if(m==7)variant=6;run_case();total++;
 }
 static const unsigned unusual_prim[]={17,63,255};
 for(unsigned i=0;i<3;i++){defaults();primitive=unusual_prim[i];groups=1;run_case();total++;}
 static const uint32_t invalid_callbacks[]={0,0x1e000000u,0xdeadbeefu};
 for(unsigned i=0;i<3;i++){
  defaults();groups=1;callback_mode=1;forced_callback=invalid_callbacks[i];run_case();total++;
 }
 defaults();printf("BATTLE PACKET RENDER HOST PASS %u original/native comparisons, packed descriptors, signed counts/strides, live cursors and callback selection\n",total);fflush(stdout);
 unsigned refusals=0;
 for(unsigned path=0;path<4;path++){
  defaults();cull_word=path&1u;variant=path;run_case();unsigned n=expected_calls;
  for(stop_at=1;stop_at<=n;stop_at++)for(stop_mode=1;stop_mode<=2;stop_mode++){run_case();refusals++;}
 }
 static const unsigned unusual[]={32768,65535};
 for(unsigned i=0;i<2;i++){defaults();groups=unusual[i];stop_at=1;stop_mode=2;run_case();refusals++;}
 defaults();printf("BATTLE PACKET RENDER HOST PASS %u callee refusals including high-count partial state\n",refusals);fflush(stdout);
 if(!faults()||!controls()||!pending()||!boundaries()||!high_counts()||!child_gates()||!data_bindings())return 1;
 puts("BATTLE PACKET RENDER HOST ALL PASS native interpreted instructions=0 and graphics commands=0; culling and rendering children modeled");return 0;
}
