/* Popup task prefix versus retail CPU, with task allocation refused.
 * Both use PsyCross GTE: this is not an independent hardware oracle. */
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


static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t) {
 (void)p;(void)c;return t==0x8001d0a4u ? -1 : 0;
}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 assert(!fseek(f,0x800bcd98u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcd98u),1,0x114u,f)==0x114u);fclose(f);
 BattleMipsRuntime rt={0};
 for(unsigned k=0;k<3;k++) {
  memset(PSX_ADDR(0x800ccb68u),0,44u);
  if(k<2)store_le(PSX_ADDR(0x800ccb68u+(k?4u:12u)),4,0x80140000u);
  uint32_t sp=0x801ff000u-48u;
  for(unsigned i=0;i<7;i++)store_le(PSX_ADDR(sp+16u+4u*i),4,i==6?0x8008067cu:0x44550000u+i);
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[2]=0;cpu.gpr[17]=0x800c3eb0u;cpu.gpr[18]=8u;
  cpu.gpr[19]=0;cpu.gpr[20]=0x800c0000u;cpu.gpr[21]=0x8cb8u;
  cpu.gpr[29]=sp;cpu.gpr[31]=0x800bce4cu;
  PcPortMipsCpu host=cpu;
  int expected_status=k<2?PC_PORT_MIPS_UNRESOLVED_CALL:PC_PORT_MIPS_HALTED;
  assert(PcPortMipsRun(&cpu,0x800bce4cu,0x8008067cu,1000)==expected_status);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();
  BattleEntryCall e={.runtime=&rt,.cpu=&host};
  int failed=setjmp(e.failure);
  if(!failed) { battle_popup_owner_miss_loop(&e); assert(k==2); }
  else assert(k<2);
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
 }
 PcPortMipsCpu guard;initialize_cpu(&guard,&rt);guard.gpr[31]=0x800bce1cu;
 assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_HIGHLIGHT_PREFIX_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_OWNER_PREFIX_EXPERIMENT","1",1));
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_OWNER_LOOP_EXPERIMENT","1",1));
 assert(runtime_bridge_caller_hosts(&rt,&guard,0x8001d0a4u)==-1);
 puts("popup owner miss loop PASS three synthetic CPU comparisons; subsequent lookup refused");
}
