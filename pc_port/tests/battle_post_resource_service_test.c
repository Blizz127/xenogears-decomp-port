/* Concrete service ABI probes plus exact native C service bodies. Internal
 * heap/SPU/event dependencies are explicit models; no audio hardware proof. */
#define main post_resource_caller_fixture_main
#include "battle_post_resource_host_test.c"
#undef main
static unsigned abi_calls,reset_calls,alloc_calls,free_calls,events,error_code;
static uint32_t abi_a,abi_b,alloc_size,alloc_flags;
static void *abi_pointer;
static uint8_t allocation[0xa000];
static void probe_reset(void){abi_calls++;}
static void probe_gfx(int a,unsigned int b){abi_calls++;abi_a=(uint32_t)a;abi_b=b;}
static void probe_sound(SoundWDSEntry *p){abi_calls++;abi_pointer=p;}
static void probe_mask(int a){abi_calls++;abi_a=(uint32_t)a;}
/* Definitions needed by the unchanged bodies copied from their native TUs. */
void *g_TimerWorkList,*g_WorkList,*g_GfxWorkBuffers,*g_GfxWorkBuffer2;
int g_NumTimerWorkListEntries,g_NumWorkListEntries,g_WorkListCurTimer,g_GfxWorkBufferSize;
u32 D_80059304,D_80059300,g_GfxImageList[1];
SoundWDSEntry *g_SoundWdsLinkedList;u32 g_unk_SoundEvent;
static void *service_heap_alloc(u_int size,u_int flags){alloc_calls++;alloc_size=size;alloc_flags=flags;return allocation;}
static void service_list_reset(void){reset_calls++;}
static int service_spu_free(int address){free_calls++;return address;}
static void service_sound_free(void *p){abi_pointer=p;}
static void service_sound_error(s32 code){error_code=(unsigned)code;}
static int service_event(unsigned int event){assert(event==123);events++;return 1;}
#define NUM_RENDER_CONTEXTS 2
#define HeapAlloc service_heap_alloc
#define func_8001D298 service_list_reset
#define SoundSpuMemoryFreeBlock service_spu_free
#define SoundHeapFree service_sound_free
#define SoundHandleError service_sound_error
#define DisableEvent service_event
#define EnableEvent service_event
#define XENO_GPU_TRACE(...) ((void)0)
#include POST_RESOURCE_NATIVE_SERVICES
#undef HeapAlloc
#undef func_8001D298
#undef SoundSpuMemoryFreeBlock
#undef SoundHeapFree
#undef SoundHandleError
#undef DisableEvent
#undef EnableEvent
#undef XENO_GPU_TRACE
static void abi_setup(PcPortMipsCpu *cpu,uint32_t target,void *fn,int cached)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;
 rt.functions[0]=(ResolvedFunction){target,fn,"post-resource-abi-probe"};rt.function_count=1;
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,target);slot->target=target;slot->state=1;slot->entry=rt.functions[0];}
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x13579bdfu;cpu->gpr[3]=0x2468ace0u;
 cpu->gpr[29]=0x1e000000u; /* None of these four signatures reads stack args. */
 abi_calls=0;abi_pointer=NULL;abi_a=abi_b=0;observing=0;
}
static void abi_run(PcPortMipsCpu *cpu,uint32_t target,int success)
{
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();uint32_t gpr[32];memcpy(gpr,cpu->gpr,sizeof(gpr));
 int rc=PcPortMipsRun(cpu,target,BATTLE_HALT_PC,100);
 assert(rc==(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL));
 assert(!cpu->steps&&!rt.bridge_cpu&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 assert(!memcmp(gpr,cpu->gpr,sizeof(gpr)));
}
int main(void)
{
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);unsigned total=0;
 const uint32_t words[]={0,1,0x8000,0xffff,0x12345678,0x80000000,0xa0100000,0xffffffff};
 uint32_t targets4[]={0x8001c944,0x80024f64,0x80038310,0x80044534};void *probes[]={probe_reset,probe_gfx,probe_sound,probe_mask};
 PcPortMipsCpu cpu;
 for(int cached=0;cached<2;cached++){
  for(unsigned i=0;i<8;i++)for(unsigned j=0;j<8;j++){
   abi_setup(&cpu,targets4[1],probes[1],cached);cpu.gpr[4]=words[i];cpu.gpr[5]=words[j];abi_run(&cpu,targets4[1],1);assert(abi_calls==1&&abi_a==words[i]&&abi_b==words[j]);total++;
  }
  for(unsigned i=0;i<8;i++){
   abi_setup(&cpu,targets4[3],probes[3],cached);cpu.gpr[4]=words[i];abi_run(&cpu,targets4[3],1);assert(abi_calls==1&&abi_a==words[i]);total++;
  }
  abi_setup(&cpu,targets4[0],probes[0],cached);abi_run(&cpu,targets4[0],1);assert(abi_calls==1);total++;
  const uint32_t addresses[]={0,0x80100100,0xa0100100,0x80300100,0x100100,0x80050100,0x1f800100,(uint32_t)(uintptr_t)host+0x100};
  for(unsigned i=0;i<8;i++){
   abi_setup(&cpu,targets4[2],probes[2],cached);cpu.gpr[4]=addresses[i];void *expected=i?module_memory_pointer(&rt,addresses[i],sizeof(SoundWDSEntry),1):NULL;
   abi_run(&cpu,targets4[2],1);assert(abi_calls==1&&abi_pointer==expected);total++;
  }
  const uint32_t bad[]={0x1e000000,0x80100101,0x801fffe0,0x1f800ff0,(uint32_t)(uintptr_t)(host+sizeof(host)-0x20u)};
  for(unsigned i=0;i<5;i++){
   abi_setup(&cpu,targets4[2],probes[2],cached);cpu.gpr[4]=bad[i];abi_run(&cpu,targets4[2],0);assert(!abi_calls);total++;
  }
  for(unsigned i=0;i<4;i++){
   abi_setup(&cpu,targets4[i],probes[i],cached);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,targets4[i],0);assert(!abi_calls);total++;
  }
 }
 printf("POST RESOURCE SERVICE ABI PASS %u concrete signature, alias, cached, void, stack-independent and refusal cases\n",total);
 /* Pointer-width native state: a four-byte zero store would fail these. */
 g_TimerWorkList=(void *)(uintptr_t)UINT64_C(0x1234567887654321);g_WorkList=(void *)(uintptr_t)UINT64_C(0xfedcba9876543210);
 g_NumTimerWorkListEntries=7;g_NumWorkListEntries=9;g_WorkListCurTimer=11;
 abi_setup(&cpu,targets4[0],WorkListsReset,0);abi_run(&cpu,targets4[0],1);
 assert(!g_TimerWorkList&&!g_WorkList&&!g_NumTimerWorkListEntries&&!g_NumWorkListEntries&&!g_WorkListCurTimer);
 D_80059304=D_80059300=g_GfxImageList[0]=0xffffffffu;
 abi_setup(&cpu,targets4[1],GfxAllocateWorkBuffers,1);cpu.gpr[4]=0x5000;cpu.gpr[5]=0x80000001u;abi_run(&cpu,targets4[1],1);
 assert(alloc_calls==1&&alloc_size==0xa000&&alloc_flags==0x80000001u&&reset_calls==1&&g_GfxWorkBufferSize==0x5000);
 assert(g_GfxWorkBuffers==allocation&&g_GfxWorkBuffer2==allocation+0x5000&&!D_80059304&&!D_80059300&&!g_GfxImageList[0]);
 for(unsigned i=0;i<8;i++){abi_setup(&cpu,targets4[3],SetDispMask,0);cpu.gpr[4]=words[i];abi_run(&cpu,targets4[3],1);assert(g_GPUDisabledState==(words[i]==0));}
 for(unsigned which=0;which<4;which++){
  SoundWDSEntry *p=(SoundWDSEntry *)(host+0x1000);memset(p,0,3*sizeof(*p));g_SoundWdsLinkedList=p;
  p[0].pNext=SOUND_PTR_TO_PSX(p+1);p[1].pNext=SOUND_PTR_TO_PSX(p+2);p[0].spuMemoryAddress=5;p[1].spuMemoryAddress=7;p[2].spuMemoryAddress=9;
  events=free_calls=error_code=0;g_unk_SoundEvent=123;
  abi_setup(&cpu,targets4[2],SoundFreeWdsEntry,1);cpu.gpr[4]=which<3?(uint32_t)(uintptr_t)(p+which):0;abi_run(&cpu,targets4[2],1);
  if(which==3){assert(error_code==0x11&&!events&&!free_calls&&g_SoundWdsLinkedList==p);}
  else {assert(!error_code&&events==2&&free_calls==1&&abi_pointer==p+which);
   if(which==0)assert(g_SoundWdsLinkedList==p+1);else assert(g_SoundWdsLinkedList==p&&p[which-1].pNext==p[which].pNext);}
 }
 puts("POST RESOURCE SERVICE BODY PASS native pointer-width resets, allocation offsets, display flag and WDS unlink; heap/SPU/events modeled; native interpreted instructions=0");return 0;
}
