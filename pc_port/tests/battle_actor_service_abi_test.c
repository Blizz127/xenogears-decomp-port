/* Concrete actor-service ABI tests. Sound/model behavior is observed through
 * exact-signature probes; only the separately extracted heap-size body is
 * compared with its complete original instructions. */
#define main actor_caller_fixture_main
#include "battle_actor_create_host_test.c"
#undef main
static unsigned abi_calls;
static uint32_t abi_words[7],abi_result;
static void *abi_pointer,*abi_return;
static void abi_sound_mode(uint16_t a,uint16_t b,int16_t c,int16_t d)
{abi_calls++;abi_words[0]=a;abi_words[1]=b;abi_words[2]=(uint32_t)(int32_t)c;abi_words[3]=(uint32_t)(int32_t)d;}
static void abi_sound_add(SoundFile *p){abi_calls++;abi_pointer=p;}
static void *abi_sound_find(void *p,int16_t id)
{abi_calls++;abi_pointer=p;abi_words[0]=(uint32_t)(int32_t)id;return abi_return;}
static void *abi_heap_size(HeapBlock *p){abi_calls++;abi_pointer=p;return (void *)(uintptr_t)abi_result;}
static uint16_t abi_tpage(int a,int b,int c,int d)
{abi_calls++;abi_words[0]=(uint32_t)a;abi_words[1]=(uint32_t)b;abi_words[2]=(uint32_t)c;abi_words[3]=(uint32_t)d;return (uint16_t)abi_result;}
static int32_t abi_model(uint8_t *p){abi_calls++;abi_pointer=p;return (int32_t)abi_result;}
static int32_t abi_upload(uint32_t *p,int32_t a,int32_t b,int32_t c,int32_t d,uint16_t e,uint16_t f)
{abi_calls++;abi_pointer=p;abi_words[0]=(uint32_t)a;abi_words[1]=(uint32_t)b;abi_words[2]=(uint32_t)c;abi_words[3]=(uint32_t)d;abi_words[4]=e;abi_words[5]=f;return (int32_t)abi_result;}
static void abi_setup(PcPortMipsCpu *cpu,uint32_t target,void *fn)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.functions[0]=(ResolvedFunction){target,fn,"actor-service-probe"};rt.function_count=1;
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x13579bdf;cpu->gpr[3]=0x2468ace0;
 abi_calls=0;abi_pointer=NULL;memset(abi_words,0,sizeof(abi_words));
}
static void abi_run(PcPortMipsCpu *cpu,uint32_t target,int result)
{
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,target,BATTLE_HALT_PC,100);
 if(rc!=result)fprintf(stderr,"ACTOR SERVICE ABI FAIL target=%08x arg0=%08x got=%d expected=%d\n",target,cpu->gpr[4],rc,result);
 assert(rc==result);
 assert(!cpu->steps&&!rt.bridge_cpu&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
}
static unsigned abi_interfaces(void)
{
 unsigned total=0;PcPortMipsCpu cpu;
 const uint32_t words[]={0,1,0xffff,0x8000,0x12345678,0xffffffff,0x80000000,0xa0100000};
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<8;j++){
  abi_setup(&cpu,0x80030988,abi_sound_mode);cpu.gpr[4]=words[i];cpu.gpr[5]=words[j];cpu.gpr[6]=words[(i+3)%8];cpu.gpr[7]=words[(j+5)%8];
  uint32_t expected[]={cpu.gpr[4]&0xffff,cpu.gpr[5]&0xffff,(uint32_t)(int32_t)(int16_t)cpu.gpr[6],(uint32_t)(int32_t)(int16_t)cpu.gpr[7]};
  abi_run(&cpu,0x80030988,PC_PORT_MIPS_HALTED);
  assert(abi_calls==1&&!memcmp(expected,abi_words,sizeof(expected))&&cpu.gpr[2]==0x13579bdf&&cpu.gpr[3]==0x2468ace0);total++;
  abi_setup(&cpu,0x80043a1c,abi_tpage);for(unsigned k=0;k<4;k++)cpu.gpr[4+k]=words[(i+j+k)%8];
  memcpy(expected,cpu.gpr+4,sizeof(expected));abi_result=words[i];abi_run(&cpu,0x80043a1c,PC_PORT_MIPS_HALTED);
  assert(abi_calls==1&&!memcmp(expected,abi_words,sizeof(expected))&&cpu.gpr[2]==(uint16_t)abi_result&&!cpu.gpr[3]);total++;
 }
 const uint32_t addresses[]={0x80100100,0xa0100100,0x80300100,0x100100,0x80050100,0x1f800100,(uint32_t)(uintptr_t)host+0x100};
 for(unsigned a=0;a<7;a++)for(unsigned v=0;v<8;v++){
  abi_setup(&cpu,0x8002dde4,abi_upload);cpu.gpr[4]=addresses[a];uint32_t expected[6];
  for(unsigned i=0;i<6;i++){uint32_t word=words[(v+i)%8];expected[i]=i<4?word:(uint16_t)word;
   if(i<3)cpu.gpr[5+i]=word;else write_at(cpu.gpr[29]+0x10u+(i-3)*4u,4,word);}
  void *expected_pointer=module_memory_pointer(&rt,addresses[a],4,0);abi_result=words[v];
  abi_run(&cpu,0x8002dde4,PC_PORT_MIPS_HALTED);
  assert(abi_calls==1&&abi_pointer==expected_pointer&&!memcmp(abi_words,expected,sizeof(expected))&&cpu.gpr[2]==abi_result&&!cpu.gpr[3]);total++;
 }
 for(unsigned b=0;b<6;b++){
  abi_setup(&cpu,0x8002dde4,abi_upload);cpu.gpr[4]=addresses[0];
  const uint32_t bad[]={0x1e000000,0x80100101,0x801ffffe};
  if(b<3)cpu.gpr[4]=bad[b];else cpu.gpr[29]=b==3?0x1e000000:b==4?0x80100101:0x1f800fec;
  abi_run(&cpu,0x8002dde4,PC_PORT_MIPS_UNRESOLVED_CALL);assert(!abi_calls);total++;
 }
 /* 8002C4BC now preserves guest pointers in a complete native body. Its
  * differential, round-trip and cache-precedence tests live in the model
  * unrelocator suite; it must never reach a typed host-pointer probe. */
 const uint32_t service_targets[]={0x8002c644,0x80031894,0x80038428,0x8003864c};
 void *functions[]={abi_model,abi_heap_size,abi_sound_add,abi_sound_find};
 for(unsigned t=0;t<4;t++)for(unsigned a=0;a<7;a++)for(unsigned v=0;v<8;v++){
  abi_setup(&cpu,service_targets[t],functions[t]);cpu.gpr[4]=addresses[a];cpu.gpr[5]=words[v];
  void *expected=module_memory_pointer(&rt,addresses[a],1,0);assert(expected);
  abi_result=words[v];abi_return=v&1?(void *)(host+0x200):(void *)PSX_ADDR(0x80100200);
  abi_run(&cpu,service_targets[t],PC_PORT_MIPS_HALTED);assert(abi_calls==1&&abi_pointer==expected);
  if(t==2)assert(cpu.gpr[2]==0x13579bdf&&cpu.gpr[3]==0x2468ace0);
  else if(t==3)assert(abi_words[0]==(uint32_t)(int32_t)(int16_t)words[v]&&cpu.gpr[2]==(v&1?(uint32_t)(uintptr_t)(host+0x200):0x80100200)&&!cpu.gpr[3]);
  else assert(cpu.gpr[2]==words[v]&&cpu.gpr[3]==(t==1?0x2468ace0:0));
  total++;
 }
 abi_setup(&cpu,0x8003864c,abi_sound_find);cpu.gpr[4]=0;cpu.gpr[5]=0xfeed8000;abi_return=NULL;
 abi_run(&cpu,0x8003864c,PC_PORT_MIPS_HALTED);assert(abi_calls==1&&!abi_pointer&&abi_words[0]==0xffff8000&&!cpu.gpr[2]&&!cpu.gpr[3]);total++;
 /* The size result is a raw word even when its bits resemble a host pointer. */
 abi_setup(&cpu,0x80031894,abi_heap_size);cpu.gpr[4]=addresses[0];abi_result=(uint32_t)(uintptr_t)PSX_ADDR(0x80100300);
 abi_run(&cpu,0x80031894,PC_PORT_MIPS_HALTED);assert(abi_calls==1&&cpu.gpr[2]==abi_result&&cpu.gpr[3]==0x2468ace0);total++;
 for(unsigned t=0;t<4;t++){
  const uint32_t bad[]={0x1e000000,0x80100101,t==1?0x80000004:0x801fffff};
  for(unsigned b=0;b<3;b++){abi_setup(&cpu,service_targets[t],functions[t]);cpu.gpr[4]=bad[b];abi_run(&cpu,service_targets[t],PC_PORT_MIPS_UNRESOLVED_CALL);assert(!abi_calls);total++;}
 }
 for(unsigned t=0;t<7;t++){
  uint32_t target=t<4?service_targets[t]:t==4?0x80030988:t==5?0x80043a1c:0x8002dde4;
  void *fn=t<4?functions[t]:t==4?(void *)abi_sound_mode:t==5?(void *)abi_tpage:(void *)abi_upload;
  abi_setup(&cpu,target,fn);cpu.gpr[4]=addresses[0];cpu.load_valid=1;cpu.load_reg=4;
  abi_run(&cpu,target,PC_PORT_MIPS_UNRESOLVED_CALL);assert(!abi_calls);total++;
 }
 abi_setup(&cpu,0x8003864c,abi_sound_find);cpu.gpr[4]=addresses[0];abi_return=(void *)(uintptr_t)UINT64_C(0x100000000);
 abi_run(&cpu,0x8003864c,PC_PORT_MIPS_UNRESOLVED_CALL);assert(abi_calls==1&&cpu.gpr[2]==0x13579bdf&&cpu.gpr[3]==0x2468ace0);total++;
 /* Two guest mappings can be adjacent without contiguous host storage. */
 abi_setup(&cpu,0x80031894,abi_heap_size);cpu.gpr[4]=0x80060000;
 abi_run(&cpu,0x80031894,PC_PORT_MIPS_UNRESOLVED_CALL);assert(!abi_calls);total++;
 return total;
}
static int query_oracle_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{(void)opaque;(void)cpu;assert(target-0x80031894u<20u);return 0;}
static unsigned query_original(void)
{
 uint8_t original_body[20];FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(fseek(f,0x22094,SEEK_SET)==0&&fread(original_body,1,20,f)==20);fclose(f);
 const uint32_t sizes[]={0,1,4,8,0x100,0x7fffffff,0x80000000,0xfffffff8,0xffffffff};
 unsigned total=0;
 for(unsigned n=0;n<sizeof(sizes)/sizeof(sizes[0]);n++){
  PcPortMipsCpu oracle,native;abi_setup(&oracle,0x80031894,HeapGetNextBlockHeader);
  memcpy(PSX_ADDR(0x80031894),original_body,20);HeapBlock *p=(HeapBlock *)(host+0x108);p[-1].pNext=(mem_addr)(uintptr_t)p+8u+sizes[n];
  oracle.gpr[4]=(uint32_t)(uintptr_t)p;oracle.bus.bridge=query_oracle_bridge;
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  assert(PcPortMipsRun(&oracle,0x80031894,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  abi_setup(&native,0x80031894,HeapGetNextBlockHeader);native.gpr[4]=(uint32_t)(uintptr_t)p;
  abi_run(&native,0x80031894,PC_PORT_MIPS_HALTED);
  assert(native.gpr[2]==oracle.gpr[2]&&native.gpr[2]==sizes[n]&&native.gpr[3]==oracle.gpr[3]);total++;
 }
 /* Actual host heap links are host words. Alias resolution is a separate
  * native integration check, not raw guest-header representation parity. */
 const uint32_t aliases[]={0x80100108,0xa0100108,0x80300108,0x100108,0x80050108,0x1f800108};
 for(unsigned a=0;a<6;a++)for(unsigned n=0;n<sizeof(sizes)/sizeof(sizes[0]);n++){
  PcPortMipsCpu cpu;abi_setup(&cpu,0x80031894,HeapGetNextBlockHeader);cpu.gpr[4]=aliases[a];
  HeapBlock *p=module_memory_pointer(&rt,aliases[a],1,0);p[-1].pNext=(mem_addr)(uintptr_t)p+8u+sizes[n];
  abi_run(&cpu,0x80031894,PC_PORT_MIPS_HALTED);assert(cpu.gpr[2]==sizes[n]&&cpu.gpr[3]==0x2468ace0);total++;
 }
 return total;
}
int main(int argc,char **argv)
{
 assert((uintptr_t)host+sizeof(host)<=UINT32_MAX);
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0&&setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2&&strcmp(argv[1],"--query")==0){printf("ACTOR SERVICE QUERY PASS %u original/native-alias cases; native steps=0\n",query_original());return 0;}
 printf("ACTOR SERVICE ABI PASS %u concrete signature/alias/refusal cases; sound/model bodies modeled; native steps=0\n",abi_interfaces());
 printf("ACTOR SERVICE QUERY PASS %u original/native-alias cases; native steps=0\n",query_original());return 0;
}
