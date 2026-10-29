/* Original complete RotMatrixYXZ body with ordered trig-table and matrix aliases.
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
static uint8_t code[652],shared[0x1000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x2000],want_host[sizeof(host)];
static GTERegisters want_gte;
static PcPortMipsCpu *oracle_cpu,last_oracle,last_native;
static unsigned scenario,domain,alias_kind,bad_kind,table_kind,seed,fault_at;
static unsigned accesses,expected_accesses,events,expected_events,legacy_calls;
static int candidate,compare,observing;
static uint32_t input,output;
static const uint32_t vectors[][3]={
 {0,0,0},{1024,0,0},{-1024u,0,0},{0,1024,0},{0,-1024u,0},
 {0,0,1024},{0,0,-1024u},{1024,1024,1024},{-1u,-1u,-1u},{1,1,1},
 {0xffff8000u,0xffff8000u,0xffff8000u},{32767,32767,32767},
 {4096,8192,12288},{4095,8191,12287},{-4095u,-8191u,-12287u},
 {37,197,811},{-37u,197,-811u},{37,-197u,811},{-37u,-197u,-811u},
 {511,512,513},{1023,1024,1025},{2047,2048,2049},{3071,3072,3073},
 {-511u,-512u,-513u},{-1023u,-1024u,-1025u},{-2047u,-2048u,-2049u},
 {16383,16384,16385},{32765,32766,32767},{0x8000,0x8001,0x8002},
 {1024,0,-1024u},{-1024u,0,1024},{0,1024,-1024u}
};
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
typedef struct GteEvent {uint32_t command,gpr[32],hi,lo;GTERegisters before,after;} GteEvent;
static Access trace[32];static GteEvent gte_trace[4];
static void fail(const char *why,uint32_t value)
{fprintf(stderr,"ROT MATRIX YXZ FAIL scenario=%u domain=%u alias=%u bad=%u table=%u seed=%u fault=%u access=%u event=%u %s %08x\n",scenario,domain,alias_kind,bad_kind,table_kind,seed,fault_at,accesses,events,why,value);exit(1);}
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
 if(observing&&!write&&address-0x8004a92cu<652u){
  assert(!candidate&&width==4);*value=load_le(code+(address-0x8004a92cu),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=32)fail("access trace capacity",accesses);
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
 if(target==0x8004a92cu)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x8004a92cu<652u)return 0;
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
 rt.data[0]=(ResolvedData){0x80060000u,sizeof(shared),shared,"matrix-test-shared"};rt.data_count=1;rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+i;
 for(unsigned i=0;i<32;i++){cpu->cp0[i]=0xa9876500u+i;gteRegs.CP2C.r[i]=0xabcd0000u+i*7123u+seed;gteRegs.CP2D.r[i]=0x56780000u+i*1717u+seed;}
 cpu->gpr[29]=0x801ff000u;cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 input=token(0x100u);output=token(0x200u);
 if(alias_kind==1)output=input;
 if(alias_kind==2)output=input+2u;
 if(alias_kind==3)output=input-10u;
 if(alias_kind==4)input=0x800523f0u;
 for(unsigned i=0;i<3;i++)put(input+i*2u,2,scenario<32?vectors[scenario][i]:(seed*0x19660du+i*0x3c6ef35fu+0x1390423u));
 if(alias_kind==5||alias_kind==6){
  int z=(int16_t)peek(input+4u,2);unsigned index=(unsigned)(z<0?-z:z)&0xfffu;
  output=0x800523f0u+index*4u-(alias_kind==5?10u:16u);
 }
 if(table_kind){uint32_t v=table_kind==1?0:table_kind==2?0x7fff8000u:table_kind==3?0x80007fffu:0xffffffffu;for(uint32_t a=0x800523f0u;a<0x800563f0u;a+=4u)put(a,4,v);}
 if(bad_kind==1)input+=1u;
 if(bad_kind==2)output+=1u;
 if(bad_kind==3)input=0x1e000000u;
 if(bad_kind==4)output=0x1e000000u;
 if(bad_kind==5){input=(uint32_t)(uintptr_t)host+0x1ffcu;put(input,2,1);put(input+2u,2,2);}
 if(bad_kind==6)output=(uint32_t)(uintptr_t)host+0x1ff4u;
 if(bad_kind==7){output=(uint32_t)(uintptr_t)host+0x200u;rt.host_ranges[0].writable=0;}
 if(bad_kind==8){input=(uint32_t)(uintptr_t)host+0x100u;rt.host_ranges[0].writable=0;}
 if(bad_kind==9)input=0x1f800ffcu;
 if(bad_kind==10)output=0x1f800ff4u;
 cpu->gpr[4]=input;cpu->gpr[5]=output;
 accesses=events=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 int original=PcPortMipsRun(&oracle,0x8004a92cu,BATTLE_HALT_PC,1000);
 expected_accesses=accesses;expected_events=events;snapshot();setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int actual=PcPortMipsRun(&native,0x8004a92cu,BATTLE_HALT_PC,1000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
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
 defaults();printf("ROT MATRIX YXZ PASS %u ordered memory faults with GTE and partial architectural state\n",total);return total;
}
static unsigned controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 unsigned total=0;XenoRetailFileSlot original=g_XenoRetailFiles[XENO_RD_SLUS];
 const uint32_t starts[]={0x8004a92cu};const unsigned lengths[]={652};
 for(unsigned span=0;span<1;span++){
  uint8_t *bytes=original.bytes+XENO_RD_SLUS_OFF(starts[span]);
  for(unsigned i=0;i<3;i++){unsigned offset=i==0?0:i==1?lengths[span]/2:lengths[span]-1;bytes[offset]^=1;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("complete identity",offset);bytes[offset]^=1;total++;}
  g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(starts[span])+lengths[span]-1u;
  if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("short image",span);
  g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
 }
 g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("missing image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("forced",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("uninitialized",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a930u))fail("internal entry",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_ROT_MATRIX_YXZ","0",1)==0);if(battle_rot_matrix_yxz_try_host(&rt,&cpu,0x8004a92cu))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_ROT_MATRIX_YXZ")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||events)fail("gate side effect",0);
 defaults();printf("ROT MATRIX YXZ PASS %u identity/context/pending/opt-out gates\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();scenario=6;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){0x8004a92cu,legacy_probe,"RotMatrixYXZ"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x8004a92cu);slot->target=0x8004a92cu;slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *bytes=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(0x8004a92cu);if(reject==1)bytes[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_ROT_MATRIX_YXZ","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,0x8004a92cu):PcPortMipsRun(&cpu,0x8004a92cu,cpu.gpr[31],1000);
  if(reject==1)bytes[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_ROT_MATRIX_YXZ")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption",cached);
  if(!reject&&(rc!=PC_PORT_MIPS_HALTED||cpu.load_valid))fail("canonical precedence",(uint32_t)rc);
  if(reject&&((disabled?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected adoption",(uint32_t)rc);
  total++;
 }
 defaults();printf("ROT MATRIX YXZ PASS %u cached/resolved adoption and refusal boundaries\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;
 const int16_t axis[7][9]={
  {4096,0,0,0,4096,0,0,0,4096},
  {4096,0,0,0,0,-4096,0,4096,0},
  {4096,0,0,0,0,4096,0,-4096,0},
  {0,0,4096,0,4096,0,-4096,0,0},
  {0,0,-4096,0,4096,0,4096,0,0},
  {0,-4096,0,4096,0,0,0,0,4096},
  {0,4096,0,-4096,0,0,0,0,4096}
 };
 for(unsigned s=0;s<32;s++){
  defaults();scenario=s;int rc=run_case();
  if(rc!=PC_PORT_MIPS_HALTED||events||accesses!=15||last_native.gpr[2]!=output||last_native.load_valid)fail("matrix completion contract",s);
  const unsigned writes[]={10,4,16,6,8,0,2,14,12};unsigned index=0;
  for(unsigned a=0;a<accesses;a++)if(trace[a].write){
   if(index>=9||trace[a].width!=2||trace[a].address!=output+writes[index])fail("original halfword write order",index);
   index++;
  }
  if(index!=9||trace[4].address!=input+4u||trace[4].write||trace[8].write||trace[8].width!=4)fail("third trig lookup follows three stores",s);
  int z=(int16_t)vectors[s][2];unsigned zindex=(unsigned)(z<0?-z:z)&0xfffu;
  if(trace[8].address!=0x800523f0u+zindex*4u)fail("signed angle lookup",s);
  if(s<7)for(unsigned i=0;i<9;i++)if(peek(output+i*2u,2)!=(uint16_t)axis[s][i])fail("axis rotation witness",s*9u+i);
  if(peek(output+18u,2)!=0xa5a5u||peek(output+20u,4)!=0xa5a5a5a5u||peek(output+24u,4)!=0xa5a5a5a5u||peek(output+28u,4)!=0xa5a5a5a5u)fail("matrix pad and translation preserved",s);
  for(unsigned i=0;i<32;i++)if(gteRegs.CP2C.r[i]!=0xabcd0000u+i*7123u||gteRegs.CP2D.r[i]!=0x56780000u+i*1717u)fail("GTE unchanged",i);
  total++;
 }
 defaults();printf("ROT MATRIX YXZ PASS %u explicit axis/ordered-store/lookup/GTE/return witnesses\n",total);return total;
}
int main(int argc,char **argv)
{
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 memcpy(code,image->bytes+XENO_RD_SLUS_OFF(0x8004a92cu),652);defaults();
 if(argc>1){if(!strcmp(argv[1],"--basic")){run_case();return 0;}if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;return 2;}
 unsigned total=0;
 for(unsigned s=0;s<32;s++)for(unsigned d=0;d<7;d++){defaults();scenario=s;domain=d;run_case();total++;}
 for(unsigned s=0;s<32;s++)for(unsigned a=1;a<7;a++){defaults();scenario=s;alias_kind=a;run_case();total++;}
 for(unsigned s=0;s<32;s++)for(unsigned t=1;t<5;t++){defaults();scenario=s;table_kind=t;run_case();total++;}
 for(unsigned b=1;b<11;b++){defaults();scenario=6;bad_kind=b;run_case();total++;}
 for(unsigned i=0;i<256;i++){defaults();scenario=32;seed=i;run_case();total++;}
 defaults();printf("ROT MATRIX YXZ PASS %u full original/native comparisons, actual private-data loader and GTE\n",total);fflush(stdout);
 if(!faults()||!controls()||!boundaries()||!semantics())return 1;
 puts("ROT MATRIX YXZ ALL PASS native original-CPU instructions=0; actual graphics backend; no terminal pending load");return 0;
}
