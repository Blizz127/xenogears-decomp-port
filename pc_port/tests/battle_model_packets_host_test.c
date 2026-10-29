/* Complete original model dispatcher, packet builders and latch comparisons.
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
static uint32_t cop_data[32],cop_control[32];
unsigned int MFC2(int r){return cop_data[r&31];}
unsigned int CFC2(int r){return cop_control[r&31];}
void MTC2(unsigned int v,int r){cop_data[r&31]=v;}
void CTC2(unsigned int v,int r){cop_control[r&31]=v;}
int doCOP2(int op){(void)op;abort();}
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
typedef struct Body {uint32_t address,size;uint8_t bytes[752];} Body;
static Body bodies[]={{.address=0x8002c8ccu,.size=648},{.address=0x8002ccacu,.size=28},{.address=0x8002ccc8u,.size=92},{.address=0x8002cd24u,.size=64},{.address=0x8002cd64u,.size=104},{.address=0x8002cdccu,.size=360},{.address=0x8002cf34u,.size=36},{.address=0x8002cf58u,.size=360},{.address=0x8002d0c0u,.size=36},{.address=0x8002d0e4u,.size=156},{.address=0x8002d180u,.size=196},{.address=0x8002d244u,.size=272},{.address=0x8002d530u,.size=380},{.address=0x8002d6acu,.size=208},{.address=0x8002d77cu,.size=152},{.address=0x8002d814u,.size=368},{.address=0x8002d984u,.size=144},{.address=0x8002da14u,.size=232},{.address=0x8002dafcu,.size=136}};
#define BODY_COUNT (sizeof(bodies)/sizeof(bodies[0]))
static uint8_t shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned kind,domain_kind,variant,groups=1,primitives=1,row_type,flags,mode,inline_controls,live_change,alias_kind,fail_alloc;
static unsigned fault_at,accesses,call_count,expected_accesses,expected_calls,allocs,mutation_seen;
static int candidate,compare,failed,observing;
static uint32_t stop_target,pending_target,model,source,commands,output,light,vertices,normals;
static uint8_t descriptor_bytes[680];
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[65536];
typedef struct Call {uint32_t target,gpr[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[512];
static void fail(const char *what,uint32_t value)
{fprintf(stderr,"MODEL PACKETS FAIL kind=%u target=%08x domain=%u variant=%u groups=%u count=%u row=%u flags=%u mode=%u inline=%u live=%u alias=%u fault=%u access=%u call=%u %s %08x\n",kind,bodies[kind].address,domain_kind,variant,groups,primitives,row_type,flags,mode,inline_controls,live_change,alias_kind,fault_at,accesses,call_count,what,value);failed=1;}
static int body_index(uint32_t address)
{for(unsigned i=0;i<BODY_COUNT;i++)if(address-bodies[i].address<bodies[i].size)return (int)i;return -1;}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 int bi=body_index(address);
 if(observing&&!write&&bi>=0){assert(!candidate&&width==4);*value=load_le(bodies[bi].bytes+address-bodies[bi].address,4);return 0;}
 if(!observing)return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 Access row={address,width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{return observed_access(opaque,address,width,value,0);}
static int runtime_write(void *opaque,uint32_t address,unsigned width,uint32_t value)
{return observed_access(opaque,address,width,&value,1);}
static void fixture_store(uint32_t a,unsigned w,uint32_t v){assert(model_test_raw_write(&rt,a,w,v)==0);}
static void put(uint32_t a,uint32_t v){fixture_store(a,4,v);}
static void snapshot(void){memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void){return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static uint32_t domain(unsigned off)
{switch(domain_kind){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x800c0000u+off;default:return 0x80100000u+off;}}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<512);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("call registers/full memory",target);}else calls[call_count]=row;
 call_count++;
}
static uint32_t peek(uint32_t a,unsigned w){uint32_t value;assert(model_test_raw_read(&rt,a,w,&value)==0);return value;}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==bodies[kind].address&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0){
  if(target!=bodies[body_index(target)].address)return 0;
  record_call(cpu,target);if(target==stop_target)return -1;
  if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;return 1;}
  if(kind==0&&battle_model_packets_callback(target)&&live_change&&mutation_seen++==0){
   uint32_t d=0x8004fe50u+row_type*40u;
   if(live_change==1){put(d+0x1cu,12);put(d+0x20u,16);put(d+0x24u,64);}
   if(live_change==2)put(d+0x18u,0x8002dafcu);
   if(live_change==3){put(0x80059538u,source+0x80u);put(0x80059424u,output+0x80u);}
  }
  return candidate?runtime_bridge(opaque,cpu,target):0;
 }
 record_call(cpu,target);if(target==stop_target)return -1;
 uint32_t result=0x76543210u;
 /* External helpers have deterministic effects in both legs. This proves
  * complete caller semantics and boundaries, not original GTE arithmetic. */
 switch(target){
 case 0x800324b8u:if(cpu->gpr[4]!=0x26u)fail("content tag",cpu->gpr[4]);break;
 case 0x80031bdcu:
  allocs++;if(cpu->gpr[5])fail("alloc flags",cpu->gpr[5]);result=fail_alloc?0:light;break;
 case 0x8002db84u:for(unsigned i=0;i<4;i++)fixture_store(cpu->gpr[7]+i*2u,2,0x123u+i+variant);break;
 case 0x8004a19cu:put(cpu->gpr[5],0x64123456u+variant);break;
 case 0x8004a260u:put(cpu->gpr[6],0x65234567u+variant);break;
 case 0x8004a1b8u:
  put(cpu->gpr[7],0x66123456u+variant);put(peek(cpu->gpr[29]+0x10u,4),0x67123456u+variant);put(peek(cpu->gpr[29]+0x14u,4),0x68123456u+variant);break;
 case 0x8004a280u:
  for(unsigned i=0;i<3;i++)put(peek(cpu->gpr[29]+0x10u+i*4u,4),0x69123456u+i+variant);
  break;
 case 0x80043c60u:fixture_store(cpu->gpr[4]+3u,1,7);fixture_store(cpu->gpr[4]+7u,1,0x24);break;
 case 0x80043c24u:fixture_store(cpu->gpr[4]+7u,1,peek(cpu->gpr[4]+7u,1)|1u);break;
 case 0x80043a1cu:result=((cpu->gpr[4]<<7)|(cpu->gpr[5]<<5)|((cpu->gpr[7]&0x100u)>>4)|((cpu->gpr[6]&0x3ffu)>>6)|((cpu->gpr[7]&0x200u)<<2))&0xffffu;break;
 case 0x80043a58u:result=((cpu->gpr[5]<<6)|((cpu->gpr[4]>>4)&0x3fu))&0xffffu;break;
 default:return -1;
 }
 cpu->gpr[2]=result;cpu->gpr[3]=0xabc00000u+call_count;cpu->hi=0x65430000u+call_count;cpu->lo=0x34560000u+call_count;
 if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;}
 return 1;
}
static int textured(unsigned t){return t==1||t==3||t==5||t==7||t==9||t==11||t==13||t==15;}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 /* Main executable instructions are deliberately absent from production RAM. */
 memcpy(PSX_ADDR(0x8004fe50u),descriptor_bytes,sizeof(descriptor_bytes));
 rt.data[0]=(ResolvedData){0x800c0000u,sizeof(shared),shared,"packets"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&1u)cpu->gpr[29]=0x1f800fe0u;else if(variant&2u)cpu->gpr[29]|=0x20000000u;
 model=domain(0x20u);source=domain(0x100u);commands=domain(0x400u);output=0x80160000u;light=0x80170000u;vertices=domain(0x900u);normals=domain(0xa00u);
 for(unsigned i=0;i<0x300u;i+=4u)put(source+i,0x34271829u+i);
 for(unsigned i=0;i<0x300u;i+=2u)fixture_store(commands+i,2,i%8u==0?0xffffu:i%8u==2?0x8000u:i%8u==4?0x7fffu:3u);
 put(0x80059424u,output);put(0x80059538u,source);put(0x80059528u,commands);put(0x8005952cu,normals);put(0x8005953cu,vertices);put(0x80059498u,light);
 put(0x80050108u,variant%4u);put(0x8005010cu,(variant/4u)%3u);put(0x80059310u,0xabc00017u);put(0x80059314u,0xdef01230u);
 fixture_store(0x80059308u,2,0x9876u);fixture_store(0x8005930cu,2,0x4567u);put(0x800595c0u,0xfffffff0u);
 cpu->gpr[4]=source;cpu->gpr[5]=commands;cpu->gpr[6]=mode;
 if(kind==0){
  cpu->gpr[4]=model;cpu->gpr[5]=output;cpu->gpr[21]=0x12340000u|(mode&7u);
  fixture_store(model,2,flags);fixture_store(model+4u,2,0x8123u);fixture_store(model+6u,2,groups);
  put(model+8u,vertices);put(model+0xcu,normals);put(model+0x10u,commands);put(model+0x14u,source);put(model+0x18u,light);put(model+0x30u,variant%3u?0x100:0);
  uint32_t cmd=commands,src=source;
  for(unsigned g=0;g<groups;g++){
   unsigned t=groups==17?g:row_type;fixture_store(cmd,1,t);fixture_store(cmd+2u,2,primitives);cmd+=4u;
   uint32_t stride=peek(0x8004fe50u+t*40u+0x20u,4);
   for(unsigned n=0;n<primitives;n++){
    if(inline_controls&&textured(t)){
     put(src,0xc4004567u);put(src+4u,0xc80089abu);src+=8u;
    }
    put(src,0x34123456u+n);fixture_store(src+4u,2,0x2345u);fixture_store(src+6u,2,0x6789u);
    fixture_store(src+8u,2,0xabcdu);fixture_store(src+10u,2,0xef01u);src+=stride;cmd+=8u;
   }
  }
 }else{
  const unsigned codes[]={0x34,0xc4,0xc8,0xc0,0xcf,0xff};fixture_store(source+3u,1,codes[(variant/4u)%6u]);
 }
 if(alias_kind==1)cpu->gpr[4]++;
 if(alias_kind==2)cpu->gpr[29]=0x1e000000u;
 if(alias_kind==3)put(0x80059424u,source);
 if(alias_kind==4)put(0x80059498u,source);
 accesses=call_count=allocs=mutation_seen=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;uint32_t target=bodies[kind].address;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,1000000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,target,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original==PC_PORT_MIPS_BUDGET){fail("oracle budget",oracle.pc);return 0;}
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(!fault_at&&!stop_target&&!alias_kind&&original!=PC_PORT_MIPS_HALTED)fail("unexpected oracle failure",oracle.pc);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("register",i);fprintf(stderr,"native=%08x oracle=%08x oracle_pc=%08x\n",native.gpr[i],oracle.gpr[i],oracle.pc);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo)fail("HI/LO",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 if(live_change&&!mutation_seen)fail("live mutation not exercised",live_change);
 return !failed;
}
static int controls(void)
{
 for(kind=0;kind<BODY_COUNT;kind++){
  PcPortMipsCpu cpu;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  uint32_t target=bodies[kind].address;unsigned size=bodies[kind].size;PcPortMipsCpu original=cpu;
  uint8_t *code=g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);
  for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?size/2:size-1;uint8_t *p=code+off;*p^=1;if(battle_model_packets_try_host(&rt,&cpu,target))fail("identity accepted",off);*p^=1;}
  XenoRetailFileSlot original_slot=g_XenoRetailFiles[XENO_RD_SLUS];
  g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_packets_try_host(&rt,&cpu,target))fail("missing asset",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+size-1u;
  if(battle_model_packets_try_host(&rt,&cpu,target))fail("short asset",target);
  g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;
  cpu.load_valid=1;cpu.load_reg=4;if(battle_model_packets_try_host(&rt,&cpu,target))fail("pending entry",target);cpu=original;
  rt.force_interpret=1;if(battle_model_packets_try_host(&rt,&cpu,target))fail("forced entry",target);rt.force_interpret=0;
  rt.initialized=0;if(battle_model_packets_try_host(&rt,&cpu,target))fail("uninitialized entry",target);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_model_packets_try_host(&rt,&cpu,target))fail("inactive entry",target);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_model_packets_try_host(&rt,&cpu,target))fail("context",target);rt.bridge_cpu=&cpu;
  if(battle_model_packets_try_host(&rt,&cpu,target+4u))fail("target",target);
  assert(setenv("XENO_BATTLE_NATIVE_MODEL_PACKETS","0",1)==0);if(battle_model_packets_try_host(&rt,&cpu,target))fail("opt out",target);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_PACKETS")==0);
  if(memcmp(cpu.gpr,original.gpr,sizeof(cpu.gpr))||!memory_equal()||call_count||accesses)fail("gate side effect",target);
 }
 puts("MODEL PACKETS PASS 19 identity/context/opt-out gates and cached asset absence/truncation controls");return !failed;
}
static int faults(void)
{
 unsigned total=0;domain_kind=inline_controls=live_change=alias_kind=fail_alloc=0;groups=primitives=1;flags=0;row_type=1;
 for(kind=0;kind<BODY_COUNT;kind++)for(unsigned path=0;path<(kind==0?8u:6u);path++){
  static const unsigned modes[]={0,1,3,4,1,3,0,3},variants[]={1,2,3,0,4,8,1,1};mode=modes[path];variant=variants[path];inline_controls=kind==0&&path>=6;
  fault_at=0;if(!run_case())return 0;unsigned count=accesses;
  for(fault_at=1;fault_at<=count;fault_at++){if(!run_case())return 0;total++;}fault_at=0;
 }
 printf("MODEL PACKETS PASS %u injected faults across all 19 complete bodies and six callback branch configurations plus inline dispatcher paths\n",total);fflush(stdout);return !failed;
}
static int callees(void)
{
 unsigned total=0;domain_kind=inline_controls=live_change=alias_kind=fail_alloc=0;groups=primitives=1;flags=0;row_type=1;
 for(kind=0;kind<BODY_COUNT;kind++)for(unsigned path=0;path<4;path++){
  mode=3;variant=path?path*4:1;stop_target=0;if(!run_case())return 0;
  uint32_t targets[512];unsigned count=call_count;for(unsigned i=0;i<count;i++)targets[i]=calls[i].target;
  for(unsigned i=0;i<count;i++){
   stop_target=targets[i];if(!run_case())return 0;stop_target=0;
   PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_target=targets[i];
   assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
   int rc=PcPortMipsRun(&cpu,bodies[kind].address,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);pending_target=0;observing=0;
   if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending return refusal",targets[i]);
   total++;
  }
 }
 printf("MODEL PACKETS PASS %u missing callee comparisons and pending return refusals\n",total);return !failed;
}
uint16_t D_80059308[8],D_8005930C[8];uint32_t D_800595C0[4];
static int bindings(void)
{
 observing=0;memset(&rt,0,sizeof(rt));uint8_t neighbor[0x400];memset(neighbor,0xa7,sizeof(neighbor));
 rt.data[0]=(ResolvedData){0x8004fe48u,0x2b8u,neighbor,"neighbor"};
 rt.data[1]=(ResolvedData){0x80059424u,8u,neighbor+0x300u,"legacy-pointer"};rt.data_count=2;
 initialize_model_packet_data(&rt);qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 if(rt.data_count!=13)fail("binding count",rt.data_count);
 const uint32_t pointers[]={0x80059424u,0x80059528u,0x80059538u,0x8005952cu,0x8005953cu,0x80059498u,0x80059568u};
 for(unsigned i=0;i<sizeof(pointers)/sizeof(pointers[0]);i++){
  if(resolve_memory(&rt,pointers[i],4,1)!=PSX_ADDR(pointers[i]))fail("canonical storage",pointers[i]);
  put(pointers[i],0xa0123456u+i);if(peek(pointers[i]+0x20000000u,4)!=0xa0123456u+i)fail("K1 pointer alias",pointers[i]);
 }
 if(resolve_memory(&rt,0x8004fe50u,680,0)!=PSX_ADDR(0x8004fe50u))fail("descriptor storage",0);
 if(resolve_memory(&rt,0x80018980u,24,0)!=PSX_ADDR(0x80018980u))fail("render jump table storage",0);
 memset(D_80059308,0xbc,sizeof(D_80059308));memset(D_8005930C,0xbc,sizeof(D_8005930C));memset(D_800595C0,0xbc,sizeof(D_800595C0));
 fixture_store(0x80059308u,2,0x1234);fixture_store(0x8005930cu,2,0x5678);put(0x800595c0u,0x12345678);
 if(D_80059308[0]!=0x1234||D_8005930C[0]!=0x5678||D_800595C0[0]!=0x12345678||D_80059308[1]!=0xbcbc||D_8005930C[1]!=0xbcbc||D_800595C0[1]!=0xbcbcbcbc)fail("scalar width/guards",0);
 for(unsigned i=0;i<sizeof(neighbor);i++)if(neighbor[i]!=0xa7u)fail("neighbor overwrite",i);
 puts("MODEL PACKETS PASS canonical descriptor/jump tables and seven pointer slots, three real scalar bindings and neighbor guards");return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 for(unsigned i=0;i<BODY_COUNT;i++){assert(fseek(f,XENO_RD_SLUS_OFF(bodies[i].address),SEEK_SET)==0&&fread(bodies[i].bytes,1,bodies[i].size,f)==bodies[i].size);}
 assert(fseek(f,XENO_RD_SLUS_OFF(0x8004fe50u),SEEK_SET)==0&&fread(descriptor_bytes,1,sizeof(descriptor_bytes),f)==sizeof(descriptor_bytes));fclose(f);
 assert(XenoRetailData_File(XENO_RD_SLUS));assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2){
  if(!strcmp(argv[1],"--controls"))return controls()?0:1;
  if(!strcmp(argv[1],"--faults"))return faults()?0:1;
  if(!strcmp(argv[1],"--callees"))return callees()?0:1;
  if(!strcmp(argv[1],"--bindings"))return bindings()?0:1;
  if(!strcmp(argv[1],"--stream")){kind=0;row_type=13;inline_controls=1;return run_case()?0:1;}
  if(!strcmp(argv[1],"--live")){kind=0;row_type=4;live_change=1;primitives=2;mode=1;variant=1;return run_case()?0:1;}
  if(!strcmp(argv[1],"--load-fault")){kind=0;mode=1;variant=1;fault_at=13;return run_case()?0:1;}
  kind=(unsigned)strtoul(argv[1],NULL,0);assert(kind<BODY_COUNT);mode=3;variant=1;return run_case()?0:1;
 }
 unsigned total=0;
 for(kind=1;kind<BODY_COUNT;kind++)for(domain_kind=0;domain_kind<6;domain_kind++)for(mode=0;mode<8;mode++)for(unsigned code=0;code<3;code++){
  variant=mode+code*8u;if(!run_case())return 1;total++;
 }
 printf("MODEL PACKETS PASS %u complete callback/helper comparisons across six domains, eight shading words and control variants\n",total);fflush(stdout);
 kind=0;groups=primitives=1;
 static const unsigned modes[]={0,1,2,3,4,0xffffffffu};
 for(row_type=0;row_type<17;row_type++)for(unsigned m=0;m<6;m++)for(flags=0;flags<4;flags++)for(inline_controls=0;inline_controls<2;inline_controls++){
  mode=modes[m];domain_kind=(row_type+m)%6;variant=(row_type+m)%4;if(!run_case())return 1;total++;
 }
 printf("MODEL PACKETS PASS all 17 descriptor rows, six dispatcher modes, four flag combinations and inline page/CLUT control streams (%u total cases)\n",total);fflush(stdout);
 domain_kind=flags=inline_controls=0;mode=1;variant=1;row_type=4;groups=1;
 for(primitives=0;primitives<3;primitives++)if(!run_case())return 1;
 primitives=1;groups=0;if(!run_case())return 1;groups=17;inline_controls=1;if(!run_case())return 1;
 groups=1;inline_controls=0;primitives=2;
 for(live_change=1;live_change<=3;live_change++)if(!run_case())return 1;
 live_change=0;fail_alloc=1;row_type=4;if(!run_case())return 1;fail_alloc=0;
 for(kind=0;kind<BODY_COUNT;kind++)for(alias_kind=1;alias_kind<=4;alias_kind++)if(!run_case())return 1;
 alias_kind=0;
 const uint32_t edge_modes[]={0x80000000u,0x7fffffffu,0xffffffffu,0xffff0001u,0x12340004u};
 for(kind=1;kind<BODY_COUNT;kind++)for(unsigned i=0;i<5;i++){mode=edge_modes[i];if(!run_case())return 1;}
 puts("MODEL PACKETS PASS empty/multiple groups, live strides/callback/cursors, allocation failure and pointer/stack/alias edges");fflush(stdout);
 if(!faults()||!callees()||!controls()||!bindings())return 1;
 return 0;
}
