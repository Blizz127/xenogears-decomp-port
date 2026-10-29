/* Complete private original function is a test-only oracle. Native legs disable
 * the CPU interpreter and reject instruction fetches. Fault comparisons cover
 * committed registers and ordered memory prefixes, not pipeline resumption. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed motion-triple test runner"
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

#define TARGET 0x800a1b50u
#define BODY_SIZE 420u
#define STEP_LIMIT 2000000u
static BattleMipsRuntime rt;
static uint8_t original_bytes[BODY_SIZE];
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native;
static unsigned domain_kind,stream_domain,stack_kind,alias_kind,bad_kind,seed,shape;
static unsigned node_count,rotation_count,translation_count,flags,header_mode,rotation_handle,translation_handle;
static unsigned fault_at,accesses,expected_accesses,legacy_calls,interface_mode;
static int candidate,compare,observing;
static uint32_t nodes,animation,stream,first_node,initial_sp;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[1024];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"MOTION TRIPLE FAIL nodes=%u rot=%u trans=%u flags=%x header=%x shape=%u handles=%u/%u domain=%u/%u stack=%u alias=%u bad=%u seed=%u fault=%u interface=%u accesses=%u %s %08x\n",
 node_count,rotation_count,translation_count,flags,header_mode,shape,rotation_handle,translation_handle,domain_kind,stream_domain,stack_kind,alias_kind,bad_kind,seed,fault_at,interface_mode,accesses,what,value);exit(1);
}
static int observed_access(void *p,uint32_t a,unsigned w,uint32_t *v,int write)
{
 if(observing&&!write&&a-TARGET<BODY_SIZE){
  if(candidate||w!=4)fail("native instruction fetch",a);
  *v=load_le(original_bytes+(a-TARGET),4);return 0;
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
 if(target==TARGET&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(p,cpu,target):0;
 if(target-TARGET<BODY_SIZE)return 0;
 fail("unexpected child",target);return -1;
}
static uint32_t token(unsigned domain,unsigned off)
{
 switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;
 case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}
}
static uint32_t component(unsigned node,unsigned channel,unsigned part)
{
 static const int32_t values[]={-32768,-1,32767,0,1,-12345,12345,-2};
 return (uint32_t)values[(seed+node*3u+channel*2u+part)%8u];
}
static void defaults(void)
{
 domain_kind=stream_domain=stack_kind=alias_kind=bad_kind=seed=shape=0;
 node_count=3;rotation_count=translation_count=2;flags=0;header_mode=1;
 rotation_handle=translation_handle=0;fault_at=interface_mode=0;
}
static void select_alias(unsigned a)
{
 alias_kind=a;seed=a;
 /* Place the stream after node1's handles while its saved header counts
  * overlap node1's rotation outputs. Both later channels remain accessible. */
 if(a==9){header_mode=0;rotation_count=5;}
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(TARGET),original_bytes,BODY_SIZE);
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"real-lower-triple-data"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800e00u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 initial_sp=cpu->gpr[29];cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 nodes=token(domain_kind,0x80u);first_node=nodes+0x7cu;animation=token(stream_domain,0x900u);
 if(alias_kind==1)animation=nodes;
 if(alias_kind==2)animation=first_node+0x54u-0x18u;
 if(alias_kind==3)animation=first_node+0x4eu-0x18u;
 if(alias_kind==6)animation=first_node+0x68u-0x18u;
 if(alias_kind==7)animation=first_node;
 if(alias_kind==8)cpu->gpr[29]=first_node+0x58u;
 if(alias_kind==9)animation=first_node+0x48u;
 initial_sp=cpu->gpr[29];
 stream=animation+0x18u+(header_mode?0u:6u*(rotation_count+1u));
 uint32_t cursor=stream;unsigned rotations=0,translations=0;
 for(unsigned n=1;n<=8;n++){
  uint32_t node=nodes+n*0x7cu;
  put(node+4u,1,0x32);put(node+5u,1,0x45);
  for(unsigned ch=0;ch<2;ch++){
   unsigned mode=ch?translation_handle:rotation_handle;
   uint32_t handle=mode?token(domain_kind,0x700u+n*8u+ch*4u):0;
   if(handle)put(handle+3u,1,mode==1?0xffu:0x7eu);
   if(alias_kind==4)handle=node+(ch?2u:1u);
   if(alias_kind==5&&ch)handle=node+0x51u;
   put(node+(ch?0x74u:0x70u),4,handle);
   for(unsigned part=0;part<3;part++){
    uint32_t value=component(n,ch,part);
    int different=shape==0?part==0:shape==2?part==1:shape==3?part==2:shape==5?ch&&part==0:shape==6?!ch&&part==0:0;
    if(different)value^=0x1111u;
    if(shape==4&&ch)value^=0x10000u;
    put(node+(ch?0x5cu+part*4u:0x54u+part*2u),ch?4:2,value);
   }
  }
 }
 /* Write only the actually consumed fixture stream, after node initialization:
  * stream/node/handle aliases must retain their genuine overlapping bytes. */
 unsigned iterations=node_count?node_count-1u:65535u;
 if(iterations>8u)iterations=8u;
 for(unsigned n=1;n<=iterations;n++){
  if(!(flags&1u)&&rotations<rotation_count){for(unsigned p=0;p<3;p++){put(cursor,2,component(n,0,p));cursor+=2;}rotations++;}
  if(!(flags&2u)&&translations<translation_count){for(unsigned p=0;p<3;p++){put(cursor,2,component(n,1,p));cursor+=2;}translations++;}
 }
 put(animation+6u,2,header_mode);put(animation+0xcu,2,rotation_count);
 put(animation+0xeu,2,translation_count);put(animation+4u,2,flags);put(nodes+0xau,2,node_count);
 cpu->gpr[4]=nodes;cpu->gpr[5]=animation;
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[5]=0x1e000000u;
 if(bad_kind==3)cpu->gpr[4]++;
 if(bad_kind==4)cpu->gpr[5]++;
 if(bad_kind==5)put(first_node+0x70u,4,0x1e000000u);
 if(bad_kind==6)put(first_node+0x74u,4,0x1f800fffu);
 if(bad_kind==7)rt.host_ranges[0].writable=0;
 if(bad_kind==8){cpu->gpr[5]=0x1f800ff4u;put(cpu->gpr[5]+6u,2,1);}
 if(bad_kind==9)put(first_node+0x70u,4,(uint32_t)(uintptr_t)host+sizeof(host)-3u);
 if(bad_kind==10)cpu->gpr[29]=4u;
 accesses=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu original,native;setup(&original);candidate=compare=0;
 int original_rc=PcPortMipsRun(&original,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
 if(!fault_at&&!alias_kind&&!bad_kind&&original_rc!=PC_PORT_MIPS_HALTED)fail("unexpected original failure",original.pc);
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
 unsigned total=0;
 for(unsigned d=0;d<7;d++)for(unsigned f=0;f<4;f++)for(unsigned r=0;r<4;r++)for(unsigned t=0;t<4;t++){
  defaults();domain_kind=d;stream_domain=(d+f)%7;flags=f|0x8000u;rotation_count=r;translation_count=t;
  node_count=1u+(r+t)%4u;header_mode=(r+t)&1u;seed=r+t*4u;stack_kind=t;run_case();total++;
 }
 for(unsigned s=0;s<7;s++)for(unsigned rh=0;rh<3;rh++)for(unsigned th=0;th<3;th++)for(unsigned h=0;h<3;h++){
  defaults();shape=s;rotation_handle=rh;translation_handle=th;header_mode=h==2?0xffffu:h;seed=s+rh+th;run_case();total++;
 }
 for(unsigned s=0;s<32;s++){defaults();seed=s;shape=s%7u;node_count=8;rotation_count=7;translation_count=7;run_case();total++;}
 for(unsigned a=1;a<=9;a++){defaults();select_alias(a);run_case();total++;}
 for(unsigned b=1;b<=10;b++){defaults();bad_kind=b;if(b==7)domain_kind=3;run_case();total++;}
 defaults();node_count=0;flags=3;run_case();total++;
 defaults();node_count=0;rotation_count=translation_count=1;run_case();total++;
 defaults();node_count=65535;flags=3;run_case();total++;
 defaults();rotation_count=translation_count=65535;header_mode=0;flags=3;run_case();total++;
 defaults();printf("MOTION TRIPLE BODY PASS %u committed CPU/full-memory/ordered-access comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned s=0;s<7;s++)for(unsigned rh=0;rh<3;rh++)for(unsigned th=0;th<3;th++){
  defaults();shape=s;rotation_handle=rh;translation_handle=th;header_mode=s==6?0x8000u:s&1u;domain_kind=s%7;stream_domain=domain_kind;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned f=0;f<4;f++)for(unsigned c=0;c<3;c++){
  defaults();flags=f;rotation_count=c;translation_count=2u-c;node_count=c+1u;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=9;a++){
  defaults();select_alias(a);run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("MOTION TRIPLE FAULT PASS %u ordered isolated failures\n",total);return total;
}
static void require_gate_unchanged(const PcPortMipsCpu *cpu,const PcPortMipsCpu *before,
                                   const char *gate,uint32_t detail)
{
 if(memcmp(cpu,before,sizeof(*cpu))||!memory_equal()||accesses)fail(gate,detail);
}
static unsigned controls(void)
{
 unsigned total=0;defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
 PcPortMipsCpu old=cpu;snapshot();
 for(unsigned off=0;off<BODY_SIZE;off++){
  uint8_t *p=(uint8_t *)PSX_ADDR(TARGET)+off;*p^=1;snapshot();
  if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("complete body identity",off);
  require_gate_unchanged(&cpu,&old,"identity gate side effect",off);
  *p^=1;total++;
 }
 snapshot();cpu.load_valid=1;cpu.load_reg=4;PcPortMipsCpu pending=cpu;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("pending entry",0);
 require_gate_unchanged(&cpu,&pending,"pending entry gate side effect",0);
 cpu=old;total++;
 rt.force_interpret=1;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("forced",0);
 require_gate_unchanged(&cpu,&old,"forced gate side effect",0);
 rt.force_interpret=0;total++;
 rt.initialized=0;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("uninitialized",0);
 require_gate_unchanged(&cpu,&old,"uninitialized gate side effect",0);
 rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("inactive",0);
 require_gate_unchanged(&cpu,&old,"inactive gate side effect",0);
 g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("context",0);
 require_gate_unchanged(&cpu,&old,"context gate side effect",0);
 rt.bridge_cpu=&cpu;total++;
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET+4u))fail("internal target",0);
 require_gate_unchanged(&cpu,&old,"internal target gate side effect",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_MOTION_TRIPLE","0",1)==0);
 if(battle_motion_triple_try_host(&rt,&cpu,TARGET))fail("opt-out",0);
 require_gate_unchanged(&cpu,&old,"opt-out gate side effect",0);
 assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_TRIPLE")==0);total++;
 defaults();printf("MOTION TRIPLE GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){TARGET,legacy_probe,"unusable_motion_triple"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,TARGET);slot->target=TARGET;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(TARGET);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MOTION_TRIPLE","0",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,TARGET):PcPortMipsRun(&cpu,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_TRIPLE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",0);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()))fail("refused native boundary",0);
  if(reject==1)code[0]^=1;
  total++;
 }
 defaults();printf("MOTION TRIPLE CACHE PASS %u resolved/cached controls\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 defaults();if(run_case()!=PC_PORT_MIPS_HALTED)fail("basic witness",0);
 for(unsigned n=1;n<=2;n++)for(unsigned ch=0;ch<2;ch++)for(unsigned p=0;p<3;p++){
  unsigned w=ch?4:2;uint32_t expect=component(n,ch,p);if(w==2)expect&=0xffffu;
  if(peek(nodes+n*0x7cu+(ch?0x5cu+p*4u:0x54u+p*2u),w)!=expect)fail("signed component witness",n);
  total++;
 }
 if(last_native.gpr[2]!=2||last_native.gpr[4]!=nodes+4+2*0x7cu||last_native.gpr[5]!=stream+24||
    last_native.gpr[29]!=initial_sp||peek(first_node+4,1)!=1||peek(first_node+5,1)!=1)fail("cursor/stride/dirty witness",0);
 total++;
 defaults();node_count=1;if(run_case()!=PC_PORT_MIPS_HALTED||expected_accesses!=5||last_native.gpr[2]||last_native.gpr[4]!=nodes||last_native.gpr[5]!=stream)fail("empty still reads header",0);total++;
 defaults();node_count=0;flags=3;
 if(run_case()!=PC_PORT_MIPS_HALTED||expected_accesses!=5||last_native.gpr[2]!=65535||last_native.gpr[8]!=65535||last_native.gpr[4]!=nodes+4+65535u*0x7cu)fail("zero count wraps",0);
 total++;
 defaults();flags=3;rotation_count=65535;header_mode=0;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[5]!=animation+0x18u+393216u)fail("header skip untruncated count",0);
 total++;
 defaults();shape=4;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(first_node+0x5cu,4)!=component(1,1,0)||peek(first_node+5u,1)!=0x45u)fail("fullword translation comparison",0);
 total++;
 defaults();shape=1;rotation_handle=translation_handle=1;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(first_node+4u,1)!=0x32u||peek(first_node+5u,1)!=0x45u)fail("equal leaves dirty flags",0);
 for(unsigned i=0;i<expected_accesses;i++)if(trace[i].write||trace[i].address==first_node+0x70u||trace[i].address==first_node+0x74u)fail("equal suppresses handle loads",i);
 total++;
 defaults();rotation_handle=translation_handle=1;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(first_node+4u,1)!=0x32u||peek(first_node+5u,1)!=0x45u)fail("FF handles suppress writes",0);
 total++;
 defaults();flags=1;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[10]!=0||last_native.gpr[9]!=2||last_native.gpr[5]!=stream+12||peek(first_node+5u,1)!=0x45u)fail("disabled rotation consumes no triple",0);
 total++;
 defaults();rotation_count=0;translation_count=1;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[10]!=0||last_native.gpr[9]!=1||last_native.gpr[5]!=stream+6)fail("independent exhaustion",0);
 total++;
 defaults();select_alias(9);
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[13]!=5||last_native.gpr[25]!=2||
    last_native.gpr[10]!=2||last_native.gpr[9]!=2||last_native.gpr[5]!=stream+24||
    peek(animation+0xcu,2)==5)fail("counts survive aliased header stores",0);
 total++;
 defaults();bad_kind=10;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[29]!=4)fail("stack subtraction wraps",0);
 total++;
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=stream_domain=d;
  if(run_case()!=PC_PORT_MIPS_HALTED||shared[0x80u+0x7cu+4u]!=1||((uint8_t *)PSX_ADDR(first_node+4u))[0]!=0xa5u)fail("real shared backing witness",d);
  total++;
 }
 for(unsigned f=2;f<=4;f++){
  defaults();fault_at=f;run_case();
  if(last_native.gpr[8]!=(f==2?0x12340008u:1u)||last_native.gpr[13]!=(f<=3?0x1234000du:2u)||last_native.gpr[25]!=0x12340019u)fail("header delayed commitment",f);
  total++;
 }
 defaults();printf("MOTION TRIPLE SEMANTICS PASS %u independent state/branch/alias/domain witnesses\n",total);return total;
}
static unsigned interface_case(unsigned si)
{
 PcPortMipsCpu parent,original;setup(&parent);
 const uint32_t stacks[]={0x801ff004u,0xa01ff004u,0x1f8003fcu,0x80000100u,0x80200000u};
 parent.gpr[29]=stacks[si];
 uint32_t args[20];for(unsigned i=0;i<4;i++)args[i]=parent.gpr[4+i];for(unsigned i=4;i<20;i++)args[i]=0xface0000u+i;
 original=parent;original.gpr[29]-=0x100u;original.gpr[31]=BATTLE_HALT_PC;candidate=compare=0;
 int original_rc=PC_PORT_MIPS_HALTED;
 for(unsigned i=4;i<20;i++)if(runtime_write(&rt,original.gpr[29]+0x10u+(i-4u)*4u,4,args[i])){original_rc=PC_PORT_MIPS_FAULT;break;}
 if(original_rc==PC_PORT_MIPS_HALTED)original_rc=PcPortMipsRun(&original,TARGET,BATTLE_HALT_PC,STEP_LIMIT);
 expected_accesses=accesses;snapshot();
 setup(&parent);parent.gpr[29]=stacks[si];rt.bridge_cpu=&parent;PcPortMipsCpu old=parent;candidate=compare=1;
 uint32_t v0=0x11111111u,v1=0x22222222u;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPort_BattleMipsCallJal(TARGET,args,20,&v0,&v1);
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
 for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned f=0;f<4;f++){
  defaults();domain_kind=stream_domain=d;flags=f;interface_mode=1;interface_case(si);total++;
 }
 for(unsigned d=0;d<2;d++){
  defaults();domain_kind=stream_domain=d?5:0;interface_mode=1;unsigned n=interface_case(0);
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0);failures++;}
 }
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[20]={nodes,animation,0,0};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(TARGET+(b==4?1u:0u),b==5?NULL:args,b==8?21u:4u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("MOTION TRIPLE INTERFACE PASS %u actual CallJal cases/refusals; %u ordered argument/callee failures\n",total,failures);return total;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(fseek(f,0x32060,SEEK_SET)==0);
 assert(fread(original_bytes,1,BODY_SIZE,f)==BODY_SIZE);fclose(f);assert((uintptr_t)host<=UINT32_MAX);
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
 puts("MOTION TRIPLE ALL PASS native interpreted instructions=0; production CallJal; no pipeline-resumption or whole-game claim");return 0;
}
