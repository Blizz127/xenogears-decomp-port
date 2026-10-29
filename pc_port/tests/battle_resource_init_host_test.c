/* Complete original 801E7210 caller versus structured C. External actor,
 * geometry and resource services are observable models; a separate mode uses
 * the actual memory/unpin interfaces. This is not callee adoption evidence. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
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
static uint8_t module[0x56c0],shared[0x20000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x30000],want_host[sizeof(host)];
static unsigned variant,calls,service_mode,perturb,early,allocation_fail,descriptor_case;
static int candidate,compare,failed;
static uint32_t stop_target,pending_return,frame_base,source,destination,auxiliary,actor,records,positions;
static uint64_t visited;
static const uint32_t targets[]={0x80032498,0x800320b8,0x800a8bf0,0x801e70e8,0x80031bdc,
    0x8003f968,0x800320e8,0x8003342c,0x800aa898,0x800aa934,0x8009ef3c,
    0x80049f5c,0x8004a0ec,0x8002709c,0x801e7914,0x80027d64,0x801e7ec4,0x800445d0};
static const unsigned argcounts[]={2,1,9,1,2,3,1,1,4,4,2,1,3,13,15,9,3,1};
typedef struct ResourceTrace {
 uint32_t target,ra,sp,args[15],saved[9],hi,lo;
 uint8_t frame[0xc0],ram[32],shared[32],pad[32],host[32];
} ResourceTrace;
static ResourceTrace traces[64];
static void fail(const char *what,uint32_t v)
{fprintf(stderr,"RESOURCE INIT FAIL variant=%u descriptor=%u mode=%u early=%u perturb=%u call=%u %s %08x\n",variant,descriptor_case,service_mode,early,perturb,calls,what,v);failed=1;}
static uint32_t read_at(uint32_t p,unsigned w)
{uint32_t v;assert(runtime_read(&rt,p,w,&v)==0);return v;}
static void write_at(uint32_t p,unsigned w,uint32_t v)
{int rc=runtime_write(&rt,p,w,v);if(rc){fail("fixture/model write",p);fprintf(stderr,"width=%u value=%08x candidate=%d\n",w,v,candidate);}assert(rc==0);}
static uint32_t domain(unsigned off)
{
 switch(variant%4){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;
 case 3:return (uint32_t)(uintptr_t)host+off;default:return 0x80100000u+off;}
}
static void snapshot(void)
{memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void)
{return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static void record_call(PcPortMipsCpu *cpu,uint32_t target,unsigned argc)
{
 assert(calls<64);ResourceTrace row={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.saved,cpu->gpr+16,8*4);row.saved[8]=cpu->gpr[30];
 for(unsigned i=0;i<argc;i++)row.args[i]=i<4?cpu->gpr[4+i]:read_at(cpu->gpr[29]+i*4u,4);
 memcpy(row.frame,resolve_memory(&rt,frame_base,sizeof(row.frame),0),sizeof(row.frame));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare && memcmp(&row,&traces[calls],sizeof(row))) {
  ResourceTrace *expected=&traces[calls];
  if(row.target!=expected->target)fail("target",expected->target);
  if(row.ra!=expected->ra||row.sp!=expected->sp)fail("RA/SP",row.ra);
  for(unsigned i=0;i<15;i++)if(row.args[i]!=expected->args[i]){fail("argument",i);fprintf(stderr,"got %08x expected %08x\n",row.args[i],expected->args[i]);}
  for(unsigned i=0;i<9;i++)if(row.saved[i]!=expected->saved[i]){fail("saved register",i);fprintf(stderr,"got %08x expected %08x\n",row.saved[i],expected->saved[i]);}
  for(unsigned i=0;i<sizeof(row.frame);i++)if(row.frame[i]!=expected->frame[i]){fail("frame",i);break;}
  fail("call or full memory hash",target);
 }
 if(!compare)traces[calls]=row;
 calls++;
}
static void *copy_bytes(void *to,const void *from,size_t size){return memcpy(to,from,size);}
static void unused_unpin(void){abort();}
static void color_matrix(MATRIX *m)
{
 assert(m==module_memory_pointer(&rt,rt.bridge_cpu->gpr[4],sizeof(MATRIX),0));
 for(unsigned i=0;i<5;i++){uint32_t word;memcpy(&word,(uint8_t *)m+i*4u,4);CTC2(word,16+(int)i);}
}
static void back_color(int r,int g,int b){CTC2((uint32_t)r<<4,13);CTC2((uint32_t)g<<4,14);CTC2((uint32_t)b<<4,15);}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(stop_target && target-stop_target<12u)return 0;
 if(target==0x801e7210u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x801e7210u<1796u)return 0;
 unsigned id;for(id=0;id<sizeof(targets)/sizeof(targets[0]);id++)if(target==targets[id])break;
 if(id==sizeof(targets)/sizeof(targets[0])){fail("unexpected callee",target);return -1;}
 record_call(cpu,target,argcounts[id]);if(failed)return -1;visited|=UINT64_C(1)<<id;
 uint32_t a[15]={0};for(unsigned i=0;i<argcounts[id];i++)a[i]=i<4?cpu->gpr[4+i]:read_at(cpu->gpr[29]+i*4u,4);
 if(service_mode && (target==0x800320b8u||target==0x8003f968u||target==0x80049f5cu||target==0x8004a0ecu)) {
  if(candidate){if(runtime_bridge(opaque,cpu,target)!=1){fail("concrete interface",target);return -1;}}
  else if(target==0x800320b8u){cpu->gpr[3]=0xfdffffffu;cpu->gpr[2]=read_at(a[0]-4u,4)&cpu->gpr[3];write_at(a[0]-4u,4,cpu->gpr[2]);}
  else if(target==0x8003f968u){memcpy(resolve_memory(&rt,a[0],a[2],1),resolve_memory(&rt,a[1],a[2],0),a[2]);cpu->gpr[2]=a[0];cpu->gpr[3]=0;}
  else if(target==0x80049f5cu){for(unsigned i=0;i<5;i++)cop_control[16+i]=read_at(a[0]+i*4u,4);}
  else {for(unsigned i=0;i<3;i++)cop_control[13+i]=a[i]<<4;}
 } else {
  cpu->gpr[2]=0x94000000u+calls;cpu->gpr[3]=0x64000000u+calls;cpu->hi=0x12340000u+calls;cpu->lo=0x56780000u+calls;
  switch(target){
  case 0x80032498u:if(perturb&1u)write_at(frame_base+0x58u,4,0);break;
  case 0x800320b8u:write_at(a[0]-4u,4,read_at(a[0]-4u,4)&0xfdffffffu);break;
  case 0x800a8bf0u:
   write_at(0x800d33e4u,4,actor);write_at(actor,4,domain(0x1e000));write_at(actor+4u,4,records);break;
  case 0x801e70e8u:write_at(a[0],4,read_at(a[0],4)^0x1234u);break;
  case 0x80031bdcu:cpu->gpr[2]=allocation_fail?0:destination;break;
  case 0x8003f968u:memcpy(resolve_memory(&rt,a[0],a[2],1),resolve_memory(&rt,a[1],a[2],0),a[2]);break;
  case 0x800320e8u:if(a[0])write_at(a[0]-4u,4,read_at(a[0]-4u,4)^0x80u);break;
  case 0x8003342cu:
   for(unsigned i=1;i<=read_at(a[0],4);i++)write_at(a[0]+i*4u,4,read_at(a[0]+i*4u,4)+a[0]);
   if(perturb&2u)write_at(frame_base+0x58u,4,0);
   break;
  case 0x800aa898u:write_at(a[0]+0x18u,2,read_at(a[2],2));break;
  case 0x800aa934u:write_at(a[0]+0x1cu,2,0xff00u+variant);break;
  case 0x8009ef3cu:write_at(a[0]+4u,4,a[1]);break;
  case 0x80049f5cu:write_at(a[0]+18u,2,0x1234u+variant);break;
  case 0x8004a0ecu:
   if(perturb&4u)write_at(destination+0x360u+40u,2,0xabcd);
   if(perturb&8u){write_at(0x800658c8u,4,source);write_at(source+0x340u,1,read_at(source+0x340u,1)+1u);}
   break;
  case 0x8002709cu:write_at(a[8]+2u,2,a[12]);cpu->gpr[2]=domain(0x20000)+calls*4u;break;
  case 0x801e7914u:write_at(a[11]+2u,2,a[14]);cpu->gpr[2]=domain(0x21000);break;
  case 0x80027d64u:write_at(a[0]+20u,4,a[8]);break;
  case 0x801e7ec4u:write_at(a[0],4,a[2]);break;
  case 0x800445d0u:break;
  default:abort();
  }
 }
 if(pending_return==target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;cpu->load_mask=0;}
 return 1;
}
static const uint16_t descriptors[][6]={
 {0,0,0,0,0,0},{1,2,3,5,7,9},{1,1,1,1,1,1},{2,2,2,2,2,2},
 {3,3,3,3,3,3},{7,7,7,7,7,7},{5,0,0,0,0,0},{8,0xffff,4,6,0,0},
 {7,1,2,3,5,0},{0,7,0,0,0,0},{0,0,7,0,0,0},{0,0,0,7,0,0},
 {2,3,7,5,1,2},{3,2,1,7,5,3},{5,3,2,1,7,5},{7,5,3,2,1,7}};
static void setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));
 memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memset(cop_data,0x17,sizeof(cop_data));memset(cop_control,0x43,sizeof(cop_control));
 memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};rt.data[1]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 const ResolvedFunction funcs[]={{0x800320b8,unused_unpin,"HeapUnpinBlock"},{0x8003f968,copy_bytes,"memcpy"},{0x80049f5c,color_matrix,"SetColorMatrix"},{0x8004a0ec,back_color,"SetBackColor"}};
 memcpy(rt.functions,funcs,sizeof(funcs));rt.function_count=4;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&4u)cpu->gpr[29]=0x1f800e00u;else if(variant&8u)cpu->gpr[29]|=0x20000000u;
 frame_base=cpu->gpr[29]-0xa0u;calls=0;stop_target=pending_return=0;
 source=domain(0x1004);destination=domain(0x4000);auxiliary=domain(0x8008);
 actor=domain(0xa000);records=domain(0xb000);positions=domain(0xd000);
 for(unsigned i=0;i<0x1000;i++)write_at(source+i,1,i*17u+variant);
 write_at(source-4u,4,0x1000);write_at(0x8005949cu,4,source);
 write_at(auxiliary-4u,4,0xefffffffu);write_at(auxiliary+4u,4,domain(0xf000));write_at(auxiliary+20u,4,positions);
 unsigned counts[]={0,1,2,4,16};write_at(records+10u,2,counts[(variant/4)%5]);
 for(unsigned i=0;i<32;i++)for(unsigned k=0;k<4;k++)write_at(positions+i*8u+k*2u,2,0x7ff0u+i*31u+k*7u+variant);
 write_at(source+0x50cu,4,(variant&16u)?0:0x600);write_at(source+0x510u,4,(variant&32u)?0:0x700);write_at(source+0x514u,4,0x800);
 write_at(source+0x700u,4,3);write_at(source+0x800u,4,1);write_at(source+0x804u,4,0x20);write_at(source+0x820u,4,0x10);write_at(source+0x830u,2,0x1234);
 for(unsigned i=0;i<6;i++){
  uint32_t d=source+0x360u+i*40u;for(unsigned k=0;k<20;k++)write_at(d+k*2u,2,0x7ff0u+variant*3u+k*5u+i*7u);
  write_at(d+24u,2,descriptors[descriptor_case][i]);
 }
 write_at(source+0x35eu,1,(variant&64u)?0x80:0);write_at(source+0x458u,1,17);write_at(source+0x459u,1,38);write_at(source+0x45au,1,59);
 cpu->gpr[4]=0x8005949cu;cpu->gpr[5]=0xdeadbeefu;cpu->gpr[6]=auxiliary;cpu->gpr[7]=(variant&128u)?destination+0x464u:0x800ccb94u;
 write_at(cpu->gpr[29]+16u,4,(variant&128u)?destination+0x46cu:0x800ccbb4u);
 write_at(cpu->gpr[29]+20u,4,(variant&32u)?0:0x800c4a39u);
 if(early==1)cpu->gpr[6]=0;
 if(early==2)write_at(cpu->gpr[4],4,0);
 if(early==3)cpu->gpr[4]=0;
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;
 int rc=PcPortMipsRun(&oracle,0x801e7210u,BATTLE_HALT_PC,100000);
 if(rc!=PC_PORT_MIPS_HALTED){fail(oracle.error,oracle.pc);return 0;}
 unsigned expected_calls=calls;uint32_t expected_cop[32];memcpy(expected_cop,cop_control,sizeof(expected_cop));snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 rc=PcPortMipsRun(&native,0x801e7210u,BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED){fail(native.error,native.pc);return 0;}
 if(native.steps||rt.bridge_cpu||calls!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native context/count/instructions",calls);
 for(unsigned i=16;i<32;i++)if(native.gpr[i]!=oracle.gpr[i])fail("preserved register",i);
 if(native.gpr[2]!=oracle.gpr[2]||native.gpr[3]!=oracle.gpr[3]||native.hi!=oracle.hi||native.lo!=oracle.lo){fail("return/HI/LO",native.gpr[2]);fprintf(stderr,"native v0/v1=%08x/%08x expected=%08x/%08x\n",native.gpr[2],native.gpr[3],oracle.gpr[2],oracle.gpr[3]);}
 if(!memory_equal())fail("final memory",0);
 if(memcmp(expected_cop,cop_control,sizeof(expected_cop)))fail("color register interface",0);
 return !failed;
}
static int original_unpin(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{(void)opaque;(void)cpu;return target-0x800320b8u<24u?0:-1;}
static int unpin_cases(void)
{
 uint8_t code[24];FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);assert(fseek(f,0x800+0x320b8-0x10000,SEEK_SET)==0);assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);
 const uint32_t values[]={0,0xffffffff,0x02000000,0xfdffffff,0x12345678,0xdeadbeef};unsigned count=0;
 for(unsigned d=0;d<6;d++)for(unsigned v=0;v<6;v++){
  PcPortMipsCpu oracle,native;variant=0;early=perturb=allocation_fail=0;setup(&oracle);
  uint32_t ptrs[]={0x80160008,0xa0160008,0x80360008,0x1f800808,(uint32_t)(uintptr_t)host+8,0x80060008};
  uint32_t address=ptrs[d];write_at(address-4u,4,values[v]);memcpy(PSX_ADDR(0x800320b8u),code,sizeof(code));
  oracle.gpr[4]=address;oracle.bus.bridge=original_unpin;assert(PcPortMipsRun(&oracle,0x800320b8u,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);snapshot();
  setup(&native);write_at(address-4u,4,values[v]);memcpy(PSX_ADDR(0x800320b8u),code,sizeof(code));native.gpr[4]=address;native.bus.bridge=runtime_bridge;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&native,0x800320b8u,BATTLE_HALT_PC,100);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_HALTED||native.steps||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||!memory_equal()||memcmp(oracle.gpr+1,native.gpr+1,31*4u)){fail("original unpin leaf",address);return 0;}count++;
 }
 /* Explicit pointer services also accept physical addresses; the generic
  * original-body bus uses their canonical KSEG alias. */
 PcPortMipsCpu cpu;setup(&cpu);cpu.bus.bridge=runtime_bridge;cpu.gpr[4]=0x00160008;write_at(0x80160004,4,0xffffffff);
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 if(PcPortMipsRun(&cpu,0x800320b8u,BATTLE_HALT_PC,100)!=PC_PORT_MIPS_HALTED||read_at(0x80160004,4)!=0xfdffffffu||cpu.steps){fail("physical unpin",cpu.gpr[2]);return 0;}
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 printf("RESOURCE INIT PASS %u original unpin leaf comparisons plus physical pointer service; native steps=0\n",count);
 return !failed;
}
static int controls(void)
{
 PcPortMipsCpu cpu;variant=0;descriptor_case=8;service_mode=early=perturb=allocation_fail=0;setup(&cpu);candidate=1;compare=0;rt.bridge_cpu=&cpu;
 const uint32_t starts[]={0x801e7210,0x801e4000,0x801e5840};const unsigned lengths[]={1796,32,228};
 snapshot();
 for(unsigned r=0;r<3;r++){
  unsigned offsets[]={0,lengths[r]/2,lengths[r]-1};
  for(unsigned k=0;k<3;k++){uint8_t *p=PSX_ADDR(starts[r]+offsets[k]);*p^=1;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("identity accepted",starts[r]+offsets[k]);*p^=1;}
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("pending load",0);cpu.load_valid=0;
 rt.force_interpret=1;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("forced interpreter",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("uninitialized",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("inactive runtime",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("wrong context",0);rt.bridge_cpu=&cpu;
 assert(setenv("XENO_BATTLE_NATIVE_RESOURCE_INIT","0",1)==0);if(battle_resource_init_try_host(&rt,&cpu,0x801e7210u))fail("disabled control",0);assert(unsetenv("XENO_BATTLE_NATIVE_RESOURCE_INIT")==0);
 if(battle_resource_init_try_host(&rt,&cpu,0x801e7214u)||!memory_equal()||calls)fail("gate side effects",calls);
 setup(&cpu);candidate=compare=0;visited=0;
 if(PcPortMipsRun(&cpu,0x801e7210u,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_HALTED||visited!=0x3ffffu){fail("refusal fixture coverage",(uint32_t)visited);return 0;}
 unsigned count=calls;uint32_t order[64];for(unsigned i=0;i<count;i++)order[i]=traces[i].target;
 for(unsigned id=0;id<18;id++)for(unsigned pending=0;pending<2;pending++){
  setup(&cpu);candidate=1;compare=0;if(pending)pending_return=targets[id];else stop_target=targets[id];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x801e7210u,BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  unsigned at=0;while(at<count&&order[at]!=targets[id])at++;
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||cpu.steps||rt.bridge_cpu||calls!=at+pending||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions){fail("nested refusal/retry",targets[id]);return 0;}
 }
 for(unsigned fault=0;fault<5;fault++){
  PcPortMipsCpu original,native;setup(&original);candidate=compare=0;
  if(fault==0){original.gpr[4]=0x1e000000;original.gpr[6]=0;}
  if(fault==1)original.gpr[29]=0x1f801050u;
  if(fault==2)write_at(0x8005949cu,4,0x1f800000u);
  if(fault==3)original.gpr[7]=0x1f800ff8u;
  if(fault==4)write_at(original.gpr[29]+16u,4,0x1f800ff8u);
  int orc=PcPortMipsRun(&original,0x801e7210u,BATTLE_HALT_PC,100000);if(orc==PC_PORT_MIPS_HALTED){fail("original fault missing",fault);return 0;}
  snapshot();unsigned expected_calls=calls;setup(&native);candidate=1;compare=0;
  if(fault==0){native.gpr[4]=0x1e000000;native.gpr[6]=0;}
  if(fault==1)native.gpr[29]=0x1f801050u;
  if(fault==2)write_at(0x8005949cu,4,0x1f800000u);
  if(fault==3)native.gpr[7]=0x1f800ff8u;
  if(fault==4)write_at(native.gpr[29]+16u,4,0x1f800ff8u);
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&native,0x801e7210u,BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL||native.steps||rt.bridge_cpu||calls!=expected_calls||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions){fail("partial fault state",fault);return 0;}
 }
 printf("RESOURCE INIT PASS body/module/table identities, context gates, 36 nested refusals and five partial-write faults\n");
 return !failed;
}

static uint32_t abi_words[13],abi_return;
static void *abi_pointer_result;
static unsigned abi_invoked;
static void *abi_horizon(int32_t a0,int32_t a1,int32_t a2,int32_t a3,int32_t a4,int32_t a5,
 int32_t a6,int32_t a7,int16_t *coords,uint8_t *colors,int32_t a10,int32_t a11,int32_t a12)
{
 const uint32_t a[]={(uint32_t)a0,(uint32_t)a1,(uint32_t)a2,(uint32_t)a3,(uint32_t)a4,(uint32_t)a5,(uint32_t)a6,(uint32_t)a7};
 assert(!memcmp(a,abi_words,sizeof(a)));assert((uint32_t)a10==abi_words[10]&&(uint32_t)a11==abi_words[11]&&(uint32_t)a12==abi_words[12]);
 assert(coords==module_memory_pointer(&rt,abi_words[8],12,0));assert(colors==(abi_words[9]?module_memory_pointer(&rt,abi_words[9],11,0):NULL));
 abi_invoked++;return abi_pointer_result;
}
static LineScroll *abi_line(LineScroll *line,uint16_t x,uint16_t y,uint16_t modulus,
 int16_t width,int16_t lines,uint16_t dx,uint16_t dy,int8_t *data)
{
 assert(line==module_memory_pointer(&rt,abi_words[0],sizeof(*line),1));
 assert(x==(uint16_t)abi_words[1]&&y==(uint16_t)abi_words[2]&&modulus==(uint16_t)abi_words[3]);
 assert(width==(int16_t)abi_words[4]&&lines==(int16_t)abi_words[5]&&dx==(uint16_t)abi_words[6]&&dy==(uint16_t)abi_words[7]);
 assert(data==(abi_words[8]?module_memory_pointer(&rt,abi_words[8],1,0):NULL));abi_invoked++;return abi_pointer_result;
}
static unsigned int abi_relocate(uint32_t *p)
{assert(p==module_memory_pointer(&rt,abi_words[0],4,1));abi_invoked++;return abi_return;}
static uint32_t abi_actor(uint8_t *p,uint32_t a1,uint32_t a2,uint32_t a3)
{assert(p==module_memory_pointer(&rt,abi_words[0],0x9a,1));assert(a1==abi_words[1]&&a2==abi_words[2]&&a3==abi_words[3]);abi_invoked++;return abi_return;}
static void abi_setup(PcPortMipsCpu *cpu,unsigned index)
{
 setup(cpu);cpu->bus.bridge=runtime_bridge;abi_invoked=0;
 const ResolvedFunction funcs[]={{0x8002709c,abi_horizon,"func_8002709C"},{0x80027d64,abi_line,"GfxLineScrollInitialize"},{0x8003342c,abi_relocate,"ResolveArchiveEntryPointers"},{0x800aa898,abi_actor,"func_800AA898"}};
 memcpy(rt.functions,funcs,sizeof(funcs));rt.function_count=4;
 unsigned argc=index==0?13:index==1?9:index==2?1:4;
 for(unsigned i=0;i<argc;i++){if(i<4)cpu->gpr[4+i]=abi_words[i];else write_at(cpu->gpr[29]+i*4u,4,abi_words[i]);}
}
static int abi_run(PcPortMipsCpu *cpu,unsigned index,int good)
{
 const uint32_t targets[]={0x8002709c,0x80027d64,0x8003342c,0x800aa898};
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(cpu,targets[index],BATTLE_HALT_PC,100);
 if(rc!=(good?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL)||cpu->steps||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||abi_invoked!=(unsigned)good){fail("resource ABI status",targets[index]);return 0;}
 return 1;
}
static int resource_abi_cases(void)
{
 const uint32_t pointers[]={0x80160020,0xa0160020,0x00160020,0x1f800820,(uint32_t)(uintptr_t)host+0x20};
 const uint32_t values[]={0,1,0x7fff,0x8000,0xffff,0xffffffff,0x80000000,0x80100000};
 PcPortMipsCpu cpu;variant=early=perturb=allocation_fail=0;unsigned cases=0;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(unsigned index=0;index<4;index++)for(unsigned d=0;d<5;d++)for(unsigned v=0;v<8;v++){
  for(unsigned i=0;i<13;i++)abi_words[i]=values[(v+i)%8];
  if(index==0){abi_words[8]=pointers[d];abi_words[9]=(v&1)?0:pointers[d]+16u;}
  else {abi_words[0]=pointers[d];if(index==1)abi_words[8]=(v&1)?0:pointers[d]+32u;}
  abi_pointer_result=(v&1)?NULL:PSX_ADDR(0x80180000u);abi_return=values[v];abi_setup(&cpu,index);
  if(!abi_run(&cpu,index,1))return 0;
  if(cpu.gpr[2]!=(index<2?((v&1)?0:0x80180000u):abi_return)||cpu.gpr[3]){fail("resource ABI return",cpu.gpr[2]);return 0;}cases++;
 }
 for(unsigned index=0;index<4;index++){
  for(unsigned i=0;i<13;i++)abi_words[i]=0;
  if(index==0)abi_words[8]=pointers[0];else abi_words[0]=pointers[0];
  abi_setup(&cpu,index);cpu.load_valid=1;cpu.load_reg=4;if(!abi_run(&cpu,index,0))return 0;
  abi_words[index==0?8:0]=0x1e000000u;abi_setup(&cpu,index);if(!abi_run(&cpu,index,0))return 0;
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 printf("RESOURCE INIT PASS %u concrete resource ABI cases, eight refusals; callee behavior modeled; native steps=0\n",cases);
 return !failed;
}

static void *abi_copy(void *d,const void *s,size_t n){abi_invoked++;return memcpy(d,s,n);}
static int copy_abi_cases(void)
{
 const uint32_t srcs[]={0x80160000,0xa0160000,0x00160000,0x1f800800,(uint32_t)(uintptr_t)host+0x200};
 const uint32_t dsts[]={0x80170000,0xa0170000,0x00170000,0x1f800a00,(uint32_t)(uintptr_t)host+0x400};
 const unsigned sizes[]={0,1,32};unsigned cases=0;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(unsigned a=0;a<5;a++)for(unsigned b=0;b<5;b++)for(unsigned n=0;n<3;n++){
  PcPortMipsCpu cpu;setup(&cpu);cpu.bus.bridge=runtime_bridge;abi_invoked=0;
  rt.functions[0]=(ResolvedFunction){0x8003f968,abi_copy,"memcpy"};rt.function_count=1;
  uint8_t *src=module_memory_pointer(&rt,srcs[a],32,1),*dst=module_memory_pointer(&rt,dsts[b],40,1);
  assert(src&&dst);for(unsigned i=0;i<32;i++)src[i]=(uint8_t)(i*7u+1u);memset(dst,0x96,40);
  cpu.gpr[4]=dsts[b];cpu.gpr[5]=srcs[a];cpu.gpr[6]=sizes[n];PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x8003f968,BATTLE_HALT_PC,100);
  if(rc!=PC_PORT_MIPS_HALTED||cpu.steps||rt.bridge_cpu||cpu.gpr[2]!=dsts[b]||cpu.gpr[3]||abi_invoked!=(sizes[n]!=0)||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions){fail("copy ABI",dsts[b]);return 0;}
  for(unsigned i=0;i<40;i++){if(dst[i]!=(i<sizes[n]?src[i]:0x96)){fail("copy range",i);return 0;}}
  cases++;
 }
 for(unsigned test=0;test<4;test++){
  PcPortMipsCpu cpu;setup(&cpu);cpu.bus.bridge=runtime_bridge;abi_invoked=0;
  rt.functions[0]=(ResolvedFunction){0x8003f968,abi_copy,"memcpy"};rt.function_count=1;
  cpu.gpr[4]=test==0?0x1f800fffu:0x80170000;cpu.gpr[5]=test==1?0x1e000000:0x80160000;cpu.gpr[6]=4;
  if(test==2){cpu.load_valid=1;cpu.load_reg=4;}
  if(test==3){cpu.gpr[4]=cpu.gpr[5]=0x1e000000;cpu.gpr[6]=0;}
  snapshot();int rc=PcPortMipsRun(&cpu,0x8003f968,BATTLE_HALT_PC,100);
  if(rc!=(test==3?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL)||cpu.steps||abi_invoked||!memory_equal()){fail("copy refusal/zero",test);return 0;}
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 printf("RESOURCE INIT PASS %u memcpy ABI cases, three refusals and zero-length invalid-pointer case; native steps=0\n",cases);
 return !failed;
}

int main(int argc,char **argv)
{
 FILE *f=fopen("scratchpad/native-module-20261017/module-test-2/module.bin","rb");assert(f&&fread(module,1,sizeof(module),f)==sizeof(module));fclose(f);
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 unsigned total=0;
 if(argc==2 && strcmp(argv[1],"--checks")==0)return unpin_cases()&&resource_abi_cases()&&copy_abi_cases()&&controls()?0:1;
 if(argc==2 && strcmp(argv[1],"--controls")==0)return controls()?0:1;
 if(argc==2){variant=(unsigned)strtoul(argv[1],NULL,0);descriptor_case=(variant/4)%16;perturb=(variant/16)%16;return run_case()?0:1;}
 for(service_mode=0;service_mode<2;service_mode++)for(variant=0;variant<256;variant++){
  descriptor_case=(variant/4)%16;early=allocation_fail=0;perturb=(variant/16)%16;
  if(!run_case())return 1;
  total++;
  if((variant&63u)==63u){printf("RESOURCE INIT completed %u caller cases service-mode=%u\n",total,service_mode);fflush(stdout);}
 }
 perturb=0;descriptor_case=1;
 for(variant=0;variant<16;variant++)for(early=1;early<3;early++){if(!run_case())return 1;total++;}
 early=0;allocation_fail=1;for(variant=0;variant<16;variant++){if(!run_case())return 1;total++;}
 printf("RESOURCE INIT PASS %u complete original-caller comparisons; external bodies modeled; native steps=0; visited=%llx\n",total,(unsigned long long)visited);
 if(!unpin_cases()||!resource_abi_cases()||!copy_abi_cases()||!controls())return 1;
 return failed?1:0;
}
