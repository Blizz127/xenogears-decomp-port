/* Retail battle menu instructions versus the guarded native two-glyph path.
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
static unsigned pump_calls,mutate_at,hook_mode,wrapper_calls;static uint32_t mutate_address;
void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu *cpu,uint32_t target){if(hook_mode&&target==0x80076a10u&&cpu->gpr[31]==0x8008d7b0u&&++wrapper_calls==2)cpu->gpr[16]=0x7fff0000u;}
void PcPort_ServiceVblank(void){pump_calls++;if(mutate_at&&pump_calls==mutate_at)store_le(PSX_ADDR(mutate_address),1,0x7e);}
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


static int glyph_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x8008d598u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008d598u),1,0x69c,f)==0x69c);assert(!fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x80076a10u),1,92,f)==92);fclose(f);f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_TWO_GLYPHS_EXPERIMENT","1",1));
for(unsigned k=0;k<6;k++){uint32_t state=0x80100000u,src=0x80110000u,glyph=0x80120000u,table=0x80140000u;unsigned first_count=k==0?2:k==1?0:k==2?3:k==4?7:k==5?0:1,second_count=k==0?1:k==1?2:k==2?4:k==5?7:0,id=k&1?10:7,id2=k&1?12:9,style=k>=2?(k==2?0:0x101):2;store_le(PSX_ADDR(0x800d2d28u),4,state);store_le(PSX_ADDR(0x800ccb34u),4,k&1);store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);store_le(PSX_ADDR(0x800c3000u+4*(k&1)),4,src);store_le(PSX_ADDR(0x800c2f4cu+4*(k&1)),4,glyph);store_le(PSX_ADDR(0x800d2f5cu),4,table);store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);memset(PSX_ADDR(0x80160100u),0,128);if(k>=6)store_le(PSX_ADDR(0x8016011cu+(style&15u)*2u),2,1);store_le(PSX_ADDR(src),4,k&1?0x3101ff01u:0x1400ff00u);store_le(PSX_ADDR(glyph),2,id);store_le(PSX_ADDR(glyph+2),2,0xfffd);store_le(PSX_ADDR(glyph+4),2,0x1234);store_le(PSX_ADDR(glyph+6),2,style);store_le(PSX_ADDR(glyph+8),2,id2);store_le(PSX_ADDR(glyph+10),2,0x3456);store_le(PSX_ADDR(glyph+12),2,0xfffbu);store_le(PSX_ADDR(glyph+14),2,k&1?0x101:2);store_le(PSX_ADDR(glyph+16),2,0xffffu);store_le(PSX_ADDR(table+4+2*id),2,0x100);memset(PSX_ADDR(table+0x100),0,200);memset(PSX_ADDR(table+0x200),0,200);for(unsigned n=0;n<7;n++)for(unsigned i=0;i<13;i++){store_le(PSX_ADDR(table+0x104+28*n+2*i),2,4+i+n);store_le(PSX_ADDR(table+0x204+28*n+2*i),2,5+i+n);}store_le(PSX_ADDR(table+0x100),2,first_count);store_le(PSX_ADDR(table+4+2*id2),2,0x200);store_le(PSX_ADDR(table+0x200),2,second_count);memset(PSX_ADDR(0x8013641cu),0xa5,0x1300);memset(PSX_ADDR(state),0x5a,256);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=glyph_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[5]=0x55550000u+(k&1);cpu.gpr[6]=0xaabb0000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080758u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,8000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;PcPortMipsCpu before=host;unsetenv("XENO_BATTLE_NATIVE_TWO_GLYPHS_EXPERIMENT");assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));assert(!setenv("XENO_BATTLE_NATIVE_TWO_GLYPHS_EXPERIMENT","1",1));host.gpr[31]^=4;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));host=before;host.load_valid=1;host.load_reg=4;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));host=before;host.gpr[29]=state+0x100u;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));host=before;store_le(PSX_ADDR(glyph+8),2,0xfffeu);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(glyph+8),2,id2);store_le(PSX_ADDR(glyph+6),2,0x4000u);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(glyph+6),2,style);store_le(PSX_ADDR(table+0x100),2,8);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(runtime_bridge(&runtime,&host,0x8008d598u)==-1);store_le(PSX_ADDR(table+0x100),2,first_count);store_le(PSX_ADDR(table+0x200u),2,8);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(table+0x200u),2,second_count);store_le(PSX_ADDR(glyph+16u),2,0x1234u);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(glyph+16u),2,0xffffu);if(k==2){store_le(PSX_ADDR(table+0x200),2,5);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(table+0x200),2,second_count);store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-(k&1)*0xfa0u-(k&1)*40u-480u);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);}if(first_count&&second_count){store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-(k&1)*0xfa0u-(k&1)*40u-80u);assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);}uint8_t *code=PSX_ADDR(0x80076a10u);code[0]^=1;assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,4));code[0]^=1;pump_calls=0;mutate_at=0;assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,8000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(pump_calls==5+5*(first_count+second_count)+2*first_count*((style&255u)>>1!=0)+2*second_count*((((k&1)?0x101u:2u)&255u)>>1!=0));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));if(k==0){host=before;host.gpr[29]=0x801e5100u;runtime.bridge_cpu=&host;runtime.file1.verified=1;uint64_t generation=runtime.file1.generation;assert(two_mixed_glyph_try_host(&runtime,&host,0x8008d598u)==1);assert(!runtime.file1.verified&&runtime.file1.generation>generation);const unsigned steps[]={1,2};const uint32_t addresses[]={src,0x801ff000u-120u};for(unsigned z=0;z<2;z++){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=steps[z];mutate_address=addresses[z];assert(two_mixed_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==steps[z]);}mutate_at=0;}if(k==1){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=3;mutate_address=table+0x200u;assert(two_mixed_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==3);mutate_at=0;}if(k==2){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=20;mutate_address=table+0x200u+32u;assert(two_mixed_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==20);mutate_at=0;memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;hook_mode=1;wrapper_calls=0;assert(two_mixed_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&wrapper_calls==2);hook_mode=0;}}puts("two multi-primitive glyphs native dispatcher PASS six cases");return 0;}
