/* Actual retail 80025FA8 instructions versus the decomp-owned C body.
 * Both use PsyCross GTE: this is not an independent hardware oracle. */
#include "battle_mips_runtime.c"
/* Test-only SDK dispatch uses existing native GPU bodies; it is not production routing. */
static uint32_t x_trace[2][16][32];static unsigned x_calls[2];
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


static int no_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if((t==0x80043cb0u||t==0x80043bfcu||t==0x80043c24u||t==0x80043a1cu||t==0x80043a58u)&&x_calls[0]<16)memcpy(x_trace[0][x_calls[0]++],c->gpr,sizeof(c->gpr));return 0;}
static int native_test_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target){
 if(target==0x80043cb0u||target==0x80043bfcu||target==0x80043c24u||target==0x80043a1cu||target==0x80043a58u){
  assert(x_calls[1]<16);memcpy(x_trace[1][x_calls[1]++],cpu->gpr,sizeof(cpu->gpr));BattleEntryCall entry={.runtime=opaque,.cpu=cpu};if(setjmp(entry.failure))return -1;battle_atlas_gpu_run(&entry,target);return 1;
 }
 return runtime_bridge(opaque,cpu,target);
}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x80076a10u),1,92,f)==92);fclose(f);f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};
for(unsigned k=0;k<6;k++){x_calls[0]=x_calls[1]=0;uint32_t table=0x80140000u,desc=table+0x100u,output=0x80130000u,id=k&1?10:7;store_le(PSX_ADDR(0x800d2f5cu),4,table);store_le(PSX_ADDR(0x800ccb34u),4,k&1);store_le(PSX_ADDR(table+4+id*2),2,0x100);memset(PSX_ADDR(desc),0,64);store_le(PSX_ADDR(desc),2,k/2u);for(unsigned i=0;i<13;i++){store_le(PSX_ADDR(desc+4+i*2),2,4+i);store_le(PSX_ADDR(desc+32+i*2),2,7+i);}store_le(PSX_ADDR(desc+4),2,k&1?0:4);store_le(PSX_ADDR(desc+6),2,k&2?0:5);store_le(PSX_ADDR(desc+30),1,k&1);store_le(PSX_ADDR(desc+31),1,(k>>1)&1);store_le(PSX_ADDR(desc+58),1,(k>>1)&1);store_le(PSX_ADDR(desc+59),1,k&1);memset(PSX_ADDR(output),0xa5,192);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=id;cpu.gpr[5]=output;cpu.gpr[6]=0xfffffffdu;cpu.gpr[7]=0x1234u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x8008d7b0u;PcPortMipsCpu host=cpu;host.bus.bridge=native_test_bridge;assert(PcPortMipsRun(&cpu,0x80076a10u,0x8008d7b0u,5000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x80076a10u,0x8008d7b0u,5000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");unsigned mismatches=0;for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);mismatches++;}if(cpu.hi!=host.hi||cpu.lo!=host.lo)fprintf(stderr,"case %u HI/LO mismatch\n",k);if(memcmp(expected,g_PsxRam,sizeof expected))fprintf(stderr,"case %u RAM mismatch\n",k);if(memcmp(&want,&gteRegs,sizeof(gteRegs)))fprintf(stderr,"case %u GTE mismatch\n",k);fprintf(stderr,"case %u: %u final register mismatches, service calls %u/%u\n",k,mismatches,x_calls[0],x_calls[1]);assert(x_calls[0]==x_calls[1]);for(unsigned q=0;q<x_calls[0];q++)for(unsigned r=1;r<32;r++)if(x_trace[0][q][r]!=x_trace[1][q][r]){fprintf(stderr,"call %u r%u cpu=%08x host=%08x\n",q,r,x_trace[0][q][r],x_trace[1][q][r]);return 1;}assert(!mismatches);assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected)&&!memcmp(&want,&gteRegs,sizeof(gteRegs)));}return 0;}
