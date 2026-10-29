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


void *g_TimerWorkList;
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
int main(void) {
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(!fseek(f,0x8001d0a4u-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8001d0a4u),1,0x68u,f)==0x68u);fclose(f);
 BattleMipsRuntime rt={0};rt.initialized=1;rt.data_count=1;
 rt.data[0]=(ResolvedData){0x8005958cu,4u,(uint8_t*)&g_TimerWorkList,"g_TimerWorkList"};
 rt.host_range_count=1;rt.host_ranges[0]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+PSX_RAM_SIZE,1};
 uint32_t owner=(uint32_t)(uintptr_t)PSX_ADDR(0x80140000u);
 uint32_t a=(uint32_t)(uintptr_t)PSX_ADDR(0x80141000u),b=a+0x40u;
 for(unsigned case_no=0;case_no<10;case_no++) {
  unsigned k=case_no%5u; uint32_t return_pc=case_no<5?0x800bce4cu:0x800bce1cu;
  memset(PSX_ADDR(0x80140000u),0,0x2000u);
  store_le(PSX_ADDR(0x80140010u),4,0xa0000007u);
  g_TimerWorkList=k? (void*)(uintptr_t)a : NULL;
  store_le(PSX_ADDR(0x80141000u),4,k==1?owner+0x100u:owner);
  store_le(PSX_ADDR(0x80141008u),4,k==3?0x800bcfacu:0x800bcbb4u);
  store_le(PSX_ADDR(0x80141014u),4,k==2?9u:0xe0000007u);
  store_le(PSX_ADDR(0x80141018u),4,k==4?b:0);
  store_le(PSX_ADDR(0x80141040u),4,owner);
  store_le(PSX_ADDR(0x80141048u),4,0x800bcfacu);
  store_le(PSX_ADDR(0x80141054u),4,7u);
  store_le(PSX_ADDR(0x80141058u),4,0);
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  PcPortMipsCpu cpu;initialize_battle_entry_cpu(&cpu,&rt);cpu.bus.bridge=no_bridge;
  uint32_t entry_gp=cpu.gpr[28];
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[4]=owner;cpu.gpr[5]=0x800bcfacu;cpu.gpr[28]=entry_gp;cpu.gpr[31]=return_pc;
  PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;
  assert(PcPortMipsRun(&cpu,0x8001d0a4u,return_pc,1000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();
  g_ActiveBattleRuntime=&rt;rt.bridge_cpu=&host;
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_LOOKUP_EXPERIMENT","1",1));
  PcPortMipsCpu before=host;host.gpr[28]=0;
  assert(!battle_popup_lookup_try_host(&rt,&host,0x8001d0a4u));host=before;
  host.gpr[5]^=4;assert(!battle_popup_lookup_try_host(&rt,&host,0x8001d0a4u));host=before;
  if(k) {
   PopupLookupSnapshot snap;
   assert(popup_lookup_snapshot(&rt,owner,&snap));
   uint32_t original;assert(!runtime_read(&rt,a+8u,4,&original));
   assert(!runtime_write(&rt,a+8u,4,original^4u));
   assert(!popup_lookup_unchanged(&rt,owner,&snap));
   assert(!runtime_write(&rt,a+8u,4,original));
   void *head=g_TimerWorkList;g_TimerWorkList=(void*)(uintptr_t)0x100000000ull;
   assert(!battle_popup_lookup_try_host(&rt,&host,0x8001d0a4u));g_TimerWorkList=head;
   uint32_t old;assert(!runtime_read(&rt,a+0x18u,4,&old));
   assert(!runtime_write(&rt,a+0x18u,4,a));
   assert(!battle_popup_lookup_try_host(&rt,&host,0x8001d0a4u));
   assert(!runtime_write(&rt,a+0x18u,4,old));
  }
  assert(battle_popup_lookup_try_host(&rt,&host,0x8001d0a4u)==1);
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {
   fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;
  }
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
 }
 /* A synthetic guest callback writes GP to its argument; exercise the real
  * native-work-list callback entry both with and without a live guest parent. */
 store_le(PSX_ADDR(0x800b0000u),4,0xac9c0068u);
 store_le(PSX_ADDR(0x800b0004u),4,0x03e00008u);
 store_le(PSX_ADDR(0x800b0008u),4,0u);
 for(unsigned nested=0;nested<2;nested++) {
  PcPortMipsCpu parent;initialize_cpu(&parent,&rt);parent.gpr[28]=0x80059200u;
  rt.bridge_cpu=nested?&parent:NULL;
  store_le(PSX_ADDR(0x80140068u),4,0u);
  assert(run_guest_callback(&rt,0x800b0000u,PSX_ADDR(0x80140000u))==0);
  assert(load_le(PSX_ADDR(0x80140068u),4)==(nested?parent.gpr[28]:0x80059170u));
  assert(rt.bridge_cpu==(nested?&parent:NULL));
 }
 rt.bridge_cpu=NULL;
 puts("popup lookup PASS ten synthetic CPU comparisons at both owner call sites; bounded list and context guards");
}
