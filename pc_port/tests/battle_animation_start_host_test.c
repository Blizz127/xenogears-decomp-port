/* Complete private original function is a test-only oracle. Native legs disable
 * the CPU interpreter and reject instruction fetches. Fault comparisons cover
 * committed registers and ordered memory prefixes, not pipeline resumption. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed animation-start test runner"
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

#define PARENT 0x800a2434u
#define CHILD 0x800a2accu
#define PARENT_SIZE 720u
#define CHILD_SIZE 236u
#define STEP_LIMIT 3000000u
typedef struct Body {uint32_t target;unsigned size;const char *sha,*control;uint8_t bytes[720];} Body;
static Body bodies[]={
 {PARENT,PARENT_SIZE,"caaba7ed66d982ae863a9136db5a0f2d07bac493eaea5dc47466de87e9b4d2bd","XENO_BATTLE_NATIVE_ANIMATION_START",{0}},
 {CHILD,CHILD_SIZE,"020a0eba7e385c5fa2917a872a19d5c1f7bb4f9f579419e8f4aa6aa947e5344c","XENO_BATTLE_NATIVE_ANIMATION_START",{0}},
 {0x800a1b50u,420u,"e12a4093ee078d91cb5fbfd8699e8ced3af26cd2f1fe848bacc52b1e57bef7d4","XENO_BATTLE_NATIVE_MOTION_TRIPLE",{0}},
 {0x800a2330u,184u,"77e70c10a2a207af117001db0a033bae64fa9bb7cf51128e5b82c824e241bbde","XENO_BATTLE_NATIVE_ROTATION_RETARGET",{0}},
 {0x800a23e8u,76u,"fe43ec9555086fac2eb4fe394f84f3e8a6ca57ee8656260b7d4f0e47aef9cbfa","XENO_BATTLE_NATIVE_MOTION_TRACK",{0}}
};
static BattleMipsRuntime rt;
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native;
typedef struct Call {uint32_t target;PcPortMipsCpu cpu;} Call;
static Call child_entries[128];
static unsigned domain_kind,pool_domain,slot_domain,animation_domain,stack_kind,alias_kind,bad_kind,seed;
static unsigned immediate,node_count,rotation_count,translation_count,flags,frame_count,loop_word,tag_word,key_shape,handle_mode,handle_channels,first_free,capacity,occupied,direct_child;
static unsigned fault_at,accesses,expected_accesses,legacy_calls,child_calls,want_child_calls,interface_mode;
static unsigned returned,want_returned;
static uint32_t node,pool,slots,animation,records,stream,initial_sp,return_pc,want_return_pc;
static int candidate,compare,observing;
static uint32_t oracle_refusal_target;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[2048];
static void fail(const char *what,uint32_t value)
{
 fprintf(stderr,"ANIMATION START FAIL target=%08x imm=%u nodes=%u rot/pos=%u/%u flags=%x frames=%u loop=%08x tag=%08x keys=%u handles=%u/%u pool=%u/%u occ=%x domain=%u/%u/%u/%u stack=%u alias=%u bad=%u seed=%u fault=%u interface=%u accesses=%u %s %08x\n",
 direct_child?CHILD:PARENT,immediate,node_count,rotation_count,translation_count,flags,frame_count,loop_word,tag_word,key_shape,handle_mode,handle_channels,first_free,capacity,occupied,domain_kind,pool_domain,slot_domain,animation_domain,stack_kind,alias_kind,bad_kind,seed,fault_at,interface_mode,accesses,what,value);exit(1);
}
static int body_index(uint32_t a){for(unsigned i=0;i<5;i++)if(a-bodies[i].target<bodies[i].size)return (int)i;return -1;}
static int observed_access(void *p,uint32_t a,unsigned w,uint32_t *v,int write)
{
 int bi=body_index(a);
 if(observing&&!write&&bi>=0){
  if(candidate||w!=4||(a&3u))fail("native instruction fetch",a);
  *v=load_le(bodies[bi].bytes+(a-bodies[bi].target),4);return 0;
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
 if(accesses&&target==cpu->gpr[31]&&cpu->gpr[29]==initial_sp){returned=1;return_pc=target;return -1;}
 int bi=body_index(target);
 if(bi<0)return -1; /* Bound an aliased nested return to the complete graph. */
 if(target!=bodies[bi].target)return 0;
 if(child_calls>=sizeof(child_entries)/sizeof(child_entries[0]))fail("call capacity",child_calls);
 if(compare){
  if(child_calls>=want_child_calls||child_entries[child_calls].target!=target||
     !registers_equal(cpu,&child_entries[child_calls].cpu)||cpu->load_valid!=child_entries[child_calls].cpu.load_valid)fail("full graph entry ABI",target);
 }else child_entries[child_calls]=(Call){target,*cpu};
 child_calls++;
 if(!candidate&&target==oracle_refusal_target)return -1;
 return candidate?runtime_bridge(p,cpu,target):0;
}
static uint32_t token(unsigned domain,unsigned off)
{
 switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;
 case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}
}
static void defaults(void)
{
 domain_kind=pool_domain=slot_domain=animation_domain=stack_kind=alias_kind=bad_kind=seed=direct_child=0;
 immediate=0;node_count=3;rotation_count=translation_count=2;flags=0;frame_count=5;loop_word=1;tag_word=0x12345656u;
 key_shape=handle_mode=occupied=0;handle_channels=7;first_free=12;capacity=32;fault_at=interface_mode=0;
}
static void select_path(unsigned p)
{
 switch(p){
 case 0:break;
 case 1:handle_mode=1;break;
 case 2:handle_mode=2;break;
 case 3:handle_mode=1;key_shape=3;break;
 case 4:handle_mode=3;key_shape=5;break;
 case 5:occupied=1;break;
 case 6:first_free=capacity;break;
 case 7:immediate=1;handle_mode=3;break;
 case 8:direct_child=1;handle_mode=1;break;
 case 9:direct_child=1;handle_mode=2;break;
 }
}
static uint32_t fixture_handle(unsigned n,unsigned ch)
{
 return slots+20u*(n*3u+ch);
}
static uint32_t forced_handle(void)
{
 if(alias_kind>=1&&alias_kind<=3)return records+(alias_kind==1?4u:alias_kind==2?0u:0xfffffffeu);
 if(alias_kind==4)return initial_sp+0x10u;
 if(alias_kind==5)return initial_sp-0x38u;
 if(alias_kind>=6&&alias_kind<=25)return initial_sp-40u+(alias_kind-6u)*2u;
 if(alias_kind==31)return pool;
 if(alias_kind==33)return node+0x70u;
 if(alias_kind==34)return initial_sp-4u;
 if(alias_kind==35)return animation-16u;
 return 0;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(want_pad));memset(host,0xb6,sizeof(host));
 for(unsigned i=0;i<5;i++)memcpy(PSX_ADDR(bodies[i].target),bodies[i].bytes,bodies[i].size);
 rt.data[0]=(ResolvedData){0x80060000u,0x3000u,shared,"real-lower-animation-data"};
 rt.data[1]=(ResolvedData){0x80110000u,0x1000u,shared+0x3000u,"ignored-overlay-decoy"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800300u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 node=token(domain_kind,0x100u);pool=token(pool_domain,0x20u);animation=token(animation_domain,0x500u);slots=token(slot_domain,0x800u);
 if(alias_kind==36)cpu->gpr[29]=animation+8u;
 initial_sp=cpu->gpr[29];
 if(alias_kind==26)node=initial_sp-0x42u;
 if(alias_kind==27)animation=initial_sp-0x44u;
 if(alias_kind==28)animation=node+0x60u;
 records=animation+0x18u;
 if(alias_kind==29)pool=records;
 if(alias_kind==30)slots=records;
 stream=records+6u*(rotation_count+1u)+((flags&1u)?0:6u*rotation_count)+((flags&2u)?0:6u*translation_count);
 for(unsigned i=0;i<48;i++)put(slots+20u*i,1,i>=first_free&&i-first_free<8u?((occupied>>(i-first_free))&1u):0);
 for(unsigned n=0;n<8;n++){
  uint32_t at=node+n*0x7cu;
  put(at+4u,1,0x32);put(at+5u,1,0x45);
  for(unsigned j=0;j<3;j++){put(at+0x54u+j*2u,2,(n+j)&1u?0xffffu:0x7fffu);put(at+0x5cu+j*4u,4,0x11110000u+n*3u+j);}
  for(unsigned ch=0;ch<3;ch++){
   unsigned mode=handle_mode==3?(n+ch)%3u:handle_mode;
   uint32_t h=n<4&&(handle_channels&(1u<<ch))&&mode?fixture_handle(n,ch):0;
   if(n==0&&ch==0&&forced_handle())h=forced_handle();
   if(alias_kind==32&&n==0&&ch<2)h=fixture_handle(0,0);
   if(alias_kind==33&&n==0)h=node+0x70u;
   if(alias_kind==34&&n==0&&ch==2)h=initial_sp-4u;
   if(h){put(h,1,1);put(h+3u,1,mode==2?0xffu:0x44u);}
   put(at+0x70u+ch*4u,4,h);
  }
 }
 for(unsigned n=0;n<8;n++){
  unsigned rk=0x10u+n*6u,tk=0x40u+n*6u;
  if(key_shape==1||key_shape==3||(key_shape==4&&n==0)||(key_shape==5&&n==1)||(key_shape==7&&(n&1u)))rk=0xffffu;
  if(key_shape==2||key_shape==3||(key_shape==4&&n==0)||(key_shape==5&&n==1)||(key_shape==7&&!(n&1u)))tk=0xffffu;
  if(key_shape==6){rk=0xfffeu;tk=0x8000u;}
  put(records+n*6u,2,rk);put(records+n*6u+2u,2,tk);
  put(records+n*6u+4u,1,0x12u+seed);put(records+n*6u+5u,1,0x34u+seed);
 }
 if(immediate)for(unsigned i=0;i<24;i++)put(records+i*2u,2,(i&1u)?(0xffffu-i):(0x7fffu-i));
 put(animation+2u,2,frame_count);put(animation+4u,2,flags);put(animation+6u,2,immediate);
 put(animation+0xcu,2,rotation_count);put(animation+0xeu,2,translation_count);put(node+0xau,2,node_count);
 put(pool,4,slots);put(pool+4u,2,first_free);put(pool+6u,2,capacity);put(initial_sp+0x10u,4,tag_word);
 cpu->gpr[4]=pool;cpu->gpr[5]=node;cpu->gpr[6]=animation;cpu->gpr[7]=loop_word;
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[5]=0x1e000000u;
 if(bad_kind==3)cpu->gpr[6]=0x1e000000u;
 if(bad_kind==4)cpu->gpr[29]++;
 if(bad_kind==5)cpu->gpr[5]++;
 if(bad_kind==6)cpu->gpr[6]++;
 if(bad_kind==7)rt.host_ranges[0].writable=0;
 if(bad_kind==8)put(node+0x70u,4,0x1e000000u);
 if(bad_kind==9)put(node+0x70u,4,slots+1u);
 if(bad_kind==10)put(node+0x70u,4,0x1f800ffcu);
 if(bad_kind==11)cpu->gpr[6]=0x1f800ff4u;
 if(bad_kind==12)cpu->gpr[4]=0x1f800ffau;
 if(bad_kind==13)cpu->gpr[29]=4u;
 if(bad_kind==14)put(node+0x70u,4,0xfffffffcu);
 if(bad_kind==15)put(node+0x70u,4,(uint32_t)(uintptr_t)host+sizeof(host)-4u);
 if(bad_kind==16)put(pool,4,0xfffffff0u);
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
 for(unsigned p=0;p<10;p++)for(unsigned d=0;d<7;d++)for(unsigned l=0;l<4;l++){
  defaults();select_path(p);domain_kind=pool_domain=slot_domain=animation_domain=d;stack_kind=l;
  loop_word=l==0?0:l==1?1:l==2?0xfffffffeu:0xffffffffu;seed=p;run_case();total++;
 }
 for(unsigned k=0;k<8;k++)for(unsigned h=0;h<4;h++)for(unsigned f=0;f<4;f++){
  defaults();key_shape=k;handle_mode=h;flags=f|0x8000u;frame_count=f?0:65535;run_case();total++;
 }
 for(unsigned n=0;n<5;n++)for(unsigned r=0;r<5;r++)for(unsigned t=0;t<3;t++){
  defaults();node_count=n;rotation_count=r;translation_count=t;run_case();total++;
 }
 for(unsigned ch=0;ch<8;ch++)for(unsigned h=0;h<4;h++)for(unsigned n=0;n<5;n++){
  defaults();direct_child=1;handle_channels=ch;handle_mode=h;node_count=n;run_case();total++;
 }
 for(unsigned a=1;a<=36;a++)for(unsigned p=0;p<5;p++){
  defaults();alias_kind=a;select_path(p);if(a==5)loop_word=0;run_case();total++;
 }
 for(unsigned b=1;b<=16;b++)for(unsigned p=0;p<10;p++){
  defaults();bad_kind=b;select_path(p);if(b==7)domain_kind=pool_domain=slot_domain=animation_domain=3;
  run_case();total++;
 }
 for(unsigned f=0;f<4;f++){defaults();rotation_count=translation_count=65535;flags=f;run_case();total++;}
 defaults();node_count=65535;run_case();total++;
 /* Zero node count wraps the triple loop to 65535. Counts of one limit
  * data accesses to its first iteration; subsequent iterations are control
  * only, and the disabled-channel variant never accesses node data. */
 for(unsigned f=0;f<2;f++){defaults();immediate=1;node_count=0;rotation_count=translation_count=1;flags=f?3:0;run_case();total++;}
 defaults();printf("ANIMATION START BODY PASS %u committed CPU/full-memory/ordered-access comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned p=0;p<10;p++)for(unsigned d=0;d<3;d++){
  defaults();select_path(p);domain_kind=pool_domain=slot_domain=animation_domain=d==0?0:d==1?3:5;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=36;a++)for(unsigned p=0;p<3;p++){
  defaults();alias_kind=a;select_path(p);if(a==5)loop_word=0;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned ch=0;ch<3;ch++){
  defaults();direct_child=1;handle_mode=1;handle_channels=1u<<ch;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("ANIMATION START FAULT PASS %u ordered isolated failures\n",total);return total;
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
   if(battle_animation_start_try_host(&rt,&cpu,target))fail("complete body identity",off);
   require_gate_unchanged(&cpu,&old,"identity gate side effect",off);*p^=1;total++;
  }
  snapshot();cpu.load_valid=1;cpu.load_reg=4;PcPortMipsCpu pending=cpu;
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("pending entry",0);
  require_gate_unchanged(&cpu,&pending,"pending gate side effect",0);cpu=old;total++;
  rt.force_interpret=1;
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("forced",0);
  require_gate_unchanged(&cpu,&old,"forced gate side effect",0);rt.force_interpret=0;total++;
  rt.initialized=0;
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("uninitialized",0);
  require_gate_unchanged(&cpu,&old,"uninitialized gate side effect",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("inactive",0);
  require_gate_unchanged(&cpu,&old,"inactive gate side effect",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("context",0);
  require_gate_unchanged(&cpu,&old,"context gate side effect",0);rt.bridge_cpu=&cpu;total++;
  if(battle_animation_start_try_host(&rt,&cpu,target+4u))fail("internal target",0);
  require_gate_unchanged(&cpu,&old,"internal target gate side effect",0);total++;
  assert(setenv("XENO_BATTLE_NATIVE_ANIMATION_START","0",1)==0);
  if(battle_animation_start_try_host(&rt,&cpu,target))fail("opt-out",0);
  require_gate_unchanged(&cpu,&old,"opt-out gate side effect",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_ANIMATION_START")==0);total++;
 }
 defaults();printf("ANIMATION START GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static void populate_legacy(unsigned cached)
{
 static const unsigned order[]={2,3,4,0,1};
 /* These exact original names are not legacy-allowlisted. Uncached rows
  * cover refusal/context; poisoned cached entries prove strict precedence. */
 static const char *names[]={"func_800A2434","func_800A2ACC","func_800A1B50","func_800A2330","func_800A23E8"};
 for(unsigned i=0;i<5;i++){unsigned b=order[i];rt.functions[i]=(ResolvedFunction){bodies[b].target,legacy_probe,names[b]};}
 rt.function_count=5;
 for(unsigned i=0;i<5;i++)assert(find_function(&rt,bodies[i].target)!=NULL);
 if(cached)for(unsigned i=0;i<5;i++){
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
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ANIMATION_START","0",1)==0);
  if(reject==3){cpu.load_valid=1;cpu.load_reg=4;cpu.load_value=0x76543210;}
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ANIMATION_START")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",0);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()||
      cpu.load_valid!=old.load_valid||cpu.load_reg!=old.load_reg||cpu.load_value!=old.load_value))fail("refused native boundary",0);
  if(reject==1)code[0]^=1;
  total++;
 }
 defaults();printf("ANIMATION START CACHE PASS %u resolved/cached/refusal controls\n",total);return total;
}
static void dependency_path(unsigned b)
{
 if(b==1||b==2)immediate=1;
 if(b==4){direct_child=1;handle_mode=1;}
}
static unsigned child_identity(void)
{
 unsigned total=0;
 for(unsigned b=1;b<5;b++)for(unsigned cached=0;cached<2;cached++)for(unsigned kind=0;kind<4;kind++){
  if(b==1&&kind==3)continue; /* This child shares the parent's opt-out. */
  defaults();dependency_path(b);PcPortMipsCpu original,native;setup(&original);populate_legacy(cached);
  unsigned off=kind==0?0:kind==1?bodies[b].size/2u:bodies[b].size-1u;
  if(kind<3)((uint8_t *)PSX_ADDR(bodies[b].target))[off]^=1;
  candidate=compare=0;oracle_refusal_target=bodies[b].target;
  uint32_t target=direct_child?CHILD:PARENT;
  int original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,STEP_LIMIT);
  if(original_rc!=PC_PORT_MIPS_UNRESOLVED_CALL||child_calls<2)fail("oracle child refusal",b);
  oracle_refusal_target=0;expected_accesses=accesses;want_child_calls=child_calls;snapshot();
  setup(&native);populate_legacy(cached);
  if(kind<3)((uint8_t *)PSX_ADDR(bodies[b].target))[off]^=1;
  else assert(setenv(bodies[b].control,"0",1)==0);
  candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&native,target,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(kind==3)assert(unsetenv(bodies[b].control)==0);
  observing=0;
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||native.steps||legacy_calls||rt.bridge_cpu||
     !registers_equal(&native,&original)||!memory_equal()||accesses!=expected_accesses||child_calls!=want_child_calls||
     PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("conditional child identity/opt-out prefix",b);
  total++;
 }
 for(unsigned b=1;b<5;b++)for(unsigned cached=0;cached<2;cached++){
  defaults();handle_mode=2;PcPortMipsCpu cpu;setup(&cpu);populate_legacy(cached);
  ((uint8_t *)PSX_ADDR(bodies[b].target))[0]^=1;candidate=1;compare=0;
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPortMipsRun(&cpu,PARENT,BATTLE_HALT_PC,STEP_LIMIT);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc!=PC_PORT_MIPS_HALTED||cpu.steps||legacy_calls||rt.bridge_cpu||child_calls!=1||
     PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("unused child identity",b);
  total++;
 }
 defaults();printf("ANIMATION START CHILD IDENTITY PASS %u conditional dependency refusals/bypasses\n",total);return total;
}
static unsigned find_access(uint32_t address,unsigned width,unsigned write)
{
 for(unsigned i=0;i<expected_accesses;i++)if(trace[i].address==address&&trace[i].width==width&&trace[i].write==write)return i+1u;
 fail("missing witness access",address);return 0;
}
static unsigned semantics(void)
{
 unsigned total=0;
 defaults();run_case();
 for(unsigned n=0;n<3;n++)for(unsigned ch=0;ch<2;ch++){
  uint32_t h=slots+20u*(12u+n*2u+ch),key=(ch?0x40u:0x10u)+n*6u;
  if(peek(node+n*0x7cu+0x70u+ch*4u,4)!=h||peek(h,1)!=1||peek(h+1u,1)!=1||
     peek(h+2u,1)!=(ch?0x34u:0x12u)||peek(h+3u,1)!=0x56u||peek(h+4u,4)!=stream+key||
     peek(h+8u,4)!=stream+key||peek(h+0x10u,2)||peek(h+0x12u,2)!=5)fail("allocated animation track",n*2u+ch);
 }
 if(peek(pool+4u,2)!=18||last_native.gpr[2]!=0||child_calls!=7)fail("allocator sequence and streaming result",0);
 total++;
 defaults();loop_word=0xfffffffeu;frame_count=0;tag_word=0xabcdef81u;run_case();
 if(peek(slots+20u*12u+1u,1)||peek(slots+20u*12u+3u,1)!=0x81u||
    peek(slots+20u*12u+0x12u,2)!=0xffffu)fail("loop bit/frame underflow/tag truncation",0);
 total++;
 defaults();immediate=1;handle_mode=0;run_case();
 if(last_native.gpr[2]!=1||child_calls!=3)fail("immediate release-all and triple graph",0);
 for(unsigned i=0;i<expected_accesses;i++)if(!trace[i].write&&trace[i].address==initial_sp+0x10u)fail("immediate avoids fifth argument",i);
 total++;
 defaults();direct_child=1;node_count=0;run_case();
 if(last_native.gpr[2]!=0x12340002u||child_calls!=1)fail("empty release-all preserves V0",0);
 total++;
 defaults();direct_child=1;handle_mode=1;run_case();
 for(unsigned n=0;n<3;n++)for(unsigned ch=0;ch<3;ch++){
  uint32_t h=fixture_handle(n,ch);
  if(peek(node+n*0x7cu+0x70u+ch*4u,4)||peek(h,1)||peek(h+3u,1)!=0x44u)fail("all-channel release clears active byte and node slot",n*3u+ch);
 }
 if(peek(pool+4u,2)||last_native.gpr[2]||child_calls!=10||last_native.hi==0x98765432u)fail("release allocator cursor and HI propagation",0);
 total++;
 defaults();direct_child=1;handle_mode=2;run_case();
 for(unsigned ch=0;ch<3;ch++)if(peek(node+0x70u+ch*4u,4)!=fixture_handle(0,ch)||peek(fixture_handle(0,ch),1)!=1)fail("protected release-all handle",ch);
 if(child_calls!=1||peek(pool+4u,2)!=first_free)fail("protected release avoids allocator",0);
 total++;
 defaults();key_shape=3;handle_mode=1;run_case();
 for(unsigned ch=0;ch<2;ch++){
  if(peek(node+0x70u+ch*4u,4)!=fixture_handle(0,ch)||peek(node+0x7cu+0x70u+ch*4u,4))fail("sentinel retains node zero only",ch);
  for(unsigned i=0;i<expected_accesses;i++)if(!trace[i].write&&trace[i].address==fixture_handle(0,ch)+3u)fail("sentinel node-zero tag short circuit",i);
 }
 total++;
 defaults();alias_kind=5;loop_word=0;run_case();
 if(peek(initial_sp-0x38u,2)!=1||peek(node+0x7cu+0x70u,4)||peek(node+0x7cu+0x74u,4))fail("live frame count shortened by handle",0);
 total++;
 defaults();run_case();unsigned at=find_access(initial_sp+0x10u,1,0);
 defaults();fault_at=at;run_case();
 if(last_native.gpr[2]!=1)fail("mode remains pending across fifth-argument read fault",0);
 total++;
 defaults();run_case();at=find_access(slots+20u*12u+0x10u,2,1);
 defaults();fault_at=at;run_case();
 if(last_native.gpr[2]!=0x12u)fail("key reload remains pending across time store fault",0);
 total++;
 defaults();direct_child=1;handle_mode=1;handle_channels=4;node_count=1;run_case();at=find_access(fixture_handle(0,2)+3u,1,0);
 defaults();direct_child=1;handle_mode=1;handle_channels=4;node_count=1;fault_at=at;run_case();
 if(last_native.gpr[17]!=1)fail("third-channel count precedes protection fault",0);
 total++;
 defaults();run_case();at=find_access(node+0xau,2,0);
 defaults();fault_at=at;run_case();
 if(last_native.gpr[5]!=node)fail("rotation count remains pending across node-count read fault",0);
 total++;
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=pool_domain=slot_domain=animation_domain=d;run_case();
  if(load_le(shared+0x170u,4)!=slots+20u*12u||shared[0x8f0u]!=1||
     load_le(PSX_ADDR(node+0x70u),4)!=0xa5a5a5a5u||load_le(PSX_ADDR(pool+4u),2)!=0xa5a5u)fail("real lower shared backing",d);
  total++;
 }
 defaults();run_case();
 for(unsigned i=0x3000;i<0x4000;i++)if(shared[i]!=0x5au)fail("overlay decoy untouched",i);
 total++;
 defaults();printf("ANIMATION START SEMANTICS PASS %u independent graph/track/release/delay/domain witnesses\n",total);return total;
}

static unsigned interface_case(unsigned si,unsigned argc)
{
 PcPortMipsCpu parent,original;setup(&parent);
 const uint32_t stacks[]={0x801ff004u,0xa01ff004u,0x1f8003fcu,0x80000100u,0x80200000u};
 parent.gpr[29]=stacks[si];
 uint32_t args[20];for(unsigned i=0;i<4;i++)args[i]=parent.gpr[4+i];
 args[4]=tag_word;for(unsigned i=5;i<20;i++)args[i]=0xface0000u+i;
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
 for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned p=0;p<10;p++){
  defaults();domain_kind=pool_domain=slot_domain=animation_domain=d;select_path(p);interface_mode=1;interface_case(si,5);total++;
 }
 for(unsigned d=0;d<2;d++)for(unsigned p=0;p<10;p++){
  defaults();domain_kind=pool_domain=slot_domain=animation_domain=d?5:0;select_path(p);interface_mode=1;
  unsigned n=interface_case(0,5);total++;
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0,5);failures++;}
 }
 /* The lifted caller may forward its full twenty-word home area. */
 defaults();interface_mode=1;interface_case(0,20);total++;
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[5]={pool,node,animation,loop_word,tag_word};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(PARENT+(b==4?1u:0u),b==5?NULL:args,b==8?21u:5u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("ANIMATION START INTERFACE PASS %u actual five-argument CallJal cases/refusals; %u ordered argument/callee failures\n",total,failures);return total;
}

int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert((uintptr_t)host<=UINT32_MAX);
 for(unsigned b=0;b<5;b++){
  assert(fseek(f,(long)(bodies[b].target-0x8006faf0u),SEEK_SET)==0);
  assert(fread(bodies[b].bytes,1,bodies[b].size,f)==bodies[b].size);
  uint8_t digest[32];file1_digest(bodies[b].bytes,bodies[b].size,digest);
  for(unsigned i=0;i<32;i++)assert(file1_hex_digit(digest[i]>>4)==bodies[b].sha[i*2]&&file1_hex_digit(digest[i]&15u)==bodies[b].sha[i*2+1]);
 }
 fclose(f);
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
 puts("ANIMATION START ALL PASS native interpreted instructions=0; production CallJal; no pipeline-resumption or whole-game claim");return 0;
}

