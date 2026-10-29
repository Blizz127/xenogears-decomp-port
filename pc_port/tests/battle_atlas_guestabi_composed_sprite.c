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

static int native_atlas;
static int sdk_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target==0x80025fa8u && native_atlas) {
        BattleMipsRuntime *runtime=opaque;PcPortMipsCpu *previous=runtime->bridge_cpu;runtime->bridge_cpu=cpu;BattleEntryCall e={.runtime=runtime,.cpu=cpu};
        if(setjmp(e.failure)){runtime->bridge_cpu=previous;return -1;}battle_atlas_render_run(&e);runtime->bridge_cpu=previous;return 1;
    }
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
int main(void){
    BattleMipsRuntime runtime={0};PcPortMipsCpu cpu,host;
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x800,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);
    f=fopen("disc/battle.bin","rb");assert(f&&fseek(f,0x6c20,SEEK_SET)==0);assert(fread(PSX_ADDR(0x80076710u),1,0x2d8,f)==0x2d8);fclose(f);
#define SDK(addr,fn) runtime.functions[runtime.function_count++]=(ResolvedFunction){addr,fn,#fn}
    SDK(0x8004960cu,PushMatrix);SDK(0x800496acu,PopMatrix);SDK(0x8004974cu,ScaleMatrixL);SDK(0x80049efcu,SetRotMatrix);SDK(0x80049f8cu,SetTransMatrix);SDK(0x8004a0bcu,ReadGeomOffset);SDK(0x8004a0dcu,ReadGeomScreen);SDK(0x8004a12cu,SetGeomOffset);SDK(0x8004a14cu,SetGeomScreen);SDK(0x8004a73cu,RotTransPers4);SDK(0x8004b18cu,RotMatrixZ);
    store_le(PSX_ADDR(0x800d2d28u),4,0x80100000u);store_le(PSX_ADDR(0x800c3ea4u),4,0x80150000u);store_le(PSX_ADDR(0x800d2f5cu),4,0x80180000u);store_le(PSX_ADDR(0x800ccb34u),4,0);
    store_le(PSX_ADDR(0x801000a9u),1,1);store_le(PSX_ADDR(0x801000abu),1,1);store_le(PSX_ADDR(0x80100034u),4,100);store_le(PSX_ADDR(0x8010003cu),4,16);store_le(PSX_ADDR(0x80100044u),4,28);store_le(PSX_ADDR(0x8010004cu),4,152);store_le(PSX_ADDR(0x80100054u),4,256);store_le(PSX_ADDR(0x8010005cu),4,256);store_le(PSX_ADDR(0x80100104u),2,0x800);
    uint8_t*table=PSX_ADDR(0x80180000u);for(unsigned k=0;k<2;k++){unsigned index=0x52+k,off=0x200+k*0x40;store_le(table+index*2+4,2,off);store_le(table+off,2,1);uint8_t*item=table+off+4;const uint16_t values[]={124,247,116,8,(uint16_t)-56,(uint16_t)-5,4,0,0,128,497,991,247,0};for(unsigned j=0;j<14;j++)store_le(item+2*j,2,values[j]);}
    memcpy(initial,g_PsxRam,sizeof(initial));initial_gte();initialize_cpu(&cpu,&runtime);cpu.bus.bridge=sdk_bridge;cpu.gpr[4]=0;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=BATTLE_HALT_PC;native_atlas=0;
    int rc=PcPortMipsRun(&cpu,0x80076710u,BATTLE_HALT_PC,100000);if(rc!=PC_PORT_MIPS_HALTED){fprintf(stderr,"original sprite failed: %s\n",cpu.error);return 1;}
    memcpy(expected,g_PsxRam,sizeof(expected));GTERegisters want_gte=gteRegs;PcPortMipsCpu want=cpu;
    memcpy(g_PsxRam,initial,sizeof(initial));initial_gte();initialize_cpu(&host,&runtime);host.bus.bridge=sdk_bridge;host.gpr[4]=0;host.gpr[29]=0x801ff000u;host.gpr[31]=BATTLE_HALT_PC;native_atlas=1;runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;BattleEntryCall entry={.runtime=&runtime,.cpu=&host};
    if(setjmp(entry.failure)){fprintf(stderr,"host sprite failed before comparison\n");return 1;}battle_frame_sprite_run(&entry);
    int differences=0;for(unsigned i=1;i<32;i++)if(host.gpr[i]!=want.gpr[i]){fprintf(stderr,"register %u original=%08x host=%08x\n",i,want.gpr[i],host.gpr[i]);differences++;}
    if(host.hi!=want.hi||host.lo!=want.lo){fprintf(stderr,"HI/LO differ\n");differences++;}
    for(unsigned i=0;i<sizeof(expected);i++)if(expected[i]!=g_PsxRam[i]){fprintf(stderr,"RAM %08x original=%02x host=%02x\n",0x80000000u+i,expected[i],g_PsxRam[i]);differences++;break;}
    if(memcmp(&want_gte,&gteRegs,sizeof(gteRegs))){fprintf(stderr,"GTE state differs\n");differences++;}
    if(differences)return 1;puts("76710 + register/stack atlas draft composed fixture PASS (SDK services shared)");return 0;
}
