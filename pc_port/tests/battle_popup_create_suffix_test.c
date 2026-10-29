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


extern void func_80021B04(void *,s16,s16,s16);
extern u32 func_800B168C(u32,u32);
extern u32 func_800B16A4(const u8 *);
static int native_side;
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t) {
 (void)p;
 if(t==0x80021b04u) {
  if(!native_side)return 0; /* CPU executes the pinned retail initializer. */
  uint8_t *p=resolve_memory(c->bus.opaque,c->gpr[4],6u,1);assert(p);
  func_80021B04(p,(s16)c->gpr[5],(s16)c->gpr[6],(s16)c->gpr[7]);return 1;
 }
 if(t==0x800b168cu) {if(!native_side)return 0;c->gpr[2]=func_800B168C(c->gpr[4],c->gpr[5]);return 1;}
 if(t==0x800b16a4u) {
  if(!native_side)return 0;
  uint32_t *r=c->gpr,v;uint8_t *data=resolve_memory(c->bus.opaque,r[4],0x80u,0);assert(data);
  u32 native=func_800B16A4(data);
  r[5]=0;r[6]=0;assert(!c->bus.read(c->bus.opaque,r[4]+0x10u,4,&r[2]));
  assert(!c->bus.read(c->bus.opaque,r[4]+0x14u,4,&r[7]));assert(r[7]<=2u);r[4]+=r[2];
  while(r[5]!=r[7]) {
   r[5]++;assert(!c->bus.read(c->bus.opaque,r[4],1,&r[3]));
   assert(!c->bus.read(c->bus.opaque,r[4]+1u,1,&v));
   r[3]=(r[3]+1u)<<2;r[6]+=r[3];r[2]=(v+1u)<<2;r[4]+=r[2];
  }
  r[2]=r[6];assert(r[2]==native);return 1;
 }
 return t==0x80031bdcu ? -1 : 0;
}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
#include "battle_popup_create_suffix_body.inc"
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x800bd098u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bd098u),1,0x164u,f)==0x164u);
 assert(!fseek(f,0x800b168cu-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800b168cu),1,0x18u,f)==0x18u);
 assert(fread(PSX_ADDR(0x800b16a4u),1,0x4cu,f)==0x4cu);fclose(f);
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80021b04u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x80021b04u),1,0x10u,f)==0x10u);fclose(f);
 BattleMipsRuntime rt={0};
 for(unsigned k=0;k<2;k++) {
  store_le(PSX_ADDR(0x800591acu),1,k);store_le(PSX_ADDR(0x80059464u),4,9);
  store_le(PSX_ADDR(0x80140014u),4,0xc1234567u);store_le(PSX_ADDR(0x80141036u),2,0x1234u+k);
  memset(PSX_ADDR(0x8001c778u),0,0x80u);
  store_le(PSX_ADDR(0x8001c788u),4,0x40u);store_le(PSX_ADDR(0x8001c78cu),4,k?2u:0u);
  store_le(PSX_ADDR(0x8001c7b8u),1,2u);store_le(PSX_ADDR(0x8001c7b9u),1,1u);
  store_le(PSX_ADDR(0x8001c7c0u),1,3u);store_le(PSX_ADDR(0x8001c7c1u),1,1u);

  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[2]=0x80140000u;cpu.gpr[21]=0x80141000u;cpu.gpr[29]=0x801ff000u-56u;cpu.gpr[31]=0x800bd0e4u;
  PcPortMipsCpu host=cpu;
  native_side=0;assert(PcPortMipsRun(&cpu,0x800bd0e4u,0x800bce2cu,1000)==PC_PORT_MIPS_UNRESOLVED_CALL);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();BattleEntryCall e={.runtime=&rt,.cpu=&host};
  native_side=1;if(setjmp(e.failure)==0) {popup_create_suffix_test_run(&e);assert(!"expected guarded frame buffer allocator");}
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
 }
 puts("popup create suffix PASS two synthetic CPU comparisons; retail/native initializer, record and size; synthetic frame data; frame buffer allocator refused");
}
