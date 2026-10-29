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


static int no_bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x8008d328u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008d328u),1,0x270,f)==0x270);assert(fread(PSX_ADDR(0x8008d598u),1,84,f)==84);assert(!fseek(f,0x8008dc00u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008dc00u),1,52,f)==52);fclose(f);f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};
for(unsigned k=0;k<6;k++){uint32_t state=0x80100000u,prim=0x80110000u,fade=0x80130000u;store_le(PSX_ADDR(0x800d2d28u),4,state);store_le(PSX_ADDR(0x800c3ea4u),4,prim);store_le(PSX_ADDR(0x800c3eacu),4,fade);memset(PSX_ADDR(state),0xa5,256);store_le(PSX_ADDR(state+0xa3),1,k&1);store_le(PSX_ADDR(state+0xd0),4,k>=4?7:k==3?2:1);store_le(PSX_ADDR(state+0xd4),4,k>=4?7:k>=2?1:0);memset(PSX_ADDR(prim+0x641c),k==0||k==4?0:0x85,0x1300);store_le(PSX_ADDR(fade+0x2e0),1,3);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080758u;cpu.gpr[6]=1;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,12000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_NONEMPTY_FADE_EXPERIMENT","1",1));PcPortMipsCpu before=host;host.gpr[31]^=4;assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));host=before;host.gpr[29]=state+0xd4u;assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));host=before;store_le(PSX_ADDR(state+0xd0u),4,8);assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(runtime_bridge(&runtime,&host,0x8008d598u)==-1);store_le(PSX_ADDR(state+0xd0u),4,k>=4?7:k==3?2:1);store_le(PSX_ADDR(state+0xa3u),1,2);assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(state+0xa3u),1,k&1);store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x6420u-(k&1)*40u);assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(0x800c3ea4u),4,prim);uint8_t *helper=PSX_ADDR(0x80043bfcu);helper[0]^=1;assert(!fade_nonempty_try_host(&runtime,&host,0x8008d598u));helper[0]^=1;assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,12000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));if(k==0){host=before;host.gpr[29]=0x801e5100u;runtime.bridge_cpu=&host;runtime.file1.verified=1;uint64_t generation=runtime.file1.generation;assert(fade_nonempty_try_host(&runtime,&host,0x8008d598u)==1);assert(!runtime.file1.verified && runtime.file1.generation>generation);}}puts("nonempty fade runtime PASS six cases");return 0;}
