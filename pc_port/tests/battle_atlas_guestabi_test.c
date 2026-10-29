/* Actual retail 80025FA8 instructions versus the decomp-owned C body.
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


static uint8_t initial[PSX_RAM_SIZE], expected[PSX_RAM_SIZE];
int main(void){BattleMipsRuntime runtime={0};PcPortMipsCpu cpu,host;FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x800,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);
#define SDK(addr,fn) runtime.functions[runtime.function_count++]=(ResolvedFunction){addr,fn,#fn}
SDK(0x8004960cu,PushMatrix);SDK(0x800496acu,PopMatrix);SDK(0x8004974cu,ScaleMatrixL);SDK(0x80049efcu,SetRotMatrix);SDK(0x80049f8cu,SetTransMatrix);SDK(0x8004a0bcu,ReadGeomOffset);SDK(0x8004a0dcu,ReadGeomScreen);SDK(0x8004a12cu,SetGeomOffset);SDK(0x8004a14cu,SetGeomScreen);SDK(0x8004a73cu,RotTransPers4);SDK(0x8004b18cu,RotMatrixZ);
for(unsigned k=0;k<2;k++){uint8_t*table=PSX_ADDR(0x80180000u),*packets=PSX_ADDR(0x80190008u);memset(table,0,384);unsigned index=0x52+k;store_le(table+index*2+4,2,192);store_le(table+192,2,1);uint8_t*item=table+196;const uint16_t values[]={124,247,116,8,(uint16_t)-56,(uint16_t)-5,4,0,0,128,497,991,247,0};for(unsigned j=0;j<14;j++)store_le(item+2*j,2,values[j]);memset(packets-8,0xa5,256);memcpy(initial,g_PsxRam,sizeof(initial));initial_gte();initialize_cpu(&cpu,&runtime);cpu.bus.bridge=sdk_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=0x80180000u;cpu.gpr[5]=index;cpu.gpr[6]=0x80190008u;cpu.gpr[7]=k;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=BATTLE_HALT_PC;const int args[]={221,34,2048,2048,0};for(unsigned i=0;i<5;i++)store_le(PSX_ADDR(0x801ff010u+i*4),4,args[i]);memcpy(initial,g_PsxRam,sizeof(initial));host=cpu;
if(PcPortMipsRun(&cpu,0x80025fa8u,BATTLE_HALT_PC,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"original failed %s\n",cpu.error);return 1;}memcpy(expected,g_PsxRam,sizeof(expected));GTERegisters want_gte=gteRegs;memcpy(g_PsxRam,initial,sizeof(initial));initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;BattleEntryCall entry={.runtime=&runtime,.cpu=&host};if(setjmp(entry.failure)){fprintf(stderr,"host service failed\n");return 1;}battle_atlas_render_run(&entry);
for(unsigned i=1;i<32;i++)if(host.gpr[i]!=cpu.gpr[i]){fprintf(stderr,"case %u register %u original=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}if(host.hi!=cpu.hi||host.lo!=cpu.lo){fprintf(stderr,"case %u HI/LO differs\n",k);return 1;}for(unsigned i=0;i<sizeof(expected);i++)if(expected[i]!=g_PsxRam[i]){fprintf(stderr,"case %u RAM %08x original=%02x host=%02x\n",k,0x80000000u+i,expected[i],g_PsxRam[i]);return 1;}if(memcmp(&want_gte,&gteRegs,sizeof(gteRegs))){fprintf(stderr,"case %u GTE differs\n",k);return 1;}}
puts("25FA8 register/stack draft PASS (two fixed quads; SDK services shared)");return 0;}
