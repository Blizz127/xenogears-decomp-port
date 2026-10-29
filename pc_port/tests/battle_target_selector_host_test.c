/* Complete private original function is a test-only oracle. Native legs disable
 * the CPU interpreter and reject instruction fetches. Fault comparisons cover
 * committed registers and ordered memory prefixes, not pipeline resumption. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed target-selector test runner"
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

#define PARENT 0x800af438u
#define CHILD 0x800af400u
#define PARENT_SIZE 224u
#define CHILD_SIZE 56u
#define GLOBAL_BYTE 0x800c3d40u
#define GLOBAL_MASK 0x800c3e30u
#define STEP_LIMIT 1000u
static BattleMipsRuntime rt;
static uint8_t parent_bytes[PARENT_SIZE],child_bytes[CHILD_SIZE];
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native,child_entry;
static unsigned domain_kind,stack_kind,alias_kind,bad_kind,seed,overlay_decoy;
static unsigned selector,actor_byte,other_byte,global_byte,global_mask,direct_child;
static unsigned fault_at,accesses,expected_accesses,legacy_calls,child_calls,want_child_calls,interface_mode;
static unsigned returned,want_returned;
static uint32_t actor,output,initial_sp,return_pc,want_return_pc;
static int candidate,compare,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[128];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"TARGET SELECTOR FAIL target=%08x selector=%08x actorbyte=%u other=%u global=%u/%04x domain=%u stack=%u alias=%u bad=%u seed=%u decoy=%u fault=%u interface=%u accesses=%u %s %08x\n",
 direct_child?CHILD:PARENT,selector,actor_byte,other_byte,global_byte,global_mask,domain_kind,stack_kind,alias_kind,bad_kind,seed,overlay_decoy,fault_at,interface_mode,accesses,what,value);exit(1);
}
static int in_body(uint32_t a){return a-CHILD<CHILD_SIZE+PARENT_SIZE;}
static int observed_access(void *p,uint32_t a,unsigned w,uint32_t *v,int write)
{
 if(observing&&!write&&in_body(a)){
  if(candidate||w!=4||(a&3u))fail("native instruction fetch",a);
  *v=load_le(a<PARENT?child_bytes+(a-CHILD):parent_bytes+(a-PARENT),4);return 0;
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
 if(target==CHILD){
  child_calls++;
  if(compare){if(!registers_equal(cpu,&child_entry)||cpu->load_valid!=child_entry.load_valid)fail("child entry ABI",target);}
  else child_entry=*cpu;
  return candidate?runtime_bridge(p,cpu,target):0;
 }
 if(target==PARENT)return candidate?runtime_bridge(p,cpu,target):0;
 if(in_body(target))return 0;
 /* A halfword output can change saved RA. Stop at that actual outer return,
  * without executing a single instruction at the resulting arbitrary address. */
 if(target==cpu->gpr[31]&&cpu->gpr[29]==initial_sp){
  returned=1;return_pc=target;return -1;
 }
 fail("unexpected branch/child",target);return -1;
}
static uint32_t token(unsigned domain,unsigned off)
{
 switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;
 case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}
}
static void defaults(void)
{
 domain_kind=stack_kind=alias_kind=bad_kind=seed=overlay_decoy=direct_child=0;
 selector=0xffu;actor_byte=3;other_byte=11;global_byte=7;global_mask=0x1200;
 fault_at=interface_mode=0;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(want_pad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(PARENT),parent_bytes,PARENT_SIZE);memcpy(PSX_ADDR(CHILD),child_bytes,CHILD_SIZE);
 rt.data[0]=(ResolvedData){0x80060000u,0x3000u,shared,"real-lower-selector-data"};rt.data_count=1;
 /* Globals above the overlay base must stay in raw RAM even if a shared
  * binding is present. Only the lower actor/output range uses real backing. */
 if(overlay_decoy)rt.data[rt.data_count++]=(ResolvedData){0x800c3d00u,0x200u,shared+0x3000u,"ignored-overlay-global-decoy"};
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800300u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 actor=token(domain_kind,0x80u);output=token(domain_kind,0x180u);
 if(alias_kind>=12&&alias_kind<=15)cpu->gpr[29]=(alias_kind<14?GLOBAL_BYTE:GLOBAL_MASK)+((alias_kind&1u)?4u:8u);
 initial_sp=cpu->gpr[29];
 if(alias_kind>=1&&alias_kind<=4)output=initial_sp-8u+(alias_kind-1u)*2u;
 if(alias_kind==5)output=actor+0x20u;
 if(alias_kind==6)output=GLOBAL_BYTE;
 if(alias_kind==7)output=GLOBAL_MASK;
 if(alias_kind>=8&&alias_kind<=11)actor=initial_sp-8u+(alias_kind-8u)*2u-((selector&0xffu)==0xfcu?0x21u:0x20u);
 if(alias_kind>=18&&alias_kind<=21)actor=initial_sp-7u+(alias_kind-18u)*2u-((selector&0xffu)==0xfcu?0x21u:0x20u);
 if(alias_kind==16)output=actor+0x21u;
 if(alias_kind==17)output=actor;
 put(actor+0x20u,1,actor_byte);put(actor+0x21u,1,other_byte);
 put(GLOBAL_BYTE,1,global_byte);put(GLOBAL_MASK,2,global_mask);
 cpu->gpr[4]=actor;cpu->gpr[5]=selector;cpu->gpr[6]=output;
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[6]=0x1e000000u;
 if(bad_kind==3)cpu->gpr[6]++;
 if(bad_kind==4)cpu->gpr[29]++;
 if(bad_kind==5)cpu->gpr[29]=4;
 if(bad_kind==6)rt.host_ranges[0].writable=0;
 if(bad_kind==7)cpu->gpr[6]=0x1f800fffu;
 if(bad_kind==8)cpu->gpr[4]=0x1f800fdfu;
 if(bad_kind==9)cpu->gpr[6]=(uint32_t)(uintptr_t)host+sizeof(host)-1u;
 if(bad_kind==10)cpu->gpr[4]=(uint32_t)(uintptr_t)host+sizeof(host)-0x20u;
 if(bad_kind==11)cpu->gpr[6]=0x1f800ffeu;
 if(bad_kind==12)cpu->gpr[6]=(uint32_t)(uintptr_t)host+sizeof(host)-2u;
 initial_sp=cpu->gpr[29];accesses=legacy_calls=child_calls=returned=0;return_pc=BATTLE_HALT_PC;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu original,native;setup(&original);candidate=compare=0;
 uint32_t target=direct_child?CHILD:PARENT;
 int original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,STEP_LIMIT);
 if(returned)original_rc=PC_PORT_MIPS_HALTED;
 if(!fault_at&&!alias_kind&&!bad_kind&&original_rc!=PC_PORT_MIPS_HALTED)fail("unexpected original failure",original.pc);
 expected_accesses=accesses;want_child_calls=child_calls;want_returned=returned;want_return_pc=return_pc;snapshot();
 setup(&native);candidate=compare=1;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPortMipsRun(&native,target,BATTLE_HALT_PC,STEP_LIMIT);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(returned)rc=PC_PORT_MIPS_HALTED;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)rc);
 if(!registers_equal(&original,&native)){
  for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i])fprintf(stderr,"GPR%u native=%08x original=%08x pc=%08x\n",i,native.gpr[i],original.gpr[i],original.pc);
  fail("committed CPU state",0);
 }
 if(!memory_equal())fail("full memory/GTE",0);
 if(original_rc==PC_PORT_MIPS_HALTED&&(original.load_valid||native.load_valid))fail("pending terminal load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||legacy_calls||child_calls!=want_child_calls||
    returned!=want_returned||return_pc!=want_return_pc||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts/return",0);
 if(fault_at&&accesses!=fault_at)fail("unreached fault",fault_at);
 last_original=original;last_native=native;return original_rc;
}
static const unsigned branches[]={0,15,16,31,32,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
static unsigned comparisons(void)
{
 unsigned total=0;
 for(unsigned high=0;high<3;high++)for(unsigned s=0;s<256;s++){
  defaults();selector=s|(high==1?0x12345600u:high==2?0xffffff00u:0u);seed=s;run_case();total++;
 }
 for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++)for(unsigned d=0;d<7;d++)for(unsigned st=0;st<4;st++){
  defaults();selector=branches[s];domain_kind=d;stack_kind=st;overlay_decoy=(s+d)&1u;seed=st+d;run_case();total++;
 }
 const unsigned byte_selectors[]={0xf7,0xf8,0xf9,0xfc,0xfd,0xfe};
 for(unsigned s=0;s<6;s++)for(unsigned b=0;b<256;b++){
  defaults();selector=byte_selectors[s]|0xfedcba00u;actor_byte=other_byte=global_byte=b;run_case();total++;
 }
 for(unsigned child=0;child<2;child++)for(unsigned m=0;m<84;m++){
  defaults();direct_child=child;overlay_decoy=m&1u;
  global_mask=m<16?1u<<m:m==16?0:m==17?0xffffu:m==18?0xe000u:m==19?0x1555u:(m*17431u)&0xffffu;
  run_case();total++;
 }
 for(unsigned a=1;a<=21;a++)for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++){
  defaults();alias_kind=a;selector=branches[s];overlay_decoy=s&1u;seed=s+1;run_case();total++;
 }
 for(unsigned b=1;b<=12;b++)for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++){
  defaults();bad_kind=b;selector=branches[s];if(b==6)domain_kind=3;run_case();total++;
 }
 for(unsigned b=1;b<=12;b++){
  defaults();direct_child=1;bad_kind=b;if(b==6)domain_kind=3;run_case();total++;
 }
 defaults();printf("TARGET SELECTOR BODY PASS %u committed CPU/full-memory/ordered-access comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++)for(unsigned d=0;d<7;d++){
  defaults();selector=branches[s];domain_kind=d;overlay_decoy=d&1u;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=21;a++)for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++){
  defaults();alias_kind=a;selector=branches[s];overlay_decoy=s&1u;seed=s+1;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned d=0;d<2;d++){defaults();direct_child=1;overlay_decoy=d;fault_at=1;run_case();total++;}
 defaults();printf("TARGET SELECTOR FAULT PASS %u ordered isolated failures\n",total);return total;
}
static void require_gate_unchanged(const PcPortMipsCpu *cpu,const PcPortMipsCpu *before,const char *gate,uint32_t detail)
{if(memcmp(cpu,before,sizeof(*cpu))||!memory_equal()||accesses)fail(gate,detail);}
static unsigned controls(void)
{
 unsigned total=0;
 for(unsigned child=0;child<2;child++){
  defaults();direct_child=child;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
  uint32_t target=child?CHILD:PARENT;unsigned size=child?CHILD_SIZE:PARENT_SIZE;PcPortMipsCpu old=cpu;snapshot();
  for(unsigned off=0;off<size;off++){
   uint8_t *p=(uint8_t *)PSX_ADDR(target)+off;*p^=1;snapshot();
   if(battle_target_selector_try_host(&rt,&cpu,target))fail("complete body identity",off);
   require_gate_unchanged(&cpu,&old,"identity gate side effect",off);*p^=1;total++;
  }
  snapshot();cpu.load_valid=1;cpu.load_reg=4;PcPortMipsCpu pending=cpu;
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("pending entry",0);
  require_gate_unchanged(&cpu,&pending,"pending gate side effect",0);cpu=old;total++;
  rt.force_interpret=1;
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("forced",0);
  require_gate_unchanged(&cpu,&old,"forced gate side effect",0);rt.force_interpret=0;total++;
  rt.initialized=0;
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("uninitialized",0);
  require_gate_unchanged(&cpu,&old,"uninitialized gate side effect",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("inactive",0);
  require_gate_unchanged(&cpu,&old,"inactive gate side effect",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("context",0);
  require_gate_unchanged(&cpu,&old,"context gate side effect",0);rt.bridge_cpu=&cpu;total++;
  if(battle_target_selector_try_host(&rt,&cpu,target+4u))fail("internal target",0);
  require_gate_unchanged(&cpu,&old,"internal target gate side effect",0);total++;
  assert(setenv("XENO_BATTLE_NATIVE_TARGET_SELECTOR","0",1)==0);
  if(battle_target_selector_try_host(&rt,&cpu,target))fail("opt-out",0);
  require_gate_unchanged(&cpu,&old,"opt-out gate side effect",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_TARGET_SELECTOR")==0);total++;
 }
 defaults();printf("TARGET SELECTOR GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static void populate_legacy(unsigned cached)
{
 rt.functions[0]=(ResolvedFunction){PARENT,legacy_probe,"unusable_target_selector"};
 rt.functions[1]=(ResolvedFunction){CHILD,legacy_probe,"unusable_first_bit"};rt.function_count=2;
 if(cached)for(unsigned i=0;i<2;i++){
  BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);
  slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];
 }
}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned child=0;child<2;child++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<4;reject++){
  defaults();direct_child=child;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  populate_legacy(cached);uint32_t target=child?CHILD:PARENT;uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_TARGET_SELECTOR","0",1)==0);
  if(reject==3){cpu.load_valid=1;cpu.load_reg=4;cpu.load_value=0x76543210;}
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_TARGET_SELECTOR")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",0);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()||
      cpu.load_valid!=old.load_valid||cpu.load_reg!=old.load_reg||cpu.load_value!=old.load_value))fail("refused native boundary",0);
  if(reject==1)code[0]^=1;
  total++;
 }
 /* Parent identity is independent of the helper. Corrupt every helper byte:
  * FF must stop at the child gate, while each non-FF branch still completes. */
 for(unsigned cached=0;cached<2;cached++)for(unsigned off=0;off<CHILD_SIZE;off++)for(unsigned ff=0;ff<2;ff++){
  defaults();selector=ff?0xffu:branches[off%14u];PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;
  cpu.bus.bridge=runtime_bridge;populate_legacy(cached);((uint8_t *)PSX_ADDR(CHILD))[off]^=1;
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(ff){
   uint32_t before_s0=peek(initial_sp-8u,4),before_ra=peek(initial_sp-4u,4);
   put(initial_sp-8u,4,cpu.gpr[16]);put(initial_sp-4u,4,cpu.gpr[31]);snapshot();
   put(initial_sp-8u,4,before_s0);put(initial_sp-4u,4,before_ra);
  }
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,PARENT,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(cpu.steps||legacy_calls||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("corrupt child interpreter/cache",off);
  if(ff){
   old.gpr[29]-=0x18u;old.gpr[16]=old.gpr[6];old.gpr[2]=old.gpr[3]=0xffu;old.gpr[31]=0x800af45cu;
   if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||!registers_equal(&cpu,&old)||!memory_equal())fail("corrupt child committed prefix",off);
  }else if(rc!=PC_PORT_MIPS_HALTED)fail("unrelated child identity gates parent",off);
  total++;
 }
 defaults();printf("TARGET SELECTOR CACHE PASS %u resolved/cached/refusal/helper-identity controls\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned s=0;s<256;s++){
  defaults();selector=s|0xabcdef00u;run_case();
  unsigned index=s==0xff?9:s==0xfe?7:s==0xfd||s==0xf9?3:s==0xfc?11:s==0xfa?31:s==0xf8?19:s==0xf7?20:s;
  uint32_t bit=1u<<(index&31u);
  if(last_native.gpr[2]!=index||last_native.gpr[3]!=bit||peek(output,2)!=(bit&0xffffu))fail("selector/mask witness",s);
  if(last_native.gpr[5]!=(s>=0xf7&&s!=0xfb?index:selector))fail("A1 witness",s);
  if(last_native.gpr[4]!=(s==0xff?global_mask:actor))fail("child A0 witness",s);
  total++;
 }
 for(unsigned s=0xf7;s<=0xf8;s++){
  defaults();selector=s;actor_byte=255;run_case();unsigned index=s==0xf7?524u:523u;
  if(last_native.gpr[5]!=index||last_native.gpr[2]!=(index&255u)||last_native.gpr[3]!=(1u<<(index&31u)))fail("wide A1/byte return",s);
  total++;
 }
 for(unsigned bit=0;bit<16;bit++){
  defaults();direct_child=1;global_mask=1u<<bit;run_case();unsigned index=bit<13?bit:13;
  if(expected_accesses!=1||last_native.gpr[2]!=index||last_native.gpr[3]!=index||last_native.gpr[4]!=global_mask)fail("thirteen-bit scan",bit);
  total++;
 }
 defaults();global_mask=0;run_case();
 if(last_native.gpr[2]!=13||last_native.gpr[3]!=0x2000u||last_native.gpr[4]!=0)fail("empty mask",0);
 total++;
 for(unsigned a=1;a<=4;a++){
  defaults();alias_kind=a;selector=3;run_case();
  uint32_t old=0x12340010u,expected_s0=a==1?(old&0xffff0000u)|8u:a==2?(old&0xffffu)|0x80000u:old;
  uint32_t expected_ra=a==3?(BATTLE_HALT_PC&0xffff0000u)|8u:a==4?(BATTLE_HALT_PC&0xffffu)|0x80000u:BATTLE_HALT_PC;
  if(last_native.gpr[16]!=expected_s0||last_native.gpr[31]!=expected_ra||last_native.gpr[29]!=initial_sp)fail("saved stack alias witness",a);
  total++;
 }
 defaults();selector=0xff;fault_at=3;run_case();
 if(last_native.gpr[4]!=0x800c0000u||last_native.gpr[2]!=0xffu||last_native.gpr[3]||last_native.gpr[31]!=0x800af45cu)fail("child load fault prefix",0);
 total++;
 defaults();selector=0xfd;fault_at=3;run_case();
 if(last_native.gpr[2]!=0xf9u||last_native.gpr[5]!=0xfdu)fail("branch delay before actor load",0);
 total++;
 defaults();selector=3;fault_at=5;run_case();
 if(last_native.gpr[31]!=BATTLE_HALT_PC||last_native.gpr[16]!=output||last_native.gpr[29]!=initial_sp-24u)fail("saved RA load deferred",0);
 total++;
 defaults();selector=0xff;fault_at=6;run_case();
 if(last_native.gpr[31]!=0x800af45cu||last_native.gpr[16]!=output)fail("child RA retained across saved S0 fault",0);
 total++;
 defaults();overlay_decoy=1;selector=0xfe;run_case();
 if(last_native.gpr[2]!=global_byte||((uint8_t *)PSX_ADDR(GLOBAL_BYTE))[0]!=global_byte||
    load_le(PSX_ADDR(GLOBAL_MASK),2)!=global_mask)fail("overlay globals stay in raw RAM",0);
 for(unsigned i=0x3000;i<0x3200;i++)if(shared[i]!=0x5au)fail("overlay decoy untouched",i);
 total++;
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=d;selector=0xfd;run_case();
  if(last_native.gpr[2]!=actor_byte||shared[0xa0u]!=actor_byte||load_le(shared+0x180u,2)!=8u||
     load_le(PSX_ADDR(actor+0x20u),1)!=0xa5u||load_le(PSX_ADDR(output),2)!=0xa5a5u)fail("lower shared actor/output backing",d);
  total++;
 }
 defaults();printf("TARGET SELECTOR SEMANTICS PASS %u independent selector/shift/alias/delay witnesses\n",total);return total;
}
static unsigned interface_case(unsigned si)
{
 PcPortMipsCpu parent,original;setup(&parent);
 const uint32_t stacks[]={0x801ff004u,0xa01ff004u,0x1f8003fcu,0x80000100u,0x80200000u};
 parent.gpr[29]=stacks[si];
 uint32_t args[20];for(unsigned i=0;i<4;i++)args[i]=parent.gpr[4+i];for(unsigned i=4;i<20;i++)args[i]=0xface0000u+i;
 original=parent;original.gpr[29]-=0x100u;original.gpr[31]=BATTLE_HALT_PC;initial_sp=original.gpr[29];candidate=compare=0;
 int original_rc=PC_PORT_MIPS_HALTED;uint32_t target=direct_child?CHILD:PARENT;
 for(unsigned i=4;i<20;i++)if(runtime_write(&rt,original.gpr[29]+0x10u+(i-4u)*4u,4,args[i])){original_rc=PC_PORT_MIPS_FAULT;break;}
 if(original_rc==PC_PORT_MIPS_HALTED)original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,STEP_LIMIT);
 expected_accesses=accesses;want_child_calls=child_calls;snapshot();
 setup(&parent);parent.gpr[29]=stacks[si];rt.bridge_cpu=&parent;PcPortMipsCpu old=parent;candidate=compare=1;
 uint32_t v0=0x11111111u,v1=0x22222222u;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPort_BattleMipsCallJal(target,args,20,&v0,&v1);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 int completed=original_rc==PC_PORT_MIPS_HALTED;
 if(original_rc==PC_PORT_MIPS_BUDGET||(rc==0)!=completed||
    (completed?(v0!=original.gpr[2]||v1!=original.gpr[3]):(v0!=0x11111111u||v1!=0x22222222u))||
    !memory_equal()||accesses!=expected_accesses||memcmp(&old,&parent,sizeof(parent))||rt.bridge_cpu!=&parent||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("actual CallJal interface",(uint32_t)rc);
 if(fault_at&&accesses!=fault_at)fail("unreached CallJal fault",fault_at);
 return expected_accesses;
}
static unsigned interfaces(void)
{
 unsigned total=0,failures=0;
 for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned s=0;s<sizeof(branches)/sizeof(branches[0]);s++){
  defaults();domain_kind=d;selector=branches[s];overlay_decoy=d&1u;interface_mode=1;interface_case(si);total++;
 }
 for(unsigned child=0;child<2;child++)for(unsigned d=0;d<2;d++){
  defaults();direct_child=child;domain_kind=d?5:0;overlay_decoy=d;interface_mode=1;unsigned n=interface_case(0);
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0);failures++;}
 }
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[20]={actor,selector,output,0};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(PARENT+(b==4?1u:0u),b==5?NULL:args,b==8?21u:4u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("TARGET SELECTOR INTERFACE PASS %u actual CallJal cases/refusals; %u ordered argument/callee failures\n",total,failures);return total;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(fseek(f,0x3f910,SEEK_SET)==0);
 assert(fread(child_bytes,1,CHILD_SIZE,f)==CHILD_SIZE);assert(fread(parent_bytes,1,PARENT_SIZE,f)==PARENT_SIZE);fclose(f);assert((uintptr_t)host<=UINT32_MAX);
 const struct{uint8_t *bytes;unsigned size;const char *sha;} bodies[]={
  {parent_bytes,PARENT_SIZE,"d886f9d31b12dc1934b50bcaeb611d2358a2a3550b205432bb9a9a48771601c8"},
  {child_bytes,CHILD_SIZE,"d8f1f9a0bfa426fc0272bb8d71bde1fa7c5313bf132d67c5e19118e10095d065"}};
 for(unsigned b=0;b<2;b++){uint8_t digest[32];file1_digest(bodies[b].bytes,bodies[b].size,digest);
  for(unsigned i=0;i<32;i++)assert(file1_hex_digit(digest[i]>>4)==bodies[b].sha[i*2]&&file1_hex_digit(digest[i]&15u)==bodies[b].sha[i*2+1]);}
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
 puts("TARGET SELECTOR ALL PASS native interpreted instructions=0; production CallJal; no pipeline-resumption or whole-game claim");return 0;
}

