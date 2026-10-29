/* Original constructor versus native with the actual existing heap and
 * StoreImage/GR_ReadVRAM implementations. DrawSync uses actual LIBGPU with
 * its graphics-upload boundary observed by a linker wrapper; no queued
 * primitives or hardware GPU execution is accepted by this fixture. */
#define main effect_constructor_model_main
#include "battle_effect_constructor_host_test.c"
#undef main
extern void *HeapAlloc(uint32_t,uint32_t);
extern void HeapChangeCurrentUser(uint32_t,char **);
extern void allocation_fixture_reset(unsigned);
extern void allocation_fixture_state(uint64_t *);
extern unsigned short vram[1024*512];
extern void (*drawsync_callback)(void);
int g_splitIndex;
static unsigned upload_calls,sync_callbacks,store_calls,heap_calls,user_calls;
static unsigned expected_uploads,expected_syncs,expected_stores,expected_heaps,expected_users;
static uint64_t expected_heap[16],heap_at_call[8][16];
void __wrap_GR_UpdateVRAM(void){upload_calls++;}
static void sync_callback(void)
{sync_callbacks++;if(sync_callbacks!=upload_calls)fail("DrawSync upload/callback order",sync_callbacks);}
static uint16_t pixel(unsigned x,unsigned y,unsigned salt)
{return (uint16_t)(x*23u+y*173u+salt*997u+(x^y)*41u);}
static int actual_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==targets[0]&&cpu->gpr[31]==BATTLE_HALT_PC)return candidate?runtime_bridge(opaque,cpu,target):0;
 if(body_index(target)>=0)return 0;
 uint64_t heap[16];allocation_fixture_state(heap);
 if(compare){if(call_count>=expected_calls||memcmp(heap,heap_at_call[call_count],sizeof(heap)))fail("heap metadata at service boundary",target);}
 else{assert(call_count<8);memcpy(heap_at_call[call_count],heap,sizeof(heap));}
 record_call(cpu,target);
 if(target==0x80032498u){user_calls++;if(cpu->gpr[4]!=4||cpu->gpr[5])fail("actual heap user",cpu->gpr[4]);}
 else if(target==0x80031bdcu){heap_calls++;if(cpu->gpr[5])fail("actual heap flags",cpu->gpr[5]);}
 else if(target==0x800448f8u)store_calls++;
 else if(target!=0x800445d0u)fail("unexpected actual service",target);
 if(candidate)return runtime_bridge(opaque,cpu,target);
 if(target==0x80032498u)HeapChangeCurrentUser(cpu->gpr[4],NULL);
 else if(target==0x80031bdcu){
  void *p=HeapAlloc(cpu->gpr[4],cpu->gpr[5]);
  if(p&&((uintptr_t)p<(uintptr_t)g_PsxRam||(uintptr_t)p>=(uintptr_t)g_PsxRam+PSX_RAM_SIZE))fail("actual heap outside RAM",cpu->gpr[4]);
  cpu->gpr[2]=p?0x80000000u+(uint32_t)((uintptr_t)p-(uintptr_t)g_PsxRam):0;cpu->gpr[3]=0;
 }else if(target==0x800448f8u){
  RECT16 *r=module_memory_pointer(&rt,cpu->gpr[4],sizeof(*r),0);assert(r&&r->w>0&&r->h>0);
  u_long *p=module_memory_pointer(&rt,cpu->gpr[5],(uint32_t)r->w*(uint32_t)r->h*2u,1);assert(p);
  cpu->gpr[2]=(uint32_t)StoreImage(r,p);
 }else{cpu->gpr[2]=(uint32_t)DrawSync((int32_t)cpu->gpr[4]);cpu->gpr[3]=0;}
 return 1;
}
static void actual_setup(PcPortMipsCpu *cpu,unsigned cached)
{
 setup(cpu);observing=0;cpu->bus.bridge=actual_bridge;allocation_fixture_reset(seed);
 rt.functions[0]=(ResolvedFunction){0x80031bdcu,HeapAlloc,"HeapAlloc"};
 rt.functions[1]=(ResolvedFunction){0x80032498u,HeapChangeCurrentUser,"HeapChangeCurrentUser"};
 rt.functions[2]=(ResolvedFunction){0x800445d0u,DrawSync,"DrawSync"};
 rt.functions[3]=(ResolvedFunction){0x800448f8u,StoreImage,"StoreImage"};rt.function_count=4;
 if(cached)for(unsigned i=0;i<4;i++){
  BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,rt.functions[i].address);
  slot->target=rt.functions[i].address;slot->state=1;slot->entry=rt.functions[i];
 }
 for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)vram[y*1024u+x]=pixel(x,y,seed);
 upload_calls=sync_callbacks=store_calls=heap_calls=user_calls=0;g_splitIndex=0;drawsync_callback=sync_callback;
 observing=1;
}
static void actual_pixels(void)
{
 if(scenario==1)return;
 unsigned type=type_arg&0xffffu;
 if(type!=0&&type!=1&&type!=4&&type!=5)return;
 uint32_t flags=flags_arg&0xffffu;if(!(type&1u))flags&=0xfd0fu;if(type&4u)flags&=0xfeffu;
 unsigned w=type<2?((width_arg+1u)/2u)*2u:width_arg,h=height_arg;
 for(unsigned bank=0;bank<2;bank++){
  unsigned mode=(flags>>(bank*4u))&15u;if(mode!=1&&mode!=2)continue;
  uint32_t address=peek(effect+4u+bank*4u,4);assert(address);
  for(unsigned row=0;row<h;row++)for(unsigned col=0;col<w;col++){
   uint16_t value;
   if(type<2&&mode==1)value=pixel((bank?11u:3u)+col,(bank?13u:5u)+row,seed);
   else if(type<2)value=(uint16_t)(bank?((17u<<10)+(13u<<5))|11u:((7u<<10)+(5u<<5))|3u);
   else if(mode==1)value=(uint16_t)peek(source+((bank?13u:5u)+row)*6u+((bank?11u:3u)+col)*2u,2);
   else{static const uint16_t colors[2][3]={{3,5,7},{11,13,17}};value=colors[bank][(out_y+row)%3u];}
   if(peek(address+(row*w+col)*2u,2)!=value)fail("independent composed pixel witness",row*w+col);
  }
 }
 for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)if(vram[y*1024u+x]!=pixel(x,y,seed))fail("StoreImage changed VRAM",y*1024u+x);
}
static void actual_case(unsigned cached)
{
 PcPortMipsCpu original,native;actual_setup(&original,cached);candidate=compare=0;
 int rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,200000);
 if(rc!=PC_PORT_MIPS_HALTED)fail("original with actual heap/SDK",original.pc);
 actual_pixels();expected_calls=call_count;expected_accesses=accesses;
 expected_uploads=upload_calls;expected_syncs=sync_callbacks;expected_stores=store_calls;expected_heaps=heap_calls;expected_users=user_calls;
 snapshot();allocation_fixture_state(expected_heap);
 actual_setup(&native,cached);candidate=compare=1;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;uint64_t heap[16];allocation_fixture_state(heap);
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||native_gte_commands||rt.bridge_cpu||call_count!=expected_calls||accesses!=expected_accesses||
    upload_calls!=expected_uploads||sync_callbacks!=expected_syncs||store_calls!=expected_stores||heap_calls!=expected_heaps||user_calls!=expected_users||
    memcmp(heap,expected_heap,sizeof(heap))||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("actual service/counter/heap state",native.pc);
 if(memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||!memory_equal())fail("actual composition complete state",seed);
 actual_pixels();
}
static unsigned sync_probe_calls;
static uint32_t sync_probe_value;
static int sync_probe_result;
static int sync_probe(int mode)
{sync_probe_calls++;sync_probe_value=(uint32_t)mode;return sync_probe_result;}
static unsigned sync_interfaces(void)
{
 static const uint32_t values[]={0,1,0xffffffffu,0x80000000u,0x7fffffffu,0x80100000u,0xa0100000u,0x1f800100u};
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned v=0;v<8;v++)for(unsigned pending=0;pending<2;pending++)for(unsigned sign=0;sign<2;sign++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=1;candidate=1;compare=0;cpu.bus.bridge=runtime_bridge;
  rt.functions[0]=(ResolvedFunction){0x800445d0u,sync_probe,"DrawSync"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x800445d0u);slot->target=0x800445d0u;slot->state=1;slot->entry=rt.functions[0];}
  cpu.gpr[4]=values[v];cpu.gpr[29]=0x1e000001u;cpu.load_valid=(uint8_t)pending;cpu.load_reg=4;
  PcPortMipsCpu before=cpu;snapshot();sync_probe_calls=0;sync_probe_value=0;sync_probe_result=sign?-12345:0x1234567;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,0x800445d0u,BATTLE_HALT_PC,100);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
  if(!pending){before.gpr[2]=(uint32_t)sync_probe_result;before.gpr[3]=0;}
  if(rc!=(pending?PC_PORT_MIPS_UNRESOLVED_CALL:PC_PORT_MIPS_HALTED)||cpu.steps||rt.bridge_cpu||accesses||call_count||
     sync_probe_calls!=(pending?0u:1u)||(!pending&&sync_probe_value!=values[v])||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr))||
     memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||cpu.hi!=before.hi||cpu.lo!=before.lo||!memory_equal()||
     PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("DrawSync typed scalar/return/no-stack/pending boundary",values[v]);
  total++;
 }
 defaults();return total;
}
#ifndef EFFECT_CONSTRUCTOR_INTEGRATION_MAIN
#define EFFECT_CONSTRUCTOR_INTEGRATION_MAIN main
#endif
int EFFECT_CONSTRUCTOR_INTEGRATION_MAIN(int argc,char **argv)
{
 load_inputs();defaults();
 if(argc>1){
  if(!strcmp(argv[1],"--integration-basic")){actual_case(0);return 0;}
  if(!strcmp(argv[1],"--sync-interface")){printf("EFFECT CONSTRUCTOR DRAWSYNC ABI PASS %u\n",sync_interfaces());return 0;}
  if(!strcmp(argv[1],"--integration"))goto integration;
  return effect_constructor_model_main(argc,argv);
 }
 if(effect_constructor_model_main(argc,argv))return 1;
integration:
 printf("EFFECT CONSTRUCTOR DRAWSYNC ABI PASS %u cached/resolved scalar and return cases, pending refusals, no stack reads\n",sync_interfaces());fflush(stdout);
 ;unsigned total=0;const unsigned types[]={0,1,2,3,4,5};
 for(unsigned type=0;type<6;type++)for(unsigned mode=0;mode<4;mode++)for(unsigned s=0;s<8;s++)for(unsigned cached=0;cached<2;cached++){
  defaults();seed=s;task_domain=s%6u;stack_kind=s%4u;type_arg=types[type];flags_arg=0x700u+(mode&1u?0x10u:0x20u)+(mode&2u?1u:2u);
  width_arg=2+s%4u;height_arg=1+s%3u;out_y=s%3u;actual_case(cached);total++;
 }
 defaults();scenario=1;actual_case(0);total++;
 printf("EFFECT CONSTRUCTOR ACTUAL SERVICES PASS %u original/native constructor compositions; actual heap and StoreImage/GR_ReadVRAM; actual DrawSync with observed upload boundary and no queued primitives; native interpreted instructions=0\n",total);
 return 0;
}
