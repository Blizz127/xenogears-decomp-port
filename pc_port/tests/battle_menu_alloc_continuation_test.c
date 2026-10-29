/* Test-only 8007FEC4 continuation versus retail CPU instructions.
 * Zeroing is synthetic; no real allocator or heap is used. Shared GTE. */
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


static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
static int synthetic_zero(void *opaque,PcPortMipsCpu *cpu,uint32_t target) {
 BattleMipsRuntime *rt=opaque;
 if(target!=0x8003f8e8u || cpu->gpr[31]!=0x8007feecu) return 0;
 assert(cpu->gpr[5]==0x5da4u);
 uint8_t *p=resolve_memory(rt,cpu->gpr[4],0x5da4u,1); assert(p);
 memset(p,0,0x5da4u); cpu->gpr[8]=0x42424242u;
 return 1;
}
static void suffix(BattleEntryCall *e) {
 uint32_t *r=e->cpu->gpr;
 r[4]=r[2];r[1]=0x800d0000u;battle_entry_write(e,0x800d2db4u,4,r[4]);
 r[31]=0x8007feecu;r[5]=0x5da4u;
 assert(synthetic_zero(e->runtime,e->cpu,0x8003f8e8u)==1);
 r[3]=0x800d0000u;r[3]=battle_entry_read(e,0x800d2db4u,4);
 r[2]=0xa0u;battle_entry_write(e,r[3]+0x5d9cu,2,r[2]);
 r[2]=0x64u;battle_entry_write(e,r[3]+0x5d9eu,2,r[2]);
 r[31]=battle_entry_read(e,r[29]+16u,4);r[29]+=24u;
}
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 assert(!fseek(f,0x8007fec4u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8007fec4u),1,80,f)==80);fclose(f);
 BattleMipsRuntime rt={0};
 for(unsigned k=0;k<2;k++) {
  uint32_t ptr=0x80140000u+k*0x8000u,sp=0x801ff000u-24u;
  memset(PSX_ADDR(ptr),0x5a,0x5da4u);store_le(PSX_ADDR(sp+16u),4,0x800805acu);
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=synthetic_zero;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[2]=ptr;cpu.gpr[29]=sp;cpu.gpr[31]=0x8007fed8u;PcPortMipsCpu host=cpu;
  assert(PcPortMipsRun(&cpu,0x8007fed8u,0x800805acu,1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();
  BattleEntryCall e={.runtime=&rt,.cpu=&host};suffix(&e);
  for(unsigned i=1;i<32;i++) if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
 }
 puts("scratch menu allocation continuation PASS two cases; synthetic zero only");return 0;
}
