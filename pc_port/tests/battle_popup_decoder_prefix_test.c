/* Popup task lookup versus retail CPU. Both sides use PsyCross GTE. */
#include "battle_mips_runtime.c"
#include "psx/gtereg.h"
/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
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
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;return t==0x80043c24u?-1:0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
#include "battle_popup_decoder_prefix_body.inc"
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800b1720u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b1720u),1,0x780u,f)==0x780u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);
 ResolvedData *d=find_data(&rt,0x8001c76cu,0x170u);assert(d && d->host==D_8001C76C && d->size==0x170u);
 uint8_t data[0x170];memcpy(data,D_8001C76C,sizeof data);
 for(unsigned mode=0;mode<2;mode++) {
  memset(PSX_ADDR(0x80140000u),0xa5,0x1c0u);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=0x8001c778u;cpu.gpr[5]=0x80140020u;cpu.gpr[6]=0;cpu.gpr[7]=mode;
  cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x800bd190u;PcPortMipsCpu host=cpu;
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  assert(PcPortMipsRun(&cpu,0x800b1720u,0x800bd190u,20000)==PC_PORT_MIPS_UNRESOLVED_CALL);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();BattleEntryCall e={.runtime=&rt,.cpu=&host};
  if(setjmp(e.failure)==0) {popup_decoder_prefix_test_run(&e);assert(!"expected ShadeTex refusal");}
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"decoder prefix mode %u r%u cpu=%08x host=%08x\n",mode,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)) && !memcmp(data,D_8001C76C,sizeof data));
 }
 puts("popup decoder prefix PASS two retail CPU comparisons; first ShadeTex guarded");
}
