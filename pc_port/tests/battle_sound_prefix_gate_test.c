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


static int no_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;return t==0x800404b4u?-1:0;}
static unsigned event_calls;
static uintptr_t fake_event(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f,uintptr_t g,uintptr_t h,uintptr_t i,uintptr_t j,uintptr_t k,uintptr_t l){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;event_calls++;return 1;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x800,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};
for(unsigned k=0;k<3;k++){store_le(PSX_ADDR(0x80059440u),4,0x80100000u);store_le(PSX_ADDR(0x800595d8u),4,0x80120000u);store_le(PSX_ADDR(0x80120014u),1,16);store_le(PSX_ADDR(0x80059558u),4,k?0x80140000u:0);store_le(PSX_ADDR(0x80140020u),2,0x11);store_le(PSX_ADDR(0x8014002cu),4,0);store_le(PSX_ADDR(0x800595bcu),4,0x12345678u);store_le(PSX_ADDR(0x80059404u),4,2);store_le(PSX_ADDR(0x80100014u),2,0x33);store_le(PSX_ADDR(0x80100016u),2,0x11);store_le(PSX_ADDR(0x80100018u),2,0x80);store_le(PSX_ADDR(0x80100082u),1,k?255:64);store_le(PSX_ADDR(0x8010001cu),4,0);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=0xffff8003u;cpu.gpr[5]=0x00330002u;cpu.gpr[6]=k==2?0xffff8000u:0x6000u;cpu.gpr[7]=0x4000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80039e08u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8003b644u,0x80039e08u,1000)==PC_PORT_MIPS_UNRESOLVED_CALL);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;runtime.functions[0]=(ResolvedFunction){0x800404b4u,(void*)fake_event,"DisableEvent"};runtime.function_count=1;event_calls=0;assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_MENU_SOUND_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_SOUND_ENABLED_EXPERIMENT","1",1));unsetenv("XENO_BATTLE_NATIVE_SOUND_MATCH_EXPERIMENT");assert(!setenv("XENO_BATTLE_NATIVE_WDS_LOOKUP_EXPERIMENT","1",1));unsetenv("XENO_BATTLE_NATIVE_SOUND_PREFIX_EXPERIMENT");assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));assert(!setenv("XENO_BATTLE_NATIVE_SOUND_PREFIX_EXPERIMENT","1",1));PcPortMipsCpu event_cpu=host;event_cpu.gpr[31]=0x8003b76cu;unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(runtime_bridge(&runtime,&event_cpu,0x800404b4u)==-1);assert(event_calls==0);PcPortMipsCpu before=host;host.gpr[31]^=4;assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));host=before;host.load_valid=1;host.load_reg=4;assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));host=before;host.gpr[29]=0x80100038u;assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));host=before;XenoRetailFileSlot *img=XenoRetailData_File(XENO_RD_SLUS);assert(img);size_t off=XENO_RD_SLUS_OFF(0x8003b644u);img->bytes[off+1]^=1;assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));img->bytes[off+1]^=1;store_le(PSX_ADDR(0x80100014u),2,0x11);assert(!battle_sound_prefix_try_host(&runtime,&host,0x8003b644u));store_le(PSX_ADDR(0x80100014u),2,0x33);host.gpr[31]=0x8003b6c8u;host.gpr[4]=0x11;assert(battle_wds_lookup_try_host(&runtime,&host,0x800383ecu)==1);host=before;unsetenv("XENO_BATTLE_NATIVE_WDS_LOOKUP_EXPERIMENT");assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(battle_sound_prefix_try_host(&runtime,&host,0x8003b644u)==-1);assert(host.gpr[31]==0x8003b6c8u);unsetenv("XENO_DISABLE_CPU_INTERPRETER");memcpy(g_PsxRam,initial,sizeof initial);host=before;assert(!setenv("XENO_BATTLE_NATIVE_WDS_LOOKUP_EXPERIMENT","1",1));assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(battle_sound_prefix_try_host(&runtime,&host,0x8003b644u)==-1);unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(event_calls==0);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));}puts("sound event prefix gate/body PASS three cases, native WDS and event refusal");return 0;}
