/* Original instruction execution is test-only. Native legs permit identity
 * bytes in the overlay but reject instruction fetches and disable decoding. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the bus-observed motion-slot test runner"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "../src/psx_memory.c"
#include "psx/gtereg.h"
#include <openssl/sha.h>
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

static BattleMipsRuntime rt;
static const struct {uint32_t address,size,offset;} bodies[]={
 {0x800af518u,352u,0x3fa28u},{0x8009c050u,144u,0x2c560u}
};
static uint8_t original_bytes[2][352];
static uint8_t shared[0x8000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x10000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu last_original,last_native,pending_cpu;
static unsigned kind,domain_kind,stack_kind,alias_kind,bad_kind,seed,mutation,mixed,health_shape,type_override;
static unsigned fault_at,accesses,expected_accesses,call_count,expected_calls,stop_at,pending_at,mutation_seen,legacy_calls;
static unsigned interface_mode,child_identity_bad;
static int candidate,compare,observing;
static uint32_t actor,alternate,flag,alternate_flag,lower,upper,global,initial_sp,frame,case_halt;
static uint32_t code_word,index_word,cached_slot,actor_status,status_bits,blocker;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[1024];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[8];
static void fail(const char *what,uint32_t n)
{fprintf(stderr,"MOTION SLOT FAIL kind=%u code=%08x idx=%08x flags=%u status=%04x blocker=%u domain=%u stack=%u alias=%u bad=%u seed=%u mutation=%u fault=%u stop=%u pending=%u interface=%u accesses=%u calls=%u %s %08x\n",kind,code_word,index_word,status_bits,actor_status,blocker,domain_kind,stack_kind,alias_kind,bad_kind,seed,mutation,fault_at,stop_at,pending_at,interface_mode,accesses,call_count,what,n);exit(1);}
static int body_index(uint32_t a)
{for(unsigned i=0;i<2;i++)if(a-bodies[i].address<bodies[i].size)return (int)i;return -1;}
static int observed_access(void *p,uint32_t a,unsigned w,uint32_t *v,int write)
{
 int bi=body_index(a);
 if(observing&&!write&&bi>=0){
  if(candidate||w!=4)fail("native instruction fetch",a);
  *v=load_le(original_bytes[bi]+(a-bodies[bi].address),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(p,a,w,*v):model_test_raw_read(p,a,w,v);
 if(accesses>=1024)fail("access capacity",accesses);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(p,a,w,*v):model_test_raw_read(p,a,w,v);
 Access row={a,w,rc&&!write?0:*v,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered access",a);}else trace[accesses]=row;
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
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 if(call_count>=8)fail("call capacity",call_count);
 Call row;memset(&row,0,sizeof(row));row.target=target;row.hi=cpu->hi;row.lo=cpu->lo;row.gte=gteRegs;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("call/full state",target);}else calls[call_count]=row;
 call_count++;
}
static void mutate_after(PcPortMipsCpu *cpu,unsigned call)
{
 if(!mutation||call!=1||mutation_seen)return;
 mutation_seen++;
 if(mutation==1){uint32_t old=cpu->gpr[29],next=0x801d0000u;for(unsigned i=0;i<0x20u;i+=4)put(next+i,4,peek(old+i,4));cpu->gpr[29]=next;}
 if(mutation==2)cpu->gpr[16]=alternate;
 if(mutation==3)cpu->gpr[17]=alternate_flag;
 if(mutation==4)put(global+0x36u,2,1);
 if(mutation==5){cpu->cp0[7]^=0xabcdef01u;cpu->hi^=0x98765432u;cpu->lo^=0x76543210u;}
 if(mutation==6){put(actor+0x20u,1,(index_word+1u)&0xffu);put(0x800ccce8u+((index_word+1u)&0xffu)*0x170u+0x36u,2,1);}
 if(mutation==7)put(actor+0x14u,4,upper);
 if(mutation==8){cpu->gpr[6]^=0x87654321u;cpu->gpr[7]^=0x12345678u;cpu->gpr[8]^=0xabcdef01u;}
}
static int bridge(void *p,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==bodies[kind].address&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(p,cpu,target):0;
 if(!candidate&&target==0x800af558u)mutate_after(cpu,call_count);
 if(target==bodies[1].address){
  record_call(cpu,target);if(stop_at&&call_count==stop_at)return -1;
  if(!candidate)return 0;
  unsigned call=call_count;int rc=runtime_bridge(p,cpu,target);
  if(rc==1)mutate_after(cpu,call);
  if(pending_at&&call==pending_at&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
  return rc;
 }
 if(body_index(target)>=0)return 0;
 fail("unexpected child",target);return -1;
}
static uint32_t token(unsigned domain,unsigned off)
{switch(domain%7u){case 1:return 0xa0110000u+off;case 2:return 0x80310000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x80060000u+off;case 6:return 0xa0060000u+off;default:return 0x80110000u+off;}}
static void defaults(void)
{
 kind=domain_kind=stack_kind=alias_kind=bad_kind=seed=mutation=mixed=health_shape=0;
 fault_at=stop_at=pending_at=interface_mode=child_identity_bad=0;
 code_word=0xffu;index_word=3;cached_slot=0xa5u;actor_status=0x580u;status_bits=7;blocker=0;type_override=256;
}
static void seed_status(unsigned index)
{
 uint32_t base=0x800ccce8u+(index&0xffu)*0x170u;
 uint32_t current=(status_bits&2u)?9u:10u,maximum=80u;
 static const uint32_t values[][2]={{0,0},{0,0},{0,7},{0,8},{1,8},{0x1ffffffeu,0xffffffffu},{0x1fffffffu,0xffffffffu},{0x80000000u,0xffffffffu},{0xffffffffu,0}};
 if(health_shape){current=values[health_shape][0];maximum=values[health_shape][1];}
 put(base+0x120u,2,0x2001u|((status_bits&1u)?0x400u:0));
 put(base+0x104u,4,current);put(base+0x108u,4,maximum);put(base+0x36u,2,blocker);
 put(base+0x148u,1,type_override<256?type_override:(status_bits&4u)?4u:3u);
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 for(unsigned i=0;i<2;i++)memcpy(PSX_ADDR(bodies[i].address),original_bytes[i],bodies[i].size);
 if(child_identity_bad)((uint8_t *)PSX_ADDR(bodies[1].address))[0]^=1;
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"real-lower-slot-data"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800e00u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 initial_sp=cpu->gpr[29];frame=initial_sp-0x20u;case_halt=BATTLE_HALT_PC;
 cpu->gpr[31]=case_halt;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 actor=token(domain_kind,0x100u);alternate=token(domain_kind,0xc00u);
 flag=token(domain_kind+(mixed?3:0),0x80u);alternate_flag=token(domain_kind,0xe80u);
 lower=token(domain_kind+(mixed?1:0),0x400u);upper=token(domain_kind+(mixed?2:0),0x700u);
 global=0x800ccce8u+(index_word&0xffu)*0x170u;
 seed_status(0);seed_status(index_word);seed_status(index_word+1u);
 for(unsigned i=1;i<=64;i++)put(lower+i*4u,4,0x11000000u+i);
 for(unsigned i=1;i<=192;i++)put(upper+i*4u,4,0x22000000u+i);
 for(unsigned i=0;i<2;i++){
  uint32_t a=i?alternate:actor;put(a+0x14u,4,lower);put(a+0x18u,4,upper);
  put(a+0x20u,1,index_word+i);put(a+0x2au,1,i?0x41u:cached_slot);put(a+0x4au,2,actor_status);
 }
 put(flag,4,0xfeed1357u);put(alternate_flag,4,0xf00d2468u);
 if(alias_kind==1)flag=actor+0x20u;
 if(alias_kind==2)flag=actor+0x28u;
 if(alias_kind==3)flag=actor+0x48u;
 if(alias_kind==4)flag=actor+0x14u;
 if(alias_kind==5)flag=actor+0x18u;
 if(alias_kind==6)flag=lower+4u;
 if(alias_kind==7)flag=frame+0x10u;
 if(alias_kind==8)flag=frame+0x14u;
 if(alias_kind==9){flag=frame+0x18u;case_halt=0;}
 if(alias_kind==10)flag=global+0x104u;
 if(alias_kind==11)flag=global+0x120u;
 if(alias_kind==12)flag=upper+4u;
 cpu->gpr[4]=kind?index_word:actor;cpu->gpr[5]=code_word;cpu->gpr[6]=flag;
 if(bad_kind==1)cpu->gpr[4]=0x1e000000u;
 if(bad_kind==2)cpu->gpr[6]=0x1e000000u;
 if(bad_kind==3)cpu->gpr[6]++;
 if(bad_kind==4)cpu->gpr[29]++;
 if(bad_kind==5)cpu->gpr[4]++;
 if(bad_kind==6){put(actor+0x14u,4,0x1f800ffcu);put(actor+0x18u,4,0x1f800ffcu);}
 if(bad_kind==7){cpu->gpr[6]=(uint32_t)(uintptr_t)host+0x80u;rt.host_ranges[0].writable=0;}
 if(bad_kind==8)cpu->gpr[6]=(uint32_t)(uintptr_t)host+sizeof(host)-2u;
 accesses=call_count=mutation_seen=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu original,native;setup(&original);candidate=compare=0;
 uint32_t target=bodies[kind].address;int original_rc=PcPortMipsRun(&original,target,case_halt,10000);
 if(!fault_at&&!stop_at&&!alias_kind&&!bad_kind&&!mutation&&original_rc!=PC_PORT_MIPS_HALTED)fail("unexpected original failure",original.pc);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,target,case_halt,10000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)rc);
 if(!registers_equal(&original,&native)){
  for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i])fprintf(stderr,"GPR%u native=%08x original=%08x pc=%08x\n",i,native.gpr[i],original.gpr[i],original.pc);
  fail("full CPU state",0);
 }
 if(!memory_equal())fail("full memory/GTE",0);
 if(original_rc==PC_PORT_MIPS_HALTED&&(original.load_valid||native.load_valid))fail("pending terminal load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||legacy_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",0);
 if(fault_at&&accesses!=fault_at)fail("unreached fault",fault_at);
 if(mutation&&!mutation_seen)fail("unreached mutation",mutation);
 last_original=original;last_native=native;return original_rc;
}
static unsigned comparisons(void)
{
 unsigned total=0;
 for(unsigned c=0;c<256;c++)for(unsigned h=0;h<2;h++){
  defaults();code_word=c|(h?0xfedcba00u:0);domain_kind=c%7;stack_kind=c%4;mixed=h;seed=c;run_case();total++;
 }
 for(unsigned d=0;d<7;d++)for(unsigned bits=0;bits<8;bits++)for(unsigned st=0;st<8;st++){
  defaults();domain_kind=d;status_bits=bits;actor_status=((st&1)?0x80u:0)|((st&2)?0x100u:0)|((st&4)?0x400u:0);stack_kind=st%4;mixed=bits&1;run_case();total++;
 }
 const uint32_t slots[]={0,1,0x3f,0x40,0x7f,0x80,0xbf,0xff};
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<8;s++){
  defaults();code_word=0xfe;domain_kind=d;cached_slot=slots[s];run_case();total++;
 }
 for(unsigned i=0;i<256;i++)for(unsigned b=0;b<8;b++){
  defaults();kind=1;index_word=i|((i&1)?0xabcd1200u:0);status_bits=b;blocker=(i>>1)&1u;seed=i;run_case();total++;
 }
 for(unsigned h=1;h<=8;h++)for(unsigned b=0;b<4;b++){
  defaults();kind=1;health_shape=h;status_bits=b;blocker=b&1;run_case();total++;
 }
 const unsigned types[]={0,1,3,4,5,0x7f,0x80,0xff};
 for(unsigned t=0;t<8;t++)for(unsigned b=0;b<4;b++){
  defaults();kind=1;status_bits=b;type_override=types[t];run_case();total++;
 }
 for(unsigned a=1;a<=12;a++){
  defaults();alias_kind=a;if(a==9)code_word=1;if(a==5||a==12)code_word=0x40;if(a==6)code_word=0;run_case();total++;
 }
 for(unsigned b=1;b<=8;b++){defaults();bad_kind=b;run_case();total++;}
 for(unsigned m=1;m<=8;m++){defaults();mutation=m;actor_status=0x80;run_case();total++;}
 defaults();printf("MOTION SLOT BODY PASS %u full-state actual-child comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 const unsigned codes[]={0,0x3f,0x40,0xfd,0xfe};
 for(unsigned c=0;c<5;c++){
  defaults();code_word=codes[c];run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned bits=0;bits<8;bits++)for(unsigned st=0;st<8;st++){
  defaults();status_bits=bits;actor_status=((st&1)?0x80u:0)|((st&2)?0x100u:0)|((st&4)?0x400u:0);domain_kind=st==7?5:0;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned bits=0;bits<8;bits++)for(unsigned block=0;block<2;block++){
  defaults();kind=1;status_bits=bits;blocker=block;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 for(unsigned a=1;a<=12;a++){
  defaults();alias_kind=a;if(a==9)code_word=1;if(a==5||a==12)code_word=0x40;if(a==6)code_word=0;
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("MOTION SLOT FAULT PASS %u ordered isolated failures\n",total);return total;
}
static unsigned children(void)
{
 unsigned total=0;defaults();run_case();unsigned n=expected_calls;
 for(stop_at=1;stop_at<=n;stop_at++){run_case();total++;}stop_at=0;
 for(pending_at=1;pending_at<=n;pending_at++){
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,bodies[0].address,case_halt,10000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||call_count!=pending_at||!registers_equal(&cpu,&pending_cpu)||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending child prefix",pending_at);
  total++;
 }
 defaults();printf("MOTION SLOT CHILD PASS %u refusals/pending returns\n",total);return total;
}
static unsigned controls(void)
{
 unsigned total=0;
 for(unsigned k=0;k<2;k++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t target=bodies[k].address;
  for(unsigned off=0;off<bodies[k].size;off++){
   uint8_t *p=(uint8_t *)PSX_ADDR(target)+off;*p^=1;
   if(battle_motion_slot_try_host(&rt,&cpu,target))fail("complete body identity",off);
   *p^=1;total++;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_motion_slot_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
  rt.force_interpret=1;if(battle_motion_slot_try_host(&rt,&cpu,target))fail("forced",0);rt.force_interpret=0;total++;
  rt.initialized=0;if(battle_motion_slot_try_host(&rt,&cpu,target))fail("uninitialized",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;if(battle_motion_slot_try_host(&rt,&cpu,target))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;if(battle_motion_slot_try_host(&rt,&cpu,target))fail("context",0);rt.bridge_cpu=&cpu;total++;
  if(battle_motion_slot_try_host(&rt,&cpu,target+4u))fail("internal target",target);
  total++;
  assert(setenv("XENO_BATTLE_NATIVE_MOTION_SLOT","0",1)==0);if(battle_motion_slot_try_host(&rt,&cpu,target))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_SLOT")==0);total++;
  if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||call_count)fail("gate side effect",target);
 }
 defaults();printf("MOTION SLOT GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned k=0;k<2;k++)for(unsigned cached=0;cached<2;cached++)for(unsigned disable=0;disable<2;disable++)for(unsigned reject=0;reject<3;reject++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  uint32_t target=bodies[k].address;rt.functions[0]=(ResolvedFunction){target,legacy_probe,"unusable_motion_slot"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=PSX_ADDR(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MOTION_SLOT","0",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  if(disable)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=reject&&!disable?runtime_bridge(&rt,&cpu,target):PcPortMipsRun(&cpu,target,case_halt,10000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MOTION_SLOT")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",target);
  if(reject&&((disable?rc!=PC_PORT_MIPS_INTERPRETER_DISABLED:rc!=0)||!registers_equal(&cpu,&old)||!memory_equal()))fail("refused native boundary",target);
  if(reject==1)code[0]^=1;
  total++;
 }
 defaults();printf("MOTION SLOT CACHE PASS %u resolved/cached controls\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 const uint32_t codes[]={0,0x3f,0x40,0xfd,0xfe,0xff};
 const uint32_t results[]={0x11000001u,0x11000040u,0x22000001u,0x220000beu,0x11000026u,0x1100001cu};
 for(unsigned i=0;i<6;i++){
  defaults();code_word=codes[i];if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[2]!=results[i]||peek(flag,4)!=(i>=4?0x80u:0))fail("independent table/flag witness",i);
  total++;
 }
 for(unsigned b=0;b<8;b++){
  defaults();kind=1;status_bits=b;index_word=0xffff1205u;
  if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[2]!=b||last_native.gpr[6]!=b||last_native.gpr[4]!=0x800cd560u||last_native.gpr[7]!=0x800cd418u||last_native.hi!=0x98765432u||last_native.lo!=0xfedcba98u)fail("independent status register witness",b);
  total++;
 }
 defaults();status_bits=2;actor_status=0x80;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(actor+0x2a,1)!=6||last_native.gpr[2]!=0x11000007u)fail("status selects six",0);
 total++;
 defaults();status_bits=0;actor_status=0;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(actor+0x2a,1)!=1||last_native.gpr[2]!=0x11000002u)fail("status selects one",0);
 total++;
 defaults();status_bits=0;actor_status=0x400;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(actor+0x2a,1)!=0xa5||last_native.gpr[2]!=0x11000026u)fail("status keeps cached",0);
 total++;
 defaults();status_bits=2;actor_status=0x80;mutation=4;
 if(run_case()!=PC_PORT_MIPS_HALTED||peek(actor+0x2a,1)!=0xa5||last_native.gpr[2]!=0x11000026u)fail("live status keeps cached without default",0);
 total++;
 defaults();code_word=0;alias_kind=6;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[2]!=0||peek(flag,4)!=0)fail("flag before table read",0);
 total++;
 defaults();code_word=1;alias_kind=7;
 if(run_case()!=PC_PORT_MIPS_HALTED||last_native.gpr[16]!=0)fail("flag saved-S0 alias",0);
 total++;
 for(unsigned d=5;d<=6;d++){
  defaults();domain_kind=d;
  if(run_case()!=PC_PORT_MIPS_HALTED||peek(flag,4)!=0x80||shared[0x80]!=0x80||((uint8_t *)PSX_ADDR(flag))[0]!=0xa5||shared[0x12a]!=0x9b||((uint8_t *)PSX_ADDR(actor+0x2a))[0]!=0xa5)fail("real shared backing witness",d);
  total++;
 }
 for(unsigned c=0;c<2;c++){
  defaults();code_word=c?0xfe:0;child_identity_bad=1;
  if(run_case()!=PC_PORT_MIPS_HALTED||call_count)fail("unreached child identity",c);
  total++;
 }
 /* The low-table load has not committed when RA restoration faults. The
  * high-table jump commits it first. Derive the fault index from the trace. */
 for(unsigned high=0;high<2;high++){
  defaults();code_word=high?0x40:0;run_case();unsigned chosen=0;
  for(unsigned i=0;i<expected_accesses;i++)if(!trace[i].write&&trace[i].address==frame+0x18u)chosen=i+1;
  if(!chosen)fail("missing RA read",high);
  fault_at=chosen;
  if(run_case()==PC_PORT_MIPS_HALTED||last_native.gpr[2]!=(high?0x22000001u:lower))fail("table load-delay failure witness",high);
  total++;
 }
 defaults();printf("MOTION SLOT SEMANTICS PASS %u independent register/branch/alias/domain witnesses\n",total);return total;
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
 if(original_rc==PC_PORT_MIPS_HALTED)original_rc=PcPortMipsRun(&original,bodies[kind].address,BATTLE_HALT_PC,10000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&parent);parent.gpr[29]=stacks[si];
 rt.bridge_cpu=&parent;PcPortMipsCpu old=parent;candidate=compare=1;uint32_t v0=0x11111111u,v1=0x22222222u;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 int rc=PcPort_BattleMipsCallJal(bodies[kind].address,args,20,&v0,&v1);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 /* CallJal initializes its own production CPU bus. Its actual child executes;
  * this interface proof compares bus effects and the exposed return contract.
  * Direct-entry cases separately compare complete child CPU/call snapshots. */
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
 unsigned total=0;
 for(unsigned k=0;k<2;k++)for(unsigned d=0;d<7;d++)for(unsigned si=0;si<5;si++)for(unsigned m=0;m<4;m++){
  defaults();kind=k;domain_kind=d;code_word=m==0?0:m==1?0x40:m==2?0xfe:0xff;interface_mode=1;interface_case(si);total++;
 }
 unsigned failures=0;
 for(unsigned k=0;k<2;k++)for(unsigned d=0;d<2;d++){
  defaults();kind=k;domain_kind=d?5:0;interface_mode=1;unsigned n=interface_case(0);
  for(fault_at=1;fault_at<=n;fault_at++){interface_case(0);failures++;}
 }
 printf("MOTION SLOT INTERFACE FAULT PASS %u ordered argument/callee failures\n",failures);
 for(unsigned b=0;b<9;b++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;uint32_t args[20]={actor,code_word,flag,0};
  if(b==0)cpu.gpr[29]++;
  if(b==1)cpu.gpr[29]=0x1f800404u;
  if(b==2)cpu.gpr[29]=(uint32_t)(uintptr_t)host+0x7000u;
  if(b==3)cpu.gpr[29]=0x800000fcu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t v0=0x11223344,v1=0x55667788;
  if(b==6)rt.initialized=0;
  if(b==7)rt.bridge_cpu=NULL;
  int rc=PcPort_BattleMipsCallJal(bodies[0].address+(b==4?1u:0u),b==5?NULL:args,b==8?21u:4u,&v0,&v1);
  if(rc!=-1||memcmp(&old,&cpu,sizeof(cpu))||!memory_equal()||v0!=0x11223344u||v1!=0x55667788u)fail("CallJal refusal",b);
  total++;
 }
 defaults();printf("MOTION SLOT INTERFACE PASS %u actual CallJal cases/refusals\n",total);return total;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 for(unsigned i=0;i<2;i++){assert(fseek(f,bodies[i].offset,SEEK_SET)==0);assert(fread(original_bytes[i],1,bodies[i].size,f)==bodies[i].size);}fclose(f);
 assert((uintptr_t)host<=UINT32_MAX);
 if(argc>1){defaults();if(!strcmp(argv[1],"--basic")){run_case();return 0;}if(!strcmp(argv[1],"--bodies"))return comparisons()?0:1;if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--children"))return children()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;if(!strcmp(argv[1],"--interface"))return interfaces()?0:1;return 2;}
 if(!comparisons()||!faults()||!children()||!controls()||!boundaries()||!semantics()||!interfaces())return 1;
 puts("MOTION SLOT ALL PASS native interpreted instructions=0; actual child and production CallJal; no whole-game claim");return 0;
}
