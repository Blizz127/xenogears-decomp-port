/* Private original instructions are available only to this test oracle.
 * Native legs have no guest .text and disable original-CPU interpretation. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the observed-runtime test runner"
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

static BattleMipsRuntime rt;
typedef struct Body {uint32_t address,size,caller;} Body;
static const Body bodies[]={
 {0x8002db84u,280,0},{0x8002dc9cu,132,0},{0x8004a4d8u,88,0x8002dc14u},
 {0x80048c4cu,132,0x8002dc3cu},{0x8004a19cu,28,0},{0x8004a1b8u,60,0},
 {0x8004a260u,32,0},{0x8004a280u,68,0},{0x80048d68u,20,0},{0x80048dbcu,216,0},
 {0x8002c8ccu,648,0},{0x8002ccacu,28,0},{0x8002ccc8u,92,0},{0x8002cd24u,64,0},
 {0x8002cd64u,104,0},{0x8002cdccu,360,0},{0x8002cf34u,36,0},{0x8002cf58u,360,0},
 {0x8002d0c0u,36,0},{0x8002d0e4u,156,0},{0x8002d180u,196,0},{0x8002d244u,272,0},
 {0x8002d530u,380,0},{0x8002d6acu,208,0},{0x8002d77cu,152,0},{0x8002d814u,368,0},
 {0x8002d984u,144,0},{0x8002da14u,232,0}
};
#define NEW_BODIES 8u
#define BODY_COUNT (sizeof(bodies)/sizeof(bodies[0]))
static uint8_t shared[0x20000],want_shared[sizeof(shared)],want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu *oracle_cpu,last_oracle,last_native,pending_cpu;
static unsigned kind,shape,domain_kind,alias_kind,bad_kind,seed,stack_kind,table_kind;
static unsigned fault_at,accesses,expected_accesses,events,expected_events,call_count,expected_calls;
static unsigned stop_at,pending_at,mutation,mutation_seen,legacy_calls;
static int candidate,compare,observing;
static uint32_t v[3],out[3],color,initial_sp,frame;
static const uint32_t values[]={0,1,2,3,4,15,16,63,64,255,256,4095,4096,32767,32768,65535,65536,0x7fffffffu,0x80000000u,0xffffffffu,0xffff8000u,0xffff0000u,0x1000000u,0xff000000u};
static const int16_t triangles[][9]={
 {0,0,0,0,0,0,0,0,0},{0,0,0,1,0,0,0,1,0},{0,0,0,0,1,0,1,0,0},
 {0,0,0,0,1,0,0,0,1},{0,0,0,0,0,1,1,0,0},{1,2,3,4,6,8,-1,-4,-9},
 {32767,32767,32767,-32768,-32768,-32768,0,0,0},
 {-32768,0,0,32767,0,0,-32768,1,0},{0,0,0,32767,0,0,0,32767,0},
 {0,0,0,-32768,0,0,0,-32768,0},{0,0,0,256,0,0,0,256,0},
 {0,0,0,1,1,1,2,2,2},{-3,-4,-5,6,7,8,9,10,11},
 {32767,-32768,0,0,32767,-32768,-32768,0,32767},
 {0,0,0,256,255,0,0,256,255},{7,9,11,7,9,12,7,10,11}
};
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[8192];
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[128];
typedef struct Event {uint32_t command,gpr[32],cp0[32],hi,lo;GTERegisters before,after;} Event;
static Event event_trace[128];
static void fail(const char *what,uint32_t n)
{fprintf(stderr,"MODEL LIGHTING FAIL kind=%u target=%08x shape=%u domain=%u alias=%u bad=%u seed=%u stack=%u table=%u fault=%u stop=%u pending=%u mutation=%u access=%u call=%u event=%u %s %08x\n",kind,bodies[kind].address,shape,domain_kind,alias_kind,bad_kind,seed,stack_kind,table_kind,fault_at,stop_at,pending_at,mutation,accesses,call_count,events,what,n);exit(1);}
extern int color_fade_backend_command(int);
int doCOP2(int op)
{
 if(!observing)return color_fade_backend_command(op);
 if(events>=128)fail("GTE capacity",events);
 PcPortMipsCpu *cpu=candidate?rt.bridge_cpu:oracle_cpu;if(!cpu)fail("GTE CPU context",0);
 Event row;memset(&row,0,sizeof(row));row.command=(uint32_t)op;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));row.hi=cpu->hi;row.lo=cpu->lo;row.before=gteRegs;
 int rc=color_fade_backend_command(op);row.after=gteRegs;
 if(compare){
  if(events>=expected_events||memcmp(&row,&event_trace[events],sizeof(row))){
   if(events<expected_events)for(unsigned i=0;i<32;i++)if(row.gpr[i]!=event_trace[events].gpr[i])fprintf(stderr,"GTE GPR%u native=%08x original=%08x\n",i,row.gpr[i],event_trace[events].gpr[i]);
   fail("GTE command/full state",(uint32_t)op);
  }
 }else event_trace[events]=row;
 events++;return rc;
}
static int body_index(uint32_t a)
{for(unsigned i=0;i<BODY_COUNT;i++)if(a-bodies[i].address<bodies[i].size)return (int)i;return -1;}
static int observed_access(void *opaque,uint32_t a,unsigned w,uint32_t *value,int write)
{
 if(observing&&!write&&body_index(a)>=0){
  if(candidate||w!=4)fail("native instruction fetch",a);
  const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
  *value=load_le(image->bytes+XENO_RD_SLUS_OFF(a),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,a,w,*value):model_test_raw_read(opaque,a,w,value);
 if(accesses>=8192)fail("access capacity",accesses);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,a,w,*value):model_test_raw_read(opaque,a,w,value);
 Access row={a,w,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered access",a);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *p,uint32_t a,unsigned w,uint32_t *v){return observed_access(p,a,w,v,0);}
static int runtime_write(void *p,uint32_t a,unsigned w,uint32_t v){return observed_access(p,a,w,&v,1);}
static void put(uint32_t a,unsigned w,uint32_t n){if(model_test_raw_write(&rt,a,w,n))fail("fixture write",a);}
static uint32_t peek(uint32_t a,unsigned w){uint32_t n;if(model_test_raw_read(&rt,a,w,&n))fail("fixture read",a);return n;}
static void snapshot(void)
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 if(call_count>=128)fail("call capacity",call_count);
 Call row;memset(&row,0,sizeof(row));row.target=target;row.hi=cpu->hi;row.lo=cpu->lo;row.gte=gteRegs;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){
  if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row))){
   if(call_count<expected_calls)for(unsigned i=0;i<32;i++)if(row.gpr[i]!=calls[call_count].gpr[i])fprintf(stderr,"call GPR%u native=%08x original=%08x\n",i,row.gpr[i],calls[call_count].gpr[i]);
   fail("call/full memory state",target);
  }
 }else calls[call_count]=row;
 call_count++;
}
static void mutate_child(PcPortMipsCpu *cpu)
{
 if(!mutation||call_count!=2)return;
 mutation_seen++;
 if(mutation==1){uint32_t old=cpu->gpr[29],next=0x801d0000u;for(unsigned i=0;i<0x50u;i+=4)put(next+i,4,peek(old+i,4));cpu->gpr[29]=next;}
 if(mutation==2)cpu->gpr[17]=out[1];
 if(mutation==3)cpu->gpr[16]=v[0];
 if(mutation==4){cpu->cp0[3]^=0x78901234u;cpu->hi^=0xdeadbeefu;cpu->lo^=0x87654321u;}
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 uint32_t halt=bodies[kind].caller?bodies[kind].caller:BATTLE_HALT_PC;
 if(target==bodies[kind].address&&cpu->gpr[31]==halt)return candidate?runtime_bridge(opaque,cpu,target):0;
 int bi=body_index(target);
 if(bi>=0){
  if(target!=bodies[bi].address||bi==9)return 0;
  record_call(cpu,target);if(stop_at&&call_count==stop_at)return -1;
  mutate_child(cpu);
  if(!candidate)return 0;
  unsigned call=call_count;int rc=runtime_bridge(opaque,cpu,target);
  if(pending_at&&call==pending_at&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
  return rc;
 }
 fail("unexpected child",target);return -1;
}
static uint32_t token(unsigned offset)
{switch(domain_kind){case 1:return 0xa0110000u+offset;case 2:return 0x80310000u+offset;case 3:return 0x80060000u+offset;case 4:return (uint32_t)(uintptr_t)host+offset;case 5:return 0x1f800000u+offset;default:return 0x80110000u+offset;}}
static void defaults(void)
{kind=shape=domain_kind=alias_kind=bad_kind=seed=stack_kind=table_kind=fault_at=stop_at=pending_at=mutation=0;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 if(PsxMemory_LoadStaticDataFromImage(image->bytes,image->size))fail("static data loader",0);
 for(unsigned i=0;i<BODY_COUNT;i++)if(peek(bodies[i].address,4))fail("guest text present",bodies[i].address);
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"lighting-shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+seed*256u+i;
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0x98760000u+i;gteRegs.CP2D.r[i]=0x12340000u+seed*139u+i*7123u;gteRegs.CP2C.r[i]=(i*173u+seed*71u)&0x1fffu;}
 cpu->gpr[29]=stack_kind==1?0xa01ff000u:stack_kind==2?0x1f800f00u:stack_kind==3?(uint32_t)(uintptr_t)host+0x7000u:0x801ff000u;
 initial_sp=cpu->gpr[29];frame=initial_sp-0x50u;
 cpu->gpr[31]=bodies[kind].caller?bodies[kind].caller:BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 for(unsigned i=0;i<3;i++){v[i]=token(0x100u+i*0x20u);out[i]=token(0x200u+i*0x20u);}color=token(0x180u);
 if(alias_kind==1)out[0]=v[0];
 if(alias_kind==2)out[0]=v[0]+4u;
 if(alias_kind==3)out[0]=frame+0x10u;
 if(alias_kind==4)out[0]=frame+0x40u;
 if(alias_kind==5)v[0]=frame+0x10u;
 if(alias_kind==6)out[0]=0x80056b14u;
 if(alias_kind==7)out[1]=out[2]=out[0];
 if(alias_kind==8)out[0]=initial_sp+0x14u;
 if(alias_kind==9)v[0]=initial_sp-8u;
 if(alias_kind==10)v[1]=v[0]+(kind==2?4u:2u);
 for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)put(v[i]+j*2u,2,j==3?0x7a5bu:shape<16?(uint16_t)triangles[shape][i*3u+j]:(seed*731u+i*631u+j*333u));
 if(kind==2)for(unsigned i=0;i<3;i++){put(v[0]+i*4u,4,values[(shape+i)%24u]);put(v[1]+i*4u,4,values[(shape*3u+i*7u+1u)%24u]);}
 put(color,4,0x6d204080u+seed);for(unsigned i=0;i<3;i++)put(initial_sp+0x10u+i*4u,4,out[i]);
 if(kind==5){put(initial_sp+0x10u,4,out[1]);put(initial_sp+0x14u,4,out[2]);}
 if(table_kind){
  if(table_kind==4){rt.data[1]=(ResolvedData){0x80056800u,0x600u,shared+0x10000u,"shared-normalization-tables"};rt.data_count=2;}
  for(uint32_t a=0x80056800u;a<0x80056e00u;a+=2)put(a,2,table_kind==1?0:table_kind==2?0x7fff:table_kind==3?0x8000:0x1234);
 }
 if(bad_kind==1)v[0]++;
 if(bad_kind==2)out[0]++;
 if(bad_kind==3)v[0]=0x1e000000u;
 if(bad_kind==4)out[0]=0x1e000000u;
 if(bad_kind==5){v[0]=(uint32_t)(uintptr_t)host+sizeof(host)-4u;put(v[0],4,1);}
 if(bad_kind==6)out[0]=(uint32_t)(uintptr_t)host+sizeof(host)-4u;
 if(bad_kind==7){out[0]=(uint32_t)(uintptr_t)host+0x200u;rt.host_ranges[0].writable=0;}
 if(bad_kind==8)cpu->gpr[29]++;
 cpu->gpr[4]=v[0];cpu->gpr[5]=v[1];cpu->gpr[6]=v[2];cpu->gpr[7]=out[0];
 if(kind==1){cpu->gpr[4]=values[shape%24u];cpu->gpr[5]=values[(shape*3u+seed)%24u];cpu->gpr[6]=values[(shape*7u+seed)%24u];}
 if(kind==2)cpu->gpr[6]=out[0];
 if(kind==3)cpu->gpr[4]=values[shape%24u];
 if(kind==4)cpu->gpr[5]=out[0];
 if(kind==6){cpu->gpr[5]=color;cpu->gpr[6]=out[0];}
 if(kind==7){cpu->gpr[7]=color;put(initial_sp+0x10u,4,out[0]);}
 accesses=events=call_count=legacy_calls=mutation_seen=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu original,native;setup(&original);candidate=compare=0;oracle_cpu=&original;
 uint32_t target=bodies[kind].address,halt=original.gpr[31];int original_rc=PcPortMipsRun(&original,target,halt,100000);
 expected_accesses=accesses;expected_events=events;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,target,halt,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)rc);
 for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i]){fprintf(stderr,"GPR%u native=%08x original=%08x original_pc=%08x\n",i,native.gpr[i],original.gpr[i],original.pc);fail("register",i);}
 if(original.hi!=native.hi||original.lo!=native.lo||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",0);
 if(original_rc==PC_PORT_MIPS_HALTED&&(original.load_valid||native.load_valid))fail("pending terminal load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||events!=expected_events||call_count!=expected_calls||legacy_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",0);
 if(fault_at&&accesses!=fault_at)fail("unreached fault",fault_at);
 if(mutation&&!mutation_seen)fail("unreached mutation",mutation);
 last_oracle=original;last_native=native;return original_rc;
}
static unsigned comparisons(void)
{
 unsigned total=0;
 for(unsigned k=0;k<NEW_BODIES;k++)for(unsigned s=0;s<24;s++)for(unsigned d=0;d<6;d++){defaults();kind=k;shape=s;domain_kind=d;seed=s*13u;stack_kind=d%4u;run_case();total++;}
 for(unsigned k=0;k<NEW_BODIES;k++)for(unsigned a=1;a<=10;a++){defaults();kind=k;shape=5;alias_kind=a;run_case();total++;}
 for(unsigned k=0;k<NEW_BODIES;k++)for(unsigned b=1;b<=8;b++){defaults();kind=k;shape=5;bad_kind=b;run_case();total++;}
 for(unsigned s=0;s<16;s++)for(unsigned t=1;t<=4;t++){defaults();shape=s;table_kind=t;run_case();total++;}
 for(unsigned m=1;m<=4;m++){defaults();shape=5;mutation=m;run_case();total++;}
 defaults();printf("MODEL LIGHTING BODY PASS %u full-state actual-helper comparisons\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned k=0;k<NEW_BODIES;k++)for(unsigned s=0;s<6;s++){
  defaults();kind=k;shape=s;alias_kind=s%3u;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("MODEL LIGHTING FAULT PASS %u ordered access failures\n",total);return total;
}
static unsigned children(void)
{
 unsigned total=0;defaults();shape=5;run_case();unsigned n=expected_calls;
 for(stop_at=1;stop_at<=n;stop_at++){run_case();total++;}stop_at=0;
 for(pending_at=1;pending_at<=n;pending_at++){
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,bodies[0].address,BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||call_count!=pending_at||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending child refusal",pending_at);
  total++;
 }
 defaults();printf("MODEL LIGHTING CHILD PASS %u refusals/pending returns\n",total);return total;
}
static unsigned controls(void)
{
 unsigned total=0;
 for(unsigned k=0;k<NEW_BODIES;k++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;
  PcPortMipsCpu old=cpu;snapshot();uint32_t target=bodies[k].address;XenoRetailFileSlot image=g_XenoRetailFiles[XENO_RD_SLUS];
  for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?bodies[k].size/2u:bodies[k].size-1u;uint8_t *p=image.bytes+XENO_RD_SLUS_OFF(target)+off;*p^=1;
   if(battle_model_lighting_try_host(&rt,&cpu,target))fail("body identity",off);
   *p^=1;total++;}
  g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+bodies[k].size-1u;
  if(battle_model_lighting_try_host(&rt,&cpu,target))fail("short image",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=image;total++;
  g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("missing image",target);g_XenoRetailFiles[XENO_RD_SLUS]=image;total++;
  cpu.load_valid=1;cpu.load_reg=4;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("pending entry",0);cpu=old;total++;
  rt.force_interpret=1;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("forced",0);rt.force_interpret=0;total++;
  rt.initialized=0;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("uninitialized",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("context",0);rt.bridge_cpu=&cpu;total++;
  if(battle_model_lighting_try_host(&rt,&cpu,target+4u))fail("internal target",target);
  total++;
  if(bodies[k].caller){cpu.gpr[31]^=4u;if(battle_model_lighting_try_host(&rt,&cpu,target))fail("scope",target);cpu=old;total++;}
  assert(setenv("XENO_BATTLE_NATIVE_MODEL_LIGHTING","0",1)==0);if(battle_model_lighting_try_host(&rt,&cpu,target))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_LIGHTING")==0);total++;
  if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||events||call_count)fail("gate side effect",target);
 }
 defaults();printf("MODEL LIGHTING GATES PASS %u identity/context/opt-out controls\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned k=0;k<NEW_BODIES;k++)for(unsigned cached=0;cached<2;cached++)for(unsigned reject=0;reject<3;reject++){
  defaults();kind=k;shape=1;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  uint32_t target=bodies[k].address,halt=cpu.gpr[31];rt.functions[0]=(ResolvedFunction){target,legacy_probe,"unusable_model_lighting"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *code=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MODEL_LIGHTING","0",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&cpu,target,halt,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_LIGHTING")==0);if(reject==1)code[0]^=1;
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy precedence",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("valid native boundary",target);
  if(reject&&(rc!=PC_PORT_MIPS_INTERPRETER_DISABLED||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("refused native boundary",target);
  total++;
 }
 defaults();printf("MODEL LIGHTING CACHE PASS %u resolved/cached controls\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned s=0;s<3;s++){
  defaults();shape=s;if(run_case()!=PC_PORT_MIPS_HALTED)fail("face witness status",s);
  const int expected[][3]={{-2364,-2364,-2364},{0,0,-4096},{0,0,4096}};
  for(unsigned i=0;i<3;i++)if(peek(out[0]+i*2u,2)!=(uint16_t)expected[s][i])fail("face winding/degenerate",i);
  if(peek(out[0]+6u,2)!=0xa5a5u||events!=3||call_count!=4)fail("face pad/composition",s);
  total++;
 }
 for(unsigned k=4;k<8;k++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  memset(&gteRegs,0,sizeof(gteRegs));gteRegs.CP2C.r[13]=32;gteRegs.CP2C.r[14]=512;gteRegs.CP2C.r[15]=2048;gteRegs.CP2D.r[6]=0x5a9abcdeu;
  gteRegs.CP2D.r[20]=0x11223344u;gteRegs.CP2D.r[21]=0x22334455u;gteRegs.CP2D.r[22]=0x33445566u;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,bodies[k].address,cpu.gpr[31],100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  uint32_t expected=k<6?0x5a802002u:0x6d100801u;unsigned n=k==5||k==7?3:1;
  for(unsigned i=0;i<n;i++)if(peek(out[i],4)!=expected)fail("independent color",i);
  if(rc!=PC_PORT_MIPS_HALTED||cpu.steps||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||gteRegs.CP2D.r[22]!=expected||gteRegs.CP2D.r[20]!=(n==3?expected:0x22334455u)||gteRegs.CP2D.r[21]!=(n==3?expected:0x33445566u))fail("independent FIFO",k);
  total++;
 }
 /* Raw signed DIV corners use the production arithmetic with independent
  * constants; there is no division oracle shared with this expectation. */
 const uint32_t ns[]={0,1,0xffffffffu,0x80000000u},ds[]={0,0,0,0xffffffffu},qs[]={0xffffffffu,0xffffffffu,1,0x80000000u},rs[]={0,1,0xffffffffu,0};
 for(unsigned i=0;i<4;i++){PcPortMipsCpu cpu={0};BattleEntryCall entry={.cpu=&cpu};battle_model_lighting_divide(&entry,ns[i],ds[i]);if(cpu.lo!=qs[i]||cpu.hi!=rs[i])fail("raw signed DIV",i);total++;}
 defaults();printf("MODEL LIGHTING SEMANTICS PASS %u winding/degenerate/color/FIFO/DIV witnesses\n",total);return total;
}
int main(int argc,char **argv)
{
 if(argc>1){defaults();if(!strcmp(argv[1],"--basic")){run_case();return 0;}if(!strcmp(argv[1],"--all-bodies"))return comparisons()?0:1;if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;if(!strcmp(argv[1],"--children"))return children()?0:1;return 2;}
 if(!comparisons()||!faults()||!children()||!controls()||!boundaries()||!semantics())return 1;
 puts("MODEL LIGHTING ALL PASS native interpreted instructions=0; actual GTE, static-data loader and nested helpers; no full-game claim");return 0;
}
