/* Original word-input normalization entry, shared output tail and GTE kernel.
 * Private code is fetched only by the test oracle. The native leg has zero
 * guest text and disables the original-CPU interpreter. Both use actual GTE. */
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

static BattleMipsRuntime rt;
static uint8_t code[20],kernel[216],shared[0x1000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x2000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu *oracle_cpu,last_oracle,last_native;
static unsigned scenario,domain,alias_kind,bad_kind,table_kind,seed,fault_at;
static unsigned accesses,expected_accesses,events,expected_events,legacy_calls;
static int candidate,compare,observing;
static uint32_t input,output;
static const uint32_t vectors[][3]={
 {0,0,0},{1,0,0},{-1u,0,0},{0,1,0},{0,0,-1u},{3,4,0},
 {-3u,4,-12u},{4096,4096,4096},{32767,0,0},{0xffff8000u,0,0},
 {0xffff8000u,0xffff8000u,0},{30000,30000,30000},{32767,32767,0},
 {0x12340001u,0x8765ffffu,0x80000000u},{0x7fffffffu,0x80000000u,0xffffffffu},
 {7,7,7},{8,8,8},{15,16,17},{63,64,65},{127,128,129},{255,256,257},
 {511,512,513},{1023,1024,1025},{2047,2048,2049},{16383,16384,16385},
 {0xffff8000u,32767,255},{0xffff8000u,32767,256},{65536,131072,196608},
 {0xffffff80u,0xffffff00u,0xfffffe00u},{32767,32767,256},
 {0xffff8001u,0xffff8001u,0xffff8001u},{0x80008000u,0x7fff7fffu,0}
};
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
typedef struct GteEvent {uint32_t command,gpr[32],hi,lo;GTERegisters before,after;} GteEvent;
static Access trace[16];static GteEvent gte_trace[4];
static void fail(const char *why,uint32_t value)
{fprintf(stderr,"VECTOR NORMAL S FAIL scenario=%u domain=%u alias=%u bad=%u table=%u seed=%u fault=%u access=%u event=%u %s %08x\n",scenario,domain,alias_kind,bad_kind,table_kind,seed,fault_at,accesses,events,why,value);exit(1);}
extern int color_fade_backend_command(int);
int doCOP2(int op)
{
 if(!observing)return color_fade_backend_command(op);
 if(events>=4)fail("GTE trace capacity",events);
 PcPortMipsCpu *cpu=candidate?rt.bridge_cpu:oracle_cpu;if(!cpu)fail("GTE CPU context",0);
 GteEvent row;memset(&row,0,sizeof(row));row.command=(uint32_t)op;
 memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));row.hi=cpu->hi;row.lo=cpu->lo;row.before=gteRegs;
 int rc=color_fade_backend_command(op);row.after=gteRegs;
 if(compare){
  if(events>=expected_events||memcmp(&row,&gte_trace[events],sizeof(row))){
   if(events<expected_events)for(unsigned i=0;i<32;i++)if(row.gpr[i]!=gte_trace[events].gpr[i])fprintf(stderr,"GTE boundary GPR %u native=%08x original=%08x\n",i,row.gpr[i],gte_trace[events].gpr[i]);
   fail("GTE command/state sequence",(uint32_t)op);
  }
 }else gte_trace[events]=row;
 events++;return rc;
}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 if(observing&&!write&&(address-0x80048d68u<20u||address-0x80048dbcu<216u)){
  assert(!candidate&&width==4);*value=load_le(address<0x80048dbcu?code+(address-0x80048d68u):kernel+(address-0x80048dbcu),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=16)fail("access trace capacity",accesses);
 /* Only the original CPU bus gets implicit physical-RAM translation.
  * The native entry must perform its own declared-pointer conversion. */
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
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x80048d68u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x80048d68u<20u||target-0x80048dbcu<216u)return 0;
 fail("unexpected child",target);return -1;
}
static uint32_t token(unsigned offset)
{switch(domain){case 1:return 0xa0110000u+offset;case 2:return 0x80310000u+offset;case 3:return 0x110000u+offset;case 4:return 0x80060000u+offset;case 5:return (uint32_t)(uintptr_t)host+offset;case 6:return 0x1f800000u+offset;default:return 0x80110000u+offset;}}
static void defaults(void){scenario=domain=alias_kind=bad_kind=table_kind=seed=fault_at=0;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 /* Exercise the actual production static-data loader; do not fabricate a
  * fixture-only owner or copy original executable text into native RAM. */
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 if(PsxMemory_LoadStaticDataFromImage(image->bytes,image->size))fail("static data loader",0);
 for(uint32_t a=PSX_EXE_TEXT_START;a<PSX_EXE_TEXT_END;a++)if(*(uint8_t *)PSX_ADDR(a))fail("guest text loaded",a);
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"vector-test-shared"};rt.data_count=1;rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+i;
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0xa9876500u+i;gteRegs.CP2C.r[i]=0xabcd0000u+i*7123u+seed;gteRegs.CP2D.r[i]=0x56780000u+i*1717u+seed;}
 cpu->gpr[29]=0x801ff000u;cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 input=token(0x100u);output=token(0x200u);
 if(alias_kind==1)output=input;
 if(alias_kind==2)output=input+2u;
 if(alias_kind==3)output=input+8u;
 if(alias_kind==4)input=0x80056b14u;
 if(alias_kind==5)output=0x80056b14u;
 if(alias_kind==6)output=0x80056b94u;
 for(unsigned i=0;i<3;i++)put(input+i*4u,4,scenario<32?vectors[scenario][i]:(seed*0x19660du+i*0x3c6ef35fu+0x101390423u));
 if(table_kind){uint32_t v=table_kind==1?0:table_kind==2?0x7fff:table_kind==3?0x8000:0xffff;for(uint32_t a=0x80056b14u;a<0x80056d14u;a+=2u)put(a,2,v);}
 if(bad_kind==1)input+=1u;
 if(bad_kind==2)output+=1u;
 if(bad_kind==3)input=0x1e000000u;
 if(bad_kind==4)output=0x1e000000u;
 if(bad_kind==5){input=(uint32_t)(uintptr_t)host+0x1ff8u;put(input,4,1);put(input+4u,4,2);}
 if(bad_kind==6)output=(uint32_t)(uintptr_t)host+0x1ffcu;
 if(bad_kind==7){output=(uint32_t)(uintptr_t)host+0x200u;rt.host_ranges[0].writable=0;}
 if(bad_kind==8){input=(uint32_t)(uintptr_t)host+0x100u;rt.host_ranges[0].writable=0;}
 if(bad_kind==9)input=0x1f800ff8u;
 if(bad_kind==10)output=0x1f800ffcu;
 cpu->gpr[4]=input;cpu->gpr[5]=output;
 accesses=events=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 int original=PcPortMipsRun(&oracle,0x80048d68u,BATTLE_HALT_PC,1000);
 expected_accesses=accesses;expected_events=events;snapshot();setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int actual=PcPortMipsRun(&native,0x80048d68u,BATTLE_HALT_PC,1000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)actual);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x original=%08x pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",0);
 if(original==PC_PORT_MIPS_HALTED&&(native.load_valid!=oracle.load_valid||(oracle.load_valid&&(native.load_reg!=oracle.load_reg||native.load_value!=oracle.load_value||native.load_mask!=oracle.load_mask))))fail("terminal pending load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||events!=expected_events||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instruction/context/event count",accesses);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 last_oracle=oracle;last_native=native;return original;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned s=0;s<32;s++)for(unsigned a=0;a<7;a++){
  defaults();scenario=s;alias_kind=a;run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("VECTOR NORMAL S PASS %u ordered memory faults with GTE and partial architectural state\n",total);return total;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 unsigned total=0;XenoRetailFileSlot original=g_XenoRetailFiles[XENO_RD_SLUS];
 const uint32_t starts[]={0x80048d68u,0x80048dbcu,0x80048dd8u};const unsigned lengths[]={20,28,188};
 for(unsigned span=0;span<3;span++){
  uint8_t *bytes=original.bytes+XENO_RD_SLUS_OFF(starts[span]);
  for(unsigned i=0;i<3;i++){unsigned offset=i==0?0:i==1?lengths[span]/2:lengths[span]-1;bytes[offset]^=1;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("complete identity",offset);bytes[offset]^=1;total++;}
  g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(starts[span])+lengths[span]-1u;
  if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("short image",span);
  g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
 }
 g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("missing image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("forced",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("uninitialized",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048dd8u))fail("internal entry",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_VECTOR_NORMAL_S","0",1)==0);if(battle_vector_normal_s_try_host(&rt,&cpu,0x80048d68u))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_VECTOR_NORMAL_S")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||events)fail("gate side effect",0);
 defaults();printf("VECTOR NORMAL S PASS %u identity/context/pending/opt-out gates\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();scenario=6;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){0x80048d68u,legacy_probe,"VectorNormalS"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x80048d68u);slot->target=0x80048d68u;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *bytes=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(0x80048d68u);if(reject==1)bytes[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_VECTOR_NORMAL_S","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,0x80048d68u):PcPortMipsRun(&cpu,0x80048d68u,cpu.gpr[31],1000);
  if(reject==1)bytes[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_VECTOR_NORMAL_S")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption",cached);
  if(!reject&&(rc!=PC_PORT_MIPS_HALTED||cpu.load_valid))fail("canonical precedence",(uint32_t)rc);
  if(reject&&((disabled?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected adoption",(uint32_t)rc);
  total++;
 }
 defaults();printf("VECTOR NORMAL S PASS %u cached/resolved adoption and refusal boundaries\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 for(unsigned s=0;s<32;s++){
  defaults();scenario=s;int rc=run_case();
  int64_t x=(int16_t)vectors[s][0],y=(int16_t)vectors[s][1],z=(int16_t)vectors[s][2];
  int overflow=x*x+y*y+z*z>INT32_MAX;
  if(overflow){
   uint32_t site=x*x+y*y>INT32_MAX?0x80048df8u:0x80048dfcu;
   char message[64];snprintf(message,sizeof(message),"add overflow at pc=0x%08x",site);
   if(rc!=PC_PORT_MIPS_FAULT||events!=1||accesses!=3||!strstr(last_oracle.error,message))fail("signed ADD overflow",site);
   if(last_native.gpr[13]!=(site==0x80048df8u?0x1234000du:(uint32_t)(z*z)))fail("pending third MAC at overflow",site);
  }else{
   if(rc!=PC_PORT_MIPS_HALTED||events!=2||accesses!=7||last_native.gpr[2]!=(uint32_t)(x*x+y*y+z*z)||last_native.load_valid)fail("normalization contract",s);
   if(gte_trace[0].command!=0x4aa00428u||gte_trace[1].command!=0x4b90003du)fail("SQR/GPF order",s);
   if(trace[3].width!=2||trace[3].write)fail("lookup is signed halfword input",s);
   if(!x&&!y&&!z){
    if(trace[3].address!=0x80056b14u||trace[3].value!=0x1c6cu||last_native.gpr[14]!=0xffffffffu)fail("zero vector original table read and shift",s);
    if(peek(output,2)||peek(output+2u,2)||peek(output+4u,2))fail("zero vector output",s);
   }
   if((s==1||s==2||s==3||s==4)&&peek(output+(s==3?2u:s==4?4u:0u),2)!=(uint16_t)((s==2||s==4)?-4096:4096))fail("axis unit vector",s);
   if(last_native.gpr[31]!=BATTLE_HALT_PC||last_native.gpr[7]!=BATTLE_HALT_PC)fail("shared-tail return",s);
  }
  total++;
 }
 defaults();printf("VECTOR NORMAL S PASS %u explicit zero/axis/overflow/GTE/return witnesses\n",total);return total;
}
int main(int argc,char **argv)
{
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 memcpy(code,image->bytes+XENO_RD_SLUS_OFF(0x80048d68u),20);memcpy(kernel,image->bytes+XENO_RD_SLUS_OFF(0x80048dbcu),216);defaults();
 if(argc>1){if(!strcmp(argv[1],"--basic")){run_case();return 0;}if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;return 2;}
 unsigned total=0;
 for(unsigned s=0;s<32;s++)for(unsigned d=0;d<7;d++){defaults();scenario=s;domain=d;run_case();total++;}
 for(unsigned s=0;s<32;s++)for(unsigned a=1;a<7;a++){defaults();scenario=s;alias_kind=a;run_case();total++;}
 for(unsigned s=0;s<32;s++)for(unsigned t=1;t<5;t++){defaults();scenario=s;table_kind=t;run_case();total++;}
 for(unsigned b=1;b<11;b++){defaults();scenario=6;bad_kind=b;run_case();total++;}
 for(unsigned i=0;i<256;i++){defaults();scenario=32;seed=i;run_case();total++;}
 defaults();printf("VECTOR NORMAL S PASS %u full original/native comparisons, actual private-data loader and GTE\n",total);fflush(stdout);
 if(!faults()||!controls()||!boundaries()||!semantics())return 1;
 puts("VECTOR NORMAL S ALL PASS native original-CPU instructions=0; actual graphics backend; no terminal pending load");return 0;
}
