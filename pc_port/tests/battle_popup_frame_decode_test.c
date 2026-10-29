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
extern void func_800B1720(u8*,u8*,s32,s32);
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800b1720u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b1720u),1,0x780u,f)==0x780u);fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80043c24u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x80043c24u),1,0x28u,f)==0x28u);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);
 ResolvedData *d=find_data(&rt,0x8001c76cu,0x170u);assert(d && d->host==D_8001C76C && d->size==0x170u);
 assert((uintptr_t)D_8001C76C<0x80000000u && (uintptr_t)PSX_ADDR(0x80140020u)<0x80000000u);
 uint8_t original[0x170],expected[0x1c0];memcpy(original,D_8001C76C,sizeof original);
 for(unsigned mode=0;mode<2;mode++) {
  memset(PSX_ADDR(0x80140000u),0xa5,sizeof expected);
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  cpu.gpr[4]=0x8001c778u;cpu.gpr[5]=0x80140020u;cpu.gpr[6]=0;cpu.gpr[7]=mode;
  cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0xfffffffcu;
  int rc=PcPortMipsRun(&cpu,0x800b1720u,0xfffffffcu,20000);
  if(rc!=PC_PORT_MIPS_HALTED)fprintf(stderr,"frame decoder CPU: %s\n",cpu.error);
  assert(rc==PC_PORT_MIPS_HALTED);assert(!memcmp(original,D_8001C76C,sizeof original));
  memcpy(expected,PSX_ADDR(0x80140000u),sizeof expected);memset(PSX_ADDR(0x80140000u),0xa5,sizeof expected);
  func_800B1720(D_8001C76C+12,PSX_ADDR(0x80140020u),0,mode);
  if(memcmp(expected,PSX_ADDR(0x80140000u),sizeof expected)) {fprintf(stderr,"frame decoder mismatch mode=%u\n",mode);return 1;}
  assert(!memcmp(original,D_8001C76C,sizeof original));
 }
 puts("popup frame decoder PASS two retail CPU/native output comparisons, eight original records; no presentation");
}
