/* Complete effect-grid body with independently checked native SDK boundaries.
 * Private instructions are fetched only by the test oracle. Overlay bytes
 * remain in guest RAM for identity checks; SLUS text is absent. Native
 * legs disable the CPU decoder. Both legs use the actual GTE backend. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the test runner to produce the bus-observed runtime snapshot"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "../src/psx_memory.c"
#include "psx/gtereg.h"
int g_GPUDisabledState;unsigned g_PcPortPresentedFrames;
int32_t D_80050104,D_800500F8,D_800500FC,D_80050100,D_80059578;
uint32_t D_800595C0;
uint16_t D_80059308,D_8005930C;
void ControllerPushState(void){}
void ControllerPoll(void){}
void PsyX_UpdateInput(void){}
int PsyX_Sys_GetVBlankCount(void){return 0;}
int PcPort_QuickCheckpointFieldIsActive(void){return 0;}
char PsyX_BeginScene(void){abort();}
void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();}
void ParsePrimitivesLinkedList(u_long *p,int s){(void)p;(void)s;abort();}

#include <openssl/sha.h>

static BattleMipsRuntime rt;
static uint8_t battle[343936],shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static GTERegisters want_gte;
static unsigned scenario,domain,parameter_domain,stack_kind,alias_kind,bank,caller;
static unsigned fault_at,stop_at,stop_mode,pending_return,hash_calls,real_sdk,in_sdk;
static unsigned accesses,expected_accesses,events,expected_events,call_count,expected_calls;
static unsigned body_rtps,legacy_calls,rejected,expected_rejected,real_calls,expected_real_calls;
static uint64_t sdk_steps;
static int candidate,compare,observing;
static PcPortMipsCpu *oracle_cpu,last_oracle,last_native,pending_cpu;
static uint32_t object,position1,position2,matrix,ot;
static const uint32_t sdk_targets[]={0x80048d68u,0x80049efcu,0x80049f8cu,0x80048c4cu,0x8004b32cu,0x8004a14cu,0x8003f738u,0x8004a92cu,0x8004931cu};
static const char *sdk_names[]={"VectorNormalS","SetRotMatrix","SetTransMatrix","SquareRoot0","ratan2","SetGeomScreen","RotMatrix","RotMatrixYXZ","CompMatrix"};
static void *sdk_functions[]={VectorNormalS,SetRotMatrix,SetTransMatrix,SquareRoot0,ratan2,SetGeomScreen,RotMatrix,RotMatrixYXZ,CompMatrix};
static const struct {uint32_t start,length;} sdk_spans[]={
 {0x80048d68u,20},{0x80048dbcu,216},{0x80049efcu,48},{0x80049f8cu,32},
 {0x80048c4cu,132},{0x8004b32cu,384},{0x8004a14cu,16},{0x8003f738u,376},
 {0x8004a92cu,652},{0x8004931cu,352}
};
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
typedef struct GteEvent {uint32_t command,gpr[32],hi,lo;GTERegisters before,after;} GteEvent;
typedef struct Call {uint32_t target,gpr[32],cp0[32],hi,lo;GTERegisters gte;uint8_t memory[128];} Call;
static Access trace[8192];static GteEvent gte_trace[256];static Call calls[64];static uint32_t rejected_targets[8];
static void fail(const char *why,uint32_t value)
{fprintf(stderr,"EFFECT GRID FAIL scenario=%u domain=%u params=%u stack=%u alias=%u bank=%08x caller=%u sdk=%u fault=%u stop=%u/%u pending=%u access=%u call=%u event=%u %s %08x\n",scenario,domain,parameter_domain,stack_kind,alias_kind,bank,caller,real_sdk,fault_at,stop_at,stop_mode,pending_return,accesses,call_count,events,why,value);exit(1);}
extern int color_fade_backend_command(int);
int doCOP2(int op)
{
 if(!observing)return color_fade_backend_command(op);
 if(events>=256)fail("GTE trace capacity",events);
 PcPortMipsCpu *cpu=candidate?rt.bridge_cpu:oracle_cpu;if(!cpu)fail("GTE CPU context",0);
 GteEvent row;memset(&row,0,sizeof(row));row.command=(uint32_t)op;row.hi=cpu->hi;row.lo=cpu->lo;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));row.before=gteRegs;
 int rc=color_fade_backend_command(op);
 if((uint32_t)op==0x4a180001u){
  body_rtps++;
  /* Explicit branch-boundary witnesses alter only SXY2 after actual RTPS,
   * on both legs. All other cases retain unmodified graphics results. */
  static const int y[]={-32768,-1,0,239,240,241,479,480,32767};
  if(scenario>=10&&scenario<=18&&body_rtps==1)gteRegs.CP2D.r[14]=(gteRegs.CP2D.r[14]&0xffffu)|((uint32_t)(uint16_t)y[scenario-10]<<16);
  if(scenario>=19&&scenario<=21&&body_rtps==2)gteRegs.CP2D.r[14]=(gteRegs.CP2D.r[14]&0xffffu)|((uint32_t)(uint16_t)(scenario==19?-1:scenario==20?239:240)<<16);
  if(scenario>=22&&scenario<=23&&body_rtps==3)gteRegs.CP2D.r[14]=(gteRegs.CP2D.r[14]&0xffffu)|((uint32_t)(uint16_t)(scenario==22?-100:400)<<16);
 }
 row.after=gteRegs;
 if(compare){
  if(events>=expected_events||memcmp(&row,&gte_trace[events],sizeof(row))){
   if(events<expected_events)for(unsigned i=0;i<32;i++)if(row.gpr[i]!=gte_trace[events].gpr[i])fprintf(stderr,"GTE boundary GPR %u native=%08x original=%08x\n",i,row.gpr[i],gte_trace[events].gpr[i]);
   fail("GTE command/state sequence",(uint32_t)op);
  }
 }else gte_trace[events]=row;
 events++;return rc;
}
static const uint8_t *instruction(uint32_t a)
{
 if(a-0x800a4db8u<2500u||a-0x800a4654u<460u)return battle+(a-0x8006faf0u);
 for(unsigned i=0;i<sizeof(sdk_spans)/sizeof(sdk_spans[0]);i++)if(a-sdk_spans[i].start<sdk_spans[i].length){
  const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe);return exe->bytes+XENO_RD_SLUS_OFF(a);
 }
 return NULL;
}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 const uint8_t *code=instruction(address);
 if((observing||in_sdk)&&!write&&code){assert(!candidate&&width==4);*value=load_le(code,4);return 0;}
 if(!observing){
  uint32_t mapped=in_sdk?shard_sdk_address(address):address;
  return write?model_test_raw_write(opaque,mapped,width,*value):model_test_raw_read(opaque,mapped,width,value);
 }
 if(accesses>=8192)fail("access trace capacity",accesses);
 uint32_t mapped=candidate?address:shard_sdk_address(address);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,mapped,width,*value):model_test_raw_read(opaque,mapped,width,value);
 Access row={shard_sdk_address(address),width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *p,uint32_t a,unsigned w,uint32_t *v){return observed_access(p,a,w,v,0);}
static int runtime_write(void *p,uint32_t a,unsigned w,uint32_t v){return observed_access(p,a,w,&v,1);}
static void put(uint32_t a,unsigned w,uint32_t v){if(model_test_raw_write(&rt,shard_sdk_address(a),w,v))fail("fixture write",a);}
static uint32_t peek(uint32_t a,unsigned w){uint32_t v;if(model_test_raw_read(&rt,shard_sdk_address(a),w,&v))fail("fixture read",a);return v;}
static void snapshot(void)
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 if(call_count>=64)fail("call trace capacity",call_count);
 Call row;memset(&row,0,sizeof(row));row.target=target;row.hi=cpu->hi;row.lo=cpu->lo;row.gte=gteRegs;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));memcpy(row.cp0,cpu->cp0,sizeof(row.cp0));
 if(hash_calls){assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.memory));assert(SHA256(shared,sizeof(shared),row.memory+32));assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.memory+64));assert(SHA256(host,sizeof(host),row.memory+96));}
 if(compare&& (call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))){
  if(call_count<expected_calls)for(unsigned i=0;i<32;i++)if(row.gpr[i]!=calls[call_count].gpr[i])fprintf(stderr,"callee GPR %u native=%08x original=%08x\n",i,row.gpr[i],calls[call_count].gpr[i]);
  fail("callee architectural/GTE/memory-hash state",target);
 }
 if(!compare)calls[call_count]=row;
 call_count++;
}
static int sdk_service(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 int saved=observing;observing=0;int rc;
 if(candidate||!real_sdk)rc=runtime_bridge(opaque,cpu,target);
 else{
  PcPortMipsCpu original=*cpu;uint64_t before=original.steps;in_sdk=1;
  rc=PcPortMipsRun(&original,target,cpu->gpr[31],10000);in_sdk=0;sdk_steps+=original.steps-before;
  if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
  real_calls++;
  if(target==0x80048d68u||target==0x8004a92cu){
   memcpy(cpu->gpr,original.gpr,sizeof(cpu->gpr));memcpy(cpu->cp0,original.cp0,sizeof(cpu->cp0));cpu->hi=original.hi;cpu->lo=original.lo;
   if(original.load_valid)fail("original fixed SDK return pending",target);
  }else{
   /* Established typed SDK boundaries normalize caller-saved CPU state to
    * their C ABI. Their actual memory/GTE effects remain unmodified. */
   for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("SDK saved register",i);
   if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31])fail("SDK frame/return",target);
   /* The earlier camera typed bridges handle these addresses and publish
    * only v0; the later legacy shard thunk's v1 clearing is not reached. */
   if(target==0x80048c4cu||target==0x8004b32cu)cpu->gpr[2]=original.gpr[2];
   if(target==0x8003f738u||target==0x8004931cu)cpu->gpr[2]=original.gpr[2];
   if(target==0x8003f738u){cpu->hi=original.hi;cpu->lo=original.lo;}
  }
  rc=1;
 }
 observing=saved;return rc;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(target==0x800a4db8u||target==0x800a4654u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x800a4db8u<2500u||target-0x800a4654u<460u)return 0;
 int known=0;for(unsigned i=0;i<9;i++)if(target==sdk_targets[i])known=1;
 int modeled=caller&&(target==0x800a6ae8u||target==0x80027eacu||target==0x800273c4u);
 if(!known&&!modeled){
  if(!alias_kind||rejected>=8)fail("unexpected child",target);
  if(compare){if(rejected>=expected_rejected||rejected_targets[rejected]!=target)fail("corrupted return target",target);}else rejected_targets[rejected]=target;
  rejected++;return -1;
 }
 record_call(cpu,target);
 if(stop_at&&call_count==stop_at){
  if(stop_mode==2){put(0x80190000u,4,target);cpu->gpr[2]=0x76543210u;cpu->hi=0x12345678u;cpu->lo=0x87654321u;cpu->cp0[3]=0xabcdef01u;}
  return -1;
 }
 /* Only the caller's five unrelated children are modeled. All nine grid
  * dependencies execute their actual native implementations. */
 int rc=modeled?1:sdk_service(opaque,cpu,target);
 if(pending_return&&call_count==pending_return&&rc==1){pending_cpu=*cpu;snapshot();cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeefu;}
 return rc;
}
static uint32_t token(unsigned d,uint32_t address,unsigned offset)
{switch(d){case 1:return address|0x20000000u;case 2:return address+0x200000u;case 3:return address&0x1fffffu;case 4:return 0x80060000u+offset;case 5:return (uint32_t)(uintptr_t)host+offset;case 6:return 0x1f800000u+offset;default:return address;}}
static void defaults(void)
{scenario=domain=parameter_domain=stack_kind=alias_kind=bank=caller=fault_at=stop_at=stop_mode=pending_return=hash_calls=real_sdk=in_sdk=0;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));memset(&gteRegs,0,sizeof(gteRegs));
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&PsxMemory_LoadStaticDataFromImage(exe->bytes,exe->size)==0);
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"effect-grid-shared"};rt.data_count=1;rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 for(unsigned i=0;i<9;i++)rt.functions[rt.function_count++]=(ResolvedFunction){sdk_targets[i],sdk_functions[i],sdk_names[i]};
 qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0xa9876500u+i;
 cpu->gpr[29]=0x801ff000u;cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 object=token(domain,0x80100000u,0x1000u);position1=token(parameter_domain,0x80120000u,0x100u);
 position2=position1+8u;matrix=position1+0x20u;ot=0x80180000u;
 if(domain==6)object=0x1f800000u;
 if(stack_kind==1)cpu->gpr[29]=0xa01ff000u;
 if(stack_kind==2)cpu->gpr[29]=0x1f800f00u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x70000u;
 if(stack_kind==4)cpu->gpr[29]=0x8007f000u;
 if(stack_kind==5)cpu->gpr[29]=0;
 if(stack_kind==6)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==7)cpu->gpr[29]=0x801ff001u;
 if(alias_kind==1)ot=object+0x3cu;
 if(alias_kind==2)ot=object+0x2dcu;
 if(alias_kind==3)ot=object+0x2e8u;
 if(alias_kind==4)position1=object+0x2dcu;
 if(alias_kind==4){position2=position1+8u;matrix=position1+0x20u;}
 if(alias_kind==5)cpu->gpr[29]=object+0x2dcu+0xe8u;
 if(alias_kind==6)matrix=object+0x3cu;
 if(alias_kind==7)ot=cpu->gpr[29]-0x18u;
 if(alias_kind==8)position2=position1;
 for(unsigned i=0;i<32;i++)put(matrix+i,1,0);
 for(unsigned i=0;i<3;i++)put(matrix+i*8u,2,4096);
 put(matrix+26u,2,0); /* translation bytes already initialized */
 for(unsigned i=0;i<8;i++)put(position1+i,1,0);
 for(unsigned i=0;i<8;i++)put(position2+i,1,0);
 put(position2+4u,2,512);
 if(scenario==4){put(position1,2,32767);put(position2,2,0x8000);put(position2+2u,2,0x8000);}
 if(scenario==5){put(position1,2,0x8000);put(position2,2,32767);put(position1+4u,2,0x8000);put(position2+4u,2,32767);}
 if(scenario==6){put(position1,2,0);put(position1+4u,2,0);put(position2,2,0);put(position2+4u,2,0);}
 if(scenario==7){put(position1+2u,2,0xfffd);put(position2+2u,2,0xffe1);}
 if(scenario==8){put(position1,2,0xffff);put(position1+4u,2,0xfff3);put(position2,2,0xffd0);put(position2+4u,2,101);}
 unsigned capacity=domain==6?0x1000u:0x1800u;
 for(unsigned i=0;i<capacity;i++)put(object+i,1,(i*17u+0x53u)&255u);
 put(object,2,scenario==24?0x7fffu:scenario==25?0xfff2u:19u);put(object+2u,2,scenario==26?0x7fffu:scenario==27?0xfff2u:31u);
 put(object+4u,2,scenario==24?1:scenario==25?0xffffu:3);put(object+6u,2,scenario==26?1:scenario==27?0xffffu:5);
 if(scenario==33){put(object,2,0xffffu);put(object+2u,2,0xffffu);put(object+4u,2,0);put(object+6u,2,0);}
 if(scenario==34){put(object,2,0xfff0u);put(object+2u,2,0xfff0u);put(object+4u,2,0xffffu);put(object+6u,2,0xffffu);}
 if(scenario==35){put(object,2,0xffffu);put(object+2u,2,0xffffu);put(object+4u,2,1);put(object+6u,2,1);}
 put(object+8u,2,scenario==28?0:scenario==29?1:scenario==30?63:scenario==31?0x8001u:64);
 put(object+0xau,2,scenario==7?0xffffu:0);put(object+0xcu,2,scenario==9?0xff80u:128);put(object+0xeu,2,scenario==9?0xff00u:256);
 put(object+0x10u,2,256);put(object+0x12u,2,scenario==7?0x7fffu:scenario==8?0x8000u:0);
 put(object+0x16u,2,scenario==9?0xffc4u:60);put(object+0x18u,2,scenario==9?0xff80u:128);
 if(scenario==36){
  put(position2,2,300);put(position2+4u,2,400);
  put(object+0xcu,2,(uint16_t)-127);put(object+0xeu,2,(uint16_t)-255);
  put(object+0x16u,2,61);put(object+0x18u,2,129);
 }
 for(unsigned row=0;row<9;row++)for(unsigned col=0;col<9;col++){
  int x=((int)col-4)*32,y=((int)row-4)*16,z=-7680;
  if(scenario==1)x=-x;
  if(scenario==2&&(row&1u))x=-x;
  if(scenario==3)x=y=0;
  if(scenario==9)z=-9000;
  uint32_t a=object+0x54u+row*72u+col*8u;put(a,2,(uint16_t)x);put(a+2u,2,(uint16_t)y);put(a+4u,2,(uint16_t)z);put(a+6u,2,0x7135u);
 }
 if(domain!=6)for(unsigned i=0;i<128;i++)put(object+0x2dcu+i*40u,4,0x09000000u|i);
 put(ot,4,0xab012345u);
 if(stack_kind<5){put(cpu->gpr[29]+0x10u,4,ot);put(cpu->gpr[29]+0x14u,4,bank);}
 gteRegs.CP2C.r[24]=160u<<16;gteRegs.CP2C.r[25]=120u<<16;gteRegs.CP2C.r[26]=256;
 cpu->gpr[4]=object;cpu->gpr[5]=position1;cpu->gpr[6]=position2;cpu->gpr[7]=matrix;
 if(scenario==32)cpu->gpr[4]=0;
 if(caller){
  put(0x800d33e4u,4,0x80190000u);put(0x8019010eu,1,0);put(0x80190118u,4,0);
  put(0x800c3e38u,4,0);put(0x800c3d50u,4,0);put(0x800c3d54u,4,0);put(0x800c3ea0u,4,scenario==32?0:object);
  cpu->gpr[4]=matrix;cpu->gpr[5]=0;cpu->gpr[6]=0;cpu->gpr[7]=ot;
  if(stack_kind<5){put(cpu->gpr[29]+0x10u,4,bank);put(cpu->gpr[29]+0x14u,4,position1);put(cpu->gpr[29]+0x18u,4,position2);put(cpu->gpr[29]+0x1cu,4,1);}
 }
 accesses=events=call_count=body_rtps=legacy_calls=rejected=real_calls=0;sdk_steps=0;observing=1;
}

static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 uint32_t target=caller?0x800a4654u:0x800a4db8u;
 int original=PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,100000);
 expected_accesses=accesses;expected_events=events;expected_calls=call_count;expected_rejected=rejected;expected_real_calls=real_calls;
 uint64_t original_sdk_steps=sdk_steps;snapshot();setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int actual=PcPortMipsRun(&native,target,BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)actual);
 if(!fault_at&&!stop_at&&!alias_kind&&stack_kind<5&&domain<6&&bank<2&&original!=PC_PORT_MIPS_HALTED)fail("nominal case did not complete",oracle.pc);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x original=%08x pc=%08x error=%s\n",native.gpr[i],oracle.gpr[i],oracle.pc,oracle.error);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0)))fail("HI/LO/CP0",native.hi);
 if(!memory_equal()){
  for(unsigned i=0;i<PSX_RAM_SIZE;i++)if(want_ram[i]!=g_PsxRam[i]){fprintf(stderr,"RAM difference %08x native=%02x original=%02x\n",i|0x80000000u,g_PsxRam[i],want_ram[i]);break;}
  for(unsigned i=0;i<32;i++)if(want_gte.CP2D.r[i]!=gteRegs.CP2D.r[i])fprintf(stderr,"GTE data difference %u native=%08x original=%08x\n",i,gteRegs.CP2D.r[i],want_gte.CP2D.r[i]);
  fail("full final state",0);
 }
 if(original==PC_PORT_MIPS_HALTED&&(native.load_valid!=oracle.load_valid||(oracle.load_valid&&(native.load_reg!=oracle.load_reg||native.load_value!=oracle.load_value||native.load_mask!=oracle.load_mask))))fail("terminal pending load",native.load_reg);
 if(native.steps||sdk_steps||rt.bridge_cpu||accesses!=expected_accesses||events!=expected_events||call_count!=expected_calls||rejected!=expected_rejected||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instruction/context/event count",accesses);
 if(real_sdk&&expected_real_calls&& !original_sdk_steps)fail("original SDK not executed",0);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 last_oracle=oracle;last_native=native;return original;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 unsigned total=0;uint8_t *bytes=PSX_ADDR(0x800a4db8u);
 for(unsigned i=0;i<3;i++){unsigned offset=i==0?0:i==1?1250:2499;bytes[offset]^=1;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("complete identity",offset);bytes[offset]^=1;total++;}
 cpu.load_valid=1;cpu.load_reg=4;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("forced",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("uninitialized",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_effect_grid_try_host(&rt,&cpu,0x800a4dbcu))fail("internal entry",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_EFFECT_GRID","0",1)==0);if(battle_effect_grid_try_host(&rt,&cpu,0x800a4db8u))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_GRID")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||events||call_count)fail("gate side effect",0);
 defaults();printf("EFFECT GRID PASS %u identity/context/pending/opt-out gates\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();scenario=32;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){0x800a4db8u,legacy_probe,"effect-grid-legacy"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x800a4db8u);slot->target=0x800a4db8u;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *bytes=PSX_ADDR(0x800a4db8u);if(reject==1)bytes[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_EFFECT_GRID","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,0x800a4db8u):PcPortMipsRun(&cpu,0x800a4db8u,cpu.gpr[31],100000);
  if(reject==1){bytes[0]^=1;want_ram[0x000a4db8u]^=1;}
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_GRID")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption",cached);
  if(!reject&&(rc!=PC_PORT_MIPS_HALTED||cpu.load_valid))fail("canonical precedence",(uint32_t)rc);
  if(reject&&((disabled?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected adoption",(uint32_t)rc);
  total++;
 }
 defaults();printf("EFFECT GRID PASS %u cached/resolved adoption and refusal boundaries\n",total);return total;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<8;path++){
  defaults();scenario=path<4?path==3?32:path:path==4?10:path==5?19:0;
  if(path==6)alias_kind=2;
  if(path==7){caller=1;scenario=1;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("EFFECT GRID PASS %u ordered body/caller memory faults with GTE and partial architectural state\n",total);return total;
}
static unsigned refusals(void)
{
 unsigned total=0;
 for(unsigned mode=1;mode<=2;mode++)for(unsigned call=1;call<=15;call++){
  defaults();stop_mode=mode;stop_at=call;run_case();if(call_count!=call)fail("callee refusal not reached",call);total++;
 }
 for(unsigned call=1;call<=15;call++){
  defaults();pending_return=call;PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x800a4db8u,BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||call_count!=call||memcmp(cpu.gpr,pending_cpu.gpr,sizeof(cpu.gpr))||memcmp(cpu.cp0,pending_cpu.cp0,sizeof(cpu.cp0))||cpu.hi!=pending_cpu.hi||cpu.lo!=pending_cpu.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending child refusal",call);
  total++;
 }
 defaults();printf("EFFECT GRID PASS %u child refusal/partial-state/pending-return checks\n",total);return total;
}
static unsigned integrations(void)
{
 unsigned total=0;
 for(unsigned s=0;s<37;s++)for(unsigned b=0;b<2;b++){defaults();scenario=s;bank=b;caller=1;run_case();total++;}
 for(unsigned s=0;s<37;s++)if(s<10||s>23)for(unsigned d=0;d<3;d++){
  defaults();scenario=s;parameter_domain=d==0?0:d==1?3:5;real_sdk=1;run_case();total++;
  if(s!=32&&expected_real_calls!=15)fail("complete actual SDK graph",expected_real_calls);
 }
 for(unsigned s=0;s<4;s++){defaults();scenario=s;caller=real_sdk=hash_calls=1;run_case();total++;}
 defaults();printf("EFFECT GRID PASS %u real coordinator/native SDK and independent original-SDK integrations\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned s=0;s<37;s++){
  defaults();scenario=s;int rc=run_case();if(rc!=PC_PORT_MIPS_HALTED)fail("semantic case did not complete",s);
  if(s==32){if(events||call_count)fail("null body still performs graphics",0);total++;continue;}
  unsigned rtpt=0,nclip=0,rtps=0,accepted=0,index=0,previous=0;
  for(unsigned i=0;i<events;i++){
   if(gte_trace[i].command==0x4a280030u)rtpt++;
   if(gte_trace[i].command==0x4a180001u)rtps++;
   if(gte_trace[i].command!=0x4b400006u)continue;
   nclip++;uint32_t packet=object+0x2dcu+index*40u;
   if((int32_t)gte_trace[i].after.CP2D.r[24]>=0){
    if(i+1>=events||gte_trace[i+1].command!=0x4a180001u)fail("accepted cell fourth projection",index);
    for(unsigned v=0;v<3;v++)if(peek(packet+8u+v*8u,4)!=gte_trace[i].after.CP2D.r[12+v])fail("accepted cell first three XY",index);
    if(peek(packet+32u,4)!=gte_trace[i+1].after.CP2D.r[14])fail("accepted cell fourth XY",index);
    if((peek(packet,4)&0xff000000u)!=0x09000000u)fail("packet high tag",index);
    if(accepted&&(peek(packet,4)&0xffffffu)!=(previous&0xffffffu))fail("accepted packet link sequence",index);
    accepted++;previous=packet;
   }else if(peek(packet,4)!=(0x09000000u|index))fail("negative-area packet unchanged",index);
   index++;
  }
  if(rtpt!=64||nclip!=64||index!=64||rtps!=3+accepted||call_count!=15||gteRegs.CP2C.r[26]!=512u)fail("complete 64-cell traversal and screen restore",s);
  if((s==0||s==3)&&accepted!=64)fail("front/zero area acceptance",accepted);
  if(s==1&&accepted)fail("negative area rejection",accepted);
  if(s==2&&(!accepted||accepted==64))fail("mixed cell clipping",accepted);
  if(peek(ot,4)!=(0xab000000u|((object+0x24u)&0xffffffu)))fail("final packet and OT high byte",s);
  if(s==36){
   /* First SetRotMatrix call retains normalized T0/T2, before later transforms. */
   int32_t nx=(int16_t)calls[1].gpr[8],nz=(int16_t)calls[1].gpr[10];
   for(unsigned i=1;i<=2;i++){
    int32_t coefficient=i==1?-255:-127;
    int32_t products[]={(nx*129/4096)*coefficient,61*coefficient,(nz*129/4096)*coefficient};
    int32_t expected[]={300+products[0]/256,products[1]/256,400+products[2]/256};
    uint32_t xy=gte_trace[i].before.CP2D.r[0],z=gte_trace[i].before.CP2D.r[1];
    int32_t actual[]={(int16_t)xy,(int16_t)(xy>>16),(int16_t)z};
    for(unsigned j=0;j<3;j++)if(products[j]>=0||products[j]%256==0||actual[j]!=expected[j])fail("nonintegral negative Q8 truncation",i*3u+j);
   }
  }
  if(s==0){
   if(peek(object,2)!=22||peek(object+2u,2)!=36)fail("UV scroll updates",0);
   for(unsigned row=0;row<8;row++)for(unsigned col=0;col<8;col++){
    uint32_t packet=object+0x2dcu+(row*8u+col)*40u;unsigned u=1u+(col&1u)*32u,v=44u+(row&1u)*32u;
    const unsigned off[]={12,13,20,21,28,29,36,37};const unsigned val[]={u,v,u+31u,v,u,v+31u,u+31u,v+31u};
    for(unsigned j=0;j<8;j++)if(peek(packet+off[j],1)!=(val[j]&255u))fail("alternating grid UV",row*8u+col);
   }
  }
  total++;
 }
 defaults();printf("EFFECT GRID PASS %u explicit clipping/XY/link/UV/loop/null witnesses\n",total);return total;
}
static void load_inputs(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f&&fread(battle,1,sizeof(battle),f)==sizeof(battle)&&fclose(f)==0);
 uint8_t hash[32];assert(SHA256(battle+(0x800a4db8u-0x8006faf0u),2500,hash));char text[65];
 for(unsigned i=0;i<32;i++)snprintf(text+i*2,3,"%02x",hash[i]);
 assert(!strcmp(text,"93aa82abdf0041d3295926bc2b94cd7235baca996c97c9da8115561eef02593a"));
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--basic")){run_case();return 0;}
  if(!strcmp(argv[1],"--faults"))return faults()?0:1;
  if(!strcmp(argv[1],"--gates"))return controls()?0:1;
  if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;
  if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;
  if(!strcmp(argv[1],"--integration"))return integrations()?0:1;
  if(!strcmp(argv[1],"--refusals"))return refusals()?0:1;
  return 2;
 }
 unsigned total=0;
 for(unsigned s=0;s<37;s++)for(unsigned b=0;b<2;b++){defaults();scenario=s;bank=b;run_case();total++;}
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<3;s++){defaults();domain=d;scenario=s;run_case();total++;}
 for(unsigned d=0;d<7;d++)for(unsigned s=0;s<3;s++){defaults();parameter_domain=d;scenario=s;run_case();total++;}
 for(unsigned k=1;k<8;k++){defaults();stack_kind=k;run_case();total++;}
 for(unsigned a=1;a<=8;a++)for(unsigned s=0;s<2;s++){defaults();alias_kind=a;scenario=s;run_case();total++;}
 const uint32_t banks[]={2,0xffffffffu,0x80000000u,0x40000001u,0x01000000u};
 for(unsigned i=0;i<5;i++){defaults();bank=banks[i];run_case();total++;}
 defaults();printf("EFFECT GRID PASS %u full original/native body comparisons with actual native SDK dependencies\n",total);fflush(stdout);
 if(!faults()||!controls()||!boundaries()||!refusals()||!integrations()||!semantics())return 1;
 puts("EFFECT GRID ALL PASS native original-CPU instructions=0; actual GTE/SDK; full 64-cell renderer");return 0;
}
