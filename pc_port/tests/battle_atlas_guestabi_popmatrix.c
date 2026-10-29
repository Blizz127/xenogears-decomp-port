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


extern int matrixLevel;extern MATRIX stack[],*currentMatrix;
static int no_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x800,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};
for(unsigned depth=1;depth<=2;depth++){memset(PSX_ADDR(0x80056d30u),0,640);store_le(PSX_ADDR(0x80056d2cu),4,depth*32);memset(stack,0,20*sizeof(MATRIX));for(unsigned i=0;i<depth*32;i++)((uint8_t*)stack)[i]=(uint8_t)(i*7u+19u);memcpy(PSX_ADDR(0x80056d30u),stack,depth*32);matrixLevel=(int)depth;currentMatrix=stack+depth;memcpy(initial,g_PsxRam,sizeof(initial));initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80026300u;PcPortMipsCpu host=cpu;if(PcPortMipsRun(&cpu,0x800496acu,0x80026300u,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CPU pop: %s\n",cpu.error);return 1;}memcpy(expected,g_PsxRam,sizeof(expected));GTERegisters want_gte=gteRegs;memcpy(g_PsxRam,initial,sizeof(initial));initial_gte();matrixLevel=(int)depth;currentMatrix=stack+depth;runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;unsetenv("XENO_BATTLE_NATIVE_POPMATRIX_EXPERIMENT");assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));assert(!setenv("XENO_BATTLE_NATIVE_POPMATRIX_EXPERIMENT","1",1));PcPortMipsCpu before=host;host.gpr[31]^=4u;assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));host=before;host.load_valid=1;host.load_reg=4;assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));host=before;XenoRetailFileSlot *img=XenoRetailData_File(XENO_RD_SLUS);assert(img);size_t off=XENO_RD_SLUS_OFF(0x800496acu);img->bytes[off+1]^=1;assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));img->bytes[off+1]^=1;matrixLevel=0;currentMatrix=stack;store_le(PSX_ADDR(0x80056d2cu),4,0);assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));matrixLevel=(int)depth;currentMatrix=stack+depth;store_le(PSX_ADDR(0x80056d2cu),4,depth*32);matrixLevel=(int)depth-1;assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));matrixLevel=(int)depth;((uint8_t*)PSX_ADDR(0x80056d30u))[0]^=1;assert(!battle_popmatrix_try_host(&runtime,&host,0x800496acu));((uint8_t*)PSX_ADDR(0x80056d30u))[0]^=1;assert(battle_popmatrix_try_host(&runtime,&host,0x800496acu)==1);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"depth %u register %u original=%08x host=%08x\n",depth,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo);if(memcmp(expected,g_PsxRam,sizeof(expected))){fputs("guest RAM differs\n",stderr);return 1;}if(memcmp(&want_gte,&gteRegs,sizeof(gteRegs))){fputs("GTE differs\n",stderr);return 1;}if(matrixLevel!=(int)depth-1||currentMatrix!=stack+depth-1||memcmp(stack,PSX_ADDR(0x80056d30u),depth*32)){fputs("native matrix stack differs\n",stderr);return 1;}}
puts("PopMatrix gate/body PASS two depths, original CPU guest state and native stack");return 0;}
