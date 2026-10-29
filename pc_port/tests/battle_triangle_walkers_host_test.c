/* Complete original triangle entries and real packet-caller integrations.
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
int32_t D_800500F8,D_800500FC,D_80050100,D_80059578;
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
static uint8_t code[540],caller_code[460],shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static GTERegisters want_gte;
static const uint32_t targets[]={0x8002e010u,0x8002e024u,0x8002e038u,0x8002e04cu};
static unsigned kind,count=3,scenario,domain,stack_kind,alias_kind,fault_at,accesses,expected_accesses;
static unsigned events,expected_events,caller,legacy_calls;
static int candidate,compare,observing;
static PcPortMipsCpu *oracle_cpu;
static uint32_t commands,vertices,output,ot,model;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
typedef struct GteEvent {uint32_t command,gpr[32],hi,lo;GTERegisters before,after;} GteEvent;
static Access trace[65536];
static GteEvent gte_trace[4096];
static void fail(const char *why,uint32_t value)
{fprintf(stderr,"TRIANGLE WALKERS FAIL kind=%u count=%u scenario=%u domain=%u stack=%u alias=%u caller=%u fault=%u access=%u event=%u %s %08x\n",kind,count,scenario,domain,stack_kind,alias_kind,caller,fault_at,accesses,events,why,value);exit(1);}
extern int color_fade_backend_command(int);
int doCOP2(int op)
{
 if(!observing)return color_fade_backend_command(op);
 if(events>=sizeof(gte_trace)/sizeof(gte_trace[0]))fail("GTE trace capacity",events);
 PcPortMipsCpu *cpu=candidate?rt.bridge_cpu:oracle_cpu;if(!cpu)fail("GTE CPU context",0);
 GteEvent row;memset(&row,0,sizeof(row));row.command=(uint32_t)op;memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));row.hi=cpu->hi;row.lo=cpu->lo;row.before=gteRegs;
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
 if(observing&&!write&&(address-0x8002e010u<540u||address-0x8002c700u<460u)){
  assert(!candidate&&width==4);*value=address>=0x8002e010u?load_le(code+(address-0x8002e010u),4):load_le(caller_code+(address-0x8002c700u),4);return 0;
 }
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 if(accesses>=sizeof(trace)/sizeof(trace[0]))fail("access trace capacity",accesses);
 /* The original CPU bus has physical RAM. The port's generic mixed-domain
  * resolver requires an explicit translation from each adopted service.
  * Only the oracle gets that bus mapping here: omitting production's own
  * translation must still fail the native leg. Trace equivalent RAM aliases. */
 uint32_t mapped=candidate?address:shard_sdk_address(address);
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,mapped,width,*value):model_test_raw_read(opaque,mapped,width,value);
 Access row={shard_sdk_address(address),width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *p,uint32_t a,unsigned w,uint32_t *v){return observed_access(p,a,w,v,0);}
static int runtime_write(void *p,uint32_t a,unsigned w,uint32_t v){return observed_access(p,a,w,&v,1);}
static void put(uint32_t a,unsigned w,uint32_t v){if(model_test_raw_write(&rt,shard_sdk_address(a),w,v))fail("fixture write",a);}
static uint32_t peek(uint32_t a){uint32_t v;if(model_test_raw_read(&rt,shard_sdk_address(a),4,&v))fail("fixture read",a);return v;}
static void snapshot(void)
{want_gte=gteRegs;memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(&want_gte,&gteRegs,sizeof(gteRegs))&&!memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(battle_triangle_walkers_target(target)||target==0x8002c700u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x8002e010u<540u||target-0x8002c700u<460u)return 0;
 fail("unexpected child",target);return -1;
}
static uint32_t token(uint32_t address,unsigned shared_offset,unsigned host_offset,unsigned pad_offset)
{switch(domain){case 1:return address|0x20000000u;case 2:return address+0x200000u;case 3:return address&0x1fffffu;case 4:return 0x80050000u+shared_offset;case 5:return (uint32_t)(uintptr_t)host+host_offset;case 6:return 0x1f800000u+pad_offset;default:return address;}}
static void defaults(void){kind=scenario=domain=stack_kind=alias_kind=fault_at=caller=0;count=3;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));memset(&gteRegs,0,sizeof(gteRegs));
 memset(PSX_ADDR(PSX_EXE_TEXT_START),0,PSX_EXE_TEXT_END-PSX_EXE_TEXT_START);
 rt.data[0]=(ResolvedData){0x80050000u,0x1f000u,shared,"triangle-data"};rt.data_count=1;rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x12340000u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0xa9876500u+i;
 cpu->gpr[29]=0x801ff000u;cpu->gpr[31]=BATTLE_HALT_PC;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 commands=token(0x80110000u,0x2000u,0x2000u,0x400u);vertices=token(0x80120000u,0x1000u,0x1000u,0x100u);output=0x80160000u;ot=0x80170000u;model=0x80108000u;
 if(stack_kind==1)cpu->gpr[29]=0xa01ff000u;
 if(stack_kind==2)cpu->gpr[29]=0x1f800fc0u;
 if(stack_kind==3)cpu->gpr[29]=(uint32_t)(uintptr_t)host+0x10000u;
 if(stack_kind==4)cpu->gpr[29]=0x8006e000u;
 if(stack_kind==5)cpu->gpr[29]=0;
 if(stack_kind==6)cpu->gpr[29]=0x1e000000u;
 if(stack_kind==7)cpu->gpr[29]=0x801ff001u;
 if(alias_kind==1)cpu->gpr[29]=output+(caller?0x54u:0x24u);
 if(alias_kind==2)ot=output;
 if(alias_kind==3)output=vertices;
 if(alias_kind==4)output=commands;
 if(alias_kind==5)cpu->gpr[29]=commands+0x24u+(caller?0x30u:0u);
 if(alias_kind==6)cpu->gpr[29]=0x800595a0u+(caller?0x30u:0u);
 if(alias_kind==7)ot=0x80160008u;
 unsigned n=count<300?count:8;
 for(unsigned i=0;i<=n+1;i++){
  uint32_t first=scenario==1?1:0,second=scenario==1?0:1;
  if(scenario==8)second|=0xe000u;
  put(commands+i*8u,4,first|(second<<16));put(commands+i*8u+4u,2,2);put(commands+i*8u+6u,2,0xd00du+i);
 }
 const int xy[3][2]={{-40,-30},{40,-30},{0,40}};
 for(unsigned i=0;i<3;i++){
  int x=xy[i][0],y=xy[i][1],z=scenario==7?-20:512;
  if(scenario==2)x+=2000;
  if(scenario==3)y+=2000;
  if(scenario==6)x=y=0;
  put(vertices+i*8u,4,(uint16_t)x|((uint32_t)(uint16_t)y<<16));put(vertices+i*8u+4u,4,0xabcd0000u|(uint16_t)z);
 }
 for(unsigned i=0;i<4096;i++)put(0x80170000u+i*4u,4,0x00180000u+i*4u);
 put(0x8005953cu,4,vertices);put(0x80059424u,4,output);put(0x80059568u,4,ot);put(0x80059578u,4,0x7fffffffu);
 put(0x800500f8u,4,scenario==9?0xffffffffu:319u);put(0x800500fcu,4,scenario==9?0xffffffffu:0x00ef0000u);
 put(0x80050100u,4,scenario==10?31u:scenario==11?32u:2u);put(0x80050104u,4,0);
 gteRegs.CP2C.r[0]=gteRegs.CP2C.r[2]=gteRegs.CP2C.r[4]=4096;
 gteRegs.CP2C.r[24]=160u<<16;gteRegs.CP2C.r[25]=120u<<16;gteRegs.CP2C.r[26]=256;
 gteRegs.CP2C.r[29]=scenario==4?0:1365;gteRegs.CP2C.r[30]=1024;
 gteRegs.CP2D.r[31]=scenario==5?0x80000000u:7;
 cpu->gpr[4]=commands;cpu->gpr[5]=count;
 if(caller){
  const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
  memcpy(PSX_ADDR(0x80018980u),image->bytes+XENO_RD_SLUS_OFF(0x80018980u),24);
  put(model+4u,2,3);put(model+6u,2,1);put(model+8u,4,vertices);put(model+0xcu,4,0x80140000u);put(model+0x10u,4,commands-4u);put(model+0x18u,4,0x80150000u);
  put(commands-4u,1,0);put(commands-2u,2,count);put(0x8004fe50u,4,targets[kind]);put(0x8004fe6cu,4,8);
  cpu->gpr[4]=model;cpu->gpr[5]=output;cpu->gpr[6]=ot;cpu->gpr[7]=0;
 }
 accesses=events=legacy_calls=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;oracle_cpu=&oracle;
 uint32_t target=caller?0x8002c700u:targets[kind];int original=PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,1000000);
 expected_accesses=accesses;expected_events=events;snapshot();setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int actual=PcPortMipsRun(&native,target,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET)fail("oracle budget",oracle.pc);
 if(!fault_at&&!alias_kind&&stack_kind<5&&original!=PC_PORT_MIPS_HALTED)fail("nominal case did not complete",oracle.pc);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("status",(uint32_t)actual);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fprintf(stderr,"native=%08x oracle=%08x pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);fail("register",i);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo||memcmp(native.cp0,oracle.cp0,sizeof(native.cp0))||!memory_equal())fail("full final state",0);
 if(original==PC_PORT_MIPS_HALTED&&(native.load_valid!=oracle.load_valid||(oracle.load_valid&&(native.load_reg!=oracle.load_reg||native.load_value!=oracle.load_value||native.load_mask!=oracle.load_mask))))fail("terminal pending load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||events!=expected_events||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instruction/context/event count",accesses);
 if(fault_at&&accesses!=fault_at)fail("fault not reached",accesses);
 return 1;
}
static unsigned faults(void)
{
 unsigned total=0;
 for(unsigned path=0;path<10;path++){
  defaults();kind=path%4;scenario=path%8;count=path==0?0:2;caller=path>=7;if(path==9){scenario=0;alias_kind=1;count=1;}
  run_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){run_case();total++;}
 }
 defaults();printf("TRIANGLE WALKERS PASS %u ordered memory faults with GTE and partial architectural state\n",total);return total;
}
static unsigned integration(void)
{
 unsigned total=0;
 for(unsigned k=0;k<4;k++)for(unsigned s=0;s<12;s++)for(unsigned n=0;n<3;n++){
  defaults();caller=1;kind=k;scenario=s;count=n;run_case();total++;
 }
 for(unsigned k=0;k<4;k++)for(unsigned alias=1;alias<=7;alias++){
  defaults();caller=1;kind=k;count=1;alias_kind=alias;run_case();total++;
 }
 defaults();printf("TRIANGLE WALKERS PASS %u real packet-caller integrations including live return-load aliases\n",total);return total;
}
static unsigned controls(void)
{
 unsigned total=0;
 for(unsigned k=0;k<4;k++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
  XenoRetailFileSlot original=g_XenoRetailFiles[XENO_RD_SLUS];uint8_t *bytes=original.bytes+XENO_RD_SLUS_OFF(0x8002e010u);
  for(unsigned i=0;i<3;i++){unsigned offset=i==0?0:i==1?270:539;bytes[offset]^=1;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("complete identity",offset);bytes[offset]^=1;total++;}
  g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("missing image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
  g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(0x8002e010u)+539u;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("short image",0);g_XenoRetailFiles[XENO_RD_SLUS]=original;total++;
  cpu.load_valid=1;cpu.load_reg=4;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("pending entry",0);cpu=old;total++;
  rt.force_interpret=1;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("forced",0);rt.force_interpret=0;total++;
  rt.initialized=0;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("uninitialized",0);rt.initialized=1;total++;
  g_ActiveBattleRuntime=NULL;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("inactive",0);g_ActiveBattleRuntime=&rt;total++;
  rt.bridge_cpu=NULL;if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("context",0);rt.bridge_cpu=&cpu;total++;
  if(battle_triangle_walkers_try_host(&rt,&cpu,0x8002e058u))fail("internal entry",0);
  total++;
  assert(setenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS","0",1)==0);if(battle_triangle_walkers_try_host(&rt,&cpu,targets[k]))fail("opt-out",0);assert(unsetenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS")==0);total++;
  if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal()||accesses||events)fail("gate side effect",0);
 }
 defaults();printf("TRIANGLE WALKERS PASS %u identity/context/pending/opt-out gates\n",total);return total;
}
static uintptr_t legacy_probe(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;legacy_calls++;return 0;}
static unsigned boundaries(void)
{
 unsigned total=0;
 for(unsigned k=0;k<4;k++)for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();kind=k;PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;cpu.gpr[31]=0x8002c86cu;
  rt.functions[0]=(ResolvedFunction){targets[k],legacy_probe,"triangle-legacy-probe"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,targets[k]);slot->target=targets[k];slot->state=1;slot->entry=rt.functions[0];}
  uint8_t *bytes=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(0x8002e010u);if(reject==1)bytes[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,targets[k]):PcPortMipsRun(&cpu,targets[k],cpu.gpr[31],100000);
  if(reject==1)bytes[0]^=1;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);assert(unsetenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS")==0);
  if(legacy_calls||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("legacy adoption",k);
  if(!reject&&(rc!=PC_PORT_MIPS_HALTED||cpu.load_valid!=1||cpu.load_reg!=23||cpu.load_mask!=0))fail("canonical precedence",(uint32_t)rc);
  if(reject&&((disabled?rc>=0:rc!=0)||memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr))||!memory_equal()))fail("rejected adoption",(uint32_t)rc);
  total++;
 }
 defaults();printf("TRIANGLE WALKERS PASS %u cached/resolved native adoption and refusal boundaries\n",total);return total;
}
static unsigned semantics(void)
{
 unsigned total=0;const unsigned scenarios[]={0,0,0,1,2,3,4,5,6,8};
 const unsigned counts[]={0,1,17,2,2,2,2,2,2,2};
 const unsigned commands_per_triangle[]={3,3,3,3,2,2,3,1,3,3};
 const unsigned strides[]={40,28,20,32};const uint32_t tags[]={0x09000000u,0x06000000u,0x04000000u,0x07000000u};
 for(unsigned k=0;k<4;k++)for(unsigned path=0;path<10;path++){
  defaults();kind=k;scenario=scenarios[path];count=counts[path];run_case();
  if(events!=1+commands_per_triangle[path]*count)fail("required transform/cull branch",events);
  int emitted=scenario==0||scenario==4||scenario==8;
  if(peek(0x80059578u)!=0x7fffffffu+(emitted?count:0u))fail("emitted count/zero depth",scenario);
  uint32_t expected=output+strides[k]*count;
  if(count&&(emitted||scenario==1||scenario==6))expected&=0xffffffu;
  if(peek(0x80059424u)!=expected)fail("physical cursor/mask branch",scenario);
  if(count&&emitted&&scenario!=4){
   unsigned depth=MFC2(7)>>2;uint32_t last=(output&0xffffffu)+(count-1u)*strides[k];
   if(peek(ot+depth*4u)!=last)fail("required OT link",last);
   if(peek(output)!=(tags[k]|(0x00180000u+depth*4u)))fail("first packet tag",peek(output));
   for(unsigned i=1;i<count;i++)if(peek(output+i*strides[k])!=(tags[k]|((output&0xffffffu)+(i-1u)*strides[k])))fail("packet link chain",i);
  }
  if(scenario==4){
   if(peek(output)!=0xa5a5a5a5u||peek(output+8u)==0xa5a5a5a5u||peek(ot)!=0x00180000u)fail("XY before zero-OTZ unlink",0);
  }
  total++;
 }
 defaults();printf("TRIANGLE WALKERS PASS %u explicit transform/lookahead/cull/packet-link semantic witnesses\n",total);return total;
}
static unsigned bindings(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.data_count=0;
 const uint32_t addresses[]={0x800500f8u,0x800500fcu,0x80050100u,0x80059578u};
 int32_t *owners[]={&D_800500F8,&D_800500FC,&D_80050100,&D_80059578};
 for(unsigned i=0;i<4;i++)rt.data[rt.data_count++]=(ResolvedData){addresses[i],32u,host+0x100u+i*64u,"oversized-old-owner"};
 memcpy(want_host,host,sizeof(host));initialize_model_packet_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 unsigned entries=(unsigned)rt.data_count,total=0;initialize_model_packet_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 if(entries!=17||rt.data_count!=entries)fail("data initialization/idempotence",entries);
 for(unsigned i=0;i<4;i++){
  ResolvedData *d=find_data(&rt,addresses[i],4);if(!d||d->address!=addresses[i]||d->size!=4||d->host!=(uint8_t *)owners[i])fail("scalar owner/width",addresses[i]);
  for(unsigned alias=0;alias<3;alias++){
   uint32_t a=alias==0?addresses[i]:alias==1?addresses[i]|0x20000000u:addresses[i]&0x1fffffu;
   uint32_t v=0x87654300u+i*16u+alias;put(a,4,v);if(peek(a)!=v||(uint32_t)*owners[i]!=v)fail("canonical scalar alias",a);total++;
  }
 }
 if(memcmp(want_host,host,sizeof(host)))fail("old owners changed",0);
 defaults();printf("TRIANGLE WALKERS PASS %u scalar binding aliases, four exact owners and idempotence\n",total);return total;
}
int main(int argc,char **argv)
{
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 memcpy(code,image->bytes+XENO_RD_SLUS_OFF(0x8002e010u),540);memcpy(caller_code,image->bytes+XENO_RD_SLUS_OFF(0x8002c700u),460);defaults();
 if(argc>1){if(!strcmp(argv[1],"--basic"))return run_case()?0:1;if(!strcmp(argv[1],"--faults"))return faults()?0:1;if(!strcmp(argv[1],"--integration"))return integration()?0:1;if(!strcmp(argv[1],"--gates"))return controls()?0:1;if(!strcmp(argv[1],"--boundary"))return boundaries()?0:1;if(!strcmp(argv[1],"--semantics"))return semantics()?0:1;if(!strcmp(argv[1],"--bindings"))return bindings()?0:1;return 2;}
 unsigned total=0;const unsigned counts[]={0,1,2,3,17};
 for(unsigned k=0;k<4;k++)for(unsigned s=0;s<12;s++)for(unsigned n=0;n<5;n++){
  defaults();kind=k;scenario=s;count=counts[n];run_case();total++;
 }
 for(unsigned k=0;k<4;k++)for(unsigned d=0;d<7;d++)for(unsigned n=0;n<2;n++){
  defaults();kind=k;domain=d;count=n?3:0;run_case();total++;
 }
 for(unsigned k=0;k<4;k++)for(unsigned s=1;s<8;s++){defaults();kind=k;stack_kind=s;count=1;run_case();total++;}
 for(unsigned k=0;k<4;k++)for(unsigned a=1;a<=7;a++){defaults();kind=k;alias_kind=a;count=1;run_case();total++;}
 for(unsigned k=0;k<4;k++){defaults();kind=k;count=257;run_case();total++;}
 for(unsigned k=0;k<4;k++)for(unsigned n=0;n<2;n++){defaults();kind=k;count=n?0x80000000u:0xffffffffu;fault_at=100;run_case();total++;}
 defaults();printf("TRIANGLE WALKERS PASS %u full original/native comparisons, all four entries and actual GTE state\n",total);fflush(stdout);
 if(!faults()||!integration()||!controls()||!boundaries()||!semantics()||!bindings())return 1;
 puts("TRIANGLE WALKERS ALL PASS native original-CPU instructions=0; actual graphics backend; final return load retained");return 0;
}

