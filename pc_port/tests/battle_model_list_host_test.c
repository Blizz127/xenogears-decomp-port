/* Complete original model relocator and pointer-list caller comparisons.
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
static uint8_t list_body[164],reloc_body[212],shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned variant,kind,fault_at,accesses,call_count,expected_accesses,expected_calls;
static unsigned mode,records,flag,subkind,alias_kind,alloc_null;
static int candidate,compare,failed,observing;
static uint32_t model,list,allocation,stop_target,pending_target;
typedef struct Access {uint32_t address,width,value,write;int rc;} Access;
static Access trace[65536];
typedef struct Call {uint32_t target,gpr[32],hi,lo;uint8_t ram[32],shared[32],pad[32],host[32];} Call;
static Call calls[8];
static void fail(const char *what,uint32_t value)
{fprintf(stderr,"MODEL LIST FAIL kind=%u variant=%u domain=%u count=%u flags=%u sub=%u alias=%u null=%u fault=%u access=%u call=%u %s %08x\n",kind,variant,mode,records,flag,subkind,alias_kind,alloc_null,fault_at,accesses,call_count,what,value);failed=1;}
static int code_address(uint32_t a)
{return a-0x8009eba8u<sizeof(list_body)||a-0x8002c3e8u<sizeof(reloc_body);}
static int observed_access(void *opaque,uint32_t address,unsigned width,uint32_t *value,int write)
{
 /* Supply only the test oracle's main-executable instruction fetches here.
  * Production leaves that text absent from RAM; both legs keep the same RAM. */
 if(observing&&!write&&address-0x8002c3e8u<sizeof(reloc_body)){
  assert(!candidate&&width==4);*value=load_le(reloc_body+(address-0x8002c3e8u),4);return 0;
 }
 if(!observing||(!write&&code_address(address)))return write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 assert(accesses<sizeof(trace)/sizeof(trace[0]));
 int rc=fault_at==accesses+1?-1:write?model_test_raw_write(opaque,address,width,*value):model_test_raw_read(opaque,address,width,value);
 Access row={address,width,rc&&!write?0:*value,(unsigned)write,rc};
 if(compare){if(accesses>=expected_accesses||memcmp(&row,&trace[accesses],sizeof(row)))fail("ordered memory access",address);}
 else trace[accesses]=row;
 accesses++;return rc;
}
static int runtime_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{return observed_access(opaque,address,width,value,0);}
static int runtime_write(void *opaque,uint32_t address,unsigned width,uint32_t value)
{return observed_access(opaque,address,width,&value,1);}
static void put(uint32_t a,uint32_t v){assert(model_test_raw_write(&rt,a,4,v)==0);}
static void snapshot(void){memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void){return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static uint32_t domain(unsigned off)
{switch(mode){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;case 4:return 0x1f800000u+off;case 5:return 0x800c0000u+off;default:return 0x80100000u+off;}}
static void record_call(PcPortMipsCpu *cpu,uint32_t target)
{
 assert(call_count<8);Call row={.target=target,.hi=cpu->hi,.lo=cpu->lo};memcpy(row.gpr,cpu->gpr,sizeof(row.gpr));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare){if(call_count>=expected_calls||memcmp(&row,&calls[call_count],sizeof(row)))fail("call registers/full memory",target);}
 else calls[call_count]=row;
 call_count++;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x8009eba8u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target==0x8002c3e8u){
  record_call(cpu,target);if(target==stop_target)return -1;
  if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;return 1;}
  return candidate?runtime_bridge(opaque,cpu,target):0;
 }
 if(code_address(target))return 0;
 if(target!=0x80032498u&&target!=0x80031bdcu){fail("unexpected callee",target);return -1;}
 record_call(cpu,target);if(target==stop_target)return -1;
 if(target==0x80032498u){
  if(cpu->gpr[4]!=4||cpu->gpr[5])fail("heap user arguments",cpu->gpr[4]);
 }else{
  if(cpu->gpr[4]!=(records<<2)||cpu->gpr[5])fail("heap allocation arguments",cpu->gpr[4]);
 }
 cpu->gpr[2]=target==0x80031bdcu?(alloc_null?0:allocation):0x76543210u;
 cpu->gpr[3]=0xabc00000u+call_count;cpu->hi=0x65430000u+call_count;cpu->lo=0x34560000u+call_count;
 if(target==pending_target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 observing=0;memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x8009eba8u),list_body,sizeof(list_body));
 rt.data[0]=(ResolvedData){0x800c0000u,sizeof(shared),shared,"models"};rt.data_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&1u)cpu->gpr[29]=0x1f800fe0u;else if(variant&2u)cpu->gpr[29]|=0x20000000u;
 model=domain(0x100);list=0x80120000u;allocation=0x80140000u;
 put(model,records);put(model+4u,flag?0x87654321u:0x87654320u);
 unsigned n=(int32_t)records>0&&records<1024?records:0;
 for(unsigned i=0;i<n;i++){
  uint32_t rec=model+0x10u+i*0x38u;
  put(rec+8u,0xfffffff0u+i*4u);put(rec+12u,0x80000001u+i*7u);put(rec+16u,0x7ffffffcu-i*13u);put(rec+20u,0x1234u+i*17u);
  uint32_t off=(records>16?0x10000u:0x800u)+i*0x30u;put(rec+28u,subkind?off:0);
  if(subkind){
   unsigned last=subkind==1?UINT32_MAX:subkind-2;
   put(model+off,last);
   if(last!=UINT32_MAX)for(unsigned j=0;j<=last;j++){put(model+off+4u+j*12u,0xbad00000u+j);put(model+off+8u+j*12u,0xfffffffcu-j);put(model+off+12u+j*12u,0x80000000u+j);}
  }
 }
 if(alias_kind==1)allocation=list;
 if(alias_kind==2)list=model;
 if(alias_kind==3){assert(records==1);put(model+0x18u,0u-model);put(model+0x2cu,0x18u);}
 if(alias_kind==4){assert(records==1);put(model+0x2cu,0u-model+0x1e000000u);}
 if(alias_kind==5){assert(records==1);put(model+0x2cu,0u-model+0x1f800ffcu);put(0x1f800ffcu,0xfffffffeu);}
 if(alias_kind==6)list=0x1f800ffcu;
 if(alias_kind==7)allocation=0x1f800ffcu;
 cpu->gpr[4]=alias_kind==8?model+2u:alias_kind==9?0x1e000000u:model;cpu->gpr[5]=list;
 accesses=call_count=0;observing=1;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;uint32_t target=kind?0x8009eba8u:0x8002c3e8u;
 setup(&oracle);candidate=compare=0;int original=PcPortMipsRun(&oracle,target,BATTLE_HALT_PC,1000000);
 expected_accesses=accesses;expected_calls=call_count;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,target,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 observing=0;
 if(original==PC_PORT_MIPS_BUDGET){fail("oracle instruction budget",oracle.pc);return 0;}
 int expected_failure=fault_at||stop_target||alias_kind==4||alias_kind==5||alias_kind==8||alias_kind==9||(kind&&(alias_kind==6||(alias_kind==7&&records>1)));
 if((original<0)!=!!expected_failure)fail("unexpected oracle completion/fault",(uint32_t)original);
 if((original==PC_PORT_MIPS_HALTED)!=(actual==PC_PORT_MIPS_HALTED))fail("return status",(uint32_t)actual);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||call_count!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native instructions/context/counts",accesses);
 for(unsigned i=0;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("register",i);fprintf(stderr,"native=%08x oracle=%08x\n",native.gpr[i],oracle.gpr[i]);}
 if(native.hi!=oracle.hi||native.lo!=oracle.lo)fail("HI/LO",native.hi);
 if(!memory_equal())fail("complete final memory",0);
 return !failed;
}
static int controls(void)
{
 for(kind=0;kind<2;kind++){
  PcPortMipsCpu cpu;mode=flag=alias_kind=alloc_null=subkind=0;records=2;setup(&cpu);observing=0;candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
  uint32_t target=kind?0x8009eba8u:0x8002c3e8u;unsigned size=kind?sizeof(list_body):sizeof(reloc_body);PcPortMipsCpu original=cpu;
  uint8_t *code=kind?PSX_ADDR(target):g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(target);
  for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?size/2:size-1;uint8_t *p=code+off;*p^=1;if(battle_model_list_try_host(&rt,&cpu,target))fail("identity accepted",off);*p^=1;}
  if(!kind){
   XenoRetailFileSlot original_slot=g_XenoRetailFiles[XENO_RD_SLUS];
   g_XenoRetailFiles[XENO_RD_SLUS].state=-1;if(battle_model_list_try_host(&rt,&cpu,target))fail("missing cached asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;
   g_XenoRetailFiles[XENO_RD_SLUS].size=XENO_RD_SLUS_OFF(target)+size-1u;
   if(battle_model_list_try_host(&rt,&cpu,target))fail("short cached asset",target);
   g_XenoRetailFiles[XENO_RD_SLUS]=original_slot;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_model_list_try_host(&rt,&cpu,target))fail("pending entry",target);cpu=original;
  rt.force_interpret=1;if(battle_model_list_try_host(&rt,&cpu,target))fail("forced entry",target);rt.force_interpret=0;
  rt.initialized=0;if(battle_model_list_try_host(&rt,&cpu,target))fail("uninitialized entry",target);rt.initialized=1;
  g_ActiveBattleRuntime=NULL;if(battle_model_list_try_host(&rt,&cpu,target))fail("inactive entry",target);g_ActiveBattleRuntime=&rt;
  rt.bridge_cpu=NULL;if(battle_model_list_try_host(&rt,&cpu,target))fail("wrong context",target);rt.bridge_cpu=&cpu;
  if(battle_model_list_try_host(&rt,&cpu,target+4u))fail("wrong target",target);
  assert(setenv("XENO_BATTLE_NATIVE_MODEL_LIST","0",1)==0);if(battle_model_list_try_host(&rt,&cpu,target))fail("opt out",target);assert(unsetenv("XENO_BATTLE_NATIVE_MODEL_LIST")==0);
  if(memcmp(cpu.gpr,original.gpr,sizeof(cpu.gpr))||!memory_equal()||call_count||accesses)fail("gate side effect",target);
 }
 kind=1;const uint32_t targets[]={0x80032498u,0x8002c3e8u,0x80031bdcu};
 for(unsigned i=0;i<3;i++){
  stop_target=targets[i];if(!run_case())return 0;stop_target=0;
  PcPortMipsCpu cpu;setup(&cpu);candidate=1;compare=0;pending_target=targets[i];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x8009eba8u,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);pending_target=0;observing=0;
  if(rc>=0||cpu.steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("pending callee refusal",targets[i]);
 }
 puts("MODEL LIST PASS both identities/context/opt-out, absent/short cached assets and six nested refusal controls");return !failed;
}
static int faults(void)
{
 unsigned total=0;mode=flag=alloc_null=alias_kind=0;records=2;subkind=3;
 for(kind=0;kind<2;kind++){
  fault_at=0;if(!run_case())return 0;unsigned count=accesses;
  for(fault_at=1;fault_at<=count;fault_at++){if(!run_case())return 0;total++;}
  fault_at=0;
 }
 printf("MODEL LIST PASS %u injected faults across every data access of both complete bodies\n",total);fflush(stdout);return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(fseek(f,0x8009eba8u-0x8006faf0u,SEEK_SET)==0&&fread(list_body,1,sizeof(list_body),f)==sizeof(list_body));fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(fseek(f,0x1cbe8,SEEK_SET)==0&&fread(reloc_body,1,sizeof(reloc_body),f)==sizeof(reloc_body));fclose(f);
 assert(XenoRetailData_File(XENO_RD_SLUS));assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2&&strcmp(argv[1],"--controls")==0)return controls()?0:1;
 if(argc==2&&strcmp(argv[1],"--faults")==0)return faults()?0:1;
 if(argc==2){
  records=2;subkind=3;kind=1;
  if(strcmp(argv[1],"--relocator")==0)kind=0;
  else if(strcmp(argv[1],"--live-list")==0){records=3;alias_kind=1;}
  else if(strcmp(argv[1],"--zero-sub")==0){records=1;subkind=2;}
  else if(strcmp(argv[1],"--relocated")==0)flag=1;
  else if(strcmp(argv[1],"--negative-count")==0){records=0x80000000u;alloc_null=1;}
  else if(strcmp(argv[1],"--null")==0)alloc_null=1;
  else assert(strcmp(argv[1],"--caller")==0);
  return run_case()?0:1;
 }
 unsigned total=0;const unsigned counts[]={0,1,2,3,16};
 for(kind=0;kind<2;kind++)for(mode=0;mode<6;mode++)for(unsigned n=0;n<5;n++)for(flag=0;flag<2;flag++)for(subkind=0;subkind<5;subkind++){
  records=counts[n];variant=total++;if(!run_case())return 1;
 }
 printf("MODEL LIST PASS %u full original comparisons across six address domains\n",total);fflush(stdout);
 mode=flag=alias_kind=subkind=0;
 const unsigned edges[]={0x7fffffffu,0x80000000u,0xffffffffu,0x40000000u};
 for(kind=0;kind<2;kind++)for(unsigned i=0;i<4;i++){records=edges[i];flag=1;alloc_null=1;variant++;if(!run_case())return 1;}
 flag=0;
 for(kind=0;kind<2;kind++)for(unsigned i=1;i<=2;i++){records=edges[i];alloc_null=1;variant++;if(!run_case())return 1;}
 flag=alloc_null=0;
 for(kind=0;kind<2;kind++)for(mode=0;mode<4;mode++)for(unsigned i=0;i<2;i++){records=i?255:64;subkind=4;variant++;if(!run_case())return 1;}
 puts("MODEL LIST PASS twelve signed/wrapped count edges and sixteen large-count comparisons through 255 records");fflush(stdout);
 mode=flag=alloc_null=subkind=0;records=1;
 for(kind=0;kind<2;kind++)for(alias_kind=1;alias_kind<=9;alias_kind++){records=alias_kind==7?2:1;variant++;if(!run_case())return 1;}
 alias_kind=0;kind=1;records=3;alloc_null=1;if(!run_case())return 1;alloc_null=0;
 kind=1;records=3;alias_kind=1;if(!run_case())return 1;alias_kind=0;
 puts("MODEL LIST PASS aliases, natural partial faults, allocation failure and live list-pointer reload");fflush(stdout);
 if(!faults())return 1;
 return controls()?0:1;
}
