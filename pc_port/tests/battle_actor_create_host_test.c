/* Complete original 800A8BF0 caller versus structured C. External actor,
 * geometry and resource services are observable models; a separate mode uses
 * the actual memcpy interface. This is not callee adoption evidence. */
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
static uint8_t body[2384],shared[0x30000],want_shared[sizeof(shared)];
static uint8_t want_ram[PSX_RAM_SIZE],want_pad[4096];
static _Alignas(8) uint8_t host[0x80000],want_host[sizeof(host)];
static unsigned variant,calls,service_mode,perturb,early,fault,capacity,part_call,alloc_call;
static int candidate,compare,failed,modeled_refusal,last_refused;
static uint32_t stop_target,pending_return,frame_base,model,sound,table,header,actor,geometry,source,positions;
static uint64_t visited;
static const uint32_t targets[]={0x80032498,0x80031bdc,0x8003342c,0x8003864c,0x80038428,
 0x80030988,0x8002dde4,0x8003f968,0x8009eba8,0x8009ec4c,0x80043cb0,0x80043bfc,
 0x80043a58,0x80043a1c,0x800aa6e0,0x800a7064,0x800aa898,0x800aa934,0x800aff9c,
 0x8002c644,0x8002c4bc,0x80031894,0x800320e8,0x8009f794};
static const unsigned argcounts[]={2,2,1,2,1,4,7,3,2,8,1,2,2,4,1,20,4,4,1,1,1,1,1,2};
typedef struct ActorTrace {uint32_t target,ra,sp,args[20],saved[9],hi,lo;uint8_t frame[0x100],ram[32],shared[32],pad[32],host[32];} ActorTrace;
static ActorTrace traces[512];
static void fail(const char *what,uint32_t value)
{fprintf(stderr,"ACTOR CREATE FAIL variant=%u mode=%u perturb=%u early=%u fault=%u call=%u %s %08x\n",variant,service_mode,perturb,early,fault,calls,what,value);failed=1;}
static uint32_t read_at(uint32_t p,unsigned w){uint32_t v;int rc=runtime_read(&rt,p,w,&v);if(rc){fail("fixture read",p);fprintf(stderr,"width=%u candidate=%d\n",w,candidate);}assert(rc==0);return v;}
static void write_at(uint32_t p,unsigned w,uint32_t v){int rc=runtime_write(&rt,p,w,v);if(rc)fail("fixture write",p);assert(rc==0);}
static uint32_t domain(unsigned off)
{switch(variant%4){case 1:return 0xa0100000u+off;case 2:return 0x80300000u+off;case 3:return (uint32_t)(uintptr_t)host+off;default:return 0x80100000u+off;}}
static void snapshot(void){memcpy(want_ram,g_PsxRam,sizeof(want_ram));memcpy(want_shared,shared,sizeof(shared));memcpy(want_pad,g_PsxScratchpad,sizeof(want_pad));memcpy(want_host,host,sizeof(host));}
static int memory_equal(void){return !memcmp(want_ram,g_PsxRam,sizeof(want_ram))&&!memcmp(want_shared,shared,sizeof(shared))&&!memcmp(want_pad,g_PsxScratchpad,sizeof(want_pad))&&!memcmp(want_host,host,sizeof(host));}
static void record_call(PcPortMipsCpu *cpu,uint32_t target,unsigned argc)
{
 assert(calls<512);ActorTrace row={.target=target,.ra=cpu->gpr[31],.sp=cpu->gpr[29],.hi=cpu->hi,.lo=cpu->lo};
 memcpy(row.saved,cpu->gpr+16,32);row.saved[8]=cpu->gpr[30];
 for(unsigned i=0;i<argc;i++)row.args[i]=i<4?cpu->gpr[4+i]:read_at(cpu->gpr[29]+i*4u,4);
 void *frame=resolve_memory(&rt,frame_base,sizeof(row.frame),0);if(frame)memcpy(row.frame,frame,sizeof(row.frame));
 assert(SHA256(g_PsxRam,sizeof(g_PsxRam),row.ram));assert(SHA256(shared,sizeof(shared),row.shared));
 assert(SHA256(g_PsxScratchpad,sizeof(g_PsxScratchpad),row.pad));assert(SHA256(host,sizeof(host),row.host));
 if(compare&&memcmp(&row,&traces[calls],sizeof(row))){ActorTrace *want=&traces[calls];
  if(row.target!=want->target)fail("target",want->target);
  if(row.ra!=want->ra||row.sp!=want->sp)fail("RA/SP",row.ra);
  for(unsigned i=0;i<20;i++)if(row.args[i]!=want->args[i]){fail("argument",i);fprintf(stderr,"got=%08x want=%08x\n",row.args[i],want->args[i]);}
  for(unsigned i=0;i<9;i++)if(row.saved[i]!=want->saved[i]){fail("saved",i);fprintf(stderr,"got=%08x want=%08x\n",row.saved[i],want->saved[i]);}
  for(unsigned i=0;i<sizeof(row.frame);i++)if(row.frame[i]!=want->frame[i]){fail("frame",i);fprintf(stderr,"got=%02x want=%02x\n",row.frame[i],want->frame[i]);break;}
  fail("ordered call/full memory/HI-LO",target);
 }
 if(!compare)traces[calls]=row;
 calls++;
}
static void *copy_bytes(void *to,const void *from,size_t size){return memcpy(to,from,size);}
static int bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(stop_target&&target-stop_target<12u)return 0;
 if(target==0x800a8bf0u)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(target-0x800a8bf0u<2384u)return 0;
 unsigned id;for(id=0;id<sizeof(targets)/sizeof(targets[0]);id++)if(target==targets[id])break;
 if(id==sizeof(targets)/sizeof(targets[0])){fail("unexpected callee",target);return -1;}
 record_call(cpu,target,argcounts[id]);if(failed)return -1;visited|=UINT64_C(1)<<id;
 uint32_t a[20]={0};for(unsigned i=0;i<argcounts[id];i++)a[i]=i<4?cpu->gpr[4+i]:read_at(cpu->gpr[29]+i*4u,4);
 /* A fully occupied 20-entry list aliases the adjacent count/resource globals.
  * Compare an explicit modeled callee refusal at that original boundary. */
 if(target==0x8009eba8u&&cpu->gpr[31]==0x800a8ed8u&&a[1]==0x800c3b6cu){modeled_refusal=1;return -1;}
 if(service_mode&&target==0x8003f968u){
  if(candidate){if(runtime_bridge(opaque,cpu,target)!=1){fail("native copy interface",target);return -1;}}
  else{memcpy(resolve_memory(&rt,a[0],a[2],1),resolve_memory(&rt,a[1],a[2],0),a[2]);cpu->gpr[2]=a[0];cpu->gpr[3]=0;}
 }else{
  cpu->gpr[2]=0x94000000u+calls;cpu->gpr[3]=0x64000000u+calls;cpu->hi=0x12340000u+calls;cpu->lo=0x56780000u+calls;
  switch(target){
  case 0x80032498u:
   if(perturb&1u){write_at(frame_base+0x50u,4,3);write_at(frame_base+0x60u,2,0x8001);}
   break;
  case 0x80031bdcu:{
   alloc_call++;uint32_t result=domain(0x30000u+alloc_call*0x4000u);
   if(cpu->gpr[31]==0x800a8c94u)result=actor;
   if(fault==2&&alloc_call==1)result=0;
   if(cpu->gpr[31]==0x800a9164u&&(perturb&2u))write_at(actor+0x10eu,1,1);
   if(cpu->gpr[31]==0x800a9200u&&(perturb&4u))write_at(actor+0x10du,1,1);
   cpu->gpr[2]=result;break;}
  case 0x8003342cu:
   /* Visible relocation boundary with pre-resolved fixture tables. */
   write_at(a[0],4,read_at(a[0],4)^0x100);break;
  case 0x8003864cu:cpu->gpr[2]=(variant>>7)&1u;if(perturb&2u)write_at(a[0]+4u,4,0x12340000u);break;
  case 0x80038428u:write_at(a[0]+4u,4,0x88776655u);break;
  case 0x80030988u:if(perturb&4u)write_at(actor+0x4au,2,read_at(actor+0x4au,2)^4u);break;
  case 0x8002dde4u:write_at(a[0]+8u,4,a[4]^a[5]^a[6]);break;
  case 0x8003f968u:memcpy(resolve_memory(&rt,a[0],a[2],1),resolve_memory(&rt,a[1],a[2],0),a[2]);break;
  case 0x8009eba8u:write_at(a[1],4,a[0]);write_at(a[1]+4u,4,0x12340000u+calls);break;
  case 0x8009ec4cu:write_at(geometry+0x50u,4,a[4]^a[5]^a[6]^a[7]);cpu->gpr[2]=geometry;break;
  case 0x80043cb0u:write_at(a[0]+3u,1,9);write_at(a[0]+7u,1,0x2c);break;
  case 0x80043bfcu:write_at(a[0]+7u,1,read_at(a[0]+7u,1)|2u);if(perturb&8u)write_at(0x800658c8u,4,domain(0xe000));break;
  case 0x80043a58u:cpu->gpr[2]=(a[1]<<6)|((a[0]>>4)&63u);break;
  case 0x80043a1cu:cpu->gpr[2]=((a[0]&3u)<<7)|((a[1]&3u)<<5)|((a[2]>>6)&15u)|((a[3]&0x100u)>>4);break;
  case 0x800aa6e0u:write_at(a[0]+0x22u,1,0x17);if(perturb&8u)write_at(header+16u,1,2);break;
  case 0x800a7064u:{uint32_t keys=domain(0x10000u+part_call++*0x100u);write_at(a[0]+24u,4,keys);write_at(a[0]+4u,4,a[19]);if(perturb&16u)write_at(actor+0x10du,1,1);break;}
  case 0x800aa898u:write_at(a[0]+0x18u,2,read_at(a[2],2));break;
  case 0x800aa934u:write_at(a[0]+0x1au,2,a[2]);break;
  case 0x800aff9cu:write_at(a[0]+0x22u,1,0x18);break;
  case 0x8002c644u:write_at(a[0]+4u,4,read_at(a[0]+4u,4)^0x1000u);break;
  case 0x8002c4bcu:write_at(a[0]+8u,4,read_at(a[0]+8u,4)^0x2000u);break;
  case 0x80031894u:cpu->gpr[2]=0x100;break;
  case 0x800320e8u:write_at(a[0]-4u,4,0xfafafafa);break;
  case 0x8009f794u:write_at(a[0],4,0);break;
  default:abort();
  }
 }
 if(pending_return==target){cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=0xfeedbeef;cpu->load_mask=0;}
 return 1;
}
static void setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));memset(host,0xb6,sizeof(host));
 memcpy(PSX_ADDR(0x800a8bf0u),body,sizeof(body));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};rt.data[1]=(ResolvedData){0x800c0000u,0x20000u,shared+0x10000u,"shared-cd"};rt.data_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;assert((uintptr_t)host<=UINT32_MAX);rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 rt.functions[0]=(ResolvedFunction){0x8003f968,copy_bytes,"memcpy"};rt.function_count=1;
 initialize_cpu(cpu,&rt);cpu->bus.bridge=bridge;
 for(unsigned i=1;i<29;i++)cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&4u)cpu->gpr[29]=0x1f800d00u;else if(variant&8u)cpu->gpr[29]|=0x20000000u;
 if(fault==1)cpu->gpr[29]=0x1f800030u;
 frame_base=cpu->gpr[29]-0xc8u;calls=part_call=alloc_call=0;stop_target=pending_return=0;modeled_refusal=0;
 model=domain(0x1000);sound=domain(0x2000);table=domain(0x3000);header=domain(0x4000);actor=domain(0x8000);geometry=domain(0x9000);source=domain(0xa000);positions=domain(0xb000);
 uint32_t kind=variant/4u;kind=(kind&7u)|((kind&24u)<<3);if(variant&128u)kind|=0x76540000u;
 uint32_t slot=(variant/8)%32;const uint32_t edge_slots[]={32,0x7fffffff,0xffffffff,0x80000000,0,10,11,31};if(early)slot=edge_slots[(early-1)%8];
 for(unsigned i=0;i<33;i++)write_at(0x800d3364u+i*4u,4,0);
 if(early>8)write_at(0x800d3368u+(slot<<2),4,domain(0x100));
 write_at(model,4,4);write_at(model+4u,4,domain(0x7c00));write_at(model+8u,4,source);write_at(model+12u,4,source+0x100u);write_at(model+16u,4,table);
 write_at(sound,4,2);write_at(sound+4u,4,domain(0x7000));write_at(sound+8u,4,domain(0x7100));
 write_at(domain(0x7000),4,2);write_at(domain(0x7004),4,domain(0x7200));write_at(domain(0x7200),4,1);
 write_at(domain(0x7100),4,3);write_at(domain(0x7108),4,domain(0x7300));write_at(domain(0x710c),4,(variant&16u)?domain(0x7300):domain(0x7400));
 write_at(table,4,9);write_at(table+4u,4,header);for(unsigned i=0;i<256;i++)write_at(table+8u+i*4u,4,domain(0xc000u+i*0x100u));
 for(unsigned i=0;i<0x3000;i++)write_at(header+i,1,variant+i*17u);
 write_at(header+12u,2,((variant&32u)?4:0)|((variant&64u)?0x200:0));write_at(header+8u,2,0x8001u+variant*127u);
 const unsigned counts[]={0,1,2,3};unsigned small=counts[(variant/16)%4],parts=counts[(variant/32)%4];
 if(capacity){const unsigned large[]={16,128,255};small=capacity<=3?large[capacity-1]:0;parts=capacity>3?large[capacity-4]:0;}
 write_at(header+16u,1,small);write_at(header+18u,1,parts);
 uint32_t p=header+20u;
 for(unsigned i=0;i<(parts>8?parts:8);i++){
  for(unsigned j=0;j<18;j++)write_at(p+j*2u,2,0x7ff0u+variant*3u+i*7u+j*11u);
  int n=(int)((variant/8+i)%4)-1;write_at(p+0x22u,2,(uint32_t)n);p+=36u;
  for(int j=0;j<n;j++)for(unsigned k=0;k<5;k++){write_at(p,2,0x8000u+j*13u+k*7u);p+=2u;}
 }
 write_at(0x800658c8u,4,domain(0xd000));write_at(domain(0xd344),2,0x7001u+variant*41u);write_at(domain(0xe344),2,0x9007u+variant*37u);
 for(unsigned i=0;i<3;i++){write_at(domain(0xd478u+i),1,17+i*51u);write_at(domain(0xe478u+i),1,23+i*29u);write_at(positions+i*2u,2,0x7ff0u+variant*37u+i*19u);}
 write_at(0x800c3b70u,4,domain(0xf000));write_at(0x800c3b6cu,4,(variant/8)%20);
 unsigned occupied=(variant/4)%21;for(unsigned i=0;i<21;i++)write_at(0x800c3accu+i*8u,4,i<occupied?domain(0xf000):0);
 write_at(0x800c3eb7u+slot*28u,1,variant&1u);
 cpu->gpr[4]=slot;cpu->gpr[5]=kind;cpu->gpr[6]=sound;cpu->gpr[7]=model;
 for(unsigned i=0;i<4;i++)write_at(cpu->gpr[29]+16u+i*4u,4,0xabc08000u+variant*17u+i*0x1111u);
 write_at(cpu->gpr[29]+32u,4,(variant&16u)?0:positions);
 if(fault==3)write_at(cpu->gpr[29]+32u,4,0x1e000000u);
 if(fault==4)write_at(table+4u,4,0x1f800ffcu);
 if(fault==5){write_at(header+16u,1,0);write_at(header+18u,1,0);write_at(cpu->gpr[29]+32u,4,0x1f800ffeu);}
}
static int run_case(void)
{
 PcPortMipsCpu oracle,native;setup(&oracle);candidate=compare=0;
 int original=PcPortMipsRun(&oracle,0x800a8bf0u,BATTLE_HALT_PC,1000000);
 last_refused=modeled_refusal;
 if(!fault&&!last_refused&&original!=PC_PORT_MIPS_HALTED){fail(oracle.error,oracle.pc);return 0;}
 unsigned expected_calls=calls;snapshot();
 setup(&native);candidate=compare=1;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int actual=PcPortMipsRun(&native,0x800a8bf0u,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(modeled_refusal!=last_refused)fail("modeled refusal mismatch",modeled_refusal);
 if(!fault&&!last_refused&&actual!=PC_PORT_MIPS_HALTED){fail(native.error,native.pc);return 0;}
 if((fault||last_refused)&&(original>=0||actual>=0))fail("fault not propagated",(uint32_t)actual);
 if(native.steps||rt.bridge_cpu||calls!=expected_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native context/count/instructions",calls);
 for(unsigned i=16;i<32;i++)if(native.gpr[i]!=oracle.gpr[i]){fail("preserved register",i);fprintf(stderr,"native=%08x original=%08x\n",native.gpr[i],oracle.gpr[i]);}
 if(native.gpr[2]!=oracle.gpr[2]||native.gpr[3]!=oracle.gpr[3]||native.hi!=oracle.hi||native.lo!=oracle.lo){fail("return/HI/LO",native.gpr[2]);fprintf(stderr,"native=%08x/%08x original=%08x/%08x\n",native.gpr[2],native.gpr[3],oracle.gpr[2],oracle.gpr[3]);}
 if(!memory_equal())fail("final memory",0);
 return !failed;
}
static int controls(void)
{
 PcPortMipsCpu cpu;variant=0;service_mode=early=perturb=fault=0;setup(&cpu);candidate=1;compare=0;rt.bridge_cpu=&cpu;snapshot();
 const unsigned offsets[]={0,sizeof(body)/2,sizeof(body)-1};
 for(unsigned i=0;i<3;i++){uint8_t *p=PSX_ADDR(0x800a8bf0u+offsets[i]);*p^=1;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("identity accepted",offsets[i]);*p^=1;}
 cpu.load_valid=1;cpu.load_reg=4;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("pending load",0);cpu.load_valid=0;
 rt.force_interpret=1;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("force",0);rt.force_interpret=0;
 rt.initialized=0;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("init",0);rt.initialized=1;
 g_ActiveBattleRuntime=NULL;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("active",0);g_ActiveBattleRuntime=&rt;
 rt.bridge_cpu=NULL;if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("context",0);rt.bridge_cpu=&cpu;
 if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf4))fail("wrong target",0);
 assert(setenv("XENO_BATTLE_NATIVE_ACTOR_CREATE","0",1)==0);if(battle_actor_create_try_host(&rt,&cpu,0x800a8bf0))fail("opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_ACTOR_CREATE")==0);
 if(calls||!memory_equal())fail("gate side effects",0);
 for(unsigned id=0;id<sizeof(targets)/sizeof(targets[0]);id++)for(unsigned pending=0;pending<2;pending++){
  variant=targets[id]==0x80030988u?64u:targets[id]==0x800a7064u?32u:0u;
  setup(&cpu);candidate=1;compare=0;visited=0;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int reached=PcPortMipsRun(&cpu,0x800a8bf0,BATTLE_HALT_PC,1000000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(reached!=PC_PORT_MIPS_HALTED||cpu.steps||!(visited&(UINT64_C(1)<<id))){fail("unvisited refusal target",targets[id]);return 0;}
  setup(&cpu);candidate=1;compare=0;if(pending)pending_return=targets[id];else stop_target=targets[id];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x800a8bf0,BATTLE_HALT_PC,1000000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc>=0||cpu.steps||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions||rt.bridge_cpu)fail("nested refusal",targets[id]);
 }
 variant=0;for(fault=1;fault<=5;fault++){if(!run_case())return 0;}fault=0;
 printf("ACTOR CREATE PASS identities/context/opt-out, 48 nested refusals and five partial-write faults\n");return !failed;
}
int main(int argc,char **argv)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(fseek(f,0x800a8bf0u-0x8006faf0u,SEEK_SET)==0);assert(fread(body,1,sizeof(body),f)==sizeof(body));fclose(f);
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2&&strcmp(argv[1],"--capacity")==0){variant=0;for(capacity=1;capacity<=6;capacity++)if(!run_case())return 1;puts("ACTOR CREATE PASS six large pool/part-count comparisons through 255 records");return 0;}
 if(argc==2&&strcmp(argv[1],"--controls")==0)return controls()?0:1;
 if(argc==2&&strcmp(argv[1],"--faults")==0){for(fault=1;fault<=5;fault++)if(!run_case())return 1;puts("ACTOR CREATE PASS five partial-write faults");return 0;}
 if(argc==2&&strcmp(argv[1],"--live-count")==0){for(variant=64;variant<68;variant++){perturb=16;if(!run_case())return 1;}puts("ACTOR CREATE PASS four live part-count callback comparisons");return 0;}
 if(argc==2){variant=(unsigned)strtoul(argv[1],NULL,0);perturb=variant/16;return run_case()?0:1;}
 unsigned total=0,refused=0;
 for(service_mode=0;service_mode<2;service_mode++)for(variant=0;variant<256;variant++){
  perturb=variant/16;if(!run_case())return 1;total++;refused+=(unsigned)last_refused;
  if((variant&63u)==63u){printf("ACTOR CREATE completed %u caller comparisons\n",total);fflush(stdout);}
 }
 service_mode=perturb=0;for(variant=0;variant<4;variant++)for(early=1;early<=16;early++){if(!run_case())return 1;total++;}early=0;
 printf("ACTOR CREATE PASS %u complete original-caller comparisons plus %u exhausted-list refusal comparisons; external bodies modeled; native steps=0; visited=%llx\n",total-refused,refused,(unsigned long long)visited);
 variant=service_mode=perturb=0;for(capacity=1;capacity<=6;capacity++)if(!run_case())return 1;capacity=0;
 puts("ACTOR CREATE PASS six large pool/part-count comparisons through 255 records");
 for(variant=64;variant<68;variant++){perturb=16;if(!run_case())return 1;}
 puts("ACTOR CREATE PASS four live part-count callback comparisons");
 return controls()?0:1;
}
