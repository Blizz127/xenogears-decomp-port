/* Complete original model unrelocator and relocation round trips.
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
static uint8_t code[224],relocate_code[212],shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned mode,records=3,flag=1,subkind=3,alias_kind,spmode,fault_at,accesses,expected_accesses,legacy_calls;
static int compare,observing;
static uint32_t model;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[65536];
static void fail(const char *why,uint32_t value)
{fprintf(stderr,"MODEL UNRELOCATE FAIL mode=%u records=%u flag=%u sub=%u alias=%u sp=%u fault=%u access=%u %s %08x\n",mode,records,flag,subkind,alias_kind,spmode,fault_at,accesses,why,value);exit(1);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&(address-0x8002c4bcu<224u||address-0x8002c3e8u<212u)){
  assert(!candidate&&width==4);*value=address>=0x8002c4bcu?load_le(code+(address-0x8002c4bcu),4):load_le(relocate_code+(address-0x8002c3e8u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("trace capacity",accesses);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 Access row={address,width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *p,uint32_t a,unsigned w,uint32_t *v){return observed_access(p,a,w,v,0);}
static int runtime_write(void *p,uint32_t a,unsigned w,uint32_t v){return observed_access(p,a,w,&v,1);}
static void put(uint32_t a,uint32_t v){if(model_test_raw_write(&rt,a,4,v))fail("fixture",a);}
static uint32_t peek(uint32_t a){uint32_t v;if(model_test_raw_read(&rt,a,4,&v))fail("fixture read",a);return v;}
static void snapshot(void)
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x8002c4bcu||target==0x8002c3e8u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x8002c3e8u<436u)return 0;
 fail("unexpected child",target);return -1;
}
static uint32_t base_address(void)
{switch(mode){case 1:return 0xa0100100u;case 2:return 0x80300100u;case 3:return (uint32_t)(uintptr_t)host+0x100u;case 4:return 0x1f800100u;case 5:return 0x80050100u;default:return 0x80100100u;}}
static void defaults(void){mode=alias_kind=spmode=fault_at=0;records=3;flag=1;subkind=3;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));memset(&gteRegs,0xc7,sizeof(gteRegs));
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"model-data"};rt.data_count=1;rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0xa9876500u+i;
 cpu->gpr[29]=spmode==0?0x801ff000u:spmode==1?0:spmode==2?0x1e000001u:0xffffffffu;
 cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;model=base_address();
 put(model,records);put(model+4u,0x87654320u|flag);
 unsigned n=(int32_t)records>0&&records<1024?records:0;
 for(unsigned i=0;i<n;i++){
  uint32_t row=model+0x10u+i*0x38u;
  put(row+8u,model+0xfffffff0u+i*4u);put(row+12u,model+0x80000001u+i*7u);put(row+16u,model+0x7ffffffcu-i*13u);put(row+20u,model+0x1234u+i*17u);
  uint32_t offset=(records>16?0x10000u:0x800u)+i*0x30u;put(row+28u,subkind?model+offset:0);
  if(subkind){uint32_t last=subkind==1?0xffffffffu:subkind-2u;put(model+offset,last);
   if(last!=0xffffffffu)for(unsigned j=0;j<=last;j++){put(model+offset+4u+j*12u,0xbad00000u+j);put(model+offset+8u+j*12u,model+0xfffffffcu-j);put(model+offset+12u+j*12u,model+0x80000000u+j);}
  }
 }
 if(alias_kind==1){put(model+0x18u,model-1u);put(model+0x2cu,model+0x18u);}
 if(alias_kind==2){put(model+0x24u,model);put(model+0x2cu,model+0x24u);}
 if(alias_kind==3)put(model+0x2cu,0x1e000000u);
 if(alias_kind==4){put(model+0x2cu,0x1f800004u);put(0x1f800004u,0xfffffffeu);}
 cpu->gpr[4]=alias_kind==5?model+2u:alias_kind==6?0x1e000000u:alias_kind==7?0:model;
 accesses=legacy_calls=oracle_gte_commands=native_gte_commands=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,0x8002c4bcu,BATTLE_HALT_PC,1000000);expected_accesses=accesses;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,0x8002c4bcu,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED)||(!fault_at&&alias_kind<3&&original!=PC_PORT_MIPS_HALTED))fail("status",(uint32_t)actual);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",0);
 if(native.steps||native_gte_commands||rt.bridge_cpu||accesses!=expected_accesses||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instruction/context/access count",accesses);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static int controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 XenoRetailFileSlot original=g_XenoRetailFiles[XENO_RD_SLUS];uint8_t *bytes=original.bytes+XENO_RD_SLUS_OFF(0x8002c4bcu);
 for(unsigned i=0;i<3;i++){unsigned offset=i==0?0:i==1?112:223;bytes[offset]^=1;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("full identity",offset);bytes[offset]^=1;}
 g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("missing image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;
 g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(0x8002c4bcu)+223u;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("short image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("pending",0);cpu=old;
 rt.force_interpret=1;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("forced",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("uninitialized",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("inactive",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("wrong context",0);rt.bridge_cpu=&cpu;
 if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4c0u))fail("wrong target",0);
 assert(setenv("XENO_BATTLE_NATIVE_MODEL_UNRELOCATE","0",1)==0);if(battle_model_unrelocate_try_host(&rt,&cpu,0x8002c4bcu))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_UNRELOCATE")==0);
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses)fail("gate side effect",0);
 puts("MODEL UNRELOCATE PASS complete private identity, absent/short image, context, pending and opt-out gates");return 1;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static int boundaries(void)
{
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;cpu.gpr[31]=0x800a94a0u;
  rt.functions[0]=(ResolvedFunction){0x8002c4bcu,legacy_probe,"unrelocate_legacy"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8002c4bcu);slot->target=0x8002c4bcu;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *bytes=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(0x8002c4bcu);if(reject==1)bytes[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_MODEL_UNRELOCATE","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,0x8002c4bcu):PcPortMipsRun(&cpu,0x8002c4bcu,cpu.gpr[31],100000);
  if(reject==1)bytes[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_UNRELOCATE")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("legacy dispatch",cached);
  if(!reject&&rc!=PC_PORT_MIPS_HALTED)fail("canonical precedence",(uint32_t)rc);
  if(reject&&((disabled?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected dispatch",(uint32_t)rc);
 }
 puts("MODEL UNRELOCATE PASS 12 cached/resolved native precedence and rejected-adoption checks");return 1;
}
static int round_trips(void)
{
 unsigned total=0;
 for(unsigned domain=0;domain<6;domain++)for(unsigned sub=0;sub<5;sub++){
  defaults();mode=domain;subkind=sub;PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;
  const uint32_t stages[]={0x8002c4bcu,0x8002c3e8u};
  for(unsigned stage=0;stage<2;stage++){oracle.gpr[4]=model;oracle.gpr[31]=BATTLE_HALT_PC;assert(PcPortMipsRun(&oracle,stages[stage],BATTLE_HALT_PC,100000)==PC_PORT_MIPS_HALTED);}
  expected_accesses=accesses;snapshot();setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  for(unsigned stage=0;stage<2;stage++){native.gpr[4]=model;native.gpr[31]=BATTLE_HALT_PC;if(PcPortMipsRun(&native,stages[stage],BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_HALTED)fail("round trip status",stage);}
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||memcmp(native.gpr,oracle.gpr,sizeof(native.gpr))||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||native.hi!=oracle.hi||native.lo!=oracle.lo||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("original/native round trip",domain);
  /* First four pointer words return exactly, including wrapped edge values. */
  if(peek(model+0x18u)!=model-16u||peek(model+0x1cu)!=model+0x80000001u||peek(model+0x20u)!=model+0x7ffffffcu||peek(model+0x24u)!=model+0x1234u||peek(model+4u)!=0x87654321u)fail("guest-domain round trip",domain);
  total++;
 }
 defaults();printf("MODEL UNRELOCATE PASS %u complete original/native unrelocate-relocate round trips across six domains\n",total);return 1;
}
static int faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<8;path++){
  defaults();mode=path%6;records=path==0?0:2;subkind=path%5;if(path==6||path==7){records=1;alias_kind=path-5;}
  run_case();unsigned n=expected_accesses;for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("MODEL UNRELOCATE PASS %u ordered faults and partial architectural state\n",total);return 1;
}
int main(int argc,char **argv)
{
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);memcpy(code,image->bytes+XENO_RD_SLUS_OFF(0x8002c4bcu),224);memcpy(relocate_code,image->bytes+XENO_RD_SLUS_OFF(0x8002c3e8u),212);defaults();
 if(argc>1){if(!strcmp(argv[1],"--basic"))return run_case()?0:1;if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--round-trips"))return round_trips()?0:1;return 2;}
 unsigned total=0;const unsigned counts[]={0,1,2,3,16};
 for(mode=0;mode<6;mode++)for(unsigned n=0;n<5;n++)for(flag=0;flag<2;flag++)for(subkind=0;subkind<5;subkind++){records=counts[n];run_case();total++;}
 const unsigned edges[]={0x7fffffffu,0x80000000u,0xffffffffu};
 for(unsigned i=0;i<3;i++)for(unsigned f=0;f<2;f++){if(i==0&&f)continue;defaults();records=edges[i];flag=f;run_case();total++;}
 for(unsigned domain=0;domain<6;domain++)if(domain!=4)for(unsigned count=0;count<2;count++){defaults();mode=domain;records=count?255:64;subkind=4;run_case();total++;}
 for(unsigned alias=1;alias<8;alias++){defaults();records=1;alias_kind=alias;run_case();total++;}
 for(unsigned sp=1;sp<4;sp++){defaults();spmode=sp;run_case();total++;}
 defaults();printf("MODEL UNRELOCATE PASS %u full original/native comparisons including signed counts, nested lists, aliases and large loops\n",total);fflush(stdout);
 if(!faults()||!controls()||!boundaries()||!round_trips())return 1;
 puts("MODEL UNRELOCATE ALL PASS zero native original-CPU instructions and graphics commands; no external children");return 0;
}

