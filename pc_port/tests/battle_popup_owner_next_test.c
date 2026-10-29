/* Test-only owner-slot read versus retail CPU.
 * The task lookup and callbacks are not executed. */
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
static uint8_t initial[PSX_RAM_SIZE];
static void owner_next(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[2]=r[17]+r[21];
 r[16]=battle_entry_read(e,r[2],4);r[4]=r[16];
}
int main(void)
{
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800bcdfcu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcdfcu),1,0x70u,f)==0x70u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);
 for(unsigned mode=0;mode<2;mode++) {
  uint8_t *table=PSX_ADDR(0x80160000u);store_le(table,4,mode?0u:0x80161000u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[17]=mode?(uint32_t)(uintptr_t)table-0x100u:0x8015ff00u;
  cpu.gpr[21]=0x100u;cpu.gpr[2]=mode?1u:0u;
  PcPortMipsCpu host=cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PcPortMipsRun(&cpu,0x800bcdfcu,mode?0x800bce68u:0x800bce44u,200)==PC_PORT_MIPS_HALTED);
  BattleEntryCall e={.runtime=&rt,.cpu=&host};if(setjmp(e.failure)==0)owner_next(&e);else assert(!"unexpected owner-next refusal");
  assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(initial,g_PsxRam,sizeof initial));
 }
 puts("popup next-owner read PASS two retail CPU comparisons; lookup not executed");return 0;
}
