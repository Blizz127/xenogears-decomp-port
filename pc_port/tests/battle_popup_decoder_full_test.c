/* Synthetic differential only: callback mutation is simulated; no production
 * decoder dispatch, real vblank timing, GPU presentation, or rollback is verified. */
/* The shade child uses its guarded runtime bridge; decoder remains test-only. */
#include "battle_mips_runtime.c"
#include "psx/gtereg.h"
/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
uint32_t g_HeapNeedsConsolidation;
uint32_t g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304;
uintptr_t g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer;
unsigned int g_HeapIsErrorHandlerOff;
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
extern int func_80025FA8(uint8_t *, int, uint8_t *, int, int, int, int, int, int)
    __attribute__((weak));
extern int ReadGeomScreen(void);
extern void ReadGeomOffset(long *, long *);
extern MATRIX *ScaleMatrixL(MATRIX *, VECTOR *);

static int sdk_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target >= 0x80025fa8u && target < 0x80026338u) return 0;
    if (target >= 0x80043a1cu && target < 0x80043d00u) return 0;
    return runtime_bridge(opaque, cpu, target);
}

static void initial_gte(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(123, 5); CTC2(456, 6); CTC2(789, 7);
    SetGeomOffset(160, 112); SetGeomScreen(512);
}


extern unsigned char D_8001C76C[0x170];
static int native_side,simulate_callback,shade_calls;
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){
 if(t!=0x80043c24u)return 0;
 shade_calls++;
 if(simulate_callback && shade_calls==4) {
  uint8_t *mode=resolve_memory(p,c->gpr[17]+3u,1u,1);
  uint8_t *flags=resolve_memory(p,c->gpr[17]+2u,1u,1);
  assert(mode && flags);*mode^=2u;*flags^=1u;
 }
 if(!native_side)return 0;
 return runtime_bridge(p,c,t);
}
static unsigned unlinked_callback_calls;
static unsigned forbidden_legacy_calls;
static uintptr_t forbidden_legacy(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,
                                  uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,
                                  uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{
 (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;
 forbidden_legacy_calls++;return 0;
}
static int unlinked_callback_bridge(void *p,PcPortMipsCpu *c,uint32_t t)
{
 if(t!=0x8001cb48u)return 0;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int result=runtime_bridge(p,c,t);
 assert(result==1 && PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 unlinked_callback_calls++;return result;
}
static unsigned unlinked_timer_callback_calls;
static int unlinked_timer_callback_bridge(void *p,PcPortMipsCpu *c,uint32_t t)
{
 if(t!=0x8001cd94u)return 0;
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int result=runtime_bridge(p,c,t);
 assert(result==1 && PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 unlinked_timer_callback_calls++;return result;
}
static void fake_copy_heap(uint8_t *buffer)
{
 uintptr_t base=(uintptr_t)buffer;assert(base<=UINT32_MAX-512u);
 store_le(buffer-8u,4,(uint32_t)base+424u);store_le(buffer-4u,4,2u<<21);
 store_le(buffer+416u,4,(uint32_t)base+512u);store_le(buffer+420u,4,0);
 store_le(buffer+504u,4,0);store_le(buffer+508u,4,1u<<21);
 g_Heap=buffer;g_HeapNeedsConsolidation=0;
}
/* Test-only active sprite leaf model. Fixed disjoint synthetic buffers make no
 * claim about heap ownership, aliases, callbacks, or production dispatch. */
static void active_sprite_model(BattleMipsRuntime *rt, PcPortMipsCpu *cpu)
{
 uint32_t sprite=cpu->gpr[4], sp=cpu->gpr[29], flags, base, other, prim, color, value, count;
 assert(!runtime_read(rt,sprite+0x3cu,4,&flags) && (flags&3u)==1u);
 assert(!runtime_read(rt,sprite+0x20u,4,&base));
 assert(!runtime_read(rt,sprite+0x24u,4,&other));
 assert(!runtime_read(rt,other+4u,4,&value));
 assert(!runtime_write(rt,sp-8u,4,value));
 assert(!runtime_read(rt,sprite+0x40u,1,&count)); count>>=2;
 assert(!runtime_read(rt,sprite+0x28u,4,&color));
 assert(!runtime_read(rt,base+0x30u,4,&prim));
 uint32_t abr=(flags>>5)&7u; if(abr) --abr;
 cpu->gpr[9]=sprite;cpu->gpr[7]=abr;cpu->gpr[8]=0;
 cpu->gpr[2]=count;cpu->gpr[3]=base;cpu->gpr[4]=prim;cpu->gpr[5]=color;
 if(count) cpu->gpr[3]=abr<<5;
 for(unsigned i=0;i<count;i++) {
  uint32_t tpage; assert(!runtime_read(rt,prim+10u,2,&tpage));
  assert(!runtime_write(rt,prim+16u,4,color));
  assert(!runtime_write(rt,prim+10u,2,(tpage&0xff9fu)|(abr<<5)));
  prim+=24u;cpu->gpr[8]++;
 }
 cpu->gpr[4]=prim;
}
/* Test-only composition of the timer and its actual guarded leaf bridges. */
static void composed_menu_timer(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=32u;r[2]=r[4];battle_entry_write(e,r[29]+16u,4,r[16]);r[16]=r[2];
 battle_entry_write(e,r[29]+28u,4,r[31]);battle_entry_write(e,r[29]+24u,4,r[18]);battle_entry_write(e,r[29]+20u,4,r[17]);
 r[3]=battle_entry_read(e,r[2]+32u,4);r[18]=battle_entry_read(e,r[16]+28u,4);r[17]=r[3]<<6;r[4]=r[17];
 battle_phase_call(e,0x8003f8b0u,0x800bcbdcu);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;r[2]=(int32_t)r[3]<256;
 if(!r[2])r[3]=255u;battle_entry_write(e,r[18]+0x28u,1,r[3]);
 r[4]=r[17];battle_phase_call(e,0x8003f8ccu,0x800bcc04u);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;r[2]=(int32_t)r[3]<256;r[4]=r[18];
 if(!r[2])r[3]=255u;battle_entry_write(e,r[4]+0x29u,1,r[3]);battle_entry_write(e,r[4]+0x2au,1,r[3]);
 battle_phase_call(e,0x8001f6b0u,0x800bcc2cu);
 r[2]=battle_entry_read(e,r[16]+0x20u,4);r[2]++;battle_entry_write(e,r[16]+0x20u,4,r[2]);
 r[31]=battle_entry_read(e,r[29]+28u,4);r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=32u;
}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
static uintptr_t fixture_timer_head,fixture_timer_cursor;
static uintptr_t fixture_work_head;
static uint32_t fixture_work_count;
static uint32_t fixture_timer_count,fixture_timer_marked;
#if defined(__linux__)
typedef struct {BattleMipsRuntime *runtime;PcPortMipsCpu *cpu;int result;} TimerThreadProbe;
static void *timer_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_timer_remove_try_host(p->runtime,p->cpu,0x8001cd94u);return NULL;
}
static void *unlink_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_unrelated_unlink_try_host(p->runtime,p->cpu,0x8001ce74u);return NULL;
}
static void *color_work_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_color_work_remove_try_host(p->runtime,p->cpu,0x8001cb48u);return NULL;
}
static void *pinned_free_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_pinned_free_try_host(p->runtime,p->cpu,0x800320e8u);return NULL;
}
static void *unlinked_work_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_unlinked_work_try_host(p->runtime,p->cpu,0x8001cb48u);return NULL;
}
static void *unlinked_timer_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_unlinked_timer_try_host(p->runtime,p->cpu,0x8001cd94u);return NULL;
}
static void *one_null_foreign_thread(void *opaque)
{
 TimerThreadProbe *p=opaque;p->result=battle_popup_one_null_unlink_try_host(p->runtime,p->cpu,0x8001ce74u);return NULL;
}
#endif
#include "popup_cleanup_test_models.inc"
static unsigned queue_cleanup_calls;static int queue_cleanup_host,queue_unlink_actual,queue_free_actual;
static uint32_t queue_cleanup_trace[4][33];
static int queue_cleanup_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x80025180u) return queue_cleanup_host?runtime_bridge(opaque,cpu,target):0;
 const uint32_t services[]={0x8001f6b0u,0x8001ce74u,0x8001cd94u,0x800320e8u};
 if(queue_cleanup_calls>=4u || target!=services[queue_cleanup_calls])return 0;
 uint32_t *row=queue_cleanup_trace[queue_cleanup_calls++];row[0]=target;memcpy(row+1,cpu->gpr,sizeof cpu->gpr);
 if(target==0x8001f6b0u)return queue_cleanup_host?runtime_bridge(opaque,cpu,target):0;
 if(target==0x8001cd94u)return queue_cleanup_host?runtime_bridge(opaque,cpu,target):0;
 if(target==0x8001ce74u && queue_cleanup_host && queue_unlink_actual)return runtime_bridge(opaque,cpu,target);
 if(target==0x800320e8u && queue_cleanup_host && queue_free_actual)return runtime_bridge(opaque,cpu,target);
 if(!queue_cleanup_host)return 0;
 BattleEntryCall e={.runtime=opaque,.cpu=cpu};if(setjmp(e.failure))assert(!"synthetic cleanup child refused");
 if(target==0x8001ce74u)popup_unlink_no_callback(&e);
 else popup_heap_free_model(&e);
 return 1;
}
/* Test-only popup cleanup parent; list and heap children remain models. */
static void queue_cleanup_parent(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=32u;battle_entry_write(e,r[29]+16u,4,r[16]);r[16]=r[4];
 battle_entry_write(e,r[29]+24u,4,r[31]);battle_entry_write(e,r[29]+20u,4,r[17]);
 r[17]=battle_entry_read(e,r[16]+4u,4);r[4]=battle_entry_read(e,r[17]+0x38u,4);
 r[2]=battle_entry_read(e,r[4]+0x2bu,1)|1u;battle_entry_write(e,r[4]+0x2bu,1,r[2]);
 battle_phase_call(e,0x8001f6b0u,0x800bd054u);r[4]=battle_entry_read(e,r[17]+0x5cu,4);
 battle_phase_call(e,0x80025180u,0x800bd060u);r[4]=r[16];battle_phase_call(e,0x8001ce74u,0x800bd068u);
 r[4]=r[16];battle_phase_call(e,0x8001cd94u,0x800bd070u);r[4]=r[17];battle_phase_call(e,0x800320e8u,0x800bd078u);
 r[31]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);
 r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=32u;
}
#include "battle_popup_decoder_full_body.inc"
int main(void) {
 assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800b1720u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b1720u),1,0x780u,f)==0x780u);
 assert(!fseek(f,0x800b168cu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b168cu),1,0x18u,f)==0x18u);
 assert(!fseek(f,0x800bcbb4u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x800bcbb4u),1,0xacu,f)==0xacu);
 assert(!fseek(f,0x800bd024u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x800bd024u),1,0x74u,f)==0x74u);
 assert(!fseek(f,0x800b383cu-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x800b383cu),1,60u,f)==60u);fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80043c24u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x80043c24u),1,0x28u,f)==0x28u);
 assert(!fseek(f,0x8003f968u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8003f968u),1,52u,f)==52u);
 assert(!fseek(f,0x8001f6b0u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8001f6b0u),1,0xa0u,f)==0xa0u);
 assert(!fseek(f,0x80025180u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x80025180u),1,0x48u,f)==0x48u);
 assert(!fseek(f,0x8001cd94u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8001cd94u),1,0x2a0u,f)==0x2a0u);
 assert(!fseek(f,0x8001cb48u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8001cb48u),1,0xa0u,f)==0xa0u);
 assert(!fseek(f,0x800320e8u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x800320e8u),1,0xa4u,f)==0xa4u);
 assert(!fseek(f,0x8003f8b0u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8003f8b0u),1,28u,f)==28u);
 assert(!fseek(f,0x8003f8ccu-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8003f8ccu),1,28u,f)==28u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt); g_ActiveBattleRuntime=&rt;
 PcPortMipsCpu probe;initialize_cpu(&probe,&rt);rt.bridge_cpu=&probe;
 probe.gpr[4]=0x80140020u;probe.gpr[31]=0x800b1798u;
 unsetenv("XENO_BATTLE_NATIVE_POPUP_SHADE_EXPERIMENT");
 assert(!battle_atlas_gpu_try_host(&rt,&probe,0x80043c24u));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_SHADE_EXPERIMENT","1",1));
 probe.gpr[31]=0x800b179cu;assert(!battle_atlas_gpu_try_host(&rt,&probe,0x80043c24u));
 probe.gpr[31]=0x800b1798u;probe.gpr[4]=0xffffffffu;
 assert(!battle_atlas_gpu_try_host(&rt,&probe,0x80043c24u));rt.bridge_cpu=NULL;
 ResolvedData *d=find_data(&rt,0x8001c76cu,0x170u);assert(d && d->host==D_8001C76C && d->size==0x170u);
 uint8_t data[0x170];memcpy(data,D_8001C76C,sizeof data);
 unsetenv("XENO_BATTLE_NATIVE_POPUP_RECORD_EXPERIMENT");
 initialize_cpu(&probe,&rt);rt.bridge_cpu=&probe;probe.gpr[4]=0x8001c76cu;probe.gpr[31]=0x800bd150u;
 assert(!battle_popup_record_try_host(&rt,&probe,0x800b168cu));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_RECORD_EXPERIMENT","1",1));
 probe.gpr[5]=1u;assert(!battle_popup_record_try_host(&rt,&probe,0x800b168cu));
 probe.gpr[5]=0;probe.gpr[31]=0x800bd17cu;
 assert(!battle_popup_record_try_host(&rt,&probe,0x800b168cu));rt.bridge_cpu=NULL;
 const uint32_t record_returns[]={0x800bd150u,0x800bd178u,0x800bd1b4u};
 for(unsigned site=0;site<3;site++) for(unsigned representation=0;representation<2;representation++) {
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=representation?(uint32_t)(uintptr_t)D_8001C76C:0x8001c76cu;
  retail.gpr[5]=0;retail.gpr[31]=record_returns[site];PcPortMipsCpu native=retail;
  assert(PcPortMipsRun(&retail,0x800b168cu,record_returns[site],100)==PC_PORT_MIPS_HALTED);
  assert(runtime_bridge(&rt,&native,0x800b168cu)==1);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(native.gpr[2]==native.gpr[4]+12u && !memcmp(data,D_8001C76C,sizeof data));
 }
 fake_copy_heap(PSX_ADDR(0x80150000u));
 unsetenv("XENO_BATTLE_NATIVE_POPUP_COPY_EXPERIMENT");
 initialize_cpu(&probe,&rt);rt.bridge_cpu=&probe;probe.gpr[5]=0x80150000u;
 probe.gpr[4]=0x801500d0u;probe.gpr[6]=208u;probe.gpr[31]=0x800bd1a0u;
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_COPY_EXPERIMENT","1",1));
 probe.gpr[5]=0;probe.gpr[4]=208u;
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u));
 probe.gpr[5]=0x80150000u;probe.gpr[4]=0x801500cfu;
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u));
 probe.gpr[4]++;probe.gpr[29]=0x801ff000u;
 store_le(PSX_ADDR(0x80150000u)-4u,4,0);
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u)); /* free block */
 store_le(PSX_ADDR(0x80150000u)-4u,4,2u<<21);
 probe.gpr[29]=0x80150000u;
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u));
 probe.gpr[29]=0x801ff000u;probe.gpr[31]=0x800bd1a4u;
 assert(!battle_popup_copy_try_host(&rt,&probe,0x8003f968u));rt.bridge_cpu=NULL;
 for(unsigned representation=0;representation<2;representation++) {
  uint8_t *buffer=PSX_ADDR(0x80150000u);fake_copy_heap(buffer);
  for(unsigned i=0;i<416u;i++)buffer[i]=(uint8_t)(i*37u+13u);
  uint8_t before[416],after[416];memcpy(before,buffer,sizeof before);
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[5]=representation?(uint32_t)(uintptr_t)buffer:0x80150000u;
  retail.gpr[4]=retail.gpr[5]+208u;retail.gpr[6]=208u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x800bd1a0u;
  PcPortMipsCpu native=retail;
  assert(PcPortMipsRun(&retail,0x8003f968u,0x800bd1a0u,3000)==PC_PORT_MIPS_HALTED);
  memcpy(after,buffer,sizeof after);memcpy(buffer,before,sizeof before);
  assert(runtime_bridge(&rt,&native,0x8003f968u)==1);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(after,buffer,sizeof after));
 }
 /* Scoped first deferred graphics node: compare real CPU, globals and RAM. */
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_GFX_QUEUE_EXPERIMENT","1",1));
 for(unsigned context=0;context<2;context++) for(unsigned representation=0;representation<2;representation++) {
  uint8_t *work=PSX_ADDR(0x80150000u),*payload=PSX_ADDR(0x80151000u);
  uintptr_t w=(uintptr_t)work,p=(uintptr_t)payload;assert(w<=UINT32_MAX && p<=UINT32_MAX-0x500u);
  memset(work,0x5a,0x800u);memset(payload,0x39,416u);
  store_le(work-8u,4,(uint32_t)p);store_le(work-4u,4,2u<<21);
  store_le(payload-8u,4,(uint32_t)p+424u);store_le(payload-4u,4,2u<<21);
  store_le(payload+416u,4,(uint32_t)p+0x500u);store_le(payload+420u,4,0);
  store_le(payload+0x4f8u,4,0);store_le(payload+0x4fcu,4,1u<<21);
  g_Heap=work;g_HeapNeedsConsolidation=0;
  uint32_t wb=representation?(uint32_t)w:0x80150000u,pa=representation?(uint32_t)p:0x80151000u;
  g_GfxCurContext=context;g_GfxWorkBufferSize=0x400u;D_80059300=D_80059304=0;
  g_GfxWorkBuffers=wb;g_GfxWorkBuffer2=wb+0x400u;
  g_GfxCurWorkBuffer=wb+context*0x400u+0x20u;g_GfxCurWorkBufferEnd=wb+(context+1u)*0x400u;
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=pa;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x800bd068u;
  PcPortMipsCpu native=retail;rt.bridge_cpu=&native;
  uint32_t before[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  memcpy(initial,g_PsxRam,sizeof initial);
  D_80059304=1u;assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));D_80059304=0;
  native.gpr[31]+=4u;assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));native.gpr[31]-=4u;
  g_GfxCurWorkBufferEnd-=4u;assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));g_GfxCurWorkBufferEnd+=4u;
  if(sizeof(uintptr_t)>4u) {
   g_GfxWorkBuffers|=(uintptr_t)UINT32_MAX+1u;
   assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));g_GfxWorkBuffers=before[4];
  }
  native.gpr[4]=wb;assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));native.gpr[4]=pa;
  store_le(payload-4u,4,0);assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));store_le(payload-4u,4,2u<<21);
  ((uint8_t*)PSX_ADDR(0x80025180u))[0]^=1u;assert(!battle_popup_gfx_queue_try_host(&rt,&native,0x80025180u));((uint8_t*)PSX_ADDR(0x80025180u))[0]^=1u;
  uint32_t refused[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  assert(!memcmp(before,refused,sizeof before) && !memcmp(initial,g_PsxRam,sizeof initial));
  assert(PcPortMipsRun(&retail,0x80025180u,retail.gpr[31],100)==PC_PORT_MIPS_HALTED);
  uint32_t after[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  g_GfxCurContext=before[0];g_GfxWorkBufferSize=before[1];D_80059300=before[2];D_80059304=before[3];
  g_GfxWorkBuffers=before[4];g_GfxWorkBuffer2=before[5];g_GfxCurWorkBufferEnd=before[6];g_GfxCurWorkBuffer=before[7];
  assert(runtime_bridge(&rt,&native,0x80025180u)==1);
  uint32_t got[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  assert(!memcmp(after,got,sizeof after));
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));rt.bridge_cpu=NULL;
 }
 puts("popup deferred graphics queue bridge PASS fixed synthetic CPU/guard comparisons; no free executed");
 /* Install separate host bindings for the timer-list globals. */
 const uint32_t timer_addresses[]={0x8005958cu,0x80059590u,0x80059188u,0x80059464u};
 const char *timer_names[]={"g_TimerWorkList","D_80059590","g_NumTimerWorkListEntries","D_80059464"};
 uint8_t *timer_globals[]={(uint8_t*)&fixture_timer_head,(uint8_t*)&fixture_timer_cursor,(uint8_t*)&fixture_timer_count,(uint8_t*)&fixture_timer_marked};
 for(unsigned i=0;i<4;i++) {
  assert(!find_data(&rt,timer_addresses[i],4u));
  rt.data[rt.data_count++]=(ResolvedData){timer_addresses[i],4u,timer_globals[i],timer_names[i]};
 }
 assert(!find_data(&rt,0x80059594u,4u));
 rt.data[rt.data_count++]=(ResolvedData){0x80059594u,4u,(uint8_t*)&fixture_work_head,"g_WorkList"};
 qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 /* Compose guarded queue, sprite, timer and unrelated-list leaves with the parent. */
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TIMER_REMOVE_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_PINNED_FREE_EXPERIMENT","1",1));
 for(unsigned representation=0;representation<5;representation++) {
  if(representation==4)assert(!setenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT","1",1));
  else unsetenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");
  uint8_t *work=PSX_ADDR(0x80150000u),*payload=PSX_ADDR(0x80151000u),*task=PSX_ADDR(0x80152000u);
  uint8_t *object=PSX_ADDR(0x80152100u),*sprite=PSX_ADDR(0x80152200u),*child=PSX_ADDR(0x80152300u);
  uintptr_t w=(uintptr_t)work,p=(uintptr_t)payload;assert(w<=UINT32_MAX && p<=UINT32_MAX-0x500u);
  memset(work,0x5a,0x800u);memset(payload,0x39,416u);memset(task,0,0x100u);memset(object,0,0x100u);memset(sprite,0,0x100u);memset(child,0,0x40u);
  store_le(work-8u,4,(uint32_t)p);store_le(work-4u,4,2u<<21);
  store_le(payload-8u,4,(uint32_t)(uintptr_t)task);store_le(payload-4u,4,2u<<21);
  store_le(task-8u,4,(uint32_t)(uintptr_t)object);store_le(task-4u,4,2u<<21);
  store_le(object-8u,4,(uint32_t)(uintptr_t)sprite);
  store_le(sprite-8u,4,(uint32_t)(uintptr_t)child);store_le(sprite-4u,4,2u<<21);
  store_le(child-8u,4,(uint32_t)(uintptr_t)PSX_ADDR(0x80152400u));store_le(child-4u,4,2u<<21);
  uint8_t *heap_end=PSX_ADDR(representation>=2?0x801fc000u:0x80152600u);
  store_le(PSX_ADDR(0x801523f8u),4,(uint32_t)(uintptr_t)heap_end);store_le(PSX_ADDR(0x801523fcu),4,0);
  store_le(heap_end-8u,4,0);store_le(heap_end-4u,4,1u<<21);
  g_Heap=work;g_HeapNeedsConsolidation=0;
  uint32_t wb=representation?(uint32_t)w:0x80150000u,pa=representation?(uint32_t)p:0x80151000u;
  uint32_t ta=(uint32_t)(uintptr_t)task,oa=representation?(uint32_t)(uintptr_t)object:0x80152100u;
  uint32_t sa=representation?(uint32_t)(uintptr_t)sprite:0x80152200u;
  uint32_t ca=(uint32_t)(uintptr_t)child;
  store_le(task+4u,4,oa);store_le(object+0x38u,4,sa);store_le(object+0x5cu,4,pa);
  store_le(task+16u,4,0x12345u);store_le(child,4,(representation>=2 && representation!=4)?0x345678u:ta);store_le(child+20u,4,0x12345u);
  store_le(object-4u,4,0x1234u|(3u<<21)|(5u<<26)|(representation>=3?0x02000000u:0));
  MenuHeapSnapshot fixture_heap;
  assert(menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)g_Heap,0,1u,&fixture_heap));
  assert(fixture_heap.count==8u);
  const uintptr_t owned[]={(uintptr_t)work,(uintptr_t)payload,(uintptr_t)task,(uintptr_t)object,(uintptr_t)sprite,(uintptr_t)child};
  for(unsigned i=0;i<6;i++)assert(fixture_heap.blocks[i].address+8u==owned[i] && ((fixture_heap.blocks[i].meta>>21)&15u)>1u);
  fixture_timer_head=ta;fixture_timer_cursor=ta;fixture_timer_count=1;fixture_timer_marked=0;
  assert(!runtime_write(&rt,0x80059594u,4,ca));
  g_GfxCurContext=0;g_GfxWorkBufferSize=0x400u;D_80059300=D_80059304=0;
  g_GfxWorkBuffers=wb;g_GfxWorkBuffer2=wb+0x400u;g_GfxCurWorkBuffer=wb+0x20u;g_GfxCurWorkBufferEnd=wb+0x400u;
  uint32_t globals_before[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=queue_cleanup_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=ta;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu native=retail;
  memcpy(initial,g_PsxRam,sizeof initial);queue_cleanup_host=0;queue_cleanup_calls=0;
  assert(PcPortMipsRun(&retail,0x800bd024u,retail.gpr[31],1000)==PC_PORT_MIPS_HALTED && queue_cleanup_calls==4u);
  uint32_t trace[4][33];memcpy(trace,queue_cleanup_trace,sizeof trace);memcpy(expected,g_PsxRam,sizeof expected);
  uint32_t globals_after[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  uint32_t timers_after[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
  assert(g_HeapNeedsConsolidation==(representation>=3?0u:1u));
  memcpy(g_PsxRam,initial,sizeof initial);g_GfxCurContext=globals_before[0];g_GfxWorkBufferSize=globals_before[1];D_80059300=globals_before[2];D_80059304=globals_before[3];
  g_GfxWorkBuffers=globals_before[4];g_GfxWorkBuffer2=globals_before[5];g_GfxCurWorkBufferEnd=globals_before[6];g_GfxCurWorkBuffer=globals_before[7];
  fixture_timer_head=ta;fixture_timer_cursor=ta;fixture_timer_count=1;fixture_timer_marked=0;fixture_work_head=ca;
  g_HeapNeedsConsolidation=0;
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  queue_unlink_actual=representation>=2;queue_free_actual=representation>=3;queue_cleanup_host=1;queue_cleanup_calls=0;BattleEntryCall e={.runtime=&rt,.cpu=&native};
  if(setjmp(e.failure)==0)queue_cleanup_parent(&e);else assert(!"synthetic queue cleanup refused");
  uint32_t globals_got[]={g_GfxCurContext,g_GfxWorkBufferSize,D_80059300,D_80059304,g_GfxWorkBuffers,g_GfxWorkBuffer2,g_GfxCurWorkBufferEnd,g_GfxCurWorkBuffer};
  uint32_t timers_got[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
  assert(queue_cleanup_calls==4u && !memcmp(trace,queue_cleanup_trace,sizeof trace));
  assert(!memcmp(globals_after,globals_got,sizeof globals_after) && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(timers_after,timers_got,sizeof timers_after));
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  if(representation>=3)assert(PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
  assert(globals_got[2]==wb+0x20u && globals_got[7]==wb+0x28u);
  assert(load_le(work+0x20u,4)==pa && load_le(work+0x24u,4)==0 && (sprite[0x2bu]&1u));
  uint32_t state;assert(!runtime_read(&rt,0x80059594u,4,&state) && state==((representation>=2 && representation!=4)?ca:0));
  assert(!runtime_read(&rt,0x8005958cu,4,&state) && state==0);
  assert(!runtime_read(&rt,0x80059188u,4,&state) && state==0);
  assert(g_HeapNeedsConsolidation==(representation>=3?0u:1u));
  assert(load_le(object-4u,4)==(representation>=3?(0x1234u|(3u<<21)|(5u<<26)|0x02000000u):(0x21u<<26)));
 }
 unsetenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_TIMER_REMOVE_EXPERIMENT");
 unsetenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_PINNED_FREE_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");queue_unlink_actual=queue_free_actual=0;
 puts("popup cleanup guarded queue/sprite/timer/one-null/unrelated/pinned-return and modeled heap PASS fixed synthetic CPU comparisons; fixture ownership only");
 unsetenv("XENO_BATTLE_NATIVE_POPUP_GFX_QUEUE_EXPERIMENT");g_Heap=NULL;
 unsetenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT");
 initialize_cpu(&probe,&rt);rt.bridge_cpu=&probe;probe.gpr[4]=0x80160000u;probe.gpr[31]=0x800bd1ccu;
 store_le(PSX_ADDR(0x8016003cu),4,0);
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT","1",1));
 store_le(PSX_ADDR(0x8016003cu),4,1);
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));
 store_le(PSX_ADDR(0x8016003cu),4,0);probe.gpr[31]=0x800bd1d0u;
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));
 probe.gpr[31]=0x800bd05cu;store_le(PSX_ADDR(0x8016003cu),4,1u);
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));
 store_le(PSX_ADDR(0x8016003cu),4,0u);probe.gpr[31]=0x800bd060u;
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));
 probe.gpr[31]=0x800bd1ccu;probe.gpr[4]=0;
 assert(!battle_popup_sprite_noop_try_host(&rt,&probe,0x8001f6b0u));rt.bridge_cpu=NULL;
 for(unsigned mode=0;mode<5;mode++) {
  uint8_t *sprite=PSX_ADDR(0x80160000u);memset(sprite,0x5a,0x60u);
  store_le(sprite+0x3cu,4,(mode==0||mode==3)?0x13579b00u:0xabcdef02u);
  uint8_t before[0x60];memcpy(before,sprite,sizeof before);
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=(mode==0||mode==3)?0x80160000u:(uint32_t)(uintptr_t)sprite;
  retail.gpr[29]=0x801ff000u;retail.gpr[31]=mode>=3?0x800bd05cu:mode==2?0x800bcc34u:mode?0x800bcb40u:0x800bd1ccu;PcPortMipsCpu native=retail;
  assert(PcPortMipsRun(&retail,0x8001f6b0u,retail.gpr[31],200)==PC_PORT_MIPS_HALTED);
  assert(runtime_bridge(&rt,&native,0x8001f6b0u)==1);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(before,sprite,sizeof before));
 }
 /* Observed ten-primitive menu shape plus zero-count boundary. */
 for(unsigned representation=0;representation<2;representation++) for(unsigned count_index=0;count_index<2;count_index++) {
  uint8_t *sprite=PSX_ADDR(0x80160000u),*base=PSX_ADDR(0x80160100u),*other=PSX_ADDR(0x80160200u),*prim=PSX_ADDR(0x80160300u);
  uint32_t spriteaddr=representation?(uint32_t)(uintptr_t)sprite:0x80160000u;
  uint32_t baseaddr=representation?(uint32_t)(uintptr_t)base:0x80160100u;
  uint32_t otheraddr=representation?(uint32_t)(uintptr_t)other:0x80160200u;
  uint32_t primaddr=representation?(uint32_t)(uintptr_t)prim:0x80160300u;
  for(unsigned i=0;i<0x500u;i++)sprite[i]=(uint8_t)(i*17u+13u);
  store_le(sprite+0x20u,4,baseaddr);store_le(sprite+0x24u,4,otheraddr);
  store_le(sprite+0x28u,4,0x2cc0c0ffu);store_le(sprite+0x3cu,4,5u);sprite[0x40u]=count_index?40u:0u;
  store_le(base+0x30u,4,primaddr);
  memcpy(initial,g_PsxRam,sizeof initial);
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=spriteaddr;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x800bcc34u;
  PcPortMipsCpu native=retail;
  assert(PcPortMipsRun(&retail,0x8001f6b0u,retail.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  active_sprite_model(&rt,&native);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 puts("active menu sprite model PASS fixed synthetic CPU comparisons");
 assert(!setenv("XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT","1",1));
 for(unsigned representation=0;representation<2;representation++) {
  uint8_t *sprite=PSX_ADDR(0x80160000u),*base=sprite+0x100u,*other=sprite+0x200u,*prim=sprite+0x300u;
  uint32_t sa=representation?(uint32_t)(uintptr_t)sprite:0x80160000u;
  uint32_t ba=representation?(uint32_t)(uintptr_t)base:0x80160100u;
  uint32_t oa=representation?(uint32_t)(uintptr_t)other:0x80160200u;
  uint32_t pa=representation?(uint32_t)(uintptr_t)prim:0x80160300u;
  for(unsigned i=0;i<0x500u;i++)sprite[i]=(uint8_t)(i*17u+13u);
  store_le(sprite-8u,4,(uint32_t)(uintptr_t)(sprite+0x508u));store_le(sprite-4u,4,2u<<21);
  store_le(sprite+0x500u,4,(uint32_t)(uintptr_t)(sprite+0x608u));store_le(sprite+0x504u,4,0);
  store_le(sprite+0x600u,4,0);store_le(sprite+0x604u,4,1u<<21);
  g_Heap=sprite;g_HeapNeedsConsolidation=0;
  store_le(sprite+0x20u,4,ba);store_le(sprite+0x24u,4,oa);store_le(sprite+0x28u,4,0x2cc0c0ffu);
  store_le(sprite+0x3cu,4,5u);sprite[0x40u]=40u;store_le(base+0x30u,4,pa);
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=sa;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x800bcc34u;
  PcPortMipsCpu native=retail;rt.bridge_cpu=&native;
  native.gpr[31]+=4u;assert(!battle_menu_sprite_active_try_host(&rt,&native,0x8001f6b0u));native.gpr[31]-=4u;
  store_le(base+0x30u,4,sa);assert(!battle_menu_sprite_active_try_host(&rt,&native,0x8001f6b0u));store_le(base+0x30u,4,pa);
  store_le(sprite+0x3cu,4,0x25u);assert(!battle_menu_sprite_active_try_host(&rt,&native,0x8001f6b0u));store_le(sprite+0x3cu,4,5u);
  native.gpr[29]=sa+0x500u;assert(!battle_menu_sprite_active_try_host(&rt,&native,0x8001f6b0u));native.gpr[29]=0x801ff000u;
  store_le(sprite-4u,4,0);assert(!battle_menu_sprite_active_try_host(&rt,&native,0x8001f6b0u));store_le(sprite-4u,4,2u<<21);
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x8001f6b0u,retail.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  assert(runtime_bridge(&rt,&native,0x8001f6b0u)==1);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));rt.bridge_cpu=NULL;
 }
 g_Heap=NULL;unsetenv("XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT");
 puts("active menu sprite bridge PASS fixed synthetic CPU comparisons and refusal checks");
 const XenoRetailFileSlot *retail_image=XenoRetailData_File(XENO_RD_SLUS);assert(retail_image && retail_image->bytes);
 assert(!setenv("XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT","1",1));
 for(unsigned representation=0;representation<2;representation++) {
  uint8_t *sprite=PSX_ADDR(0x80160000u),*base=sprite+0x100u,*other=sprite+0x200u,*prim=sprite+0x300u,*task=PSX_ADDR(0x80161000u);
  uint32_t sa=representation?(uint32_t)(uintptr_t)sprite:0x80160000u;
  uint32_t ba=representation?(uint32_t)(uintptr_t)base:0x80160100u;
  uint32_t oa=representation?(uint32_t)(uintptr_t)other:0x80160200u;
  uint32_t pa=representation?(uint32_t)(uintptr_t)prim:0x80160300u;
  uint32_t ta=representation?(uint32_t)(uintptr_t)task:0x80161000u;
  for(unsigned i=0;i<0x500u;i++)sprite[i]=(uint8_t)(i*17u+13u);
  store_le(sprite-8u,4,(uint32_t)(uintptr_t)(sprite+0x508u));store_le(sprite-4u,4,2u<<21);
  store_le(sprite+0x500u,4,(uint32_t)(uintptr_t)(sprite+0x608u));store_le(sprite+0x504u,4,0);
  store_le(sprite+0x600u,4,0);store_le(sprite+0x604u,4,1u<<21);
  g_Heap=sprite;g_HeapNeedsConsolidation=0;
  store_le(sprite+0x20u,4,ba);store_le(sprite+0x24u,4,oa);store_le(sprite+0x28u,4,0x2cc0c0ffu);
  store_le(sprite+0x3cu,4,5u);sprite[0x40u]=40u;store_le(base+0x30u,4,pa);
  memset(task,0x36,0x28u);store_le(task+0x1cu,4,sa);store_le(task+0x20u,4,representation?3u:0u);
  for(unsigned j=0;j<2;j++) {uint32_t tick=(representation?3u:0u)+j,table=0x800523f0u+(tick<<6)*4u;
   memcpy(PSX_ADDR(table),retail_image->bytes+XENO_RD_SLUS_OFF(table),4u);}
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=ta;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;
  PcPortMipsCpu retail2=retail,native=retail,native2=retail;native.bus.bridge=native2.bus.bridge=runtime_bridge;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x800bcbb4u,retail.gpr[31],3000)==PC_PORT_MIPS_HALTED);
  assert(PcPortMipsRun(&retail2,0x800bcbb4u,retail2.gpr[31],3000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&native},e2={.runtime=&rt,.cpu=&native2};
  assert(!setjmp(e.failure));composed_menu_timer(&e);assert(!setjmp(e2.failure));composed_menu_timer(&e2);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
  assert(!memcmp(retail2.gpr,native2.gpr,sizeof retail2.gpr) && retail2.hi==native2.hi && retail2.lo==native2.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 g_Heap=NULL;unsetenv("XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT");
 puts("two composed menu timers PASS fixed synthetic CPU comparisons with guarded trig and sprite leaves");
 const uint32_t angles[]={0x1000u,0xfffffc00u,0xfa3fbfc0u};
 for(unsigned i=0;i<3;i++) {
  uint32_t address=0x800523f0u+(angles[i]&0xfffu)*4u;
  assert(!runtime_write(&rt,address,2,load_le(retail_image->bytes+XENO_RD_SLUS_OFF(address),2)));
 }
 unsetenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT");
 initialize_cpu(&probe,&rt);rt.bridge_cpu=&probe;probe.gpr[4]=angles[0];probe.gpr[31]=0x800bcb14u;
 assert(!battle_popup_trig_try_host(&rt,&probe,0x8003f8b0u));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT","1",1));
 probe.gpr[31]=0x800bcb18u;assert(!battle_popup_trig_try_host(&rt,&probe,0x8003f8b0u));rt.bridge_cpu=NULL;
 for(unsigned i=0;i<3;i++) {
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned j=1;j<32;j++)retail.gpr[j]=0x31400000u+j;
  retail.gpr[4]=angles[i];retail.gpr[31]=0x800bcb14u;PcPortMipsCpu native=retail;
  assert(PcPortMipsRun(&retail,0x8003f8b0u,0x800bcb14u,100)==PC_PORT_MIPS_HALTED);
  assert(runtime_bridge(&rt,&native,0x8003f8b0u)==1);
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
 }
 for(unsigned mode=0;mode<2;mode++) {
  memset(PSX_ADDR(0x80140000u),0xa5,0x1c0u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=0x8001c778u;cpu.gpr[5]=0x80140020u;cpu.gpr[6]=0;cpu.gpr[7]=mode;
  cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x800bd190u;PcPortMipsCpu host=cpu;
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  native_side=0;simulate_callback=mode;shade_calls=0;assert(PcPortMipsRun(&cpu,0x800b1720u,0x800bd190u,20000)==PC_PORT_MIPS_HALTED);
  assert(shade_calls==8);uint8_t expected_data[0x170];memcpy(expected_data,D_8001C76C,sizeof expected_data);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);memcpy(D_8001C76C,data,sizeof data);initial_gte();BattleEntryCall e={.runtime=&rt,.cpu=&host};
  native_side=1;shade_calls=0;if(setjmp(e.failure)==0) {popup_decoder_full_test_run(&e);} else assert(!"unexpected decoder refusal");
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"decoder full mode %u r%u cpu=%08x host=%08x\n",mode,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(shade_calls==8 && !memcmp(&want,&gteRegs,sizeof(gteRegs)) && !memcmp(expected_data,D_8001C76C,sizeof data));
  memcpy(D_8001C76C,data,sizeof data);
 }
 /* Scoped timer-list leaf, with separate host pointer globals and heap-owned
  * nodes. Refusal checks occur before the CPU/native comparisons. */
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TIMER_REMOVE_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT","1",1));
 for(unsigned caller=0;caller<2;caller++) for(unsigned mode=0;mode<4;mode++) {
  uint8_t *a=PSX_ADDR(0x80170000u),*b=PSX_ADDR(0x80170100u),*free_block=PSX_ADDR(0x80170200u),*sentinel=PSX_ADDR(caller?0x801fc000u:0x80170400u);
  uintptr_t pa=(uintptr_t)a,pb=(uintptr_t)b,pfree=(uintptr_t)free_block,pend=(uintptr_t)sentinel;
  assert(pa<=UINT32_MAX && pb<=UINT32_MAX && pfree<=UINT32_MAX && pend<=UINT32_MAX);
  memset(a,0,0x40u);memset(b,0,0x40u);
  store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
  store_le(b-8u,4,(uint32_t)pfree);store_le(b-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)pend);store_le(free_block-4u,4,0);
  store_le(sentinel-8u,4,0);store_le(sentinel-4u,4,1u<<21);
  store_le(a+24u,4,(uint32_t)pb);store_le(a+20u,4,mode==2?0x80000000u:0);
  store_le(b+20u,4,mode==3?0x80000000u:0);
  g_Heap=a;g_HeapNeedsConsolidation=0;fixture_timer_head=pa;
  fixture_timer_cursor=(mode&1u)?pb:pa;fixture_timer_count=2;fixture_timer_marked=mode>=2?1u:0u;
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=(uint32_t)((mode&1u)?pb:pa);retail.gpr[28]=0x80059170u;retail.gpr[31]=caller?0x800b385cu:0x800bd078u;PcPortMipsCpu native=retail;
  rt.bridge_cpu=&native;memcpy(initial,g_PsxRam,sizeof initial);
  uint32_t orig[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
  if(caller && mode==0) {
   unsetenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT");
   assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));
   assert(!setenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT","1",1));
  }
#if defined(__linux__)
  if(mode==0) {
   pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&native,.result=-1};
   assert(!pthread_create(&thread,NULL,timer_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
  }
#endif
  fixture_timer_count=3;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));fixture_timer_count=orig[2];
  fixture_timer_marked^=1u;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));fixture_timer_marked=orig[3];
  native.gpr[4]=0x80170000u;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));native.gpr[4]=retail.gpr[4];
  native.gpr[4]=(uint32_t)pa+0x30u;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));native.gpr[4]=retail.gpr[4];
  store_le(b+24u,4,(uint32_t)pa);assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));store_le(b+24u,4,0);
  g_HeapNeedsConsolidation=1;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));g_HeapNeedsConsolidation=0;
  if(mode==0) {
   ResolvedData *count_binding=find_data(&rt,0x80059188u,4u);uint8_t *saved_host=count_binding->host;
   g_HeapNeedsConsolidation=2;count_binding->host=(uint8_t*)&g_HeapNeedsConsolidation;
   assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));
   count_binding->host=saved_host;g_HeapNeedsConsolidation=0;
  }
  ((uint8_t*)PSX_ADDR(0x8001cd94u))[0]^=1u;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));((uint8_t*)PSX_ADDR(0x8001cd94u))[0]^=1u;
  if(sizeof(uintptr_t)>4u) {fixture_timer_head|=(uintptr_t)UINT32_MAX+1u;assert(!battle_popup_timer_remove_try_host(&rt,&native,0x8001cd94u));fixture_timer_head=orig[0];}
  assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(retail.gpr,native.gpr,sizeof retail.gpr));
  assert(PcPortMipsRun(&retail,0x8001cd94u,retail.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);uint32_t after[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
  memcpy(g_PsxRam,initial,sizeof initial);fixture_timer_head=orig[0];fixture_timer_cursor=orig[1];fixture_timer_count=orig[2];fixture_timer_marked=orig[3];
  rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&native,0x8001cd94u)==1);
  uint32_t got[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
  assert(!memcmp(after,got,sizeof after) && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
 }
 unsetenv("XENO_BATTLE_NATIVE_POPUP_TIMER_REMOVE_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 puts("popup/color guarded timer-list removal PASS fixed synthetic CPU and refusal comparisons");
 /* Paired color-fade work-list removal: separate full CPU contract. */
 assert(!find_data(&rt,0x8005918cu,4u));
 rt.data[rt.data_count++]=(ResolvedData){0x8005918cu,4u,(uint8_t*)&fixture_work_count,"g_NumWorkListEntries"};
 qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 g_ActiveBattleRuntime=&rt;assert(!setenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT","1",1));
 for(unsigned mode=0;mode<4;mode++) {
  uint8_t *a=PSX_ADDR(0x80170000u),*b=PSX_ADDR(0x80170100u),*free_block=PSX_ADDR(0x80170200u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t pa=(uintptr_t)a,pb=(uintptr_t)b,pfree=(uintptr_t)free_block,pend=(uintptr_t)end;
  assert(pend<=UINT32_MAX);memset(a,0,0x40u);memset(b,0,0x40u);
  store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
  store_le(b-8u,4,(uint32_t)pfree);store_le(b-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)pend);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);store_le(a+24u,4,(uint32_t)pb);
  g_Heap=a;g_HeapNeedsConsolidation=0;fixture_work_head=pa;fixture_work_count=2;
  fixture_timer_cursor=(mode&2u)?0:(mode&1u?pb:pa);
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=(uint32_t)(mode&1u?pb:pa);ref.gpr[28]=0x80059170u;ref.gpr[31]=0x800b3854u;PcPortMipsCpu got=ref;
  rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
  uint32_t cursor_before=fixture_timer_cursor;
  fixture_work_count=3;assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));fixture_work_count=2;
  got.gpr[4]=0x80170000u;assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));got.gpr[4]=ref.gpr[4];
  store_le(b+24u,4,(uint32_t)pa);assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));store_le(b+24u,4,0);
  if(mode==0) {
#if defined(__linux__)
   pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
   assert(!pthread_create(&thread,NULL,color_work_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
   store_le(end-4u,4,0);assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));store_le(end-4u,4,1u<<21);
   ((uint8_t*)PSX_ADDR(0x8001cb48u))[0]^=1u;assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));((uint8_t*)PSX_ADDR(0x8001cb48u))[0]^=1u;
   if(sizeof(uintptr_t)>4u) {fixture_work_head|=(uintptr_t)UINT32_MAX+1u;assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));fixture_work_head=pa;}
  }
  assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
  assert(PcPortMipsRun(&ref,0x8001cb48u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);uint32_t head_after=fixture_work_head,cursor_after=fixture_timer_cursor,count_after=fixture_work_count;
  memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=pa;fixture_timer_cursor=cursor_before;fixture_work_count=2;
  rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001cb48u)==1);
  assert(fixture_work_head==head_after && fixture_timer_cursor==cursor_after && fixture_work_count==count_after);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
 }
 unsetenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 puts("color work-list removal PASS fixed synthetic CPU and refusal comparisons; no callbacks");
 /* Default callback on a work node already unlinked by 8001CE74. */
 {
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_CALLBACK_EXPERIMENT","1",1));
  for(unsigned mode=0;mode<3;mode++) {
   uint8_t *a=PSX_ADDR(0x80170000u),*b=PSX_ADDR(0x80170100u),*c=PSX_ADDR(0x80170200u),*free_block=PSX_ADDR(0x80170300u),*end=PSX_ADDR(0x801fc000u);
   uintptr_t pa=(uintptr_t)a,pb=(uintptr_t)b,pc=(uintptr_t)c;assert((uintptr_t)end<=UINT32_MAX);
   memset(a,0,0x40u);memset(b,0,0x40u);memset(c,0,0x40u);
   store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
   store_le(b-8u,4,(uint32_t)pc);store_le(b-4u,4,2u<<21);
   store_le(c-8u,4,(uint32_t)(uintptr_t)free_block);store_le(c-4u,4,2u<<21);
   store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
   store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
   store_le(a+12u,4,0x8001cb48u);store_le(a+24u,4,mode?(uint32_t)pb:0);
   store_le(b+24u,4,mode==2?(uint32_t)pc:0);
   g_Heap=a;g_HeapNeedsConsolidation=0;fixture_work_head=mode?pb:0;fixture_work_count=mode+1u;
   fixture_timer_cursor=mode?pb:0;g_ActiveBattleRuntime=&rt;
   PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
   for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
   ref.gpr[4]=(uint32_t)pa;ref.gpr[28]=0x80059170u;ref.gpr[31]=0x8001cf34u;PcPortMipsCpu got=ref;
   rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
   fixture_work_count++;assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));fixture_work_count--;
   store_le(a+12u,4,0);assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));store_le(a+12u,4,0x8001cb48u);
   fixture_timer_cursor=pa;assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));fixture_timer_cursor=mode?pb:0;
   if(mode==0) {
#if defined(__linux__)
    pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
    assert(!pthread_create(&thread,NULL,unlinked_work_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
    got.gpr[4]=0x80170000u;assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));got.gpr[4]=ref.gpr[4];
    store_le(end-4u,4,0);assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));store_le(end-4u,4,1u<<21);
   }
   if(mode==2) {store_le(c+24u,4,(uint32_t)pb);assert(!battle_popup_unlinked_work_try_host(&rt,&got,0x8001cb48u));store_le(c+24u,4,0);}
   assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
   assert(PcPortMipsRun(&ref,0x8001cb48u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
   memcpy(expected,g_PsxRam,sizeof expected);uint32_t head_after=fixture_work_head,cursor_after=fixture_timer_cursor,count_after=fixture_work_count;
   memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=mode?pb:0;fixture_timer_cursor=mode?pb:0;fixture_work_count=mode+1u;
   rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001cb48u)==1);
   assert(fixture_work_head==head_after && fixture_timer_cursor==cursor_after && fixture_work_count==count_after);
   assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  }
  unsetenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_CALLBACK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup already-unlinked work callback PASS fixed synthetic CPU and refusal comparisons");
 /* The original 8001CE74 traversal calls the guarded leaf on an already
  * unlinked node; the parent itself is interpreted in both runs. */
 {
  uint8_t *owner=PSX_ADDR(0x80170000u),*a=PSX_ADDR(0x80170100u),*b=PSX_ADDR(0x80170200u),*c=PSX_ADDR(0x80170300u);
  uint8_t *free_block=PSX_ADDR(0x80170400u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t po=(uintptr_t)owner,pa=(uintptr_t)a,pb=(uintptr_t)b,pc=(uintptr_t)c;assert((uintptr_t)end<=UINT32_MAX);
  memset(owner,0,0x40u);memset(a,0,0x40u);memset(b,0,0x40u);memset(c,0,0x40u);
  store_le(owner-8u,4,(uint32_t)pa);store_le(owner-4u,4,2u<<21);
  store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
  store_le(b-8u,4,(uint32_t)pc);store_le(b-4u,4,2u<<21);
  store_le(c-8u,4,(uint32_t)(uintptr_t)free_block);store_le(c-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
  store_le(owner+16u,4,0x12345u);store_le(a,4,(uint32_t)po);store_le(a+12u,4,0x8001cb48u);
  store_le(a+20u,4,0x12345u);store_le(a+24u,4,(uint32_t)pb);store_le(b+24u,4,(uint32_t)pc);
  g_Heap=owner;g_HeapNeedsConsolidation=0;fixture_work_head=pa;fixture_work_count=3;
  fixture_timer_head=0;fixture_timer_cursor=pa;g_ActiveBattleRuntime=&rt;
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_CALLBACK_EXPERIMENT","1",1));
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=(uint32_t)po;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x8000deacu;
  PcPortMipsCpu got=ref;got.bus.bridge=unlinked_callback_bridge;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&ref,0x8001ce74u,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);uint32_t head_after=fixture_work_head,cursor_after=fixture_timer_cursor,count_after=fixture_work_count;
  memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=pa;fixture_timer_cursor=pa;fixture_work_count=3;
  unlinked_callback_calls=0;assert(PcPortMipsRun(&got,0x8001ce74u,got.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  assert(unlinked_callback_calls==1 && fixture_work_head==head_after && fixture_timer_cursor==cursor_after && fixture_work_count==count_after);
  assert(fixture_work_head==pb && fixture_timer_cursor==pb && fixture_work_count==3);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  unsetenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_CALLBACK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup interpreted traversal with guarded unlinked callback PASS synthetic CPU comparison");
 /* Default timer callback on a node already unlinked by 8001CE74. */
 {
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_TIMER_CALLBACK_EXPERIMENT","1",1));
  for(unsigned mode=0;mode<6;mode++) {
   unsigned length=mode%3u,marked=mode>=3u;
   uint8_t *a=PSX_ADDR(0x80170000u),*b=PSX_ADDR(0x80170100u),*c=PSX_ADDR(0x80170200u),*free_block=PSX_ADDR(0x80170300u),*end=PSX_ADDR(0x801fc000u);
   uintptr_t pa=(uintptr_t)a,pb=(uintptr_t)b,pc=(uintptr_t)c;assert((uintptr_t)end<=UINT32_MAX);
   memset(a,0,0x40u);memset(b,0,0x40u);memset(c,0,0x40u);
   store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
   store_le(b-8u,4,(uint32_t)pc);store_le(b-4u,4,2u<<21);
   store_le(c-8u,4,(uint32_t)(uintptr_t)free_block);store_le(c-4u,4,2u<<21);
   store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
   store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
   store_le(a+12u,4,0x8001cd94u);store_le(a+20u,4,marked?0x80000000u:0);store_le(a+24u,4,length?(uint32_t)pb:0);
   store_le(b+24u,4,length==2?(uint32_t)pc:0);
   g_Heap=a;g_HeapNeedsConsolidation=0;fixture_timer_head=length?pb:0;fixture_timer_count=length+1u;
   fixture_timer_cursor=length?pb:0;fixture_timer_marked=marked;g_ActiveBattleRuntime=&rt;
   PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
   for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
   ref.gpr[4]=(uint32_t)pa;ref.gpr[28]=0x80059170u;ref.gpr[31]=0x8001cff8u;PcPortMipsCpu got=ref;
   rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
   fixture_timer_count++;assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));fixture_timer_count--;
   fixture_timer_marked^=1u;assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));fixture_timer_marked=marked;
   store_le(a+12u,4,0);assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));store_le(a+12u,4,0x8001cd94u);
   fixture_timer_cursor=pa;assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));fixture_timer_cursor=length?pb:0;
   if(mode==0) {
#if defined(__linux__)
    pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
    assert(!pthread_create(&thread,NULL,unlinked_timer_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
    got.gpr[4]=0x80170000u;assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));got.gpr[4]=ref.gpr[4];
    store_le(end-4u,4,0);assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));store_le(end-4u,4,1u<<21);
   }
   if(length==2) {store_le(c+24u,4,(uint32_t)pb);assert(!battle_popup_unlinked_timer_try_host(&rt,&got,0x8001cd94u));store_le(c+24u,4,0);}
   assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
   assert(PcPortMipsRun(&ref,0x8001cd94u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
   memcpy(expected,g_PsxRam,sizeof expected);
   uint32_t outcome[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
   memcpy(g_PsxRam,initial,sizeof initial);fixture_timer_head=length?pb:0;fixture_timer_cursor=length?pb:0;fixture_timer_count=length+1u;fixture_timer_marked=marked;
   rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001cd94u)==1);
   uint32_t result[]={fixture_timer_head,fixture_timer_cursor,fixture_timer_count,fixture_timer_marked};
   assert(!memcmp(outcome,result,sizeof result));
   assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  }
  unsetenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_TIMER_CALLBACK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup already-unlinked timer callback PASS fixed synthetic CPU and refusal comparisons");
 /* The original 8001CE74 traversal calls the guarded leaf on an already
  * unlinked node; the parent itself is interpreted in both runs. */
 {
  uint8_t *owner=PSX_ADDR(0x80170000u),*a=PSX_ADDR(0x80170100u),*b=PSX_ADDR(0x80170200u),*c=PSX_ADDR(0x80170300u);
  uint8_t *free_block=PSX_ADDR(0x80170400u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t po=(uintptr_t)owner,pa=(uintptr_t)a,pb=(uintptr_t)b,pc=(uintptr_t)c;assert((uintptr_t)end<=UINT32_MAX);
  memset(owner,0,0x40u);memset(a,0,0x40u);memset(b,0,0x40u);memset(c,0,0x40u);
  store_le(owner-8u,4,(uint32_t)pa);store_le(owner-4u,4,2u<<21);
  store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
  store_le(b-8u,4,(uint32_t)pc);store_le(b-4u,4,2u<<21);
  store_le(c-8u,4,(uint32_t)(uintptr_t)free_block);store_le(c-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
  store_le(owner+16u,4,0x12345u);store_le(a,4,(uint32_t)po);store_le(a+12u,4,0x8001cd94u);
  store_le(a+20u,4,0x80012345u);store_le(a+24u,4,(uint32_t)pb);store_le(b+24u,4,(uint32_t)pc);
  g_Heap=owner;g_HeapNeedsConsolidation=0;fixture_work_head=0;fixture_timer_head=pa;fixture_timer_count=3;fixture_timer_marked=1;
  fixture_timer_cursor=pa;g_ActiveBattleRuntime=&rt;
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_TIMER_CALLBACK_EXPERIMENT","1",1));
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=(uint32_t)po;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x8000deacu;
  PcPortMipsCpu got=ref;got.bus.bridge=unlinked_timer_callback_bridge;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&ref,0x8001ce74u,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);uint32_t head_after=fixture_timer_head,cursor_after=fixture_timer_cursor,count_after=fixture_timer_count,marked_after=fixture_timer_marked;
  memcpy(g_PsxRam,initial,sizeof initial);fixture_timer_head=pa;fixture_timer_cursor=pa;fixture_timer_count=3;fixture_timer_marked=1;
  unlinked_timer_callback_calls=0;assert(PcPortMipsRun(&got,0x8001ce74u,got.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  assert(unlinked_timer_callback_calls==1 && fixture_timer_head==head_after && fixture_timer_cursor==cursor_after && fixture_timer_count==count_after && fixture_timer_marked==marked_after);
  assert(fixture_timer_head==pb && fixture_timer_cursor==pb && fixture_timer_count==2 && fixture_timer_marked==0);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  unsetenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_TIMER_CALLBACK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup interpreted traversal with guarded unlinked timer callback PASS synthetic CPU comparison");
 /* One work match with null callback and only unrelated timer nodes. */
 {
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT","1",1));
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT","1",1));
  for(unsigned mode=0;mode<4;mode++) {
   uint8_t *owner=PSX_ADDR(0x80170000u),*a=PSX_ADDR(0x80170100u),*b=PSX_ADDR(0x80170200u),*t=PSX_ADDR(0x80170300u);
   uint8_t *free_block=PSX_ADDR(0x80170400u),*end=PSX_ADDR(0x801fc000u);
   uintptr_t po=(uintptr_t)owner,pa=(uintptr_t)a,pb=(uintptr_t)b,pt=(uintptr_t)t;assert((uintptr_t)end<=UINT32_MAX);
   memset(owner,0,0x40u);memset(a,0,0x40u);memset(b,0,0x40u);memset(t,0,0x40u);
   store_le(owner-8u,4,(uint32_t)pa);store_le(owner-4u,4,2u<<21);
   store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
   store_le(b-8u,4,(uint32_t)pt);store_le(b-4u,4,2u<<21);
   store_le(t-8u,4,(uint32_t)(uintptr_t)free_block);store_le(t-4u,4,2u<<21);
   store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
   store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
   store_le(owner+16u,4,0x12345u);store_le(a+24u,4,(uint32_t)pb);
   uint8_t *match=(mode==0||mode==3)?a:b,*other=match==a?b:a;
   store_le(match,4,(uint32_t)po);store_le(match+20u,4,0x12345u);store_le(other,4,0x11223344u);
   store_le(t,4,0x55667788u);
   g_Heap=owner;g_HeapNeedsConsolidation=0;fixture_work_head=pa;
   fixture_timer_head=mode==2?pt:mode==3?po:0;fixture_timer_cursor=(uintptr_t)match;g_ActiveBattleRuntime=&rt;
   PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
   for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
   ref.gpr[4]=(uint32_t)po;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x800bd070u;
   PcPortMipsCpu got=ref;rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
   store_le(match+12u,4,0x8001cb48u);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));
   if(mode==0) {
    ResolvedFunction *legacy=find_function(&rt,0x8001ce74u);int added=legacy==NULL;ResolvedFunction saved={0};
    if(added) {
     assert(rt.function_count<sizeof(rt.functions)/sizeof(rt.functions[0]));
     rt.functions[rt.function_count++]=(ResolvedFunction){0x8001ce74u,(void*)forbidden_legacy,"forbidden_legacy"};
     qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);legacy=find_function(&rt,0x8001ce74u);
    } else {saved=*legacy;legacy->host=(void*)forbidden_legacy;legacy->name="forbidden_legacy";}
    assert(legacy);forbidden_legacy_calls=0;
    assert(runtime_bridge(&rt,&got,0x8001ce74u)<0 && forbidden_legacy_calls==0);
    if(added) {size_t index=(size_t)(legacy-rt.functions);memmove(legacy,legacy+1,(rt.function_count-index-1u)*sizeof(*legacy));rt.function_count--;}
    else *legacy=saved;
   } else assert(runtime_bridge(&rt,&got,0x8001ce74u)<0);
   store_le(match+12u,4,0);
   store_le(match+20u,4,0x40012345u);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(match+20u,4,0x12345u);
   store_le(other,4,(uint32_t)po);store_le(other+20u,4,0x12345u);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(other,4,0x11223344u);store_le(other+20u,4,0);
   got.gpr[4]=0x80170000u;assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));got.gpr[4]=ref.gpr[4];
   if(mode==0) {
#if defined(__linux__)
    pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
    assert(!pthread_create(&thread,NULL,one_null_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
    store_le(b+24u,4,(uint32_t)pa);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(b+24u,4,0);
    store_le(end-4u,4,0);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(end-4u,4,1u<<21);
   }
   assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
   assert(PcPortMipsRun(&ref,0x8001ce74u,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
   memcpy(expected,g_PsxRam,sizeof expected);uint32_t outcome[]={fixture_work_head,fixture_timer_head,fixture_timer_cursor};
   memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=pa;fixture_timer_head=mode==2?pt:mode==3?po:0;fixture_timer_cursor=(uintptr_t)match;
   rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001ce74u)==1);
   uint32_t result[]={fixture_work_head,fixture_timer_head,fixture_timer_cursor};
   assert(!memcmp(outcome,result,sizeof result));
   assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  }
  unsetenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup one null-callback unlink PASS fixed synthetic CPU and refusal comparisons");
 /* One timer match, with unrelated work nodes. */
 {
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT","1",1));
  for(unsigned mode=0;mode<2;mode++) {
   uint8_t *owner=PSX_ADDR(0x80170000u),*work=PSX_ADDR(0x80170100u),*timer=PSX_ADDR(0x80170200u);
   uint8_t *free_block=PSX_ADDR(0x80170300u),*end=PSX_ADDR(0x801fc000u);
   uintptr_t po=(uintptr_t)owner,pw=(uintptr_t)work,pt=(uintptr_t)timer;assert((uintptr_t)end<=UINT32_MAX);
   memset(owner,0,0x40u);memset(work,0,0x40u);memset(timer,0,0x40u);
   store_le(owner-8u,4,(uint32_t)pw);store_le(owner-4u,4,2u<<21);
   store_le(work-8u,4,(uint32_t)pt);store_le(work-4u,4,2u<<21);
   store_le(timer-8u,4,(uint32_t)(uintptr_t)free_block);store_le(timer-4u,4,2u<<21);
   store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
   store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
   store_le(owner+16u,4,0x12345u);store_le(work,4,0x11223344u);
   uint8_t *match=mode?owner:timer;
   if(mode) {store_le(timer,4,0x55667788u);store_le(timer+24u,4,(uint32_t)po);}
   store_le(match,4,(uint32_t)po);store_le(match+20u,4,0x12345u);
   g_Heap=owner;g_HeapNeedsConsolidation=0;fixture_work_head=pw;fixture_timer_head=pt;
   fixture_timer_cursor=(uintptr_t)match;g_ActiveBattleRuntime=&rt;
   PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
   for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
   ref.gpr[4]=(uint32_t)po;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x800bd070u;
   PcPortMipsCpu got=ref;rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
   store_le(match+12u,4,0x8001cd94u);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(match+12u,4,0);
   store_le(match+20u,4,0x40012345u);assert(!battle_popup_one_null_unlink_try_host(&rt,&got,0x8001ce74u));store_le(match+20u,4,0x12345u);
   assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
   assert(PcPortMipsRun(&ref,0x8001ce74u,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
   memcpy(expected,g_PsxRam,sizeof expected);uint32_t outcome[]={fixture_work_head,fixture_timer_head,fixture_timer_cursor};
   memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=pw;fixture_timer_head=pt;fixture_timer_cursor=(uintptr_t)match;
   rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001ce74u)==1);
   uint32_t result[]={fixture_work_head,fixture_timer_head,fixture_timer_cursor};
   assert(!memcmp(outcome,result,sizeof result));
   assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  }
  unsetenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup one null-callback timer unlink PASS fixed synthetic CPU and refusal comparisons");
 /* A guest alias and a native host list word are distinct retail values. */
 {
  uint8_t *node=PSX_ADDR(0x800c3c6cu);uintptr_t host_node=(uintptr_t)node;
  assert(host_node<=UINT32_MAX);memset(node,0,28u);
  fixture_work_head=host_node;fixture_work_count=1;fixture_timer_cursor=host_node;g_ActiveBattleRuntime=&rt;
  assert(!setenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT","1",1));
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=0x800c3c6cu;ref.gpr[28]=0x80059170u;ref.gpr[31]=0x800b3854u;PcPortMipsCpu got=ref;
  rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
  assert(!battle_color_work_remove_try_host(&rt,&got,0x8001cb48u));
  assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
  assert(PcPortMipsRun(&ref,0x8001cb48u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
  assert(fixture_work_head==host_node && fixture_work_count==1 && fixture_timer_cursor==host_node);
  assert(ref.gpr[2]==1 && ref.gpr[3]==0 && ref.gpr[5]==host_node);
  unsetenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT");g_ActiveBattleRuntime=NULL;rt.bridge_cpu=NULL;
 }
 puts("color work-list guest/host alias refusal PASS synthetic retail comparison");
 /* Two retail aliases resolve to the same native pointer but compare
  * differently.  A host pointer alone cannot reconstruct the guest word. */
 {
  uint32_t kseg0=0x800c3c6cu,kseg1=0xa00c3c6cu;
  uint8_t *node=PSX_ADDR(kseg0);memset(node,0,28u);
  assert(module_memory_pointer(&rt,kseg0,28u,0)==node && module_memory_pointer(&rt,kseg1,28u,0)==node);
  for(unsigned mode=0;mode<2;mode++) {
   uint32_t list_word=mode?kseg1:kseg0;
   fixture_work_head=list_word;fixture_work_count=1;fixture_timer_cursor=list_word;
   PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
   ref.gpr[4]=kseg0;ref.gpr[28]=0x80059170u;ref.gpr[31]=0x800b3854u;
   assert(PcPortMipsRun(&ref,0x8001cb48u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
   assert(fixture_work_head==(mode?list_word:0) && fixture_work_count==(mode?1u:0u));
   assert(fixture_timer_cursor==(mode?list_word:0));
  }
 }
 puts("color work-list KSEG alias provenance PASS synthetic retail comparison");
 /* Pinned HeapFree returns an error without freeing or modifying the heap. */
 {
  uint8_t *block=PSX_ADDR(0x80170000u),*free_block=PSX_ADDR(0x80170100u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t payload=(uintptr_t)block;assert((uintptr_t)end<=UINT32_MAX);memset(block,0,0x40u);
  store_le(block-8u,4,(uint32_t)(uintptr_t)free_block);store_le(block-4u,4,(2u<<21)|0x02000000u|0x1234u);
  store_le(free_block-8u,4,(uint32_t)(uintptr_t)end);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
  g_Heap=block;g_HeapNeedsConsolidation=0;g_ActiveBattleRuntime=&rt;
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_PINNED_FREE_EXPERIMENT","1",1));
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=(uint32_t)payload;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x800bd080u;
  PcPortMipsCpu got=ref;rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
#if defined(__linux__)
  pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
  assert(!pthread_create(&thread,NULL,pinned_free_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
  store_le(block-4u,4,(2u<<21)|0x1234u);assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));store_le(block-4u,4,(2u<<21)|0x02000000u|0x1234u);
  got.gpr[4]=(uint32_t)payload+4u;assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));got.gpr[4]=ref.gpr[4];
  got.gpr[4]=0x80170000u;assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));got.gpr[4]=ref.gpr[4];
  got.gpr[29]=0x80170040u;assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));got.gpr[29]=ref.gpr[29];
  g_HeapNeedsConsolidation=1;assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));g_HeapNeedsConsolidation=0;
  store_le(end-4u,4,0);assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));store_le(end-4u,4,1u<<21);
  ((uint8_t*)PSX_ADDR(0x800320e8u))[0]^=1u;assert(!battle_popup_pinned_free_try_host(&rt,&got,0x800320e8u));((uint8_t*)PSX_ADDR(0x800320e8u))[0]^=1u;
  assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr));
  assert(PcPortMipsRun(&ref,0x800320e8u,ref.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);rt.bridge_cpu=NULL;
  assert(runtime_bridge(&rt,&got,0x800320e8u)==1);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  assert(!g_HeapNeedsConsolidation && load_le(block-4u,4u)==((2u<<21)|0x02000000u|0x1234u));
  unsetenv("XENO_BATTLE_NATIVE_POPUP_PINNED_FREE_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("popup pinned heap-free return PASS synthetic CPU and refusal comparisons; no free");
 /* Retail color-fade cleanup with real guarded list leaves, synthetic heap. */
 {
  uint8_t *task=PSX_ADDR(0x80170000u),*free_block=PSX_ADDR(0x80170100u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t pa=(uintptr_t)task,pfree=(uintptr_t)free_block,pend=(uintptr_t)end;
  assert(pend<=UINT32_MAX);memset(task,0,0x60u);
  store_le(task-8u,4,(uint32_t)pfree);store_le(task-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)pend);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
  g_Heap=task;g_HeapNeedsConsolidation=0;g_ActiveBattleRuntime=&rt;
  fixture_work_head=pa+28u;fixture_work_count=1;fixture_timer_head=pa;fixture_timer_count=1;
  fixture_timer_cursor=pa;fixture_timer_marked=0;
  assert(!setenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT","1",1));
  assert(!setenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT","1",1));
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=(uint32_t)pa;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x8000deacu;
  PcPortMipsCpu got=ref;got.bus.bridge=runtime_bridge;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&ref,0x800b383cu,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);
  uint32_t outcome[]={fixture_work_head,fixture_work_count,fixture_timer_head,fixture_timer_count,fixture_timer_cursor,fixture_timer_marked};
  memcpy(g_PsxRam,initial,sizeof initial);fixture_work_head=pa+28u;fixture_work_count=1;fixture_timer_head=pa;fixture_timer_count=1;fixture_timer_cursor=pa;fixture_timer_marked=0;
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  BattleEntryCall entry={.runtime=&rt,.cpu=&got};
  if(setjmp(entry.failure)==0)battle_color_fade_remove(&entry);else assert(!"color fade cleanup refused");
  uint32_t result[]={fixture_work_head,fixture_work_count,fixture_timer_head,fixture_timer_count,fixture_timer_cursor,fixture_timer_marked};
  assert(!memcmp(outcome,result,sizeof result) && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
  assert(PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
  unsetenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_COLOR_TIMER_REMOVE_EXPERIMENT");g_Heap=NULL;g_ActiveBattleRuntime=NULL;
 }
 puts("color fade cleanup guarded list leaves PASS fixed synthetic CPU comparison; fixture ownership only");

 /* The empty unlink case writes only its retail stack frame. */
 g_ActiveBattleRuntime=&rt;fixture_work_head=fixture_timer_head=0;
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT","1",1));
 PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
 for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
 retail.gpr[4]=0x80154000u;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x800bd070u;
 PcPortMipsCpu native=retail;rt.bridge_cpu=&native;memcpy(initial,g_PsxRam,sizeof initial);
 fixture_work_head=1;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));fixture_work_head=0;
 fixture_timer_head=1;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));fixture_timer_head=0;
 if(sizeof(uintptr_t)>4u) {fixture_work_head=(uintptr_t)UINT32_MAX+1u;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));fixture_work_head=0;}
 ((uint8_t*)PSX_ADDR(0x8001ce74u))[0]^=1u;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));((uint8_t*)PSX_ADDR(0x8001ce74u))[0]^=1u;
 ResolvedData *work_binding=find_data(&rt,0x80059594u,4u);uint8_t *saved_work_host=work_binding->host;
 work_binding->host=(uint8_t*)&fixture_timer_head;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));work_binding->host=saved_work_host;
 native.gpr[29]=2;assert(!battle_popup_unrelated_unlink_try_host(&rt,&native,0x8001ce74u));native.gpr[29]=retail.gpr[29];
 assert(!memcmp(initial,g_PsxRam,sizeof initial) && !memcmp(retail.gpr,native.gpr,sizeof retail.gpr));
 assert(PcPortMipsRun(&retail,0x8001ce74u,retail.gpr[31],1000)==PC_PORT_MIPS_HALTED);
 memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
 rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&native,0x8001ce74u)==1);
 assert(!memcmp(expected,g_PsxRam,sizeof expected));
 assert(!memcmp(retail.gpr,native.gpr,sizeof retail.gpr) && retail.hi==native.hi && retail.lo==native.lo);
 /* Bounded unrelated work/timer nodes: no matching owner and no callbacks. */
 for(unsigned mode=0;mode<3;mode++) {
  uint8_t *a=PSX_ADDR(0x80170000u),*b=PSX_ADDR(0x80170100u),*free_block=PSX_ADDR(0x80170200u),*end=PSX_ADDR(0x801fc000u);
  uintptr_t pa=(uintptr_t)a,pb=(uintptr_t)b,pfree=(uintptr_t)free_block,pend=(uintptr_t)end;
  assert(pend<=UINT32_MAX);memset(a,0,0x40u);memset(b,0,0x40u);
  store_le(a-8u,4,(uint32_t)pb);store_le(a-4u,4,2u<<21);
  store_le(b-8u,4,(uint32_t)pfree);store_le(b-4u,4,2u<<21);
  store_le(free_block-8u,4,(uint32_t)pend);store_le(free_block-4u,4,0);
  store_le(end-8u,4,0);store_le(end-4u,4,1u<<21);
  store_le(a,4,0x12345678u);store_le(b,4,0x23456789u);
  g_Heap=a;g_HeapNeedsConsolidation=0;fixture_work_head=mode!=1?pa:0;fixture_timer_head=mode!=0?pb:0;
  PcPortMipsCpu ref;initialize_cpu(&ref,&rt);ref.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)ref.gpr[i]=0x31400000u+i;
  ref.gpr[4]=0x80154000u;ref.gpr[28]=0x80059170u;ref.gpr[29]=0x801ff000u;ref.gpr[31]=0x800bd070u;
  PcPortMipsCpu got=ref;rt.bridge_cpu=&got;memcpy(initial,g_PsxRam,sizeof initial);
  if(mode==2) {
#if defined(__linux__)
   pthread_t thread;TimerThreadProbe foreign={.runtime=&rt,.cpu=&got,.result=-1};
   assert(!pthread_create(&thread,NULL,unlink_foreign_thread,&foreign));assert(!pthread_join(thread,NULL));assert(foreign.result==0);
#endif
   store_le(a,4,ref.gpr[4]);assert(!battle_popup_unrelated_unlink_try_host(&rt,&got,0x8001ce74u));store_le(a,4,0x12345678u);
   store_le(a+24u,4,(uint32_t)pa);assert(!battle_popup_unrelated_unlink_try_host(&rt,&got,0x8001ce74u));store_le(a+24u,4,0);
   store_le(end-4u,4,0);assert(!battle_popup_unrelated_unlink_try_host(&rt,&got,0x8001ce74u));store_le(end-4u,4,1u<<21);
   got.gpr[29]=0x80170040u;assert(!battle_popup_unrelated_unlink_try_host(&rt,&got,0x8001ce74u));got.gpr[29]=ref.gpr[29];
   assert(!memcmp(initial,g_PsxRam,sizeof initial));
  }
  assert(PcPortMipsRun(&ref,0x8001ce74u,ref.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  rt.bridge_cpu=NULL;assert(runtime_bridge(&rt,&got,0x8001ce74u)==1);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(ref.gpr,got.gpr,sizeof ref.gpr) && ref.hi==got.hi && ref.lo==got.lo);
 }
 g_Heap=NULL;fixture_work_head=fixture_timer_head=0;
 puts("popup unrelated-list unlink PASS fixed synthetic CPU and refusal comparisons; no callback");
 unsetenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");g_ActiveBattleRuntime=NULL;
 puts("popup empty-list unlink PASS synthetic CPU and refusal comparisons; no callback");
 /* In hybrid builds a declined scoped guard must not bind legacy C. */
 {
  const struct {
   const char *env;uint32_t target,ra;int pending_load;
   int (*guard)(BattleMipsRuntime *,PcPortMipsCpu *,uint32_t);
  } cases[]={
   {"XENO_BATTLE_NATIVE_POPUP_RECORD_EXPERIMENT",0x800b168cu,0x800bd150u,0,battle_popup_record_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_COPY_EXPERIMENT",0x8003f968u,0x800bd1a0u,0,battle_popup_copy_try_host},
   {"XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT",0x8001f6b0u,0x800bcc34u,0,battle_menu_sprite_active_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_GFX_QUEUE_EXPERIMENT",0x80025180u,0x800bd068u,0,battle_popup_gfx_queue_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT",0x8001f6b0u,0x800bd05cu,0,battle_popup_sprite_noop_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT",0x8003f8b0u,0x800bcb14u,1,battle_popup_trig_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_SHADE_EXPERIMENT",0x80043c24u,0x800b1798u,1,battle_atlas_gpu_try_host},
   {"XENO_BATTLE_NATIVE_FRAME_SPRITE_EXPERIMENT",0x80025fa8u,0x80076814u,1,battle_atlas_render_try_host},
   {"XENO_BATTLE_NATIVE_POPUP_TRANS_MATRIX_EXPERIMENT",0x80049d9cu,0x800bceccu,1,popup_transmatrix_try_host},
   {"XENO_BATTLE_NATIVE_ATLAS_GUESTABI_EXPERIMENT",0x8004960cu,0x80026064u,1,battle_pushmatrix_try_host},
   {NULL,0x80043b48u,0x80073b24u,1,battle_fade_draw_sdk_try_host},
  };
  g_ActiveBattleRuntime=&rt;
  for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++) {
   PcPortMipsCpu probe;initialize_cpu(&probe,&rt);probe.gpr[4]=0;probe.gpr[5]=1;probe.gpr[28]=0x80059170u;probe.gpr[29]=0x801ff000u;probe.gpr[31]=cases[i].ra;
   if(cases[i].env)assert(!setenv(cases[i].env,"1",1));
   if(cases[i].pending_load) {probe.load_valid=1;probe.load_reg=4;}
   rt.bridge_cpu=&probe;int accepted=cases[i].guard(&rt,&probe,cases[i].target);rt.bridge_cpu=NULL;
   if(accepted)fprintf(stderr,"unexpected scoped guard acceptance: %s\n",cases[i].env?cases[i].env:"fade draw");
   assert(!accepted);
   rt.bridge_cpu=&probe;assert(runtime_bridge_caller_hosts(&rt,&probe,cases[i].target)==-1);rt.bridge_cpu=NULL;
   uint32_t registers[32];memcpy(registers,probe.gpr,sizeof registers);
   assert(runtime_bridge(&rt,&probe,cases[i].target)<0 && !memcmp(registers,probe.gpr,sizeof registers));
   if(cases[i].env)unsetenv(cases[i].env);
  }
  g_ActiveBattleRuntime=NULL;
 }
 puts("popup and atlas scoped hybrid guard refusals PASS; no legacy fallback");
 /* A recorded startup boundary must not pass a guest-shaped sprite field
  * to legacy C, even when a host function is registered in hybrid mode. */
 {
  PcPortMipsCpu probe;initialize_cpu(&probe,&rt);probe.gpr[4]=0x80150000u;probe.gpr[31]=0x801e68f8u;
  uint8_t *field=PSX_ADDR(0x80150058u);uint32_t saved_field=load_le(field,4u);
  store_le(field,4u,0x800c3c00u);g_ActiveBattleRuntime=&rt;
  ResolvedFunction *legacy=find_function(&rt,0x800223b0u);int added=legacy==NULL;ResolvedFunction saved={0};
  if(added) {
   assert(rt.function_count<sizeof(rt.functions)/sizeof(rt.functions[0]));
   rt.functions[rt.function_count++]=(ResolvedFunction){0x800223b0u,(void*)forbidden_legacy,"forbidden_legacy"};
   qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);legacy=find_function(&rt,0x800223b0u);
  } else {saved=*legacy;legacy->host=(void*)forbidden_legacy;legacy->name="forbidden_legacy";}
  assert(legacy);forbidden_legacy_calls=0;
  uint32_t registers[32];memcpy(registers,probe.gpr,sizeof registers);
  assert(runtime_bridge(&rt,&probe,0x800223b0u)<0 && forbidden_legacy_calls==0 && !memcmp(registers,probe.gpr,sizeof registers));
  if(added) {size_t index=(size_t)(legacy-rt.functions);memmove(legacy,legacy+1,(rt.function_count-index-1u)*sizeof(*legacy));rt.function_count--;}
  else *legacy=saved;
  store_le(field,4u,(uint32_t)(uintptr_t)PSX_ADDR(0x80150100u));
  assert(battle_startup_actors_boundary(&rt,&probe,0x800223b0u)==0);
  store_le(field,4u,saved_field);g_ActiveBattleRuntime=NULL;
 }
 puts("startup hybrid guest-pointer refusal PASS; compact boundary remains accepted");
 /* Keep the guest menu interpreter route while making refusal markers and
  * packet callback boundaries fail closed in hybrid mode. */
 {
  assert(bridge_normalize_hook(BRIDGE_REFUSE)==-1 && bridge_normalize_hook(0)==0 &&
         bridge_normalize_hook(1)==1 && bridge_normalize_hook(BRIDGE_HOOKS_NONE)==BRIDGE_HOOKS_NONE);
  g_ActiveBattleRuntime=&rt;assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));
  unsetenv("XENO_BATTLE_NATIVE_MENU_ALLOC_EXPERIMENT");
  PcPortMipsCpu probe;initialize_cpu(&probe,&rt);probe.gpr[31]=0x800805acu;
  assert(runtime_bridge(&rt,&probe,0x8007fec4u)==0);
  unsetenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT");
  initialize_cpu(&probe,&rt);probe.gpr[31]=0x8002c86cu;
  assert(runtime_bridge(&rt,&probe,0x80020000u)<0);
  g_ActiveBattleRuntime=NULL;
 }
 puts("hybrid menu fallback and packet/marker refusal PASS");
 puts("popup record, copy, sprite no-op, trig and decoder PASS retail CPU comparisons and guarded bridges; decoder remains test-only");
}
