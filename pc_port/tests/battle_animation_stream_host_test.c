/* Complete private original function is a test-only oracle. Native legs disable
 * the CPU interpreter and reject instruction fetches. Fault comparisons cover
 * committed registers and ordered memory prefixes, not pipeline resumption. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed animation-stream test runner"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "../src/psx_memory.c"
#include "psx/gtereg.h"
int g_GPUDisabledState;unsigned g_PcPortPresentedFrames;
int32_t D_80050104,D_800500F8,D_800500FC,D_80050100,D_80059578;
uint32_t D_800595C0;uint16_t D_80059308,D_8005930C;
void ControllerPushState(void){}
void ControllerPoll(void){}
void PsyX_UpdateInput(void){}
int PsyX_Sys_GetVBlankCount(void){return 0;}
int PcPort_QuickCheckpointFieldIsActive(void){return 0;}
char PsyX_BeginScene(void){abort();}
void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();}
void ParsePrimitivesLinkedList(u_long *p,int s){(void)p;(void)s;abort();}
extern int color_fade_backend_command(int);
int doCOP2(int op){return color_fade_backend_command(op);}

#define TARGET 0x800ae1bcu
#define BODY_SIZE 100u
#define STEP_LIMIT 1000u
static const char body_sha[]="1c4e52b31bceb27241444be3a73a03f2b3cd8da6391c72904f881383e32c689e";
static uint8_t body[BODY_SIZE];
static BattleMipsRuntime rt;
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native;
static unsigned domain_kind,record_domain,actor_align,record_align,alias_kind,alias_mirror,bad_kind,seed;
static unsigned header_value,frames,selector,offset_word,stack_kind;
static unsigned fault_at,accesses,expected_accesses,legacy_calls,interface_mode;
static uint32_t actor,record;
static int candidate,compare,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[64];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"ANIMATION STREAM FAIL domain=%u/%u align=%u/%u alias=%u mirror=%u bad=%u seed=%u header=%x frames=%x selector=%08x offset=%08x stack=%u fault=%u interface=%u accesses=%u %s %08x\n",
 domain_kind,record_domain,actor_align,record_align,alias_kind,alias_mirror,bad_kind,seed,header_value,frames,selector,offset_word,stack_kind,fault_at,interface_mode,accesses,what,value);exit(1);
}
static int observed_access(void *p,uint32_t a,unsigned w,uint32_t *v,int write)
{
 if(observing&&!write&&a-TARGET<BODY_SIZE){
  if(candidate||w!=4||(a&3u))fail("native instruction fetch",a);
  *v=load_le(body+(a-TARGET),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(p,a,w,*v):model_test_raw_read(p,a,w,v);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access capacity",accesses);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(p,a,w,*v):model_test_raw_read(p,a,w,v);
 Access row={a,w,rc&&!write?0:*v,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered access",a);}
 else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *p,uint32_t a,unsigned w,uint32_t *v){return observed_access(p,a,w,v,0);}
static int runtime_write(void *p,uint32_t a,unsigned w,uint32_t v){return observed_access(p,a,w,&v,1);}
static void put(uint32_t a,unsigned w,uint32_t v){if(model_test_raw_write(&rt,a,w,v))fail("fixture write",a);}
static uint32_t peek(uint32_t a,unsigned w){uint32_t v;if(model_test_raw_read(&rt,a,w,&v))fail("fixture read",a);return v;}
static void snapshot(void)
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static int registers_equal(const PcPortMipsCpu *a,const PcPortMipsCpu *b)
{return !memcmp(a->gpr,b->gpr,sizeof(a->gpr))&&!memcmp(a->cp0,b->cp0,sizeof(a->cp0))&&a->hi==b->hi&&a->lo==b->lo;}
static int bridge(void *p,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target-TARGET>=BODY_SIZE)return -1;
 return candidate&&target==TARGET?runtime_bridge(p,cpu,target):0;
}
static uint32_t token(unsigned domain,unsigned off)
{
 switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;
 case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}
}
static void defaults(void)
{
 domain_kind=record_domain=actor_align=record_align=alias_kind=alias_mirror=bad_kind=seed=stack_kind=0;
 header_value=0x1234;frames=0x5678;selector=1;offset_word=0x20;fault_at=interface_mode=0;
}
static void select_path(unsigned p)
{
 if(p==0)header_value=0;
 if(p==1)selector=0;
 if(p==2)selector=0x10000u;
 if(p==3)selector=0xffffffffu;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(want_pad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(TARGET),body,BODY_SIZE);
 rt.data[0]=(ResolvedData){0x80060000u,0x3000u,shared,"real-lower-animation-data"};
 rt.data[1]=(ResolvedData){0x80110000u,0x1000u,shared+0x3000u,"ignored-overlay-decoy"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800300u:stack_kind==3?1u:0x801ff000u;
 cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 actor=token(domain_kind,0x100u)+actor_align;record=token(record_domain,0x500u)+record_align;
 if(alias_kind)record=actor+0x70u+2u*(alias_kind-1u);
 if(alias_mirror)record+=0x20000000u;
 if(bad_kind==10)record=0x7ffffff0u;
 if(bad_kind==11)actor=0x7fffff68u;
 /* Bad-kind 10 deliberately leaves an unmapped, unused frame field. */
 if(bad_kind!=10)put(record+2u,2,frames);
 put(record+0x12u,2,header_value);put(record+0x14u,4,offset_word);
 cpu->gpr[4]=actor;cpu->gpr[5]=record;cpu->gpr[6]=selector;
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[5]=0x1e000000u;
 if(bad_kind==3)rt.host_ranges[0].writable=0;
 if(bad_kind==4)cpu->gpr[4]=0x1f800f60u; /* +A0 crosses the pad after halfword stores. */
 if(bad_kind==5)cpu->gpr[4]=0x1f800f5cu; /* Only the final word store crosses. */
 if(bad_kind==6){cpu->gpr[5]=0x1f800fecu;put(0x1f800ffeu,2,header_value);}
 if(bad_kind==7){cpu->gpr[5]=(uint32_t)(uintptr_t)host+sizeof(host)-0x14u;put(cpu->gpr[5]+0x12u,2,header_value);}
 if(bad_kind==8)cpu->gpr[4]=0xffffff68u;
 if(bad_kind==9)cpu->gpr[5]=0xffffffeeu;
 if(bad_kind==12)cpu->gpr[4]=(uint32_t)(uintptr_t)host+sizeof(host)-0xa4u;
 accesses=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu original,native;setup(&original);candidate=compare=0;
 int original_rc=PcPortMipsRun(&original,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
 if(!fault_at&&!actor_align&&!record_align&&!alias_kind&&!bad_kind&&original_rc!=PC_PORT_MIPS_HALTED)fail("unexpected original failure",original.pc);
 expected_accesses=accesses;snapshot();setup(&native);candidate=compare=1;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPortMipsRun(&native,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)rc);
 if(!registers_equal(&original,&native)){
  for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i])fprintf(stderr,"GPR%u native=%08x original=%08x pc=%08x\n",i,native.gpr[i],original.gpr[i],original.pc);
  fail("committed CPU state",0);
 }
 if(!memory_equal())fail("full memory/GTE",0);
 if(original_rc==PC_PORT_MIPS_HALTED&&(original.load_valid||native.load_valid))fail("pending terminal load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||legacy_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",0);
 if(fault_at&&accesses!=fault_at)fail("unreached fault",fault_at);
 last_original=original;last_native=native;return original_rc;
}
static unsigned comparisons(void)
{
 static const unsigned selectors[]={0,1,0x100,0x10000,0x80000000u,0xffffffffu};
 static const unsigned words[]={0,1,0xfffffff0u,0x80000000u,0xffffffffu};
 unsigned total=0;
 for(unsigned a=0;a<7;a++)for(unsigned r=0;r<7;r++)for(unsigned p=0;p<4;p++){
  defaults();domain_kind=a;record_domain=r;select_path(p);seed=a+r;run_case();total++;
 }
 for(unsigned h=0;h<3;h++)for(unsigned f=0;f<4;f++)for(unsigned s=0;s<6;s++)for(unsigned o=0;o<5;o++){
  defaults();header_value=h==0?0:h==1?1:0xffffu;frames=f==0?0:f==1?1:f==2?0x8000u:0xffffu;
  selector=selectors[s];offset_word=words[o];stack_kind=s%4u;run_case();total++;
 }
 for(unsigned alias=1;alias<=33;alias++)for(unsigned alignment=0;alignment<2;alignment++)for(unsigned p=1;p<4;p++){
  defaults();alias_kind=alias;actor_align=alignment*2u;select_path(p);run_case();total++;
 }
 for(unsigned alias=1;alias<=33;alias++){
  defaults();alias_kind=alias;alias_mirror=1;run_case();total++;
 }
 for(unsigned aa=0;aa<4;aa++)for(unsigned ra=0;ra<4;ra++)for(unsigned p=0;p<3;p++){
  defaults();actor_align=aa;record_align=ra;select_path(p);run_case();total++;
 }
 for(unsigned b=1;b<=12;b++)for(unsigned p=0;p<4;p++){
  defaults();bad_kind=b;select_path(p);if(b==3)domain_kind=record_domain=3;if(b==10)selector=0;
  run_case();total++;
 }
 defaults();printf("ANIMATION STREAM BODY PASS %u committed CPU/full-memory/ordered-access comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned p=0;p<4;p++){
  defaults();domain_kind=record_domain=d;select_path(p);run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=33;a++)for(unsigned p=1;p<4;p++){
  defaults();alias_kind=a;select_path(p);run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("ANIMATION STREAM FAULT PASS %u ordered isolated failures\n",total);return total;
}
static void require_gate_unchanged(const PcPortMipsCpu *cpu,const PcPortMipsCpu *before,const char *gate,uint32_t detail)
{if(memcmp(cpu,before,sizeof(*cpu))||!memory_equal()||accesses)fail(gate,detail);}
static unsigned controls(void)
{
 unsigned total=0;
 {
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
  uint32_t target=TARGET;unsigned size=BODY_SIZE;PcPortMipsCpu old=cpu;snapshot();
  for(unsigned off=0;off<size;off++){
   uint8_t *p=(uint8_t *)PSX_ADDR(target)+off;*p^=1;snapshot();
   if(battle_animation_stream_try_host(&rt,&cpu,target))fail("complete body identity",off);
   require_gate_unchanged(&cpu,&old,"identity gate side effect",off);*p^=1;total++;
  }
  snapshot();cpu.load_valid=1;cpu.load_reg=4;PcPortMipsCpu pending=cpu;
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("pending entry",0);
  require_gate_unchanged(&cpu,&pending,"pending gate side effect",0);cpu=old;total++;
  rt.force_interpret=1;
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("forced",0);
  require_gate_unchanged(&cpu,&old,"forced gate side effect",0);rt.force_interpret=0;total++;
  rt.initialized=0;
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("uninitialized",0);
  require_gate_unchanged(&cpu,&old,"uninitialized gate side effect",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("inactive",0);
  require_gate_unchanged(&cpu,&old,"inactive gate side effect",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("context",0);
  require_gate_unchanged(&cpu,&old,"context gate side effect",0);rt.bridge_cpu=&cpu;total++;
  if(battle_animation_stream_try_host(&rt,&cpu,target+4u))fail("internal target",0);
  require_gate_unchanged(&cpu,&old,"internal target gate side effect",0);total++;
  assert(setenv("XENO_BATTLE_NATIVE_ANIMATION_STREAM","0",1)==0);
  if(battle_animation_stream_try_host(&rt,&cpu,target))fail("opt-out",0);
  require_gate_unchanged(&cpu,&old,"opt-out gate side effect",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_ANIMATION_STREAM")==0);total++;
 }
 defaults();printf("ANIMATION STREAM GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static void populate_legacy(unsigned cached,unsigned allowlisted)
{
 /* The second name is deliberately poisoned metadata already accepted by
  * the legacy allowlist. Canonical dispatch must reject it before lookup. */
 const char *name=allowlisted?"func_800AF400":"func_800AE1BC";
 assert(overlay_leaf_host_ok(name)==(int)allowlisted);
 rt.functions[0]=(ResolvedFunction){TARGET,legacy_probe,name};rt.function_count=1;
 assert(find_function(&rt,TARGET)!=NULL);
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,TARGET);slot->target=TARGET;slot->state=1;slot->entry=rt.functions[0];}
}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned allowlisted=0;allowlisted<2;allowlisted++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<4;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  populate_legacy(cached,allowlisted);uint8_t *code=PSX_ADDR(TARGET);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ANIMATION_STREAM","0",1)==0);
  if(reject==3){cpu.load_valid=1;cpu.load_reg=4;cpu.load_value=0x76543210;}
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,TARGET):PcPortMipsRun(&cpu,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ANIMATION_STREAM")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",0);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()||
      cpu.load_valid!=old.load_valid||cpu.load_reg!=old.load_reg||cpu.load_value!=old.load_value))fail("refused native boundary",0);
  if(reject==1)code[0]^=1;
  total++;
 }
 defaults();printf("ANIMATION STREAM CACHE PASS %u resolved/cached/allowlisted-poison/refusal controls\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 defaults();run_case();uint32_t result=record+offset_word;
 if(peek(actor+0x98u,2)||peek(actor+0x9au,2)!=frames||peek(actor+0x9cu,2)||peek(actor+0x9eu,2)!=header_value||
    peek(actor+0xa0u,4)!=result||peek(actor+0xa4u,4)!=result||last_native.gpr[2]!=result||expected_accesses!=10)fail("enabled output fields",0);
 total++;
 defaults();header_value=0;run_case();
 if(peek(actor+0x98u,2)!=0xffffu||peek(actor+0x9au,2)!=0xa5a5u||peek(actor+0xa0u,4)!=0xa5a5a5a5u||last_native.gpr[2]!=0xffffffffu||expected_accesses!=2)fail("disabled short circuit",0);
 total++;
 static const unsigned selectors[]={0,0x100,0x10000,0x80000000u,0xffffffffu};
 for(unsigned i=0;i<5;i++){
  defaults();selector=selectors[i];run_case();
  if(peek(actor+0x9au,2)!=(selector?frames:0xffffu)||expected_accesses!=(selector?10u:9u))fail("fullword selector truth",i);
  total++;
 }
 defaults();offset_word=0xffffffffu;run_case();
 if(last_native.gpr[2]!=record-1u||peek(actor+0xa4u,4)!=record-1u)fail("wrapped guest sum",0);
 total++;
 /* +98 aliases +2, so the frame load must observe zero. */
 defaults();alias_kind=20;run_case();
 if(record!=actor+0x96u||peek(actor+0x9au,2)||last_native.gpr[2]!=header_value||expected_accesses!=7)fail("first write changes frame before later alignment fault",0);
 total++;
 /* +9A becomes the live header; +9C clears offset low, +9E writes high. */
 defaults();alias_kind=13;run_case();result=record+((uint32_t)frames<<16);
 if(record!=actor+0x88u||peek(actor+0x9eu,2)!=frames||last_native.gpr[2]!=result||peek(actor+0xa0u,4)!=result)fail("live header and offset after halfword writes",0);
 total++;
 /* The first word store changes the offset; the second keeps the same sum. */
 defaults();alias_kind=15;run_case();result=record+offset_word;
 if(record!=actor+0x8cu||peek(record+0x14u,4)!=result||peek(actor+0xa4u,4)!=result||last_native.gpr[2]!=result)fail("cached pointer across aliased first word store",0);
 total++;
 defaults();alias_kind=14;run_case();
 if(peek(actor+0x9eu,2)!=0||last_native.gpr[2]!=0||expected_accesses!=7)fail("zeroed live header still follows initially enabled branch",0);
 total++;
 defaults();record_align=2;run_case();
 if(last_native.gpr[2]!=header_value||peek(actor+0x9eu,2)!=header_value||expected_accesses!=7)fail("word-misaligned record commits halfword prefix",0);
 total++;
 defaults();actor_align=2;run_case();
 if(last_native.gpr[2]!=record+offset_word||peek(actor+0x9eu,2)!=header_value||expected_accesses!=8)fail("word-misaligned actor commits pointer V0",0);
 total++;
 defaults();stack_kind=3;run_case();
 if(last_native.gpr[29]!=1u||last_native.gpr[7]!=0x12340007u)fail("leaf ignores unused stack and A3",0);
 total++;
 /* Every fault prefix has an independently calculated committed V0. */
 for(unsigned p=0;p<3;p++){
  defaults();select_path(p);run_case();unsigned count=expected_accesses;
  for(unsigned at=1;at<=count;at++){
   defaults();select_path(p);fault_at=at;run_case();uint32_t want;
   if(at==1)want=0x12340002u;
   else if(p==0)want=0xffffffffu;
   else if(at==2||((p==2)&&at==3))want=header_value;
   else if(at<=(p==1?5u:6u))want=p==1?0xffffffffu:frames;
   else if(at<=(p==1?7u:8u))want=header_value;
   else want=record+offset_word;
   if(last_native.gpr[2]!=want)fail("independent fault V0",at);
   total++;
  }
 }
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=record_domain=d;run_case();
  if(load_le(shared+0x19au,2)!=frames||load_le(shared+0x1a0u,4)!=record+offset_word||load_le(PSX_ADDR(actor+0xa0u),4)!=0xa5a5a5a5u)fail("real lower shared backing",d);
  total++;
 }
 defaults();run_case();
 for(unsigned i=0x3000;i<0x4000;i++)if(shared[i]!=0x5au)fail("overlay decoy untouched",i);
 total++;
 defaults();printf("ANIMATION STREAM SEMANTICS PASS %u independent path/alias/alignment/fault/domain witnesses\n",total);return total;
}
static unsigned interface_case(unsigned si,unsigned argc)
{
 PcPortMipsCpu parent,original;setup(&parent);
 const uint32_t stacks[]={0x801ff004u,0xa01ff004u,0x1f8003fcu,0x80000100u,0x80200000u};
 parent.gpr[29]=stacks[si];
 uint32_t args[20];for(unsigned i=0;i<3;i++)args[i]=parent.gpr[4+i];
 args[3]=parent.gpr[7];for(unsigned i=4;i<20;i++)args[i]=0xface0000u+i;
 original=parent;original.gpr[29]-=0x100u;original.gpr[31]=BATTLE_HALT_PC;candidate=compare=0;
 for(unsigned i=0;i<4;i++)original.gpr[4+i]=i<argc?args[i]:0;
 int original_rc=PC_PORT_MIPS_HALTED;uint32_t target=TARGET;
 for(unsigned i=4;i<argc;i++)if(runtime_write(&rt,original.gpr[29]+0x10u+(i-4u)*4u,4,args[i])){original_rc=PC_PORT_MIPS_FAULT;break;}
 if(original_rc==PC_PORT_MIPS_HALTED)original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,STEP_LIMIT);
 expected_accesses=accesses;snapshot();
 setup(&parent);parent.gpr[29]=stacks[si];rt.bridge_cpu=&parent;PcPortMipsCpu old=parent;candidate=compare=1;
 uint32_t v0=0x11111111u,v1=0x22222222u;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPort_BattleMipsCallJal(target,args,argc,&v0,&v1);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 int completed=original_rc==PC_PORT_MIPS_HALTED;
 if(original_rc==PC_PORT_MIPS_BUDGET||(rc==0)!=completed||
    (completed?(v0!=original.gpr[2]||v1!=original.gpr[3]):(v0!=0x11111111u||v1!=0x22222222u))||
    !memory_equal()||accesses!=expected_accesses||legacy_calls||memcmp(&old,&parent,sizeof(parent))||rt.bridge_cpu!=&parent||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual CallJal interface",(uint32_t)rc);
 if(fault_at&&accesses!=fault_at)fail("unreached CallJal fault",fault_at);
 return expected_accesses;
}
static unsigned interfaces(void)
{
 unsigned total=0,failures=0;
 for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned p=0;p<4;p++){
  defaults();domain_kind=record_domain=d;select_path(p);interface_mode=1;interface_case(si,3);total++;
 }
 for(unsigned d=0;d<2;d++)for(unsigned p=0;p<4;p++){
  defaults();domain_kind=record_domain=d?5:0;select_path(p);interface_mode=1;
  unsigned n=interface_case(0,3);total++;
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0,3);failures++;}
 }
 /* The lifted caller may forward its full twenty-word home area. */
 defaults();interface_mode=1;unsigned full_count=interface_case(0,20);total++;
 for(fault_at=1;fault_at<=full_count;fault_at++){interface_case(0,20);failures++;}
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[3]={actor,record,selector};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(TARGET+(b==4?1u:0u),b==5?NULL:args,b==8?21u:3u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("ANIMATION STREAM INTERFACE PASS %u actual three-argument CallJal cases/refusals; %u ordered argument/callee failures\n",total,failures);return total;
}

int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert((uintptr_t)host<=UINT32_MAX);
 assert(fseek(f,(long)(TARGET-0x8006faf0u),SEEK_SET)==0);assert(fread(body,1,BODY_SIZE,f)==BODY_SIZE);fclose(f);
 uint8_t digest[32];file1_digest(body,BODY_SIZE,digest);
 for(unsigned i=0;i<32;i++)assert(file1_hex_digit(digest[i]>>4)==body_sha[i*2]&&file1_hex_digit(digest[i]&15u)==body_sha[i*2+1]);
 if(argc>1){
  defaults();if(!strcmp(argv[1],"--basic")){run_case();return 0;}
  if(!strcmp(argv[1],"--bodies"))return comparisons()?0:1;
  if(!strcmp(argv[1],"--faults"))return faults()?0:1;
  if(!strcmp(argv[1],"--gates"))return controls()?0:1;
  if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;
  if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;
  if(!strcmp(argv[1],"--interface"))return interfaces()?0:1;
  return 2;
 }
 if(!comparisons()||!faults()||!controls()||!boundaries()||!semantics()||!interfaces())return 1;
 puts("ANIMATION STREAM ALL PASS native interpreted instructions=0; production CallJal; no pipeline-resumption or whole-game claim");return 0;
}
