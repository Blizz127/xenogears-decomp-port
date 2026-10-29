/* Test-only task, parent return and owner-loop tail versus retail CPU.
 * The colour helper is not executed; callback timing and rollback are unverified. */
#include "battle_mips_runtime.c"
#include "psx/gtereg.h"
/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
uint32_t g_HeapNeedsConsolidation;
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
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static unsigned timer_stub_calls;
static int timer_colour_stub(void *p,PcPortMipsCpu *c,uint32_t t) {
 (void)p;if(t!=0x800bcafcu || c->gpr[31]!=0x800bd000u)return 0;
 timer_stub_calls++;c->gpr[2]=0xabcdef01u;c->gpr[3]=0x10203040u;return 1;
}
static unsigned cleanup_calls;static uint32_t cleanup_trace[5][35];static uint8_t *cleanup_sprite;
static int cleanup_stub(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 (void)opaque;const uint32_t targets[]={0x8001f6b0u,0x80025180u,0x8001ce74u,0x8001cd94u,0x800320e8u};
 if(cleanup_calls>=5u || target!=targets[cleanup_calls])return 0;
 uint32_t *row=cleanup_trace[cleanup_calls++];row[0]=target;memcpy(row+1,cpu->gpr,sizeof cpu->gpr);
 row[33]=cleanup_sprite[0x2bu];row[34]=cpu->gpr[31];cpu->gpr[2]=0x12340000u+cleanup_calls;return 1;
}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
/* Test-only render callback prefix; stop before the matrix helper. */
static void popup_render_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=88u;
 battle_entry_write(e,r[29]+80u,4,r[31]);battle_entry_write(e,r[29]+76u,4,r[17]);
 battle_entry_write(e,r[29]+72u,4,r[16]);r[17]=battle_entry_read(e,r[4]+4u,4);
 r[4]=r[29]+24u;r[5]=r[17]+60u;r[31]=0x800bceccu;
}
/* Synthetic matrix callees record the actual call order and argument registers. */
static uint32_t matrix_trace[9][5];static unsigned matrix_calls;static int32_t matrix_z;
static int matrix_bridge(void *opaque,PcPortMipsCpu *c,uint32_t target)
{
 if(target!=0x80049d9cu && target!=0x8003f738u && target!=0x8004931cu && target!=0x8004a0dcu && target!=0x80021b14u && target!=0x8004974cu && target!=0x80049efcu && target!=0x80049f8cu && target!=0x800b1f6cu)return 0;
 assert(matrix_calls<9);uint32_t *row=matrix_trace[matrix_calls++];row[0]=target;
 for(unsigned i=0;i<4;i++)row[i+1]=c->gpr[4u+i];
 if(target==0x8004a0dcu){c->gpr[2]=512u;return 1;}
 if(target==0x800b1f6cu){c->gpr[2]=0xabcdef09u;return 1;}
 if(target==0x80021b14u){for(unsigned i=0;i<3;i++)assert(!runtime_write(opaque,c->gpr[4]+4u*i,4,c->gpr[5u+i]));c->gpr[2]=0xabc00004u;return 1;}
 if(target==0x8004974cu || target==0x80049efcu || target==0x80049f8cu){c->gpr[2]=0xabc00000u+matrix_calls;return 1;}
 uint32_t address=(target==0x80049d9cu?c->gpr[4]:target==0x8003f738u?c->gpr[5]:c->gpr[6]);
 assert(!runtime_write(opaque,address+20u,4,0x10203040u+matrix_calls));
 if(target==0x8004931cu)assert(!runtime_write(opaque,address+28u,4,(uint32_t)matrix_z));
 c->gpr[2]=0xabc00000u+matrix_calls;return 1;
}
static void popup_render_matrix_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;popup_render_prefix(e);
 battle_phase_call(e,0x80049d9cu,0x800bcec4u);
 r[4]=r[17]+76u;r[5]=r[29]+24u;battle_phase_call(e,0x8003f738u,0x800bced0u);
 r[4]=0x8004fbb8u;r[5]=r[29]+24u;r[6]=r[5];battle_phase_call(e,0x8004931cu,0x800bcee4u);
 r[31]=0x800bcef4u;
}
/* Continue through the retail scale arithmetic to the vector-setter call. */
static void popup_render_scale_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;popup_render_matrix_prefix(e);
 battle_phase_call(e,0x8004a0dcu,0x800bceecu);
 r[3]=battle_entry_read(e,r[29]+52u,4);r[5]=r[2]<<12;
 if(r[3]){int32_t a=(int32_t)r[5],b=(int32_t)r[3];e->cpu->lo=(uint32_t)(a/b);e->cpu->hi=(uint32_t)(a%b);r[5]=e->cpu->lo;}
 r[2]=0x01000000u;{int32_t a=(int32_t)r[2],b=(int32_t)r[5];assert(b);e->cpu->lo=(uint32_t)(a/b);e->cpu->hi=(uint32_t)(a%b);r[2]=e->cpu->lo;}
 r[16]=r[29]+56u;r[4]=r[16];r[3]=r[2]>>31;r[2]+=r[3];r[5]=(uint32_t)((int32_t)r[2]>>1);r[6]=r[5];r[7]=r[5];r[31]=0x800bcf3cu;
}
/* Controlled matrix calls, stopping before the actual frame draw. */
static void popup_render_draw_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;popup_render_scale_prefix(e);
 battle_phase_call(e,0x80021b14u,0x800bcf34u);
 r[4]=r[29]+24u;r[5]=r[16];battle_phase_call(e,0x8004974cu,0x800bcf40u);
 r[4]=r[29]+24u;battle_phase_call(e,0x80049efcu,0x800bcf48u);
 r[4]=r[29]+24u;battle_phase_call(e,0x80049f8cu,0x800bcf50u);
 r[2]=0x800c3eb0u;r[3]=0x8000u;r[2]+=r[3];r[3]=0x80060000u;
 battle_entry_write(e,r[29]+16u,4,0);battle_entry_write(e,r[29]+20u,4,0);
 r[4]=battle_entry_read(e,r[17]+100u,4);r[2]=battle_entry_read(e,r[2]+0x0c84u,4);
 r[6]=battle_entry_read(e,r[3]+0xffff956cu,4);r[2]<<=2;r[2]+=r[17];r[5]=battle_entry_read(e,r[2]+0x5cu,4);
 r[7]=0;r[31]=0x800bcf94u;
}
/* Complete the callback around a simulated frame draw. */
static void popup_render_return(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;popup_render_draw_prefix(e);
 battle_phase_call(e,0x800b1f6cu,0x800bcf8cu);
 r[31]=battle_entry_read(e,r[29]+80u,4);r[17]=battle_entry_read(e,r[29]+76u,4);
 r[16]=battle_entry_read(e,r[29]+72u,4);r[29]+=88u;
}
/* Unintegrated zero-record renderer path, with tpage already known zero. */
static void popup_empty_renderer(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=136u;r[8]=battle_entry_read(e,r[29]+156u,4);
 battle_entry_write(e,r[29]+112u,4,r[20]);r[20]=r[4];battle_entry_write(e,r[29]+100u,4,r[17]);r[17]=r[5];
 const unsigned saved[]={31,30,23,22,21,19,18,16};const unsigned off[]={132,128,124,120,116,108,104,96};
 for(unsigned i=0;i<8;i++)battle_entry_write(e,r[29]+off[i],4,r[saved[i]]);
 if(r[8])battle_entry_fail(e,"empty renderer tpage",r[8]);battle_entry_write(e,r[29]+144u,4,r[6]);
 r[18]=r[17];battle_entry_write(e,r[29]+88u,4,0);r[2]=battle_entry_read(e,r[20]+16u,4);
 r[3]=battle_entry_read(e,r[20],4);r[9]=battle_entry_read(e,r[20]+20u,4);r[17]=r[2]+r[20];
 r[2]=battle_entry_read(e,r[20]+8u,4);r[19]=r[3]+r[20];battle_entry_write(e,r[29]+84u,4,r[9]);
 r[2]+=r[20];battle_entry_write(e,r[29]+80u,4,r[2]);if(r[9])battle_entry_fail(e,"empty renderer count",r[9]);
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};
 for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+132u-i*4u,4);r[29]+=136u;
}
/* Test-only separate menu timer prefix; stop before cosine lookup. */
static void menu_timer_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=32u;r[2]=r[4];battle_entry_write(e,r[29]+16u,4,r[16]);r[16]=r[2];
 battle_entry_write(e,r[29]+28u,4,r[31]);battle_entry_write(e,r[29]+24u,4,r[18]);battle_entry_write(e,r[29]+20u,4,r[17]);
 r[3]=battle_entry_read(e,r[2]+32u,4);r[18]=battle_entry_read(e,r[16]+28u,4);r[17]=r[3]<<6;r[4]=r[17];r[31]=0x800bcbe4u;
}
static uint32_t menu_trace[3][5];static unsigned menu_calls;static int32_t fake_cos,fake_sin;
static int menu_timer_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 (void)opaque;if(target!=0x8003f8b0u && target!=0x8003f8ccu && target!=0x8001f6b0u)return 0;
 assert(menu_calls<3);uint32_t *row=menu_trace[menu_calls++];row[0]=target;
 for(unsigned i=0;i<4;i++)row[i+1]=cpu->gpr[4u+i];
 cpu->gpr[2]=target==0x8003f8b0u?(uint32_t)fake_cos:target==0x8003f8ccu?(uint32_t)fake_sin:0xabcdef01u;
 if(target==0x8001f6b0u)cpu->gpr[3]=0x10203040u;return 1;
}
static void menu_timer_run(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;menu_timer_prefix(e);battle_phase_call(e,0x8003f8b0u,0x800bcbdcu);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;r[2]=(int32_t)r[3]<256;
 if(!r[2])r[3]=255u;battle_entry_write(e,r[18]+0x28u,1,r[3]);
 r[4]=r[17];battle_phase_call(e,0x8003f8ccu,0x800bcc04u);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;r[2]=(int32_t)r[3]<256;r[4]=r[18];
 if(!r[2])r[3]=255u;battle_entry_write(e,r[4]+0x29u,1,r[3]);battle_entry_write(e,r[4]+0x2au,1,r[3]);
 battle_phase_call(e,0x8001f6b0u,0x800bcc2cu);
 r[2]=battle_entry_read(e,r[16]+0x20u,4);r[2]++;battle_entry_write(e,r[16]+0x20u,4,r[2]);
 r[31]=battle_entry_read(e,r[29]+28u,4);r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=32u;
}
/* Test-only task step prefix ending at the colour helper boundary. */
static void task_step_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=24u;
 battle_entry_write(e,r[29]+20u,4,r[31]);battle_entry_write(e,r[29]+16u,4,r[16]);
 r[16]=battle_entry_read(e,r[4]+4u,4);r[4]=battle_entry_read(e,r[16]+0x38u,4);
 r[5]=battle_entry_read(e,r[16]+0x58u,4);r[3]=battle_entry_read(e,r[16]+0x54u,4);
 r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[4]+2u,2);
 battle_entry_write(e,r[16]+0x3cu,4,r[2]);
 r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[4]+6u,2);r[5]++;
 battle_entry_write(e,r[16]+0x40u,4,r[2]);r[2]-=r[3];
 r[6]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[4]+10u,2);r[2]-=32u;
 battle_entry_write(e,r[16]+0x58u,4,r[5]);battle_entry_write(e,r[16]+0x40u,4,r[2]);
 r[31]=0x800bd000u;battle_entry_write(e,r[16]+0x44u,4,r[6]);
}
/* Test-only suffix after the colour helper returns. */
static void task_step_suffix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 r[2]=battle_entry_read(e,r[16]+0x4eu,2);r[2]+=16u;
 battle_entry_write(e,r[16]+0x4eu,2,r[2]);
 r[31]=battle_entry_read(e,r[29]+20u,4);r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=24u;
}
/* Test-only complete timer body with a simulated colour child. */
static void task_step_full(BattleEntryCall *e)
{task_step_prefix(e);battle_phase_call(e,0x800bcafcu,0x800bcff8u);task_step_suffix(e);}
/* Test-only colour child: real guarded trig and no-op sprite leaves. */
static void timer_colour_full(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=24u;battle_entry_write(e,r[29]+16u,4,r[16]);
 r[16]=r[4];battle_entry_write(e,r[29]+20u,4,r[31]);r[4]=r[5]<<6;
 battle_phase_call(e,0x8003f8b0u,0x800bcb0cu);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;
 r[2]=(int32_t)r[3]<256;r[4]=r[16];if(!r[2])r[3]=255u;
 battle_entry_write(e,r[4]+0x28u,1,r[3]);battle_entry_write(e,r[4]+0x29u,1,r[3]);battle_entry_write(e,r[4]+0x2au,1,r[3]);
 battle_phase_call(e,0x8001f6b0u,0x800bcb38u);
 r[31]=battle_entry_read(e,r[29]+20u,4);r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=24u;
}
static int timer_composed_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x800bcafcu && cpu->gpr[31]==0x800bd000u) {
  BattleEntryCall e={.runtime=opaque,.cpu=cpu};if(setjmp(e.failure))return -1;
  timer_colour_full(&e);return 1;
 }
 return runtime_bridge(opaque,cpu,target);
}
/* Test-only retail deferred graphics queue leaf; synthetic packed nodes. */
static void popup_deferred_queue(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[5]=battle_entry_read(e,r[28]+0x410u,4);r[2]=r[5]+8u;
 battle_entry_write(e,r[28]+0x410u,4,r[2]);
 if(r[5]) {
  r[2]=battle_entry_read(e,r[28]+0x188u,4);r[3]=0x80059300u;
  battle_entry_write(e,r[5],4,r[4]);r[2]=(r[2]<<2)+r[3];
  r[3]=battle_entry_read(e,r[2],4);battle_entry_write(e,r[5]+4u,4,r[3]);
  battle_entry_write(e,r[2],4,r[5]);
 }
}
/* Test-only 8001CE74 translation for bounded lists with no matching callback.
 * The production cleanup bridge remains guarded. */
static void popup_unlink_no_callback(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=40u;
 battle_entry_write(e,r[29]+16u,4,r[16]);r[16]=battle_entry_read(e,r[28]+0x424u,4);
 battle_entry_write(e,r[29]+28u,4,r[19]);r[19]=r[4];
 battle_entry_write(e,r[29]+20u,4,r[17]);r[17]=0;
 battle_entry_write(e,r[29]+32u,4,r[31]);battle_entry_write(e,r[29]+24u,4,r[18]);
 for(unsigned list=0;list<2;list++) {
  if(list){r[16]=battle_entry_read(e,r[28]+0x41cu,4);r[17]=0;}
  if(r[16])r[18]=0x1fffffffu;
  unsigned count=0;
  while(r[16]) {
   assert(++count<=8u);r[2]=battle_entry_read(e,r[16],4);
   int match=0;
   if(r[2]==r[19]) {
    r[3]=battle_entry_read(e,r[16]+20u,4);r[2]=(r[3]>>30)&1u;r[3]&=r[18];
    if(!r[2]) {r[2]=battle_entry_read(e,r[19]+16u,4)&r[18];match=r[3]==r[2];}
   }
   if(match) {
    r[2]=battle_entry_read(e,r[16]+24u,4);
    if(r[17])battle_entry_write(e,r[17]+24u,4,r[2]);
    else battle_entry_write(e,r[28]+(list?0x41cu:0x424u),4,r[2]);
    r[2]=battle_entry_read(e,r[28]+0x420u,4);
    if(r[2]==r[16]) {r[2]=battle_entry_read(e,r[16]+24u,4);battle_entry_write(e,r[28]+0x420u,4,r[2]);}
    r[2]=battle_entry_read(e,r[16]+12u,4);assert(!r[2]);
   } else r[17]=r[16];
   r[16]=battle_entry_read(e,r[16]+24u,4);
  }
 }
 r[31]=battle_entry_read(e,r[29]+32u,4);r[19]=battle_entry_read(e,r[29]+28u,4);
 r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);
 r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=40u;
}
/* Test-only HeapFree leaf for known blocks or a null pointer with the error
 * handler disabled. No production dispatch or live allocation is attempted. */
static void popup_heap_free_model(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=32u;r[5]=r[4];battle_entry_write(e,r[29]+24u,4,r[31]);
 if(!r[5]) {r[2]=battle_entry_read(e,r[28]+0x1c0u,4);assert(r[2]);r[2]=1u;}
 else {
  r[4]=battle_entry_read(e,r[5]-4u,4);r[2]=r[4]&0x02000000u;r[3]=0x03ff0000u;
  if(r[2])r[2]=UINT32_MAX;
  else {
   r[3]=0x03ffffffu;r[3]&=r[4];r[2]=0x84000000u;r[3]|=r[2];
   r[4]=0xfe1fffffu;r[3]&=r[4];r[4]=0xffe00000u;r[3]&=r[4];
   battle_entry_write(e,r[5]-4u,4,r[3]);r[3]=1u;
   battle_entry_write(e,r[28]+0x1bcu,4,r[3]);r[2]=0;
  }
 }
 r[31]=battle_entry_read(e,r[29]+24u,4);r[29]+=32u;
}
/* Test-only bounded TimerWorkListRemoveTask translation. */
static void popup_timer_remove_model(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[2]=battle_entry_read(e,r[28]+0x41cu,4);r[3]=0;
 unsigned count=0;
 while(r[2]) {
  assert(++count<=8u);
  if(r[2]==r[4]) {
   r[2]=battle_entry_read(e,r[4]+24u,4);
   if(r[3])battle_entry_write(e,r[3]+24u,4,r[2]);
   else battle_entry_write(e,r[28]+0x41cu,4,r[2]);
   r[2]=battle_entry_read(e,r[28]+0x420u,4);
   if(r[2]==r[4]) {r[2]=battle_entry_read(e,r[4]+24u,4);battle_entry_write(e,r[28]+0x420u,4,r[2]);}
   break;
  }
  r[3]=r[2];r[2]=battle_entry_read(e,r[2]+24u,4);
 }
 r[2]=battle_entry_read(e,r[4]+20u,4);
 if((int32_t)r[2]<0) {
  r[2]=battle_entry_read(e,0x80059464u,4)-1u;r[1]=0x80060000u;
  battle_entry_write(e,0x80059464u,4,r[2]);
 }
 r[2]=battle_entry_read(e,r[28]+0x18u,4)-1u;battle_entry_write(e,r[28]+0x18u,4,r[2]);
}
/* Compose modeled leaves for a synthetic cleanup, using the actual guarded
 * sprite no-op bridge. The remaining children stay test-only. */
static int composed_cleanup_host;
static unsigned composed_sprite_calls;
static int composed_cleanup_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target==0x8001f6b0u) {
  composed_sprite_calls++;if(!composed_cleanup_host)return 0;
  return runtime_bridge(opaque,cpu,target);
 }
 if(!composed_cleanup_host)return 0;
 BattleEntryCall e={.runtime=opaque,.cpu=cpu};
 if(setjmp(e.failure))assert(!"synthetic composed child refused");
 switch(target) {
  case 0x80025180u:popup_deferred_queue(&e);return 1;
  case 0x8001ce74u:popup_unlink_no_callback(&e);return 1;
  case 0x8001cd94u:popup_timer_remove_model(&e);return 1;
  case 0x800320e8u:popup_heap_free_model(&e);return 1;
  default:return 0;
 }
}
/* Cleanup control flow only: all five child services are simulated. */
static void popup_cleanup_full(BattleEntryCall *e)
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
/* Test-only parent return after the task step. */
static void popup_create_return(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 r[31]=battle_entry_read(e,r[29]+48u,4);r[21]=battle_entry_read(e,r[29]+44u,4);
 r[20]=battle_entry_read(e,r[29]+40u,4);r[19]=battle_entry_read(e,r[29]+36u,4);
 r[18]=battle_entry_read(e,r[29]+32u,4);r[17]=battle_entry_read(e,r[29]+28u,4);
 r[16]=battle_entry_read(e,r[29]+24u,4);r[29]+=56u;
}
/* Test-only owner-loop tail after the creator returns. */
static void owner_loop_after_create(PcPortMipsCpu *cpu)
{
 uint32_t *r=cpu->gpr;r[17]+=4u;r[19]++;r[2]=r[18]&0xffffu;
 r[18]=r[2]>>1;r[2]=11u;r[2]=r[18]&1u;
}
/* Test-only owner-loop return. */
static void owner_loop_return(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 r[31]=battle_entry_read(e,r[29]+40u,4);r[21]=battle_entry_read(e,r[29]+36u,4);
 r[20]=battle_entry_read(e,r[29]+32u,4);r[19]=battle_entry_read(e,r[29]+28u,4);
 r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);
 r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=48u;
}
int main(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 assert(!fseek(f,0x800bcbb4u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcbb4u),1,0xacu,f)==0xacu);
 assert(!fseek(f,0x800bcafcu-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x800bcafcu),1,0x58u,f)==0x58u);
 assert(!fseek(f,0x800b1f6cu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b1f6cu),1,0xb80u,f)==0xb80u);
 assert(!fseek(f,0x800bceacu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bceacu),1,0x100u,f)==0x100u);
 assert(!fseek(f,0x800bcfacu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcfacu),1,0x78u,f)==0x78u);
 assert(!fseek(f,0x800bd024u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x800bd024u),1,0x74u,f)==0x74u);
 assert(!fseek(f,0x800bd1d4u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bd1d4u),1,0x28u,f)==0x28u);
 assert(!fseek(f,0x800bce2cu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bce2cu),1,0x58u,f)==0x58u);
 assert(!fseek(f,0x800bce84u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bce84u),1,0x28u,f)==0x28u);fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(!fseek(f,0x8003f8b0u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8003f8b0u),1,28u,f)==28u);
 assert(!fseek(f,0x8001f6b0u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8001f6b0u),1,0xa0u,f)==0xa0u);
 assert(!fseek(f,0x80025180u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x80025180u),1,0x48u,f)==0x48u);
 assert(!fseek(f,0x8001ce74u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8001ce74u),1,0x1c0u,f)==0x1c0u);
 assert(!fseek(f,0x800320e8u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x800320e8u),1,0xa4u,f)==0xa4u);
 assert(!fseek(f,0x8001cd94u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8001cd94u),1,0xb0u,f)==0xb0u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(stack-88,0x33,88u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;
  store_le(task+4u,4,o);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bceacu,0x80049d9cu,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_render_prefix(&e);else assert(!"unexpected render prefix refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(stack-88,0x33,88u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+4u,4,o);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=matrix_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);matrix_calls=0;
  assert(PcPortMipsRun(&cpu,0x800bceacu,0x8004a0dcu,200)==PC_PORT_MIPS_HALTED);
  assert(matrix_calls==3);uint32_t want_trace[3][5];memcpy(want_trace,matrix_trace,sizeof want_trace);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);matrix_calls=0;
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_render_matrix_prefix(&e);else assert(!"unexpected matrix prefix refusal");
  assert(matrix_calls==3 && !memcmp(want_trace,matrix_trace,sizeof want_trace));
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++)for(unsigned zcase=0;zcase<3;zcase++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(stack-88,0x33,88u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+4u,4,o);
  matrix_z=zcase==0?0:zcase==1?4096:-4096;
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=matrix_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);matrix_calls=0;
  assert(PcPortMipsRun(&cpu,0x800bceacu,0x80021b14u,200)==PC_PORT_MIPS_HALTED);
  assert(matrix_calls==4);uint32_t want_trace[4][5];memcpy(want_trace,matrix_trace,sizeof want_trace);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);matrix_calls=0;
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_render_scale_prefix(&e);else assert(!"unexpected scale prefix refusal");
  assert(matrix_calls==4 && !memcmp(want_trace,matrix_trace,sizeof want_trace));
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(object,0xa5,0x100u);memset(stack-88,0x33,88u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+4u,4,o);
  store_le(object+0x5cu,4,0x12345678u);store_le(object+0x60u,4,0x87654321u);store_le(object+0x64u,4,0x11112222u);
  assert(!runtime_write(&rt,0x800ccb34u,4,mode));assert(!runtime_write(&rt,0x8005956cu,4,0x31415926u));matrix_z=4096;
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=matrix_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);matrix_calls=0;
  assert(PcPortMipsRun(&cpu,0x800bceacu,0x800b1f6cu,300)==PC_PORT_MIPS_HALTED);
  assert(matrix_calls==8);uint32_t want_trace[8][5];memcpy(want_trace,matrix_trace,sizeof want_trace);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);matrix_calls=0;
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_render_draw_prefix(&e);else assert(!"unexpected draw prefix refusal");
  assert(matrix_calls==8 && !memcmp(want_trace,matrix_trace,sizeof want_trace));
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(object,0xa5,0x100u);memset(stack-88,0x33,88u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+4u,4,o);
  store_le(object+0x5cu,4,0x12345678u);store_le(object+0x60u,4,0x87654321u);store_le(object+0x64u,4,0x11112222u);
  assert(!runtime_write(&rt,0x800ccb34u,4,mode));assert(!runtime_write(&rt,0x8005956cu,4,0x31415926u));matrix_z=4096;
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=matrix_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);matrix_calls=0;
  assert(PcPortMipsRun(&cpu,0x800bceacu,0x8000deacu,300)==PC_PORT_MIPS_HALTED);
  assert(matrix_calls==9);uint32_t want_trace[9][5];memcpy(want_trace,matrix_trace,sizeof want_trace);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);matrix_calls=0;
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_render_return(&e);else assert(!"unexpected render return refusal");
  assert(matrix_calls==9 && !memcmp(want_trace,matrix_trace,sizeof want_trace));
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *geom=PSX_ADDR(0x80150000u),*stack=PSX_ADDR(0x801ff000u);memset(geom,0x5a,0x100u);memset(stack-136,0x33,160u);
  uint32_t model=mode?(uint32_t)(uintptr_t)geom:0x80150000u;
  store_le(geom,4,0x80u);store_le(geom+8u,4,0x40u);store_le(geom+16u,4,0x20u);store_le(geom+20u,4,0);store_le(stack+20u,4,0);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=model;cpu.gpr[5]=0x80160000u;cpu.gpr[6]=0x80170000u;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800b1f6cu,0x8000deacu,300)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_empty_renderer(&e);else assert(!"unexpected empty renderer refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(stack-32,0x33,32u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+0x1cu,4,o);store_le(task+0x20u,4,mode?0xffffffffu:0x12345678u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bcbb4u,0x8003f8b0u,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)menu_timer_prefix(&e);else assert(!"unexpected menu timer prefix refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<3;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*stack=PSX_ADDR(0x801ff000u);
  memset(task,0x5a,0x100u);memset(object,0xa5,0x100u);memset(stack-32,0x33,32u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;store_le(task+0x1cu,4,o);store_le(task+0x20u,4,mode?0xffffffffu:0x12345678u);
  fake_cos=mode==2?-20000:mode?-4096:4096;fake_sin=mode==2?-20000:mode?4096:-4096;
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=menu_timer_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);menu_calls=0;
  assert(PcPortMipsRun(&cpu,0x800bcbb4u,0x8000deacu,300)==PC_PORT_MIPS_HALTED);
  assert(menu_calls==3);uint32_t trace[3][5];memcpy(trace,menu_trace,sizeof trace);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);menu_calls=0;
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)menu_timer_run(&e);else assert(!"unexpected menu timer refusal");
  assert(menu_calls==3 && !memcmp(trace,menu_trace,sizeof trace));
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*sprite=PSX_ADDR(0x80150200u);
  memset(task,0x5a,0x100u);memset(object,0xa5,0x100u);memset(sprite,0x33,0x100u);
  uint32_t t=mode?(uint32_t)(uintptr_t)task:0x80150000u;
  uint32_t o=mode?(uint32_t)(uintptr_t)object:0x80150100u;
  uint32_t sptr=mode?(uint32_t)(uintptr_t)sprite:0x80150200u;
  store_le(task+4u,4,o);store_le(object+0x38u,4,sptr);store_le(object+0x58u,4,mode?0xffffffffu:0x1234u);
  store_le(object+0x54u,4,mode?0xfffffff0u:0x50u);
  store_le(sprite+2u,2,mode?0x8000u:0x1234u);store_le(sprite+6u,2,mode?0xffffu:0x80u);store_le(sprite+10u,2,mode?0x7fffu:0xfffeu);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=t;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x800bd1d4u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bcfacu,0x800bcafcu,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)task_step_prefix(&e);else assert(!"unexpected prefix refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *object=PSX_ADDR(0x80150100u);memset(object,0xa5,0x100u);
  store_le(object+0x4eu,2,mode?0xfff8u:0x1234u);
  store_le(PSX_ADDR(0x801ff000u)+16u,4,0x11223344u);
  store_le(PSX_ADDR(0x801ff000u)+20u,4,0x800bd1d4u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[16]=mode?(uint32_t)(uintptr_t)object:0x80150100u;cpu.gpr[29]=0x801ff000u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bd000u,0x800bd1d4u,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)task_step_suffix(&e);else assert(!"unexpected suffix refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 /* Two successive synthetic callbacks, retaining task state between calls. */
 {
  uint8_t *task=PSX_ADDR(0x80150000u),*sprite=PSX_ADDR(0x80150200u);
  memset(task,0,0x100u);memset(sprite,0x33,0x100u);
  uint32_t t=(uint32_t)(uintptr_t)task,sptr=(uint32_t)(uintptr_t)sprite;
  store_le(task+4u,4,t);store_le(task+0x38u,4,sptr);store_le(task+0x58u,4,0xe3e8feffu);
  store_le(task+0x54u,4,0x50u);store_le(task+0x4eu,2,0x10u);
  store_le(sprite+2u,2,0x1234u);store_le(sprite+6u,2,0x80u);store_le(sprite+10u,2,0xfffeu);
  PcPortMipsCpu base;initialize_cpu(&base,&rt);base.bus.bridge=timer_colour_stub;
  for(unsigned i=1;i<32;i++)base.gpr[i]=0x31400000u+i;
  base.gpr[4]=t;base.gpr[29]=0x801ff000u;base.gpr[31]=0x8000deacu;
  PcPortMipsCpu cpu1=base,cpu2=base,host1=base,host2=base;memcpy(initial,g_PsxRam,sizeof initial);timer_stub_calls=0;
  assert(PcPortMipsRun(&cpu1,0x800bcfacu,base.gpr[31],400)==PC_PORT_MIPS_HALTED);
  assert(PcPortMipsRun(&cpu2,0x800bcfacu,base.gpr[31],400)==PC_PORT_MIPS_HALTED && timer_stub_calls==2u);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);timer_stub_calls=0;
  BattleEntryCall first={.runtime=&rt,.cpu=&host1},second={.runtime=&rt,.cpu=&host2};
  if(setjmp(first.failure)==0)task_step_full(&first);else assert(!"first timer refused");
  if(setjmp(second.failure)==0)task_step_full(&second);else assert(!"second timer refused");
  assert(timer_stub_calls==2u && !memcmp(cpu1.gpr,host1.gpr,sizeof cpu1.gpr) && !memcmp(cpu2.gpr,host2.gpr,sizeof cpu2.gpr));
  assert(cpu1.hi==host1.hi && cpu1.lo==host1.lo && cpu2.hi==host2.hi && cpu2.lo==host2.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
  assert(load_le(task+0x58u,4)==0xe3e8ff01u && load_le(task+0x4eu,2)==0x30u);
 }
 /* Two complete synthetic task steps with actual guarded trig/no-op leaves. */
 {
  assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT","1",1));
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT","1",1));
  g_ActiveBattleRuntime=&rt;const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image && image->bytes);
  for(unsigned j=0;j<2;j++) {uint32_t phase=0xe3e8ff00u+j,table=0x800523f0u+((phase<<6)&0xfffu)*4u;
   store_le(PSX_ADDR(table),2,load_le(image->bytes+XENO_RD_SLUS_OFF(table),2));}
  uint8_t *task=PSX_ADDR(0x80150000u),*sprite=PSX_ADDR(0x80150200u);
  memset(task,0,0x100u);memset(sprite,0x33,0x100u);
  uint32_t t=(uint32_t)(uintptr_t)task,sptr=(uint32_t)(uintptr_t)sprite;
  store_le(task+4u,4,t);store_le(task+0x38u,4,sptr);store_le(task+0x58u,4,0xe3e8feffu);
  store_le(task+0x54u,4,0x50u);store_le(task+0x4eu,2,0x10u);store_le(sprite+0x3cu,4,0x10000000u);
  store_le(sprite+2u,2,0x1234u);store_le(sprite+6u,2,0x80u);store_le(sprite+10u,2,0xfffeu);
  PcPortMipsCpu base;initialize_cpu(&base,&rt);base.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)base.gpr[i]=0x31400000u+i;
  base.gpr[4]=t;base.gpr[29]=0x801ff000u;base.gpr[31]=0x8000deacu;
  PcPortMipsCpu cpu1=base,cpu2=base,host1=base,host2=base;host1.bus.bridge=host2.bus.bridge=timer_composed_bridge;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu1,0x800bcfacu,base.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  assert(PcPortMipsRun(&cpu2,0x800bcfacu,base.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall first={.runtime=&rt,.cpu=&host1},second={.runtime=&rt,.cpu=&host2};
  if(setjmp(first.failure)==0)task_step_full(&first);else assert(!"first composed timer refused");
  if(setjmp(second.failure)==0)task_step_full(&second);else assert(!"second composed timer refused");
  assert(!memcmp(cpu1.gpr,host1.gpr,sizeof cpu1.gpr) && !memcmp(cpu2.gpr,host2.gpr,sizeof cpu2.gpr));
  assert(cpu1.hi==host1.hi && cpu1.lo==host1.lo && cpu2.hi==host2.hi && cpu2.lo==host2.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));g_ActiveBattleRuntime=NULL;
  unsetenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT");unsetenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT");
 }
 /* Fixed synthetic deferred-queue shapes, including null cursor behavior. */
 for(unsigned mode=0;mode<3;mode++) {
  uint8_t *node=PSX_ADDR(0x80170000u);memset(node,0x5a,32u);
  uint32_t cursor=mode==0?0u:mode==1?0x80170000u:(uint32_t)(uintptr_t)node;
  uint32_t payload=mode==2?(uint32_t)(uintptr_t)PSX_ADDR(0x80173000u):0x80173000u;
  uint32_t context=mode==2?1u:0u;
  assert(!runtime_write(&rt,0x80059580u,4,cursor));assert(!runtime_write(&rt,0x800592f8u,4,context));
  assert(!runtime_write(&rt,0x80059300u+4u*context,4,0x80172000u));
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=payload;retail.gpr[28]=0x80059170u;retail.gpr[31]=0x800bd068u;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x80025180u,retail.gpr[31],100)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_deferred_queue(&e);else assert(!"synthetic deferred queue refused");
  assert(!memcmp(retail.gpr,host.gpr,sizeof retail.gpr) && retail.hi==host.hi && retail.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 puts("deferred graphics queue PASS fixed synthetic CPU comparisons; no real free");
 /* No matching callbacks: compare packed list effects and the full CPU state. */
 for(unsigned mode=0;mode<4;mode++) {
  uint8_t *owner=PSX_ADDR(0x80154000u),*a=PSX_ADDR(0x80154100u),*b=PSX_ADDR(0x80154140u);
  memset(owner,0,0x40u);memset(a,0,0x40u);memset(b,0,0x40u);
  uint32_t o=(mode&1u)?(uint32_t)(uintptr_t)owner:0x80154000u;
  uint32_t x=(mode&1u)?(uint32_t)(uintptr_t)a:0x80154100u,y=(mode&1u)?(uint32_t)(uintptr_t)b:0x80154140u;
  store_le(owner+16u,4,0x12345u);store_le(a,4,o);store_le(a+20u,4,0x12345u);store_le(a+24u,4,y);
  store_le(b,4,o);store_le(b+20u,4,mode==2?0x40012345u:0x12346u);
  assert(!runtime_write(&rt,0x80059594u,4,mode==0?0:x));
  assert(!runtime_write(&rt,0x8005958cu,4,mode==0?0:mode==3?x:0));
  if(mode==3)assert(!runtime_write(&rt,0x80059594u,4,0));
  assert(!runtime_write(&rt,0x80059590u,4,x));
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=o;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x8001ce74u,retail.gpr[31],1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_unlink_no_callback(&e);else assert(!"synthetic unlink refused");
  assert(!memcmp(retail.gpr,host.gpr,sizeof retail.gpr) && retail.hi==host.hi && retail.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 puts("popup cleanup unlink PASS fixed synthetic no-callback full CPU comparisons");
 for(unsigned mode=0;mode<5;mode++) {
  uint8_t *block=PSX_ADDR(0x80160000u);memset(block,0x5a,0x60u);
  uint32_t payload=mode==2?0u:mode>=3?(uint32_t)(uintptr_t)(block+8):0x80160008u;
  store_le(block+4u,4,0x1234u|(3u<<21)|((mode==1||mode==4)?(1u<<25):0u)|(5u<<26));
  assert(!runtime_write(&rt,0x8005932cu,4,0));assert(!runtime_write(&rt,0x80059330u,4,1u));
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=payload;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x800320e8u,retail.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_heap_free_model(&e);else assert(!"synthetic free refused");
  assert(!memcmp(retail.gpr,host.gpr,sizeof retail.gpr) && retail.hi==host.hi && retail.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 puts("popup cleanup heap free PASS fixed synthetic full CPU comparisons");
 for(unsigned mode=0;mode<6;mode++) {
  uint8_t *a=PSX_ADDR(0x80154100u),*b=PSX_ADDR(0x80154140u),*other=PSX_ADDR(0x80154180u);
  memset(a,0,0x40u);memset(b,0,0x40u);memset(other,0,0x40u);
  uint32_t x=(mode&1u)?(uint32_t)(uintptr_t)a:0x80154100u,y=(mode&1u)?(uint32_t)(uintptr_t)b:0x80154140u;
  uint32_t z=(mode&1u)?(uint32_t)(uintptr_t)other:0x80154180u;
  store_le(a+24u,4,y);store_le(b+20u,4,mode==4?0x80000000u:0u);
  uint32_t target=mode==0?x:mode==2?z:y;
  assert(!runtime_write(&rt,0x8005958cu,4,mode==3?0:x));assert(!runtime_write(&rt,0x80059590u,4,target));
  assert(!runtime_write(&rt,0x80059188u,4,7u));assert(!runtime_write(&rt,0x80059464u,4,4u));
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=target;retail.gpr[28]=0x80059170u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&retail,0x8001cd94u,retail.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_timer_remove_model(&e);else assert(!"synthetic timer removal refused");
  assert(!memcmp(retail.gpr,host.gpr,sizeof retail.gpr) && retail.hi==host.hi && retail.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
  uint32_t timer_count,marked_count;
  assert(!runtime_read(&rt,0x80059188u,4,&timer_count) && timer_count==6u);
  assert(!runtime_read(&rt,0x80059464u,4,&marked_count) && marked_count==(mode==4?3u:4u));
 }
 puts("popup cleanup timer removal PASS fixed synthetic full CPU comparisons");
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT","1",1));g_ActiveBattleRuntime=&rt;
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*sprite=PSX_ADDR(0x80150200u);
  uint8_t *child=PSX_ADDR(0x80150400u),*node=PSX_ADDR(0x80150500u);
  memset(task,0,0x100u);memset(object,0,0x100u);memset(sprite,0,0x100u);memset(child,0,0x40u);memset(node,0,0x40u);
  uint32_t ta=mode?(uint32_t)(uintptr_t)task:0x80150000u,oa=mode?(uint32_t)(uintptr_t)object:0x80150100u;
  uint32_t sa=mode?(uint32_t)(uintptr_t)sprite:0x80150200u,ca=mode?(uint32_t)(uintptr_t)child:0x80150400u;
  uint32_t na=mode?(uint32_t)(uintptr_t)node:0x80150500u,frame=mode?(uint32_t)(uintptr_t)PSX_ADDR(0x80150300u):0x80150300u;
  store_le(task+4u,4,oa);store_le(task+16u,4,0x12345u);store_le(object+0x38u,4,sa);store_le(object+0x5cu,4,frame);
  store_le(object-4u,4,0x1234u|(3u<<21)|(5u<<26));store_le(child,4,ta);store_le(child+20u,4,0x12345u);
  assert(!runtime_write(&rt,0x80059580u,4,na));assert(!runtime_write(&rt,0x800592f8u,4,0));assert(!runtime_write(&rt,0x80059300u,4,0));
  assert(!runtime_write(&rt,0x80059594u,4,ca));assert(!runtime_write(&rt,0x8005958cu,4,ta));assert(!runtime_write(&rt,0x80059590u,4,ta));
  assert(!runtime_write(&rt,0x80059188u,4,1u));assert(!runtime_write(&rt,0x8005932cu,4,0));
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=composed_cleanup_bridge;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=ta;retail.gpr[28]=0x80059170u;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);composed_cleanup_host=0;composed_sprite_calls=0;
  assert(PcPortMipsRun(&retail,0x800bd024u,retail.gpr[31],2000)==PC_PORT_MIPS_HALTED && composed_sprite_calls==1);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  composed_cleanup_host=1;composed_sprite_calls=0;BattleEntryCall e={.runtime=&rt,.cpu=&host};
  if(setjmp(e.failure)==0)popup_cleanup_full(&e);else assert(!"synthetic composed cleanup refused");
  assert(composed_sprite_calls==1 && !memcmp(retail.gpr,host.gpr,sizeof retail.gpr));
  assert(retail.hi==host.hi && retail.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  uint32_t state;
  assert(!runtime_read(&rt,0x80059594u,4,&state) && state==0);
  assert(!runtime_read(&rt,0x8005958cu,4,&state) && state==0);
  assert(!runtime_read(&rt,0x80059590u,4,&state) && state==0);
  assert(!runtime_read(&rt,0x80059188u,4,&state) && state==0);
  assert(!runtime_read(&rt,0x80059300u,4,&state) && state==na);
  assert(load_le(node,4)==frame && load_le(node+4u,4)==0);
  assert(load_le(object-4u,4)==(0x21u<<26) && (sprite[0x2bu]&1u));
 }
 g_ActiveBattleRuntime=NULL;unsetenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT");
 puts("popup cleanup composed leaves PASS fixed synthetic full CPU comparisons; guarded sprite no-op branch");
 /* Cleanup callback call order and memory effects; no actual frees. */
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *task=PSX_ADDR(0x80150000u),*object=PSX_ADDR(0x80150100u),*sprite=PSX_ADDR(0x80150200u);
  memset(task,0x5a,0x100u);memset(object,0xa5,0x100u);memset(sprite,0x32,0x100u);cleanup_sprite=sprite;
  uint32_t ta=mode?(uint32_t)(uintptr_t)task:0x80150000u,oa=mode?(uint32_t)(uintptr_t)object:0x80150100u;
  uint32_t sa=mode?(uint32_t)(uintptr_t)sprite:0x80150200u,frame=mode?(uint32_t)(uintptr_t)PSX_ADDR(0x80150300u):0x80150300u;
  store_le(task+4u,4,oa);store_le(object+0x38u,4,sa);store_le(object+0x5cu,4,frame);
  PcPortMipsCpu retail;initialize_cpu(&retail,&rt);retail.bus.bridge=cleanup_stub;
  for(unsigned i=1;i<32;i++)retail.gpr[i]=0x31400000u+i;
  retail.gpr[4]=ta;retail.gpr[29]=0x801ff000u;retail.gpr[31]=0x8000deacu;PcPortMipsCpu host=retail;
  memcpy(initial,g_PsxRam,sizeof initial);cleanup_calls=0;
  assert(PcPortMipsRun(&retail,0x800bd024u,retail.gpr[31],500)==PC_PORT_MIPS_HALTED && cleanup_calls==5u);
  uint32_t trace[5][35];memcpy(trace,cleanup_trace,sizeof trace);memcpy(expected,g_PsxRam,sizeof expected);
  memcpy(g_PsxRam,initial,sizeof initial);cleanup_calls=0;BattleEntryCall e={.runtime=&rt,.cpu=&host};
  if(setjmp(e.failure)==0)popup_cleanup_full(&e);else assert(!"synthetic cleanup refused");
  assert(cleanup_calls==5u && !memcmp(trace,cleanup_trace,sizeof trace));
  assert(!memcmp(retail.gpr,host.gpr,sizeof retail.gpr) && retail.hi==host.hi && retail.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && (sprite[0x2bu]&1u));
 }
 puts("popup cleanup PASS synthetic CPU comparison; all child services simulated");
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *stack=PSX_ADDR(0x801ff000u);memset(stack,0x5a,56u);
  for(unsigned i=0;i<6;i++)store_le(stack+24u+i*4u,4,0x11111110u+i+mode);
  store_le(stack+48u,4,0x800bccd4u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bd1d4u,0x800bccd4u,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)popup_create_return(&e);else assert(!"unexpected parent return refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 for(unsigned mode=0;mode<2;mode++) {
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[17]=0x80150000u;cpu.gpr[18]=mode?3u:0x1234u;cpu.gpr[19]=mode?10u:1u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bce2cu,mode?0x800bce84u:0x800bcdfcu,200)==PC_PORT_MIPS_HALTED);
  owner_loop_after_create(&host);
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(initial,g_PsxRam,sizeof initial));
 }
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *stack=PSX_ADDR(0x801ff000u);memset(stack,0x5a,48u);
  for(unsigned i=0;i<6;i++)store_le(stack+16u+i*4u,4,0x22222220u+i+mode);
  store_le(stack+40u,4,0x8008067cu);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[29]=mode?(uint32_t)(uintptr_t)stack:0x801ff000u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bce84u,0x8008067cu,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)owner_loop_return(&e);else assert(!"unexpected owner return refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 puts("popup callbacks and two successive task steps PASS CPU comparisons; includes composed trig/no-op and simulated colour cases");return 0;
}
