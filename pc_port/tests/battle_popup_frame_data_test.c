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
extern u32 func_800B16A4(const u8*);
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t ram_before[PSX_RAM_SIZE],data_before[0x170];
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800b16a4u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b16a4u),1,0x4cu,f)==0x4cu);fclose(f);
 BattleMipsRuntime rt={0};initialize_runtime(&rt);
 ResolvedData *binding=find_data(&rt,0x8001c76cu,0x170u);
 assert(binding && binding->address==0x8001c76cu && binding->host==D_8001C76C && binding->size==0x170u);
 assert(find_data(&rt,0x8001c8dcu,1u)!=binding);
 assert(resolve_memory(&rt,0x8001c778u,0x20u,0)==D_8001C76C+12);
 uint32_t off,count;assert(!runtime_read(&rt,0x8001c788u,4,&off));assert(!runtime_read(&rt,0x8001c78cu,4,&count));
 assert(off==28u && count==8u);
 PcPortMipsCpu c;initialize_cpu(&c,&rt);c.bus.bridge=no_bridge;c.gpr[4]=0x8001c778u;c.gpr[31]=0xfffffffcu;
 assert(PcPortMipsRun(&c,0x800b16a4u,0xfffffffcu,1000)==PC_PORT_MIPS_HALTED);
 assert(c.gpr[2]==208u && func_800B16A4(D_8001C76C+12)==c.gpr[2]);
 assert((uintptr_t)D_8001C76C<=UINT32_MAX-0x170u);
 assert(!setenv("XENO_BATTLE_NATIVE_POPUP_FRAME_SIZE_EXPERIMENT","1",1));
 for(unsigned k=0;k<2;k++) {
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=k?(uint32_t)(uintptr_t)(D_8001C76C+12):0x8001c778u;
  cpu.gpr[31]=0x800bd158u;PcPortMipsCpu host=cpu;
  memcpy(ram_before,g_PsxRam,sizeof ram_before);memcpy(data_before,D_8001C76C,sizeof data_before);
  assert(PcPortMipsRun(&cpu,0x800b16a4u,0x800bd158u,1000)==PC_PORT_MIPS_HALTED);
  g_ActiveBattleRuntime=&rt;rt.bridge_cpu=&host;
  PcPortMipsCpu before=host;host.gpr[4]++;
  assert(!battle_popup_frame_size_try_host(&rt,&host,0x800b16a4u));host=before;
  D_8001C76C[0]^=1u;assert(!battle_popup_frame_size_try_host(&rt,&host,0x800b16a4u));D_8001C76C[0]^=1u;
  uint8_t copy[0x170];memcpy(copy,D_8001C76C,sizeof copy);
  BattleEntryCall changed={.runtime=&rt,.cpu=&host};D_8001C76C[40]^=1u;
  if(setjmp(changed.failure)==0) {battle_popup_frame_size_run(&changed,D_8001C76C,copy);assert(!"changed data accepted");}
  D_8001C76C[40]^=1u;host=before;
  assert(battle_popup_frame_size_try_host(&rt,&host,0x800b16a4u)==1);
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"frame size case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(ram_before,g_PsxRam,sizeof ram_before) && !memcmp(data_before,D_8001C76C,sizeof data_before));
 }
 puts("popup frame data PASS runtime binding and two CPU/native frame-size comparisons");
}
