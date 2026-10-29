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
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);uint32_t targets[]={0x8007fce8u,0x8007fdecu};for(unsigned i=0;i<2;i++){assert(!fseek(f,targets[i]-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(targets[i]),1,80,f)==80);}fclose(f);BattleMipsRuntime runtime={0};
for(unsigned k=0;k<4;k++){uint32_t frame=0x80100000u+(k/2)*0x1000u,offset=k%2?0x96u:0xaeu,target=targets[k%2];store_le(PSX_ADDR(0x800d2d28u),4,frame);memset(PSX_ADDR(frame),0xa5,256);store_le(PSX_ADDR(frame+offset),1,0);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=k%2?0x80080a78u:0x80080a70u;PcPortMipsCpu host=cpu;assert(PcPortMipsRun(&cpu,target,cpu.gpr[31],1000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;unsetenv("XENO_BATTLE_NATIVE_INACTIVE_FREE_EXPERIMENT");assert(!battle_inactive_free_try_host(&runtime,&host,target));assert(!setenv("XENO_BATTLE_NATIVE_INACTIVE_FREE_EXPERIMENT","1",1));PcPortMipsCpu before=host;host.gpr[31]^=4;assert(!battle_inactive_free_try_host(&runtime,&host,target));host=before;host.load_valid=1;host.load_reg=4;assert(!battle_inactive_free_try_host(&runtime,&host,target));host=before;host.gpr[29]=0;assert(!battle_inactive_free_try_host(&runtime,&host,target));host=before;uint8_t *code=PSX_ADDR(target);code[1]^=1;assert(!battle_inactive_free_try_host(&runtime,&host,target));code[1]^=1;store_le(PSX_ADDR(frame+offset),1,1);assert(!battle_inactive_free_try_host(&runtime,&host,target));store_le(PSX_ADDR(frame+offset),1,0);store_le(PSX_ADDR(0x800d2d28u),4,0x801feff8u-offset);store_le(PSX_ADDR(0x801feff8u),1,0);assert(!battle_inactive_free_try_host(&runtime,&host,target));store_le(PSX_ADDR(0x800d2d28u),4,frame);memcpy(PSX_ADDR(0x801feff8u),initial+(0x1feff8u),4);assert(battle_inactive_free_try_host(&runtime,&host,target)==1);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));}puts("inactive heap-cleanup gate/body PASS four cases");return 0;}
