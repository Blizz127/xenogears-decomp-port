/* Test-only colour prefix versus retail CPU, using the guarded trig lookup.
 * Stops before sprite update; no callback timing or rollback is verified. */
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
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
/* Test-only prefix ending before the sprite update. */
static void colour_prefix(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=24u;battle_entry_write(e,r[29]+16u,4,r[16]);
 r[16]=r[4];battle_entry_write(e,r[29]+20u,4,r[31]);r[4]=r[5]<<6;
 battle_phase_call(e,0x8003f8b0u,0x800bcb0cu);
 r[3]=r[2]+4096u;r[3]=(uint32_t)((int32_t)r[3]>>6);r[3]+=128u;
 r[2]=(int32_t)r[3]<256;r[4]=r[16];if(!r[2])r[3]=255u;
 battle_entry_write(e,r[4]+0x28u,1,r[3]);battle_entry_write(e,r[4]+0x29u,1,r[3]);
 r[31]=0x800bcb40u;battle_entry_write(e,r[4]+0x2au,1,r[3]);
}
int main(void)
{
 assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT","1",1));
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800bcafcu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcafcu),1,0x44u,f)==0x44u);fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x8003f8b0u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8003f8b0u),1,56u,f)==56u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);g_ActiveBattleRuntime=&rt;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image && image->bytes);
 const uint32_t phases[]={0u,0xe3e8feffu};
 for(unsigned mode=0;mode<2;mode++) {
  uint32_t address=0x800523f0u+((phases[mode]<<6)&0xfffu)*4u;
  assert(!runtime_write(&rt,address,2,load_le(image->bytes+XENO_RD_SLUS_OFF(address),2)));
  uint8_t *sprite=PSX_ADDR(0x80160000u);memset(sprite,0x5a,0x60u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=mode?(uint32_t)(uintptr_t)sprite:0x80160000u;cpu.gpr[5]=phases[mode];
  cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x800bd000u;
  PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bcafcu,0x8001f6b0u,200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)colour_prefix(&e);else assert(!"unexpected colour prefix refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected));
 }
 const uint32_t angles[]={0u,0x400u,0xbc740400u};
 for(unsigned which=0;which<2;which++)for(unsigned i=0;i<3;i++) {
  uint32_t target=which?0x8003f8ccu:0x8003f8b0u,ret=which?0x800bcc0cu:0x800bcbe4u;
  uint32_t table=0x800523f0u+(angles[i]&0xfffu)*4u+(which?2u:0u);
  assert(!runtime_write(&rt,table,2,load_le(image->bytes+XENO_RD_SLUS_OFF(table),2)));
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned reg=1;reg<32;reg++)cpu.gpr[reg]=0x31400000u+reg;cpu.gpr[4]=angles[i];cpu.gpr[31]=ret;
  PcPortMipsCpu host=cpu;rt.bridge_cpu=&host;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,target,ret,100)==PC_PORT_MIPS_HALTED);
  assert(battle_popup_trig_try_host(&rt,&host,target)==1);
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(initial,g_PsxRam,sizeof initial));
  host=cpu;rt.bridge_cpu=&host;host.gpr[31]^=4u;assert(!battle_popup_trig_try_host(&rt,&host,target));
  host=cpu;rt.bridge_cpu=&host;uint32_t saved=load_le(PSX_ADDR(table),2);assert(!runtime_write(&rt,table,2,saved^1u));
  assert(!battle_popup_trig_try_host(&rt,&host,target));assert(!runtime_write(&rt,table,2,saved));
 }
 g_ActiveBattleRuntime=NULL;
 puts("popup colour prefix and menu trig PASS fixed retail CPU comparisons; sprite update not executed");return 0;
}
