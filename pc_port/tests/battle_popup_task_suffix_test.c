/* Popup task suffix versus retail CPU. Callback setters are synthetic;
 * no real task is allocated. Both sides use PsyCross GTE. */
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
static int setter(void *opaque,PcPortMipsCpu *cpu,uint32_t target) {
 if(target!=0x8001cd6cu && target!=0x8001cd74u)return 0;
 assert(!runtime_write(opaque,cpu->gpr[4]+(target==0x8001cd6cu?8u:12u),4,cpu->gpr[5]));
 return 1;
}
static void suffix(BattleEntryCall *e) {
 uint32_t *r=e->cpu->gpr;
 r[16]=r[2];r[4]=r[16];r[5]=0x800c0000u;r[5]+=0xffffcbb4u;
 r[31]=0x800bccecu;battle_entry_write(e,r[18]+0x3748u,4,r[16]);
 assert(setter(e->runtime,e->cpu,0x8001cd6cu));
 r[4]=r[16];r[5]=0x800c0000u;r[31]=0x800bccfcu;r[5]+=0xffffcb54u;
 assert(setter(e->runtime,e->cpu,0x8001cd74u));
 r[2]=0x80060000u;r[2]=battle_entry_read(e,r[2]+0xffff91acu,1);r[3]=0x80060000u;
 if(r[2]) {r[2]=battle_entry_read(e,r[3]+0xffff9464u,4);r[2]--;battle_entry_write(e,r[3]+0xffff9464u,4,r[2]);}
 r[3]=0x7fff0000u;r[2]=battle_entry_read(e,r[16]+0x14u,4);r[3]|=0xffffu;r[2]&=r[3];battle_entry_write(e,r[16]+0x14u,4,r[2]);
 r[2]=battle_entry_read(e,r[17]+0xa72u,1);r[3]=0x8c8cu;r[2]<<=2;r[2]+=r[17];r[2]+=r[3];r[3]=battle_entry_read(e,r[2],4);
 battle_entry_write(e,r[16]+0x1cu,4,r[3]);r[2]=battle_entry_read(e,r[17]+0xa72u,1);battle_entry_write(e,r[16]+0x24u,4,r[2]);
 r[2]=battle_entry_read(e,r[3]+0x2bu,1);r[2]&=0xfeu;battle_entry_write(e,r[3]+0x2bu,1,r[2]);
 r[31]=battle_entry_read(e,r[29]+28u,4);r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=32u;
}
int main(void) {
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 assert(!fseek(f,0x800bcc60u-0x8006faf0u,SEEK_SET));
 assert(fread(PSX_ADDR(0x800bcc60u),1,0x12cu,f)==0x12cu);fclose(f);
 BattleMipsRuntime rt={0};
 rt.host_range_count=1;rt.host_ranges[0]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+PSX_RAM_SIZE,1};
 for(unsigned k=0;k<2;k++) {
  uint32_t task=(uint32_t)(uintptr_t)PSX_ADDR(0x80140000u),obj=(uint32_t)(uintptr_t)PSX_ADDR(0x80141000u),sp=0x801ff000u-32u;
  memset(PSX_ADDR(0x80140000u),0x55,0x80);memset(PSX_ADDR(0x80141000u),0x55,0x80);
  store_le(PSX_ADDR(0x800c4922u),1,0);store_le(PSX_ADDR(0x800ccb3cu),4,obj);
  store_le(PSX_ADDR(0x800591acu),1,k);store_le(PSX_ADDR(0x80059464u),4,9);
  for(unsigned i=0;i<4;i++)store_le(PSX_ADDR(sp+16u+i*4u),4,i==3?0x800bcdd4u:0x50200000u+i);
  memcpy(initial,g_PsxRam,sizeof initial);initial_gte();
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);cpu.bus.bridge=setter;
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;
  cpu.gpr[2]=task;cpu.gpr[17]=0x800c3eb0u;cpu.gpr[18]=0x800c0000u;cpu.gpr[29]=sp;cpu.gpr[31]=0x800bccd4u;
  PcPortMipsCpu host=cpu;
  assert(PcPortMipsRun(&cpu,0x800bccd4u,0x800bcdd4u,2000)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;
  memcpy(g_PsxRam,initial,sizeof initial);initial_gte();
  BattleEntryCall e={.runtime=&rt,.cpu=&host};suffix(&e);
  for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]) {fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}
  assert(cpu.hi==host.hi && cpu.lo==host.lo && !memcmp(expected,g_PsxRam,sizeof expected));
  assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
 }
 puts("scratch popup task suffix PASS two CPU comparisons, synthetic setters");
}
