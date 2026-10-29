/* Exercise the real lifted-call helpers and copied callee CPU. The oracle
 * executes the original constructor with the same explicit incoming S1. */
#define EFFECT_CONSTRUCTOR_INTEGRATION_MAIN constructor_integration_main
#include "battle_effect_constructor_integration_test.c"
#undef EFFECT_CONSTRUCTOR_INTEGRATION_MAIN
#define BL_NO_GTE
#include "battle_lift.h"
static void call_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 actual_setup(cpu,cached);observing=0;
 /* The direct body fixture has a 4 KiB compatibility scratch buffer, while
  * the public call service correctly limits its stack to the first 1 KiB. */
 if(stack_kind==1){
  uint32_t previous=cpu->gpr[29];cpu->gpr[29]=0x1f800380u;
  for(unsigned i=0;i<15;i++)fixture_store(cpu->gpr[29]+0x10u+i*4u,4,peek(previous+0x10u+i*4u,4));
 }
}
static void call_arguments(PcPortMipsCpu *cpu,uint32_t *args,unsigned argc)
{
 for(unsigned i=0;i<4;i++)args[i]=cpu->gpr[4+i];
 for(unsigned i=4;i<19;i++)args[i]=peek(cpu->gpr[29]+0x10u+(i-4u)*4u,4);
 if(argc==20){args[19]=0xdead1234u;fixture_store(cpu->gpr[29]+0x4cu,4,args[19]);}
}
static void call_case(unsigned variant,uint32_t carry,unsigned cached)
{
 unsigned argc=variant?20:19;uint32_t args[20];PcPortMipsCpu original,caller;
 call_setup(&original,cached);candidate=compare=0;call_arguments(&original,args,argc);
 original.gpr[29]-=0x100u;original.gpr[17]=carry;
 for(unsigned i=4;i<argc;i++)fixture_store(original.gpr[29]+0x10u+(i-4u)*4u,4,args[i]);
 int rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);
 if(rc!=PC_PORT_MIPS_HALTED)fail("original explicit-S1 call",original.pc);
 snapshot();allocation_fixture_state(expected_heap);unsigned wanted_uploads=upload_calls,wanted_syncs=sync_callbacks;
 call_setup(&caller,cached);candidate=1;compare=0;call_arguments(&caller,args,argc);
 rt.bridge_cpu=&caller;PcPortMipsCpu before=caller;uint32_t v0=0xabcdef01u,v1=0x23456789u;
 PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 if(variant<2)rc=PcPort_BattleMipsCallJalS1(targets[0],args,argc,carry,&v0,&v1);
 else if(variant==2){bl_jal_s1("constructor-call-test",targets[0],args[0],args[1],args[2],args[3],caller.gpr[29],carry,&v0,&v1);rc=0;}
 else{v0=bl_call_guest_s1("constructor-call-test",targets[0],args[0],args[1],args[2],args[3],caller.gpr[29],carry);rc=0;}
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);uint64_t heap[16];allocation_fixture_state(heap);
 if(rc||v0!=original.gpr[2]||(variant<3&&v1!=original.gpr[3])||rt.bridge_cpu!=&caller||memcmp(&caller,&before,sizeof(caller))||
    memcmp(heap,expected_heap,sizeof(heap))||upload_calls!=wanted_uploads||sync_callbacks!=wanted_syncs||!memory_equal()||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual explicit-S1 lifted/native-JAL complete state",variant);
 uint16_t value=(uint16_t)carry;const uint16_t colors[2][3]={{3,5,7},{11,13,17}};
 for(unsigned bank=0;bank<2;bank++)for(unsigned row=0;row<3;row++){
  int component=((int)(int16_t)out_y+(int)row)%3;
  if(component>=0)value=colors[bank][component];
  uint32_t pixels=peek(effect+4u+bank*4u,4);
  for(unsigned col=0;col<2;col++)if(peek(pixels+(row*2u+col)*2u,2)!=value)fail("explicit caller carry pixel witness",row);
 }
 rt.bridge_cpu=NULL;
}
static unsigned call_refusals(void)
{
 unsigned total=0;
 for(unsigned reason=0;reason<6;reason++){
  defaults();PcPortMipsCpu caller;actual_setup(&caller,0);observing=0;candidate=1;compare=0;
  uint32_t args[20];call_arguments(&caller,args,20);rt.bridge_cpu=&caller;
  if(reason==4)caller.gpr[29]=0x1f800e00u;
  if(reason==5)caller.gpr[29]=0x80000004u;
  PcPortMipsCpu before=caller;snapshot();
  uint32_t v0=0x12345678u,v1=0x87654321u;unsigned argc=20;uint32_t target=targets[0];const uint32_t *words=args;
  if(reason==0)argc=21;
  if(reason==1)words=NULL;
  if(reason==2)target++;
  if(reason==3)g_ActiveBattleRuntime=NULL;
  PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPort_BattleMipsCallJalS1(target,words,argc,0xfeedbeefu,&v0,&v1);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);g_ActiveBattleRuntime=&rt;
  if(rc!=-1||v0!=0x12345678u||v1!=0x87654321u||memcmp(&caller,&before,sizeof(caller))||rt.bridge_cpu!=&caller||!memory_equal()||
     PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("explicit-S1 shape/context refusal",reason);
  rt.bridge_cpu=NULL;total++;
 }
 return total;
}
static unsigned explicit_calls(void)
{
 unsigned total=0;static const uint32_t carries[]={0,1,0xffffu,0x8000u,0xdeadbeefu,0xa0123456u,0x1f800700u,0x7fffffff};
 for(unsigned variant=0;variant<4;variant++)for(unsigned s=0;s<8;s++)for(unsigned y=0;y<4;y++){
  defaults();seed=s;task_domain=s%6u;stack_kind=(s+y)%3u;type_arg=5;flags_arg=0x622u;width_arg=2;height_arg=3;out_y=0xfffffffdu+y;
  call_case(variant,carries[s],y&1u);total++;
 }
 printf("EFFECT CONSTRUCTOR EXPLICIT S1 PASS %u actual lifted/helper/native-JAL/original compositions and %u shape refusals; 19/20 words, caller preserved, negative rows and cross-plane carry, native interpreted instructions=0\n",total,call_refusals());
 return total;
}
int main(int argc,char **argv)
{
 if(argc>1&&!strcmp(argv[1],"--explicit-s1")){load_inputs();defaults();explicit_calls();return 0;}
 if(constructor_integration_main(argc,argv))return 1;
 if(argc==1)explicit_calls();
 return 0;
}
