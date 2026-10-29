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


static int no_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;return t==0x8008d328u?-1:0;}
static unsigned render_calls;
static uintptr_t fake_render(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;render_calls++;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x8008d598u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008d598u),1,60,f)==60);fclose(f);BattleMipsRuntime runtime={0};
for(unsigned k=0;k<2;k++){memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[6]=k?0x123400ffu:1u;cpu.gpr[29]=0x801ff000u-k*0x100u;cpu.gpr[31]=0x80080758u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,1000)==PC_PORT_MIPS_UNRESOLVED_CALL);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;runtime.functions[0]=(ResolvedFunction){0x8008d328u,(void*)fake_render,"fade renderer"};runtime.function_count=1;render_calls=0;assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));unsetenv("XENO_BATTLE_NATIVE_MENU_FADE_EXPERIMENT");assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));assert(!setenv("XENO_BATTLE_NATIVE_MENU_FADE_EXPERIMENT","1",1));PcPortMipsCpu child=host;child.gpr[31]=0x8008d5d4u;unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(runtime_bridge(&runtime,&child,0x8008d328u)==-1);assert(render_calls==0);PcPortMipsCpu before=host;host.gpr[31]^=4;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));host=before;host.load_valid=1;host.load_reg=4;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));host=before;host.gpr[6]=0x12340000u;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));host=before;host.gpr[29]=0;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));host=before;host.gpr[29]=0x8008d59cu;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));host=before;uint8_t *code=PSX_ADDR(0x8008d598u);code[1]^=1;assert(!battle_menu_fade_try_host(&runtime,&host,0x8008d598u));assert(runtime_bridge(&runtime,&host,0x8008d598u)==-1);code[1]^=1;assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,1000)==PC_PORT_MIPS_UNRESOLVED_CALL);unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(render_calls==0);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));}puts("menu fade runtime gate/refusal PASS two cases");return 0;}
