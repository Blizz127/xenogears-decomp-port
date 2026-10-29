/* Concrete frame service signatures and packed four-byte OT behavior.
 * Function probes exercise the actual production dispatcher. Real service
 * semantics and the normal boot candidate are separate evidence. */
#define main frame_comparison_main
#include "battle_frame_host_test.c"
#undef main
static unsigned abi_calls;
static int32_t abi_arg,abi_result;
static void probe_reset(void){abi_calls++;}
static int probe_vsync(int value){abi_calls++;abi_arg=value;return abi_result;}
static void probe_context(int value){abi_calls++;abi_arg=value;}
static void unused_clear(void){fail("native padded OT backend called",0);}
static const uint32_t abi_targets[]={0x80019ca0u,0x800250e0u,0x80044ad8u,0x8004b54cu};
static void abi_setup(PcPortMipsCpu *cpu,unsigned which,int cached)
{
 defaults();setup(cpu);observing=0;candidate=1;compare=0;
 void *functions[]={(void *)probe_reset,(void *)probe_context,(void *)unused_clear,(void *)probe_vsync};
 const char *names[]={"GameCheckAndHandleSoftReset","func_800250E0","ClearOTagR","Vsync"};
 rt.functions[0]=(ResolvedFunction){abi_targets[which],functions[which],names[which]};rt.function_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,abi_targets[which]);slot->target=abi_targets[which];slot->state=1;slot->entry=rt.functions[0];}
 cpu->gpr[2]=0x13579bdfu;cpu->gpr[3]=0x2468ace0u;cpu->gpr[29]=0x1e000000u;
 abi_calls=0;abi_arg=0x76543210;abi_result=(int32_t)0x89abcdefu;
}
static void abi_run(PcPortMipsCpu *cpu,unsigned which,int success)
{
 PcPortMipsCpu expected=*cpu;snapshot();PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 if(success&&which==3)expected.gpr[2]=(uint32_t)abi_result;
 if(success&&which==2)expected.gpr[2]=cpu->gpr[4];
 int rc=runtime_bridge(&rt,cpu,abi_targets[which]);
 if(rc!=(success?1:-1)||memcmp(cpu,&expected,sizeof(expected))||rt.bridge_cpu||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("frame service CPU/signature/return",which);
 if(abi_calls!=(unsigned)(success&&which!=2))fail("frame service call count",which);
 if(which!=2||!success){if(!memory_equal())fail("frame service memory",which);}
}
static unsigned frame_interfaces(void)
{
 const uint32_t words[]={0,1,2,0xffffffffu,0x80000000u,0x7fffffffu,0x80100000u,0x10001u};unsigned total=0;
 for(int cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned a=0;a<8;a++)for(unsigned result=0;result<8;result++){
   abi_setup(&cpu,3,cached);cpu.gpr[4]=words[a];abi_result=(int32_t)words[result];abi_run(&cpu,3,1);
   if((uint32_t)abi_arg!=words[a])fail("Vsync scalar translated or narrowed",words[a]);
   total++;
  }
  for(unsigned a=0;a<8;a++){
   abi_setup(&cpu,0,cached);cpu.gpr[4]=words[a];abi_run(&cpu,0,1);total++;
   abi_setup(&cpu,1,cached);cpu.gpr[4]=words[a];abi_run(&cpu,1,a<2);
   if(a<2&&(uint32_t)abi_arg!=words[a])fail("context scalar",words[a]);
   total++;
  }
  const uint32_t addresses[]={0x800c4a90u,0xa00c4a90u};const unsigned counts[]={1,2,32,4096};
  for(unsigned domain=0;domain<2;domain++)for(unsigned n=0;n<4;n++){
   abi_setup(&cpu,2,cached);cpu.gpr[4]=addresses[domain];cpu.gpr[5]=counts[n];abi_run(&cpu,2,1);
   unsigned off=cpu.gpr[4]&0x1fffffu;
   for(unsigned i=0;i<counts[n];i++)store_le(want_ram+off+i*4u,4,i?((off+(i-1u)*4u)&0x00ffffffu):0x0005698cu);
   if(!memory_equal())fail("packed OT links or guard bytes",counts[n]);
   total++;
  }
  const uint32_t bad[][2]={{0,1},{0x800c4a91u,1},{0x800c4a90u,0},{0x800c4a90u,0xffffffffu},{0x801ffffcu,2},{0x1e000000u,1},{0x80050000u,1},{0x1f800000u,1}};
  for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
   abi_setup(&cpu,2,cached);cpu.gpr[4]=bad[i][0];cpu.gpr[5]=bad[i][1];abi_run(&cpu,2,0);total++;
  }
  for(unsigned which=0;which<4;which++){
   abi_setup(&cpu,which,cached);cpu.gpr[4]=which==2?0x800c4a90u:1;cpu.gpr[5]=4;cpu.load_valid=1;cpu.load_reg=4;cpu.load_value=0xfeedbeefu;abi_run(&cpu,which,0);total++;
  }
 }
 return total;
}
int main(int argc,char **argv)
{
 load_inputs();unsigned total=frame_interfaces();
 printf("BATTLE FRAME SERVICE PASS %u typed ABI and packed OT cases; native instructions=0\n",total);
 if(argc>1&&strcmp(argv[1],"--interfaces")==0)return 0;
 return frame_comparison_main(argc,argv);
}
