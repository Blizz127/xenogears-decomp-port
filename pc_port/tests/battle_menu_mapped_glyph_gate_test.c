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
static unsigned pump_calls,mutate_at;static uint32_t mutate_address;
void PcPort_ServiceVblank(void){pump_calls++;if(mutate_at && pump_calls==mutate_at)store_le(PSX_ADDR(mutate_address),1,0x7e);}
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
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x8008d598u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008d598u),1,0x69c,f)==0x69c);assert(!fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x80076a10u),1,92,f)==92);fclose(f);f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_MAPPED_GLYPH_EXPERIMENT","1",1));
for(unsigned k=0;k<8;k++){uint32_t state=0x80100000u,src=0x80110000u,glyph=0x80120000u,table=0x80140000u,desc=table+0x100u,fade=0x80160000u;unsigned slot=k&1,id=slot?10:7,raw=slot?0x4003:0x4000,count=k>=6?7:k/2u,style=k==0?0:k==1?0x2001:k==2?2:k==3?0x2013:k==4?0x101:0x20a5;uint32_t lookup=fade+0x100u+(raw&255u),mapped=fade+0x2c0u+(raw&3u);store_le(PSX_ADDR(0x800d2d28u),4,state);store_le(PSX_ADDR(0x800ccb34u),4,slot);store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);store_le(PSX_ADDR(0x800c3eacu),4,fade);store_le(PSX_ADDR(0x800c3000u+4*slot),4,src);store_le(PSX_ADDR(0x800c2f4cu+4*slot),4,glyph);store_le(PSX_ADDR(0x800d2f5cu),4,table);store_le(PSX_ADDR(src),4,slot?0x3101ff01u:0x1400ff00u);store_le(PSX_ADDR(glyph),2,raw);store_le(PSX_ADDR(glyph+2),2,0xfffd);store_le(PSX_ADDR(glyph+4),2,0x1234);store_le(PSX_ADDR(glyph+6),2,style);store_le(PSX_ADDR(glyph+8),2,0xffff);memset(PSX_ADDR(fade+0x100u),0,128);store_le(PSX_ADDR(lookup),1,id);store_le(PSX_ADDR(mapped),1,0x55);if(k==5)store_le(PSX_ADDR(fade+0x11cu+(style&15u)*2u),2,1);store_le(PSX_ADDR(table+4+2*id),2,0x100);memset(PSX_ADDR(desc),0,200);store_le(PSX_ADDR(desc),2,count);for(unsigned n=0;n<7;n++)for(unsigned i=0;i<13;i++)store_le(PSX_ADDR(desc+4+n*28+i*2),2,4+i+n);store_le(PSX_ADDR(desc+30),1,k&1);store_le(PSX_ADDR(desc+31),1,(k>>1)&1);store_le(PSX_ADDR(desc+58),1,(k>>1)&1);store_le(PSX_ADDR(desc+59),1,k&1);memset(PSX_ADDR(0x8013641cu),0xa5,0x1300);memset(PSX_ADDR(state),0x5a,256);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[5]=0x55550000u+slot;cpu.gpr[6]=0xaabb0000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080758u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,12000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;PcPortMipsCpu before=host;unsetenv("XENO_BATTLE_NATIVE_MAPPED_GLYPH_EXPERIMENT");assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));assert(!setenv("XENO_BATTLE_NATIVE_MAPPED_GLYPH_EXPERIMENT","1",1));host.gpr[31]^=4;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));host=before;host.gpr[29]=state+0x100u;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));host=before;store_le(PSX_ADDR(glyph),2,0xffffu);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));store_le(PSX_ADDR(glyph),2,raw);store_le(PSX_ADDR(lookup),1,255);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));assert(runtime_bridge(&runtime,&host,0x8008d598u)==-1);store_le(PSX_ADDR(lookup),1,id);store_le(PSX_ADDR(desc),2,8);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));store_le(PSX_ADDR(desc),2,count);host.gpr[4]=11;store_le(PSX_ADDR(mapped),1,id);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,2));store_le(PSX_ADDR(mapped),1,0x55);host=before;pump_calls=0;mutate_at=0;assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,12000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(pump_calls==(count==0?3u:count==1?10u:count==7?52u:k==4?13u:17u));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));if(k==0){host=before;host.gpr[29]=0x801e5100u;runtime.bridge_cpu=&host;runtime.file1.verified=1;uint64_t generation=runtime.file1.generation;assert(mapped_glyph_try_host(&runtime,&host,0x8008d598u)==1);assert(!runtime.file1.verified&&runtime.file1.generation>generation);}if(k<2){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=k?mapped:lookup;int rc=mapped_glyph_try_host(&runtime,&host,0x8008d598u);if(rc!=-1){fprintf(stderr,"mutation case %u rc=%d pump=%u addr=%08x\n",k,rc,pump_calls,mutate_address);return 1;}assert(one_glyph_active==NULL&&pump_calls==1);mutate_at=0;}if(k==3){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=fade+0x11cu+(style&15u)*2u;int rc=mapped_glyph_try_host(&runtime,&host,0x8008d598u);if(rc!=-1){fprintf(stderr,"mutation case %u rc=%d pump=%u addr=%08x\n",k,rc,pump_calls,mutate_address);return 1;}assert(one_glyph_active==NULL&&pump_calls==1);mutate_at=0;}if(k==6){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=33;mutate_address=mapped;assert(mapped_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==33);mutate_at=0;}}puts("mapped glyph native runtime PASS eight cases");return 0;}
