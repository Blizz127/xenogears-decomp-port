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
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x8008d598u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x8008d598u),1,0x69c,f)==0x69c);assert(!fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));assert(fread(PSX_ADDR(0x80076a10u),1,92,f)==92);fclose(f);f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));assert(!setenv("XENO_BATTLE_NATIVE_ONE_GLYPH_EXPERIMENT","1",1));
for(unsigned k=0;k<18;k++){uint32_t state=0x80100000u,src=0x80110000u,glyph=0x80120000u,table=0x80140000u,desc=table+0x100u;unsigned slot=k&1,id=slot?10:7,style=k>=14?(k==14?2:k==15?0x2013:k==16?0:0x2001):k>=10?(k==10?0x2013:k==11?0x2001:k==12?0x20a5:0x20f7):k>=8?(k==8?0:0x101):k>=6?(k==6?2:0xfe):k>=4?(k==4?0:0x101):(k&2?0xfe:2);store_le(PSX_ADDR(0x800d2d28u),4,state);store_le(PSX_ADDR(0x800ccb34u),4,slot);store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);store_le(PSX_ADDR(0x800c3000u+4*slot),4,src);store_le(PSX_ADDR(0x800c2f4cu+4*slot),4,glyph);store_le(PSX_ADDR(0x800d2f5cu),4,table);store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);memset(PSX_ADDR(0x80160100u),0,128);if(k==12||k==13||k==15)store_le(PSX_ADDR(0x8016011cu+(style&15u)*2u),2,1);store_le(PSX_ADDR(src),4,slot?0x3101ff01u:0x1400ff00u);store_le(PSX_ADDR(glyph),2,id);store_le(PSX_ADDR(glyph+2),2,0xfffd);store_le(PSX_ADDR(glyph+4),2,0x1234);store_le(PSX_ADDR(glyph+6),2,style);store_le(PSX_ADDR(glyph+8),2,0xffff);store_le(PSX_ADDR(table+4+2*id),2,0x100);memset(PSX_ADDR(desc),0,200);store_le(PSX_ADDR(desc),2,k>=14?7:((k>=6&&k<10)||k>=12)?2:1);for(unsigned n=0;n<7;n++)for(unsigned i=0;i<13;i++)store_le(PSX_ADDR(desc+4+n*28+i*2),2,4+i+n);store_le(PSX_ADDR(desc+30),1,k&1);store_le(PSX_ADDR(desc+31),1,(k>>1)&1);store_le(PSX_ADDR(desc+58),1,(k>>1)&1);store_le(PSX_ADDR(desc+59),1,k&1);memset(PSX_ADDR(0x8013641cu),0xa5,0x1300);memset(PSX_ADDR(state),0x5a,256);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=no_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[5]=0x55550000u+slot;cpu.gpr[6]=0xaabb0000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080758u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,8000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;PcPortMipsCpu before=host;unsetenv("XENO_BATTLE_NATIVE_ONE_GLYPH_EXPERIMENT");assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));assert(!setenv("XENO_BATTLE_NATIVE_ONE_GLYPH_EXPERIMENT","1",1));host.gpr[31]^=4;assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));host=before;host.gpr[29]=state+0x100u;assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));host=before;store_le(PSX_ADDR(desc),2,8);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(runtime_bridge(&runtime,&host,0x8008d598u)==-1);store_le(PSX_ADDR(desc),2,k>=14?7:((k>=6&&k<10)||k>=12)?2:1);store_le(PSX_ADDR(glyph+8u),2,0xfffe);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(glyph+8u),2,0xffff);store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-slot*0xfa0u-slot*40u);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);if((k>=6&&k<10)||k>=12){store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-slot*0xfa0u-slot*40u-80u);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);}if(style>=0x2000u){uint32_t packet=0x8013641cu+slot*0xfa0u+slot*40u;uint32_t shade_offset=((host.gpr[4]&255u)<<6)+0x1cu+((style&15u)<<1);store_le(PSX_ADDR(0x800c3eacu),4,packet-shade_offset);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);}if(k>=14){store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-slot*0xfa0u-slot*40u-480u);assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);}XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);size_t sdk=XENO_RD_SLUS_OFF(0x80043cb0u);image->bytes[sdk]^=1;assert(!one_glyph_preflight(&runtime,&host,0x8008d598u));image->bytes[sdk]^=1;assert(one_glyph_context_bridge(&runtime,&host,0xdeadbeefu)==BRIDGE_HOOKS_NONE);pump_calls=0;mutate_at=0;assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,8000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");assert(host.bus.bridge==runtime_bridge);assert(pump_calls==(k>=16?38u:k>=14?52u:k>=12?17u:k==11?8u:k==10?10u:k>=8?13u:k>=6?17u:k>=4?8u:10u));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));if(k==0){host=before;host.gpr[29]=0x801e5100u;runtime.bridge_cpu=&host;runtime.file1.verified=1;uint64_t generation=runtime.file1.generation;assert(one_glyph_try_host(&runtime,&host,0x8008d598u)==1);assert(host.bus.bridge==runtime_bridge);assert(!runtime.file1.verified&&runtime.file1.generation>generation);const unsigned steps[]={1,2,3,8,9};const uint32_t addresses[]={src,0x801ff000u-120u,0x801ff000u-232u+76u,0x801ff000u-112u+40u,state+0xd0u};for(unsigned z=0;z<5;z++){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=steps[z];mutate_address=addresses[z];uint8_t packet_before[40];memcpy(packet_before,PSX_ADDR(0x8013641cu),40);assert(one_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(host.bus.bridge==runtime_bridge&&one_glyph_active==NULL&&pump_calls==steps[z]);if(steps[z]<=3)assert(!memcmp(packet_before,PSX_ADDR(0x8013641cu),40));}mutate_at=0;}if(k==6){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=8;mutate_address=desc+32u;assert(one_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==8);mutate_at=0;}if(k==10){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=0x80160122u;assert(one_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==1);mutate_at=0;}if(k==14){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=33;mutate_address=desc+4u+6u*28u;assert(one_glyph_try_host(&runtime,&host,0x8008d598u)==-1);assert(one_glyph_active==NULL&&pump_calls==33);mutate_at=0;}}puts("one-glyph native runtime PASS eighteen cases");return 0;}
