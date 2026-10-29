/* Concrete camera interfaces plus actual native SDK composition. The oracle
 * executes original SDK instructions and normalizes only SDK CPU clobbers to
 * the native C ABI. Caller frames remain byte-exact. No oracle runs in the
 * candidate leg; both legs use the existing graphics compatibility backend. */
#define main camera_model_main
#include "battle_target_camera_host_test.c"
#undef main
#include <inline_c.h>
extern int matrixLevel;
extern MATRIX stack[],*currentMatrix;
extern void OuterProduct12(VECTOR *,VECTOR *,VECTOR *);
extern MATRIX *RotMatrixZYX(SVECTOR *,MATRIX *);
extern int32_t ReadGeomScreen(void);
extern long VectorNormal(VECTOR *,VECTOR *);
static const uint32_t sdk_targets[]={0x8004abbcu,0x80049cecu,0x8004a0dcu,0x80049efcu,0x80049f8cu,0x8004a64cu,0x80048c4cu,0x80048d7cu,0x8004a480u,0x8004960cu,0x800496acu,0x8003fa08u};
static const char *sdk_names[]={"RotMatrixZYX","ApplyMatrix","ReadGeomScreen","SetRotMatrix","SetTransMatrix","RotTransPers","SquareRoot0","VectorNormal","OuterProduct12","PushMatrix","PopMatrix","memset"};
static const unsigned spans[][4]={{6,32,0,0},{20,8,12,0},{0,0,0,0},{32,0,0,0},{32,0,0,0},{8,4,4,4},{0,0,0,0},{12,12,0,0},{12,12,12,0},{0,0,0,0},{0,0,0,0},{16,0,0,0}};
static const unsigned writes[]={2,4,0,0,0,14,0,2,4,0,0,1};
static const unsigned returns[]={1,1,1,0,0,1,1,1,0,0,0,1};
static unsigned abi_index,abi_calls,bad_return;
static void *abi_args[4];
static int32_t abi_scalar;
static size_t abi_length;
static const uint32_t output_words[]={0x91827364u,0x87654321u,0xcafebabeu};
static long abi_wide(void){return sizeof(long)>4?(long)INT64_C(0x12345678fedcba98):(long)(int32_t)0xfedcba98u;}
static MATRIX *probe_rot(SVECTOR *a,MATRIX *b)
{abi_calls++;abi_args[0]=a;abi_args[1]=b;for(unsigned i=0;i<9;i++)b->m[i/3][i%3]=(int16_t)(0x8100u+i*31u);return bad_return?(MATRIX *)((uint8_t *)b+4):b;}
static VECTOR *probe_apply(MATRIX *a,SVECTOR *b,VECTOR *c)
{abi_calls++;abi_args[0]=a;abi_args[1]=b;abi_args[2]=c;c->vx=(int32_t)output_words[0];c->vy=(int32_t)output_words[1];c->vz=(int32_t)output_words[2];return bad_return?(VECTOR *)((uint8_t *)c+4):c;}
static int32_t probe_screen(void){abi_calls++;return -12345;}
static void probe_matrix(MATRIX *a){abi_calls++;abi_args[0]=a;}
static int probe_project(SVECTOR *a,int *b,long *c,long *d)
{abi_calls++;abi_args[0]=a;*b=(int32_t)output_words[0];*c=sizeof(long)>4?(long)INT64_C(0x1234567887654321):(long)(int32_t)output_words[1];*d=sizeof(long)>4?(long)INT64_C(0x98765432cafebabe):(long)(int32_t)output_words[2];return -987;}
static int probe_sqrt(int a){abi_calls++;abi_scalar=a;return -54321;}
static long probe_normal(VECTOR *a,VECTOR *b)
{abi_calls++;abi_args[0]=a;abi_args[1]=b;b->vx=(int32_t)output_words[0];b->vy=(int32_t)output_words[1];b->vz=(int32_t)output_words[2];return abi_wide();}
static void probe_cross(VECTOR *a,VECTOR *b,VECTOR *c)
{abi_calls++;abi_args[0]=a;abi_args[1]=b;abi_args[2]=c;c->vx=(int32_t)output_words[0];c->vy=(int32_t)output_words[1];c->vz=(int32_t)output_words[2];}
static void probe_stack(void){abi_calls++;}
static void *probe_fill(void *a,int b,size_t c)
{abi_calls++;abi_args[0]=a;abi_scalar=b;abi_length=c;memset(a,b,c);return bad_return?(uint8_t *)a+4:a;}
static void *probes[]={probe_rot,probe_apply,probe_screen,probe_matrix,probe_matrix,probe_project,probe_sqrt,probe_normal,probe_cross,probe_stack,probe_stack,probe_fill};
static void *backends[]={RotMatrixZYX,ApplyMatrix,ReadGeomScreen,SetRotMatrix,SetTransMatrix,RotTransPers,SquareRoot0,VectorNormal,OuterProduct12,PushMatrix,PopMatrix,memset};
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static uint32_t base(unsigned i){return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *base_pointer(unsigned i)
{return i<4?g_PsxRam+0x120000u:i==4?shared+0x100u:i==5?g_PsxScratchpad+0x100u:host+0x100u;}
static uint8_t *expected_pointer(uint8_t *actual,unsigned width)
{
 const struct {uint8_t *a,*b;size_t n;} regions[]={{g_PsxRam,want_ram,sizeof(g_PsxRam)},{shared,want_shared,sizeof(shared)},{g_PsxScratchpad,want_pad,sizeof(g_PsxScratchpad)},{host,want_host,sizeof(host)}};
 for(unsigned i=0;i<4;i++)if((uintptr_t)actual>=(uintptr_t)regions[i].a&&(uintptr_t)actual+width<=(uintptr_t)regions[i].a+regions[i].n)return regions[i].b+(actual-regions[i].a);
 abort();
}
static void expected_outputs(PcPortMipsCpu *cpu)
{
 for(unsigned arg=0;arg<4;arg++)if(writes[abi_index]&(1u<<arg)){
  uint8_t *p=module_memory_pointer(&rt,cpu->gpr[4+arg],spans[abi_index][arg],1);assert(p);p=expected_pointer(p,spans[abi_index][arg]);
  if(abi_index==0)for(unsigned i=0;i<9;i++)store_le(p+i*2u,2,0x8100u+i*31u);
  else if(abi_index==5)store_le(p,4,output_words[arg-1u]);
  else if(abi_index==11)memset(p,(int)cpu->gpr[5],16);
  else for(unsigned i=0;i<3;i++)store_le(p+i*4u,4,output_words[i]);
 }
}
static uint32_t expected_return(PcPortMipsCpu *cpu)
{
 switch(abi_index){case 0:return cpu->gpr[5];case 1:return cpu->gpr[6];case 2:return (uint32_t)-12345;case 5:return (uint32_t)-987;case 6:return (uint32_t)-54321;case 7:return (uint32_t)abi_wide();case 11:return cpu->gpr[4]&&(int32_t)cpu->gpr[6]>0?cpu->gpr[4]:0;default:return cpu->gpr[2];}
}
static void abi_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;cpu->bus.bridge=runtime_bridge;cpu->gpr[29]=0x1e000000u;
 rt.functions[0]=(ResolvedFunction){sdk_targets[abi_index],probes[abi_index],sdk_names[abi_index]};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,sdk_targets[abi_index]);slot->target=sdk_targets[abi_index];slot->state=1;slot->entry=rt.functions[0];}
 for(unsigned arg=0;arg<4;arg++)cpu->gpr[4+arg]=base((domain+arg)%7u)+arg*0x100u;
 if(abi_index==11){cpu->gpr[5]=0x1234ff81u;cpu->gpr[6]=16;}
 abi_calls=bad_return=0;abi_scalar=0;abi_length=0;memset(abi_args,0,sizeof(abi_args));
}
static void abi_run(PcPortMipsCpu *cpu,int success,unsigned calls_expected,int effects)
{
 PcPortMipsCpu before=*cpu;snapshot();if(effects)expected_outputs(cpu);
 PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(cpu,sdk_targets[abi_index],BATTLE_HALT_PC,100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("ABI result",sdk_targets[abi_index]);
 if(success&&returns[abi_index])before.gpr[2]=expected_return(&before);
 if(abi_calls!=calls_expected||memcmp(cpu->gpr,before.gpr,sizeof(cpu->gpr))||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("ABI arguments/result/guards",sdk_targets[abi_index]);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 for(abi_index=0;abi_index<12;abi_index++)for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned domain=0;domain<7;domain++){
   abi_setup(&cpu,cached,domain);uint32_t scalar=domain&1u?0x80000001u:0x7fffffffu;if(abi_index==6)cpu.gpr[4]=scalar;
   abi_run(&cpu,1,1,1);
   for(unsigned arg=0;arg<4;arg++)if(spans[abi_index][arg]&&!(abi_index==5&&arg>0))assert(abi_args[arg]==base_pointer((domain+arg)%7u)+arg*0x100u);
   if(abi_index==6)assert((uint32_t)abi_scalar==scalar);
   if(abi_index==11)assert((uint32_t)abi_scalar==0x1234ff81u&&abi_length==16);
   total++;
  }
  for(unsigned arg=0;arg<4;arg++)if(spans[abi_index][arg]){
   for(unsigned bad=0;bad<3;bad++){
    abi_setup(&cpu,cached,0);
    if(bad==0&&abi_index!=11)cpu.gpr[4+arg]+=abi_index==0&&arg==0?1u:2u;
    else if(bad==1||bad==0)cpu.gpr[4+arg]=0x1e000000u;
    else {cpu.gpr[4+arg]=base(6);rt.host_ranges[0].end=(uintptr_t)host+0x100u+spans[abi_index][arg]-1u;}
    abi_run(&cpu,0,0,0);total++;
   }
   abi_setup(&cpu,cached,0);cpu.gpr[4+arg]=base(6);rt.host_ranges[0].writable=0;
   int output=(writes[abi_index]&(1u<<arg))!=0;abi_run(&cpu,!output,output?0:1,!output);total++;
  }
  abi_setup(&cpu,cached,0);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0,0,0);total++;
  if(writes[abi_index]){
   abi_setup(&cpu,cached,0);unsigned arg=0;while(!(writes[abi_index]&(1u<<arg)))arg++;
   cpu.gpr[4+arg]=0x801e5000u;rt.file1.verified=1;abi_run(&cpu,1,1,1);assert(!rt.file1.verified);total++;
  }
  if(abi_index==0||abi_index==1||abi_index==11){abi_setup(&cpu,cached,0);bad_return=1;abi_run(&cpu,0,1,1);total++;}
  if(abi_index==5){abi_setup(&cpu,cached,0);cpu.gpr[6]=cpu.gpr[7]=cpu.gpr[5];abi_run(&cpu,1,1,1);total++;}
  if(abi_index==11)for(unsigned path=0;path<4;path++){
   abi_setup(&cpu,cached,0);cpu.gpr[4]=path?0x1e000000u:0;cpu.gpr[6]=path==0?16:path==1?0:path==2?UINT32_MAX:0x80000000u;
   abi_run(&cpu,1,0,0);total++;
  }
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);return total;
}
static int sdk_index(uint32_t target)
{for(unsigned i=0;i<12;i++)if(target==sdk_targets[i])return (int)i;return -1;}
static unsigned in_sdk,real_count,want_real_count,geometry;
static unsigned actual_small,actual_large,actual_empty,actual_height;
static uint64_t sdk_steps;
static struct {uint32_t target,args[4];} real_calls[64];
static int real_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 if(!candidate){
  if(target==0x800bc930u)actual_small++;
  if(target==0x800bc994u)actual_large++;
  if(target==0x800bc550u&&cpu->gpr[6]==0)actual_empty++;
  if(target==0x800bc894u)actual_height++;
 }
 int index=sdk_index(target);
 if(index<0)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(real_count>=64)fail("actual SDK call capacity",target);
 if(candidate){
  if(real_count>=want_real_count||real_calls[real_count].target!=target||memcmp(real_calls[real_count].args,&cpu->gpr[4],sizeof(real_calls[0].args)))fail("actual SDK order/arguments",target);
  real_count++;return runtime_bridge(opaque,cpu,target);
 }
 real_calls[real_count].target=target;memcpy(real_calls[real_count++].args,&cpu->gpr[4],sizeof(real_calls[0].args));
 PcPortMipsCpu original=*cpu;in_sdk=1;uint64_t before=original.steps;
 int rc=PcPortMipsRun(&original,target,cpu->gpr[31],50000);in_sdk=0;sdk_steps+=original.steps-before;
 if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 for(unsigned i=16;i<=23;i++)if(original.gpr[i]!=cpu->gpr[i])fail("original SDK callee-saved",i);
 if(original.gpr[29]!=cpu->gpr[29]||original.gpr[30]!=cpu->gpr[30]||original.gpr[31]!=cpu->gpr[31]||original.load_valid)fail("original SDK frame/return",target);
 /* These leaf SDK bodies use no stack frame. Their volatile CPU registers
  * are outside the native C ABI; preserve the incoming C caller state while
  * retaining each original functional return, memory write and GTE effect. */
 if(returns[index])cpu->gpr[2]=original.gpr[2];
 return 1;
}
static void prepare_real(PcPortMipsCpu *cpu)
{
 setup(cpu);observing=0;compare=0;cpu->bus.bridge=real_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&exe->size<0x60000u);
 memcpy(PSX_ADDR(0x8000f800u),exe->bytes,exe->size);memcpy(shared,exe->bytes+XENO_RD_SLUS_OFF(0x80050000u),sizeof(shared)<exe->size-XENO_RD_SLUS_OFF(0x80050000u)?sizeof(shared):exe->size-XENO_RD_SLUS_OFF(0x80050000u));
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 /* Restore the caller data after private image loading, including domains
  * that share lower main-executable storage. Moderate geometry avoids the
  * original VectorNormal signed-ADD exception domain. */
 for(unsigned i=0;i<11;i++){
  fixture_store(0x800ccb3cu+i*4u,4,selection==2&&i%3u==0?0:owner+i*0x80u);
  fixture_store(0x800c3eb7u+i*0x1cu,1,selection==1&&i%3u==0?255u:0);
  fixture_store(0x800c3eb8u+i*0x1cu,1,i%2u?255u:0);
  int x=geometry==0?0:(int)i*900-4500,y=geometry==0?0:(int)i*50-250,z=geometry==0?0:(int)i*200-1000;
  fixture_store(owner+i*0x80u,4,(uint32_t)x*65536u+3u);
  fixture_store(owner+i*0x80u+4u,4,(uint32_t)y*65536u+0xfffeu);
  fixture_store(owner+i*0x80u+8u,4,(uint32_t)z*65536u+1u);
  fixture_store(owner+i*0x80u+0x36u,2,geometry==0?seed*97u:800u+i*17u);
 }
 for(unsigned i=0;i<3;i++){
  fixture_store(0x800c3740u+i*2u,2,geometry==2?(i==0?113u:i==1?271u:509u):0);
  fixture_store(0x800c3730u+i*2u,2,i==1?4096u:0);
  fixture_store(table+i*2u,2,seed*997u+i*131u);
  fixture_store(alternate_table+i*2u,2,seed*117u+i*257u);
  fixture_store(alternate_state+i*2u,2,i==1?4096u:0);
 }
 fixture_store(0x800c3688u,1,seed&1u);fixture_store(0x800c37c8u,1,zero_color);
 fixture_store(0x800c3cdcu,4,0x79683124u);fixture_store(0x800c3680u,4,0);fixture_store(0x800c3684u,4,0);
 fixture_store(0x80056d2cu,4,0);memset(shared+0x6d30u,0x96,640);
 matrixLevel=0;currentMatrix=stack;memset(stack,0x96,640);
 for(unsigned i=0;i<12;i++)rt.functions[i]=(ResolvedFunction){sdk_targets[i],backends[i],sdk_names[i]};
 rt.function_count=12;
 qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){CTC2(0x713da905u+i*0x1521u,i);MTC2(0x971cd063u+i*0x1753u,i);}
 CTC2(4096,0);CTC2(4096,2);CTC2(4096,4);CTC2(160u<<16,24);CTC2(164u<<16,25);CTC2(ticks,26);CTC2(0x100,27);CTC2(0x100000,28);
 real_count=in_sdk=0;sdk_steps=0;
}
static void integration_case(void)
{
 PcPortMipsCpu original,native;prepare_real(&original);candidate=0;
 int rc=PcPortMipsRun(&original,targets[kind],BATTLE_HALT_PC,200000);if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 want_real_count=real_count;snapshot();uint64_t original_sdk_steps=sdk_steps;
 prepare_real(&native);candidate=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);rc=PcPortMipsRun(&native,targets[kind],BATTLE_HALT_PC,200000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||rt.bridge_cpu||sdk_steps||real_count!=want_real_count||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual native integration",native.pc);
 if(want_real_count&&!original_sdk_steps)fail("original SDK did not execute",0);
 if(memcmp(stack,want_shared+0x6d30u,640)||matrixLevel*32!=(int)load_le(want_shared+0x6d2cu,4)||currentMatrix!=stack+matrixLevel)fail("actual SDK matrix stack",0);
 /* The SDK stack has native ownership in the candidate. Compare all 640
  * bytes above, then exclude only that duplicate oracle storage and level. */
 memcpy(want_shared+0x6d30u,shared+0x6d30u,640);memcpy(want_shared+0x6d2cu,shared+0x6d2cu,4);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal()){
  for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i])fprintf(stderr,"integration register %u original=%08x native=%08x\n",i,original.gpr[i],native.gpr[i]);
  for(unsigned i=0;i<64;i++)if(((uint32_t *)&want_gte)[i]!=((uint32_t *)&gteRegs)[i])fprintf(stderr,"GTE %u original=%08x native=%08x\n",i,((uint32_t *)&want_gte)[i],((uint32_t *)&gteRegs)[i]);
  const struct {const char *name;uint8_t *a,*b;size_t n;} regions[]={{"RAM",want_ram,g_PsxRam,sizeof(g_PsxRam)},{"shared",want_shared,shared,sizeof(shared)},{"scratch",want_pad,g_PsxScratchpad,sizeof(g_PsxScratchpad)},{"host",want_host,host,sizeof(host)}};
  for(unsigned r=0;r<4;r++){unsigned differences=0;for(size_t i=0;i<regions[r].n;i++)if(regions[r].a[i]!=regions[r].b[i]){if(differences<12)fprintf(stderr,"%s +%zx original=%02x native=%02x\n",regions[r].name,i,regions[r].a[i],regions[r].b[i]);differences++;}if(differences)fprintf(stderr,"%s differences=%u\n",regions[r].name,differences);}
  fail("actual SDK caller state, frames, memory and graphics state",seed);
 }
}
int main(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1&&strcmp(argv[1],"--interfaces")==0){printf("TARGET CAMERA ABI PASS %u concrete interface cases\n",interfaces());return 0;}
 if(argc>1&&strcmp(argv[1],"--basic")==0){ticks=256;integration_case();return 0;}
 printf("TARGET CAMERA ABI PASS %u concrete interface cases\n",interfaces());fflush(stdout);
 unsigned total=0;
 for(unsigned body=0;body<3;body++)for(unsigned domain=0;domain<6;domain++)for(unsigned profile=0;profile<3;profile++)for(unsigned geo=0;geo<3;geo++)for(unsigned gate=0;gate<2;gate++){
  defaults();kind=body;task_domain=domain==4?5:domain==5?6:domain;selection=profile;geometry=geo;seed=gate;stack_kind=domain%4u;ticks=256;integration_case();total++;
 }
 for(unsigned i=0;i<13;i++){defaults();mask=i==12?0xffff0000u:i==11?0:1u<<i;ticks=256;geometry=i%3u;integration_case();total++;}
 const uint32_t screens[]={0,1,119,256,0x7fff,0x8000,0xffff,0x12345678u};
 for(unsigned i=0;i<8;i++){defaults();ticks=screens[i];geometry=i%3u;integration_case();total++;}
 defaults();zero_color=255;ticks=256;integration_case();total++;
 assert(actual_small&&actual_large&&actual_empty&&actual_height);
 printf("TARGET CAMERA ACTUAL SDK PASS %u original/native camera sequences; small=%u large=%u empty=%u height=%u; all caller frames retained, SDK CPU clobbers normalized, native instructions=0\n",total,actual_small,actual_large,actual_empty,actual_height);return 0;
}
