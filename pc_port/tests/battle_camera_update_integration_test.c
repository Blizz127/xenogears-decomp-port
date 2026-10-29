/* Body models, concrete math interfaces, then actual SDK composition.
 * Only the original SDK's volatile CPU clobbers are normalized to the C ABI.
 * Caller frames, memory and final graphics state remain byte-exact. */
#define main camera_update_model_main
#include "battle_camera_update_host_test.c"
#undef main
#include <inline_c.h>
/* Bind conventional exports exactly as initialize_runtime does. */
#undef rsin
#undef rcos
extern VECTOR *Square0(VECTOR *,VECTOR *);
extern void OuterProduct12(VECTOR *,VECTOR *,VECTOR *);
extern long VectorNormal(VECTOR *,VECTOR *);
extern int matrixLevel;
extern MATRIX stack[],*currentMatrix;
static const uint32_t sdk_targets[]={0x8003f8ccu,0x8003f8b0u,0x8004a414u,0x8004b32cu,0x80048c4cu,
 0x80048d7cu,0x8004a480u,0x8004960cu,0x80049cecu,0x800496acu};
static const char *sdk_names[]={"rsin","rcos","Square0","ratan2","SquareRoot0",
 "VectorNormal","OuterProduct12","PushMatrix","ApplyMatrix","PopMatrix"};
static void *backends[]={rcos,rsin,Square0,ratan2,SquareRoot0,VectorNormal,OuterProduct12,PushMatrix,ApplyMatrix,PopMatrix};
static const unsigned returns[]={1,1,1,1,1,1,0,0,1,0};
static unsigned abi_index,abi_calls,bad_return;
static uint32_t abi_a,abi_b;
static VECTOR *abi_input,*abi_output;
static int probe_scalar(int a){abi_calls++;abi_a=(uint32_t)a;return (int32_t)0x87654321u;}
static int probe_angle(int a,int b){abi_calls++;abi_a=(uint32_t)a;abi_b=(uint32_t)b;return (int32_t)0x87654321u;}
static VECTOR *probe_square(VECTOR *a,VECTOR *b)
{abi_calls++;abi_input=a;abi_output=b;b->vx=(int32_t)0x87654321u;b->vy=(int32_t)0xcafebabeu;b->vz=(int32_t)0xfedcba98u;return bad_return?(VECTOR *)((uint8_t *)b+4):b;}
static void *probes[]={probe_scalar,probe_scalar,probe_square,probe_angle,probe_scalar};
static const uint32_t bases[]={0x80120000u,0xa0120000u,0x80320000u,0x00120000u,0x80050100u,0x1f800100u,0};
static uint32_t abi_base(unsigned i){return i==6?(uint32_t)(uintptr_t)(host+0x100u):bases[i];}
static uint8_t *abi_pointer(unsigned i)
{return i<4?g_PsxRam+0x120000u:i==4?shared+0x100u:i==5?g_PsxScratchpad+0x100u:host+0x100u;}
static uint8_t *expected_pointer(uint8_t *actual,unsigned width)
{
 const struct {uint8_t *a,*b;size_t n;} regions[]={{g_PsxRam,want_ram,sizeof(g_PsxRam)},{shared,want_shared,sizeof(shared)},{g_PsxScratchpad,want_pad,sizeof(g_PsxScratchpad)},{host,want_host,sizeof(host)}};
 for(unsigned i=0;i<4;i++)if((uintptr_t)actual>=(uintptr_t)regions[i].a&&(uintptr_t)actual+width<=(uintptr_t)regions[i].a+regions[i].n)return regions[i].b+(actual-regions[i].a);
 abort();
}
static void abi_setup(PcPortMipsCpu *cpu,unsigned cached,unsigned domain)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;cpu->bus.bridge=runtime_bridge;cpu->gpr[29]=0x1e000000u;
 rt.functions[0]=(ResolvedFunction){sdk_targets[abi_index],probes[abi_index],sdk_names[abi_index]};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,sdk_targets[abi_index]);slot->target=sdk_targets[abi_index];slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[4]=abi_base(domain);cpu->gpr[5]=abi_base((domain+1u)%7u)+0x100u;
 abi_calls=bad_return=abi_a=abi_b=0;abi_input=abi_output=NULL;
}
static void abi_run(PcPortMipsCpu *cpu,int success,unsigned calls_expected,int effects)
{
 PcPortMipsCpu before=*cpu;snapshot();
 if(effects){uint8_t *p=module_memory_pointer(&rt,cpu->gpr[5],12,1);assert(p);p=expected_pointer(p,12);store_le(p,4,0x87654321u);store_le(p+4,4,0xcafebabeu);store_le(p+8,4,0xfedcba98u);}
 PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(cpu,sdk_targets[abi_index],BATTLE_HALT_PC,100);
 if(rc!=(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL))fail("ABI result",sdk_targets[abi_index]);
 if(success)before.gpr[2]=abi_index==2?before.gpr[5]:0x87654321u;
 if(abi_calls!=calls_expected||memcmp(cpu->gpr,before.gpr,sizeof(cpu->gpr))||memcmp(cpu->cp0,before.cp0,sizeof(cpu->cp0))||cpu->hi!=before.hi||cpu->lo!=before.lo||cpu->steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("ABI arguments/result/guards",sdk_targets[abi_index]);
}
static unsigned interfaces(void)
{
 unsigned total=0;assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 static const uint32_t scalars[]={0,1,0xfff,0x1000,0x7fffffff,0x80000000u,0xffffffffu,0x80001001u};
 for(abi_index=0;abi_index<5;abi_index++)for(unsigned cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  if(abi_index!=2){
   for(unsigned i=0;i<8;i++){
    abi_setup(&cpu,cached,0);cpu.gpr[4]=scalars[i];cpu.gpr[5]=scalars[(i+3u)%8u];abi_run(&cpu,1,1,0);
    if(abi_a!=(abi_index<2?(scalars[i]&0xfffu):scalars[i])||(abi_index==3&&abi_b!=scalars[(i+3u)%8u]))fail("raw scalar width or trig mask",i);
    total++;
   }
  }else{
   for(unsigned domain=0;domain<7;domain++){
    abi_setup(&cpu,cached,domain);abi_run(&cpu,1,1,1);
    assert((uint8_t *)abi_input==abi_pointer(domain)&&(uint8_t *)abi_output==abi_pointer((domain+1u)%7u)+0x100u);total++;
   }
   for(unsigned arg=0;arg<2;arg++){
    for(unsigned bad=0;bad<3;bad++){
     abi_setup(&cpu,cached,0);
     if(bad==0)cpu.gpr[4+arg]+=2u;
     else if(bad==1)cpu.gpr[4+arg]=0x1e000000u;
     else{cpu.gpr[4+arg]=abi_base(6);rt.host_ranges[0].end=(uintptr_t)host+0x10bu;}
     abi_run(&cpu,0,0,0);total++;
    }
    abi_setup(&cpu,cached,0);cpu.gpr[4+arg]=abi_base(6);rt.host_ranges[0].writable=0;
    abi_run(&cpu,arg==0,arg==0?1:0,arg==0);total++;
   }
   abi_setup(&cpu,cached,0);cpu.gpr[5]=cpu.gpr[4];abi_run(&cpu,1,1,1);total++;
   abi_setup(&cpu,cached,0);bad_return=1;abi_run(&cpu,0,1,1);total++;
   abi_setup(&cpu,cached,0);cpu.gpr[5]=0x801e5000u;rt.file1.verified=1;abi_run(&cpu,1,1,1);assert(!rt.file1.verified);total++;
  }
  abi_setup(&cpu,cached,0);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0,0,0);total++;
 }
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);return total;
}
static int sdk_index(uint32_t target)
{for(unsigned i=0;i<10;i++)if(target==sdk_targets[i])return (int)i;return -1;}
static unsigned in_sdk,real_count,want_real_count;
static uint64_t sdk_steps;
static struct {uint32_t target,args[4];} real_calls[32];
static int real_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(in_sdk)return 0;
 /* The framing mode and the effects renderer remain separately modeled
  * dependencies here; the view constructor and SDK calls execute fully. */
 if(target==0x800bc460u||target==0x800a4654u)return service(opaque,cpu,target);
 int index=sdk_index(target);
 if(index<0)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(real_count>=32)fail("actual SDK call capacity",target);
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
 if(returns[index])cpu->gpr[2]=original.gpr[2];
 return 1;
}
static void prepare_real(PcPortMipsCpu *cpu)
{
 setup(cpu);observing=0;compare=0;cpu->bus.bridge=real_bridge;
 const XenoRetailFileSlot *exe=XenoRetailData_File(XENO_RD_SLUS);assert(exe&&exe->size<0x60000u);
 memcpy(PSX_ADDR(0x8000f800u),exe->bytes,exe->size);
 size_t shared_bytes=exe->size-XENO_RD_SLUS_OFF(0x80050000u);assert(shared_bytes<0x1f000u);
 memcpy(shared,exe->bytes+XENO_RD_SLUS_OFF(0x80050000u),shared_bytes);
 memcpy(PSX_ADDR(0x8006faf0u),battle,sizeof(battle));
 /* Restore domain-dependent actor storage after the private image load. */
 for(unsigned i=0;i<3;i++)fixture_store(owner+i*4u,4,((uint32_t)(uint16_t)((int)seed*37+(int)i*173-300)<<16)|(0xace0u+i));
 /* Moderate view geometry avoids the original VectorNormal signed-ADD
  * exception domain; full-width camera deltas have separate body proof. */
 if(kind==1){
  for(unsigned i=0;i<3;i++){
   fixture_store(0x800d3354u+i*2u,2,(uint16_t)((int)i*63-90));
   fixture_store(0x800d335cu+i*2u,2,(uint16_t)((int)i*117+(int)seed*19+300));
  }
 }
 for(unsigned i=0;i<3;i++)fixture_store(0x800c3730u+i*2u,2,i==1?4096u:0);
 fixture_store(0x80056d2cu,4,0);memset(shared+0x6d30u,0x96,640);
 matrixLevel=0;currentMatrix=stack;memset(stack,0x96,640);
 for(unsigned i=0;i<10;i++)rt.functions[i]=(ResolvedFunction){sdk_targets[i],backends[i],sdk_names[i]};
 rt.function_count=10;qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
 memset(&gteRegs,0,sizeof(gteRegs));
 for(unsigned i=0;i<32;i++){CTC2(0x713da905u+i*0x1521u,i);MTC2(0x971cd063u+i*0x1753u,i);}
 CTC2(4096,0);CTC2(4096,2);CTC2(4096,4);CTC2(160u<<16,24);CTC2(164u<<16,25);CTC2(256,26);
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
 if(argc>1){
  if(strcmp(argv[1],"--interfaces")==0){printf("CAMERA UPDATE ABI PASS %u concrete interface cases\n",interfaces());return 0;}
  if(strcmp(argv[1],"--integration-basic")==0){integration_case();return 0;}
  return camera_update_model_main(argc,argv);
 }
 if(camera_update_model_main(argc,argv))return 1;
 printf("CAMERA UPDATE ABI PASS %u concrete interface cases\n",interfaces());fflush(stdout);
 unsigned total=0;
 static const uint32_t factors[]={0,1,4095,4096,32767,32768,65535,0x12340001u};
 for(unsigned body=0;body<2;body++)for(unsigned mode=0;mode<4;mode++)for(unsigned domain=0;domain<6;domain++)for(unsigned gate=0;gate<2;gate++){
  defaults();kind=body;camera_mode=mode;task_domain=domain==4?5:domain==5?6:domain;stack_kind=domain%4u;
  seed=mode+domain;smooth_flag=gate;skip_effects=gate;factor=factors[seed%8u];radius=(seed&1u)?0xffff8001u:12345u;angle=seed*997u;vector_case=seed%6u;
  integration_case();total++;
 }
 for(unsigned i=0;i<8;i++){defaults();camera_mode=3;factor=factors[i];vector_case=i%6u;seed=i;angle=i*1101u;radius=i&1u?0x7fffffffu:0x80000000u;integration_case();total++;}
 printf("CAMERA UPDATE ACTUAL SDK PASS %u original/native camera sequences; framing/effects callees modeled, view/SDK execute, caller frames retained, SDK CPU clobbers normalized, native instructions=0\n",total);
 return 0;
}
