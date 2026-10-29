/* Complete private original function is a test-only oracle. Native legs disable
 * the CPU interpreter and reject instruction fetches. Fault comparisons cover
 * committed registers and ordered memory prefixes, not pipeline resumption. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed rotation-retarget test runner"
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

#define PARENT 0x800adf1cu
#define CHILD 0x800a2330u
#define PARENT_SIZE 380u
#define STEP_LIMIT 2000000u
#define CHILD_SIZE 184u
static BattleMipsRuntime rt;
static uint8_t parent_bytes[PARENT_SIZE],child_bytes[CHILD_SIZE];
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native,child_entry;
static unsigned domain_kind,pool_domain,slot_domain,stack_kind,alias_kind,bad_kind,seed;
static unsigned steps,shape,handle_mode,first_free,capacity,occupied,direct_child;
static uint32_t targets[3];
static unsigned fault_at,accesses,expected_accesses,legacy_calls,child_calls,want_child_calls,interface_mode;
static unsigned returned,want_returned;
static uint32_t node,pool,slots,handle,initial_sp,return_pc,want_return_pc;
static int candidate,compare,observing;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[512];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"ROTATION RETARGET FAIL target=%08x steps=%08x shape=%u handle=%u pool=%u/%u occ=%x domain=%u/%u/%u stack=%u alias=%u bad=%u seed=%u fault=%u interface=%u accesses=%u %s %08x\n",
 direct_child?CHILD:PARENT,steps,shape,handle_mode,first_free,capacity,occupied,domain_kind,pool_domain,slot_domain,stack_kind,alias_kind,bad_kind,seed,fault_at,interface_mode,accesses,what,value);exit(1);
}
static int in_body(uint32_t a){return a-CHILD<CHILD_SIZE||a-PARENT<PARENT_SIZE;}
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
 /* Finish at the actual restored RA, including aliases that land inside a
  * known body. The parent's SP distinguishes this from a nested return. */
 if(accesses&&target==cpu->gpr[31]&&cpu->gpr[29]==initial_sp){
  returned=1;return_pc=target;return -1;
 }
 if(target==CHILD){
  child_calls++;
  if(compare){if(!registers_equal(cpu,&child_entry)||cpu->load_valid!=child_entry.load_valid)fail("child entry ABI",target);}
  else child_entry=*cpu;
  return candidate?runtime_bridge(p,cpu,target):0;
 }
 if(target==PARENT)return candidate?runtime_bridge(p,cpu,target):0;
 if(in_body(target))return 0;
 fail("unexpected branch/child",target);return -1;
}
static uint32_t token(unsigned domain,unsigned off)
{
 switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;
 case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}
}
static uint32_t current_angle(unsigned i)
{
 static const int32_t a[]={0,1,-1,32767,-32768,2047,-2048,4095};
 return (uint32_t)a[(seed+i)%8u];
}
static void defaults(void)
{
 domain_kind=pool_domain=slot_domain=stack_kind=alias_kind=bad_kind=seed=direct_child=0;
 steps=4;shape=1;handle_mode=0;first_free=0;capacity=6;occupied=0;
 fault_at=interface_mode=0;
}
static void select_path(unsigned p)
{
 switch(p){
 case 0:steps=1;break;
 case 1:shape=0;break;
 case 2:handle_mode=1;break;
 case 3:break;
 case 4:occupied=1;break;
 case 5:first_free=capacity;break;
 case 6:shape=2;handle_mode=2;break;
 case 7:shape=3;occupied=0x1eu;break;
 }
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(want_pad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(PARENT),parent_bytes,PARENT_SIZE);memcpy(PSX_ADDR(CHILD),child_bytes,CHILD_SIZE);
 rt.data[0]=(ResolvedData){0x80060000u,0x3000u,shared,"real-lower-retarget-data"};
 /* The matching overlay-address binding is a decoy: overlay data stays RAM. */
 rt.data[1]=(ResolvedData){0x80110000u,0x1000u,shared+0x3000u,"ignored-overlay-decoy"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800300u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 initial_sp=cpu->gpr[29];node=token(domain_kind,0x200u);pool=token(pool_domain,0x80u);slots=token(slot_domain,0x600u);
 handle=handle_mode?slots+0x180u:0;
 if(alias_kind>=1&&alias_kind<=4)handle=node+0x4cu+(alias_kind-1u)*4u;
 if(alias_kind==5)handle=node+0x66u;
 if(alias_kind==6)handle=pool;
 if(alias_kind==7)node=pool-0x54u;
 if(alias_kind==8)pool=node+0x54u;
 if(alias_kind==9)slots=node+0x54u;
 if(alias_kind==10)slots=pool;
 if(alias_kind==11)slots=pool+4u;
 if(alias_kind==12)slots=pool-20u;
 if(alias_kind>=13&&alias_kind<=24)handle=initial_sp-24u+(alias_kind-13u)*2u;
 if(alias_kind>=25&&alias_kind<=30)node=initial_sp-24u+(alias_kind-25u)*4u-0x54u;
 if(alias_kind==31)pool=initial_sp-24u;
 if(alias_kind==32)pool=initial_sp-16u;
 if(alias_kind==33)node=initial_sp+0x10u-0x54u;
 if(alias_kind==34)pool=initial_sp+0x10u;
 if(alias_kind==35)slots=node+0x70u;
 for(unsigned i=0;i<3;i++){
  uint32_t a=current_angle(i);put(node+0x54u+i*2u,2,a);targets[i]=a;
  if(shape==i+1u||shape==4)targets[i]+=0x800u+seed;
  if(shape==5)targets[i]^=0x10000u;
  if(shape==6)targets[i]=0x80000000u+i;
  if(shape==7)targets[i]=0x7fffffffu-i;
  if(shape==8){static const uint32_t delta[]={0x7ffu,0x800u,0xfffu,0x1000u,0xfffff800u};targets[i]+=delta[seed%5u];}
 }
 put(node+4u,1,0x32);put(node+5u,1,0x45);put(node+0x70u,4,handle);
 if(handle)put(handle+3u,1,handle_mode==2?0xffu:0x7eu);
 for(unsigned i=0;i<8;i++)put(slots+20u*i,1,(occupied>>i)&1u);
 put(pool,4,slots-20u*first_free);put(pool+4u,2,first_free);put(pool+6u,2,capacity);
 put(initial_sp+0x10u,4,targets[1]);put(initial_sp+0x14u,4,targets[2]);
 cpu->gpr[4]=pool;cpu->gpr[5]=node;cpu->gpr[6]=steps;cpu->gpr[7]=targets[0];
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[5]=0x1e000000u;
 if(bad_kind==3)cpu->gpr[4]++;
 if(bad_kind==4)cpu->gpr[5]++;
 if(bad_kind==5)cpu->gpr[29]++;
 if(bad_kind==6)cpu->gpr[29]=4u;
 if(bad_kind==7)rt.host_ranges[0].writable=0;
 if(bad_kind==8)put(node+0x70u,4,0x1e000000u);
 if(bad_kind==9)put(node+0x70u,4,slots+1u);
 if(bad_kind==10)put(node+0x70u,4,0x1f800ffcu);
 if(bad_kind==11)cpu->gpr[5]=0x1f800faau;
 if(bad_kind==12)cpu->gpr[5]=0x1f800fa8u;
 if(bad_kind==13)cpu->gpr[4]=0x1f800ffau;
 if(bad_kind==14)put(pool,4,0xfffffff0u);
 if(bad_kind==15)cpu->gpr[5]=0xffffffb0u;
 if(bad_kind==16)put(node+0x70u,4,0xfffffffcu);
 if(bad_kind==17)put(pool,4,(uint32_t)(uintptr_t)host+sizeof(host)-1u-20u*first_free);
 if(bad_kind==18)cpu->gpr[4]=(uint32_t)(uintptr_t)host+sizeof(host)-6u;
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
static unsigned comparisons(void)
{
 unsigned total=0;
 const unsigned step_values[]={0,1,2,3,65535,65536,0x7fffffffu,0x80000000u,0xffffffffu};
 for(unsigned st=0;st<9;st++)for(unsigned sh=0;sh<8;sh++)for(unsigned hm=0;hm<3;hm++){
  defaults();steps=step_values[st];shape=sh;handle_mode=hm;seed=st+sh;
  domain_kind=st%7;pool_domain=(sh+st)%7;slot_domain=(hm+sh)%7;stack_kind=sh%4;
  run_case();total++;
 }
 for(unsigned p=0;p<8;p++)for(unsigned d=0;d<7;d++)for(unsigned seed_value=0;seed_value<8;seed_value++){
  defaults();select_path(p);seed=seed_value;domain_kind=pool_domain=slot_domain=d;run_case();total++;
 }
 for(unsigned child=0;child<2;child++)for(unsigned first=0;first<5;first++)for(unsigned cap=0;cap<7;cap++)for(unsigned occ=0;occ<4;occ++){
  defaults();direct_child=child;first_free=first;capacity=cap;occupied=occ==0?0:occ==1?1:occ==2?0x1eu:0xfeu;
  domain_kind=pool_domain=slot_domain=(first+cap)%7;run_case();total++;
 }
 for(unsigned a=1;a<=35;a++)for(unsigned p=0;p<8;p++){
  defaults();alias_kind=a;select_path(p);seed=p;run_case();total++;
 }
 for(unsigned b=1;b<=18;b++)for(unsigned p=0;p<8;p++){
  defaults();bad_kind=b;select_path(p);if(b==7)domain_kind=pool_domain=slot_domain=3;
  if(b==14)first_free=2;
  run_case();total++;
 }
 for(unsigned child=0;child<2;child++)for(unsigned first=65534;first<=65535;first++)for(unsigned cap=65534;cap<=65535;cap++){
  defaults();direct_child=child;first_free=first;capacity=cap;run_case();total++;
 }
 for(unsigned a=7;a<=12;a++){defaults();direct_child=1;alias_kind=a;run_case();total++;}
 defaults();printf("ROTATION RETARGET BODY PASS %u committed CPU/full-memory/ordered-access comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned p=0;p<8;p++)for(unsigned d=0;d<7;d++){
  defaults();select_path(p);domain_kind=pool_domain=slot_domain=d;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=35;a++)for(unsigned p=0;p<8;p++){
  defaults();alias_kind=a;select_path(p);seed=p;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned occ=0;occ<4;occ++)for(unsigned first=0;first<3;first++){
  defaults();direct_child=1;first_free=first;occupied=occ==0?0:occ==1?1:occ==2?0x1eu:0xfeu;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("ROTATION RETARGET FAULT PASS %u ordered isolated failures\n",total);return total;
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
   if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("complete body identity",off);
   require_gate_unchanged(&cpu,&old,"identity gate side effect",off);*p^=1;total++;
  }
  snapshot();cpu.load_valid=1;cpu.load_reg=4;PcPortMipsCpu pending=cpu;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("pending entry",0);
  require_gate_unchanged(&cpu,&pending,"pending gate side effect",0);cpu=old;total++;
  rt.force_interpret=1;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("forced",0);
  require_gate_unchanged(&cpu,&old,"forced gate side effect",0);rt.force_interpret=0;total++;
  rt.initialized=0;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("uninitialized",0);
  require_gate_unchanged(&cpu,&old,"uninitialized gate side effect",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("inactive",0);
  require_gate_unchanged(&cpu,&old,"inactive gate side effect",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("context",0);
  require_gate_unchanged(&cpu,&old,"context gate side effect",0);rt.bridge_cpu=&cpu;total++;
  if(battle_rotation_retarget_try_host(&rt,&cpu,target+4u))fail("internal target",0);
  require_gate_unchanged(&cpu,&old,"internal target gate side effect",0);total++;
  assert(setenv("XENO_BATTLE_NATIVE_ROTATION_RETARGET","0",1)==0);
  if(battle_rotation_retarget_try_host(&rt,&cpu,target))fail("opt-out",0);
  require_gate_unchanged(&cpu,&old,"opt-out gate side effect",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_ROTATION_RETARGET")==0);total++;
 }
 defaults();printf("ROTATION RETARGET GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static void populate_legacy(unsigned cached)
{
 rt.functions[0]=(ResolvedFunction){CHILD,legacy_probe,"func_800A2330"};
 rt.functions[1]=(ResolvedFunction){PARENT,legacy_probe,"func_800ADF1C"};rt.function_count=2;
 assert(find_function(&rt,CHILD)!=NULL&&find_function(&rt,PARENT)!=NULL);
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
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ROTATION_RETARGET","0",1)==0);
  if(reject==3){cpu.load_valid=1;cpu.load_reg=4;cpu.load_value=0x76543210;}
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ROTATION_RETARGET")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",0);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()||
      cpu.load_valid!=old.load_valid||cpu.load_reg!=old.load_reg||cpu.load_value!=old.load_value))fail("refused native boundary",0);
  if(reject==1)code[0]^=1;
  total++;
 }
 defaults();printf("ROTATION RETARGET CACHE PASS %u resolved/cached/refusal controls\n",total);return total;
}
static unsigned child_identity(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned off=0;off<CHILD_SIZE;off++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  populate_legacy(cached);((uint8_t *)PSX_ADDR(CHILD))[off]^=1;
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,PARENT,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||cpu.steps||legacy_calls||rt.bridge_cpu||
     cpu.gpr[31]!=0x800adfb4u||cpu.gpr[2]!=0||cpu.gpr[3]!=0||cpu.gpr[4]!=pool||
     peek(node+0x70u,4)!=0||peek(pool+4u,2)!=first_free||
     PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("corrupt allocator identity",off);
  total++;
 }
 for(unsigned path=0;path<3;path++){
  defaults();select_path(path);PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  ((uint8_t *)PSX_ADDR(CHILD))[0]^=1;populate_legacy(1);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,PARENT,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_HALTED||cpu.steps||legacy_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("unused allocator identity",path);
  total++;
 }
 defaults();printf("ROTATION RETARGET CHILD IDENTITY PASS %u conditional child refusals/bypasses\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 defaults();run_case();
 if(peek(node+0x70u,4)!=slots||peek(slots,4)!=0xfe030001u||peek(slots+4u,2)!=0||
    peek(slots+6u,2)!=1||peek(slots+8u,2)!=0xffffu||peek(slots+10u,2)!=0xf800u||
    peek(slots+12u,2)||peek(slots+14u,2)||peek(slots+16u,2)||peek(slots+18u,2)!=4||
    peek(pool+4u,2)!=1||peek(node+5u,1)!=0x45u||last_native.gpr[2]!=1||last_native.gpr[3]!=slots||
    last_native.gpr[5]!=capacity||last_native.gpr[6]!=slots||last_native.gpr[7]!=slots)fail("allocated record witness",0);
 total++;
 defaults();steps=1;shape=4;handle_mode=2;run_case();
 for(unsigned i=0;i<3;i++)if(peek(node+0x54u+i*2u,2)!=(targets[i]&0xffffu))fail("immediate angle",i);
 if(child_calls||peek(node+4u,1)!=0x32u||peek(node+5u,1)!=1||peek(node+0x70u,4)!=handle||
    peek(handle+3u,1)!=0xffu||peek(pool+4u,2)!=0||last_native.gpr[2]!=1)fail("immediate leaves handle/pool",0);
 total++;
 defaults();shape=0;handle_mode=2;run_case();
 if(child_calls||last_native.gpr[2]!=0xffffffffu||last_native.gpr[3]!=0x12340003u||peek(handle+3u,1)!=0xffu)fail("unchanged signed/full registers",0);
 for(unsigned i=8;i<expected_accesses-6u;i++)if(trace[i].write||trace[i].address==node+0x70u)fail("unchanged short circuit",i);
 total++;
 defaults();shape=5;run_case();
 if(child_calls!=1||last_native.gpr[3]!=slots)fail("equal low-half is not fullword equality",0);
 total++;
 defaults();handle_mode=2;run_case();
 if(child_calls||peek(handle+3u,1)!=0xfeu||last_native.gpr[3]!=handle)fail("protected existing handle reused",0);
 total++;
 for(unsigned i=0;i<5;i++){
  defaults();shape=8;seed=i;handle_mode=1;run_case();
  const unsigned deltas[]={0x7ff,0xf800,0xffff,0,0xf800};
  for(unsigned j=0;j<3;j++)if(peek(handle+10u+j*2u,2)!=deltas[i])fail("wrapped signed delta",i);
  if(last_native.gpr[2]!=(i==0||i==3))fail("last delta boolean",i);
  total++;
 }
 defaults();direct_child=1;occupied=0x1eu;run_case();
 if(last_native.gpr[2]!=slots||peek(pool+4u,2)!=5||peek(slots,1)!=0)fail("allocator scans without occupying result",0);
 total++;
 defaults();direct_child=1;occupied=1;run_case();
 if(last_native.gpr[2]!=0||peek(pool+4u,2)!=0||expected_accesses!=4)fail("occupied candidate returns immediately",0);
 total++;
 defaults();direct_child=1;first_free=capacity;run_case();
 if(last_native.gpr[2]!=0||expected_accesses!=2)fail("full pool avoids base",0);
 total++;
 defaults();direct_child=1;first_free=1;fault_at=2;run_case();
 if(last_native.gpr[3]!=0x12340003u)fail("allocator first pair pending V1",0);
 total++;
 defaults();direct_child=1;first_free=1;fault_at=6;run_case();
 if(last_native.gpr[2]!=0||last_native.gpr[5]!=node)fail("allocator second pair pending V0",0);
 total++;
 defaults();fault_at=3;run_case();
 if(last_native.gpr[18]!=0x12340012u||last_native.gpr[16]!=0x12340010u||last_native.gpr[29]!=initial_sp-40u)fail("argument load pending across saved S0 fault",0);
 total++;
 defaults();run_case();unsigned n=expected_accesses;
 for(unsigned at=n-5u;at<=n;at++){
  defaults();fault_at=at;run_case();
  if(last_native.gpr[31]!=(at<=n-4u?0x800adfb4u:BATTLE_HALT_PC)||last_native.gpr[29]!=initial_sp-40u)fail("epilogue load commitment order",at);
  total++;
 }
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=pool_domain=slot_domain=d;run_case();
  if(load_le(shared+0x270u,4)!=slots||shared[0x600u]!=1||load_le(PSX_ADDR(node+0x70u),4)!=0xa5a5a5a5u||
     load_le(PSX_ADDR(pool+4u),2)!=0xa5a5u)fail("real lower shared backing",d);
  total++;
 }
 defaults();run_case();
 for(unsigned i=0x3000;i<0x4000;i++)if(shared[i]!=0x5au)fail("overlay decoy untouched",i);
 total++;
 defaults();printf("ROTATION RETARGET SEMANTICS PASS %u independent path/delta/allocator/delay/domain witnesses\n",total);return total;
}
static unsigned interface_case(unsigned si,unsigned argc)
{
 PcPortMipsCpu parent,original;setup(&parent);
 const uint32_t stacks[]={0x801ff004u,0xa01ff004u,0x1f8003fcu,0x80000100u,0x80200000u};
 parent.gpr[29]=stacks[si];
 uint32_t args[20];for(unsigned i=0;i<4;i++)args[i]=parent.gpr[4+i];
 args[4]=targets[1];args[5]=targets[2];for(unsigned i=6;i<20;i++)args[i]=0xface0000u+i;
 original=parent;original.gpr[29]-=0x100u;original.gpr[31]=BATTLE_HALT_PC;initial_sp=original.gpr[29];candidate=compare=0;
 int original_rc=PC_PORT_MIPS_HALTED;uint32_t target=direct_child?CHILD:PARENT;
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
 for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned p=0;p<8;p++){
  defaults();domain_kind=pool_domain=slot_domain=d;select_path(p);interface_mode=1;interface_case(si,6);total++;
 }
 for(unsigned child=0;child<2;child++)for(unsigned d=0;d<2;d++)for(unsigned p=0;p<8;p++){
  defaults();direct_child=child;domain_kind=pool_domain=slot_domain=d?5:0;select_path(p);interface_mode=1;
  unsigned n=interface_case(0,6);total++;
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0,6);failures++;}
 }
 /* The lifted caller may forward its full twenty-word home area. */
 defaults();interface_mode=1;interface_case(0,20);total++;
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[6]={pool,node,steps,targets[0],targets[1],targets[2]};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(PARENT+(b==4?1u:0u),b==5?NULL:args,b==8?21u:6u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("ROTATION RETARGET INTERFACE PASS %u actual six-argument CallJal cases/refusals; %u ordered argument/callee failures\n",total,failures);return total;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(fseek(f,0x32840,SEEK_SET)==0);
 assert(fread(child_bytes,1,CHILD_SIZE,f)==CHILD_SIZE);assert(fseek(f,0x3e42c,SEEK_SET)==0);assert(fread(parent_bytes,1,PARENT_SIZE,f)==PARENT_SIZE);fclose(f);assert((uintptr_t)host<=UINT32_MAX);
 const struct{uint8_t *bytes;unsigned size;const char *sha;} bodies[]={
  {parent_bytes,PARENT_SIZE,"92efa36721970c4ecc56ef5e288d8a14739b40e6024daa7b38411ab4d2ac6af4"},
  {child_bytes,CHILD_SIZE,"77e70c10a2a207af117001db0a033bae64fa9bb7cf51128e5b82c824e241bbde"}};
 for(unsigned b=0;b<2;b++){uint8_t digest[32];file1_digest(bodies[b].bytes,bodies[b].size,digest);
  for(unsigned i=0;i<32;i++)assert(file1_hex_digit(digest[i]>>4)==bodies[b].sha[i*2]&&file1_hex_digit(digest[i]&15u)==bodies[b].sha[i*2+1]);}
 if(argc>1){
  defaults();if(!strcmp(argv[1],"--basic")){run_case();return 0;}
  if(!strcmp(argv[1],"--bodies"))return comparisons()?0:1;
  if(!strcmp(argv[1],"--faults"))return faults()?0:1;
  if(!strcmp(argv[1],"--gates"))return controls()?0:1;
  if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;
  if(!strcmp(argv[1],"--child-identity"))return child_identity()?0:1;
  if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;
  if(!strcmp(argv[1],"--interface"))return interfaces()?0:1;
  return 2;
 }
 if(!comparisons()||!faults()||!controls()||!boundaries()||!child_identity()||!semantics()||!interfaces())return 1;
 puts("ROTATION RETARGET ALL PASS native interpreted instructions=0; production CallJal; no pipeline-resumption or whole-game claim");return 0;
}

