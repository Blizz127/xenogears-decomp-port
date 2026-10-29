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


static int no_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x800,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};const uint32_t targets[]={0x80049efcu,0x80049efcu,0x80049f8cu,0x80049f8cu};const uint32_t returns[]={0x800260bcu,0x800260bcu,0x800260c4u,0x800260c4u};
for(unsigned k=0;k<4;k++){for(unsigned i=0;i<32;i++)((uint8_t*)PSX_ADDR(0x80100000u))[i]=(uint8_t)(i*13u+k*67u);memcpy(initial,g_PsxRam,sizeof(initial));initial_gte();CTC2(0xfffc0000u,24);CTC2(0x01230000u,25);CTC2(0x8001u,26);PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=0x80100000u;cpu.gpr[5]=0x80110000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=returns[k];PcPortMipsCpu host=cpu;if(PcPortMipsRun(&cpu,targets[k],returns[k],1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CPU geometry: %s\n",cpu.error);return 1;}memcpy(expected,g_PsxRam,sizeof(expected));GTERegisters want_gte=gteRegs;memcpy(g_PsxRam,initial,sizeof(initial));initial_gte();CTC2(0xfffc0000u,24);CTC2(0x01230000u,25);CTC2(0x8001u,26);runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;unsetenv("XENO_BATTLE_NATIVE_GTE_MATRIX_EXPERIMENT");assert(!battle_set_matrix_try_host(&runtime,&host,targets[k]));assert(!setenv("XENO_BATTLE_NATIVE_GTE_MATRIX_EXPERIMENT","1",1));PcPortMipsCpu before=host;host.gpr[31]^=4u;assert(!battle_set_matrix_try_host(&runtime,&host,targets[k]));host=before;host.load_valid=1;host.load_reg=4;assert(!battle_set_matrix_try_host(&runtime,&host,targets[k]));host=before;{host.gpr[4]++;assert(!battle_set_matrix_try_host(&runtime,&host,targets[k]));host=before;}XenoRetailFileSlot *img=XenoRetailData_File(XENO_RD_SLUS);assert(img);size_t off=XENO_RD_SLUS_OFF(targets[k]);img->bytes[off+1]^=1;assert(!battle_set_matrix_try_host(&runtime,&host,targets[k]));img->bytes[off+1]^=1;assert(battle_set_matrix_try_host(&runtime,&host,targets[k])==1);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u register %u original=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo);if(memcmp(expected,g_PsxRam,sizeof(expected))){fprintf(stderr,"case %u guest RAM differs\n",k);return 1;}if(memcmp(&want_gte,&gteRegs,sizeof(gteRegs))){fprintf(stderr,"case %u GTE differs\n",k);return 1;}}
puts("GTE matrix setter gate/body PASS four fixed cases");return 0;}
