/* Retail battle menu instructions versus guarded native faded glyphs.
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
int main(void){FILE*f=fopen("disc/battle.bin","rb");
assert(f);
assert(!fseek(f,0x8008d598u-0x8006faf0u,SEEK_SET));
assert(fread(PSX_ADDR(0x8008d598u),1,0x69c,f)==0x69c);
assert(!fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));
assert(fread(PSX_ADDR(0x80076a10u),1,92,f)==92);fclose(f);f=fopen("disc/SLUS_006.64","rb");
assert(f&&!fseek(f,0x800,SEEK_SET));
assert(fread(PSX_ADDR(0x80010000u),1,0x476e4,f)==0x476e4);fclose(f);BattleMipsRuntime runtime={0};
assert(!setenv("XENO_BATTLE_NATIVE_HOSTS","1",1));
assert(!setenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT","1",1));
assert(!setenv("XENO_BATTLE_NATIVE_MANY_GLYPHS_EXPERIMENT","1",1));
for(unsigned k=0;k<6;k++){uint32_t state=0x80100000u,src=0x80110000u,glyph=0x80120000u,table=0x80140000u;unsigned first_count=k==0?50:(k==2||k==5)?7:k>=4?1:0,second_count=k==1?1:(k==2||k==5)?4:k>=4?1:0,third_count=(k==2||k==5)?3:k>=4?1:0,seventh_count=(k==3||k==4),id7=23,style7=0x201cu,sixth_count=(k==3||k==4),id6=21,style6=0x201eu,fifth_count=(k==3||k==4),id5=19,style5=0x201fu,fourth_count=(k==2||k==5)?2:(k==3||k==4),id4=17,style4=0x201du,id3=15,id=k&1?10:7,id2=k&1?12:9,style=k>=4?2u:0x2013u,style2=k==5?2u:0x2007u,style3=k==4?2u:0x201bu;store_le(PSX_ADDR(0x800d2d28u),4,state);store_le(PSX_ADDR(0x800ccb34u),4,k&1);store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);store_le(PSX_ADDR(0x800c3000u+4*(k&1)),4,src);store_le(PSX_ADDR(0x800c2f4cu+4*(k&1)),4,glyph);store_le(PSX_ADDR(0x800d2f5cu),4,table);store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);memset(PSX_ADDR(0x80160100u),0,128);if(k&1)store_le(PSX_ADDR(0x8016011cu+(style2&15u)*2u),2,1);if(k&2)store_le(PSX_ADDR(0x8016011cu+(style3&15u)*2u),2,1);store_le(PSX_ADDR(src),4,k&1?0x3101ff01u:0x1400ff00u);store_le(PSX_ADDR(glyph),2,id);store_le(PSX_ADDR(glyph+2),2,0xfffd);store_le(PSX_ADDR(glyph+4),2,0x1234);store_le(PSX_ADDR(glyph+6),2,style);store_le(PSX_ADDR(glyph+8),2,id2);store_le(PSX_ADDR(glyph+10),2,0x3456);store_le(PSX_ADDR(glyph+12),2,0xfffbu);store_le(PSX_ADDR(glyph+14),2,style2);store_le(PSX_ADDR(glyph+16),2,id3);store_le(PSX_ADDR(glyph+18),2,0xfffau);store_le(PSX_ADDR(glyph+20),2,3);store_le(PSX_ADDR(glyph+22),2,style3);store_le(PSX_ADDR(glyph+24),2,id4);store_le(PSX_ADDR(glyph+26),2,2);store_le(PSX_ADDR(glyph+28),2,3);store_le(PSX_ADDR(glyph+30),2,style4);store_le(PSX_ADDR(glyph+32),2,id5);store_le(PSX_ADDR(glyph+34),2,2);store_le(PSX_ADDR(glyph+36),2,3);store_le(PSX_ADDR(glyph+38),2,style5);store_le(PSX_ADDR(glyph+40),2,id6);store_le(PSX_ADDR(glyph+42),2,2);store_le(PSX_ADDR(glyph+44),2,3);store_le(PSX_ADDR(glyph+46),2,style6);store_le(PSX_ADDR(glyph+48),2,id7);store_le(PSX_ADDR(glyph+50),2,2);store_le(PSX_ADDR(glyph+52),2,3);store_le(PSX_ADDR(glyph+54),2,style7);store_le(PSX_ADDR(table+4+2*id),2,0x100);memset(PSX_ADDR(table+0x100),0,256);memset(PSX_ADDR(table+0x200),0,256);memset(PSX_ADDR(table+0x300),0,256);memset(PSX_ADDR(table+0x400),0,256);memset(PSX_ADDR(table+0x500),0,256);memset(PSX_ADDR(table+0x600),0,256);memset(PSX_ADDR(table+0x700),0,256);for(unsigned n=0;n<9;n++)for(unsigned i=0;i<13;i++){store_le(PSX_ADDR(table+0x104+28*n+2*i),2,4+i+n);store_le(PSX_ADDR(table+0x204+28*n+2*i),2,5+i+n);store_le(PSX_ADDR(table+0x304+28*n+2*i),2,6+i+n);store_le(PSX_ADDR(table+0x404+28*n+2*i),2,7+i+n);store_le(PSX_ADDR(table+0x504+28*n+2*i),2,8+i+n);store_le(PSX_ADDR(table+0x604+28*n+2*i),2,9+i+n);store_le(PSX_ADDR(table+0x704+28*n+2*i),2,10+i+n);}store_le(PSX_ADDR(table+0x100),2,first_count);store_le(PSX_ADDR(table+4+2*id2),2,0x200);store_le(PSX_ADDR(table+0x200),2,second_count);store_le(PSX_ADDR(table+4+2*id3),2,0x300);store_le(PSX_ADDR(table+0x300),2,third_count);store_le(PSX_ADDR(table+4+2*id4),2,0x400);store_le(PSX_ADDR(table+0x400),2,fourth_count);store_le(PSX_ADDR(table+4+2*id5),2,0x500);store_le(PSX_ADDR(table+0x500),2,fifth_count);store_le(PSX_ADDR(table+4+2*id6),2,0x600);store_le(PSX_ADDR(table+0x600),2,sixth_count);store_le(PSX_ADDR(table+4+2*id7),2,0x700);store_le(PSX_ADDR(table+0x700),2,seventh_count);memset(PSX_ADDR(0x8013641cu),0xa5,0x1300);if(k==0){uint32_t d=table+0x8000u;memset(PSX_ADDR(d),0,1404);for(unsigned n=0;n<50;n++)for(unsigned i=0;i<13;i++)store_le(PSX_ADDR(d+4u+n*28u+2u*i),2,4u+i+n);store_le(PSX_ADDR(d),2,first_count);store_le(PSX_ADDR(table+4u+2u*id),2,0x8000u);}unsigned ng=k==1?21u:k==2?25u:k==4?17u:(k&1?32u:17u),extra_sprites=0;for(unsigned j=0;j<ng-7u;j++){uint32_t d=table+0x800u+j*0x100u;unsigned idextra=24u+j,count=(k==3&&j<2)?1u:(k==5&&j<17)?2u:0u;extra_sprites+=count;memset(PSX_ADDR(d),0,60);for(unsigned n=0;n<2;n++)for(unsigned i=0;i<13;i++)store_le(PSX_ADDR(d+4u+28u*n+2u*i),2,10u+i+j+n);store_le(PSX_ADDR(d),2,count);store_le(PSX_ADDR(table+4u+2u*idextra),2,0x800u+j*0x100u);store_le(PSX_ADDR(glyph+56u+j*8u),2,idextra);store_le(PSX_ADDR(glyph+58u+j*8u),2,2);store_le(PSX_ADDR(glyph+60u+j*8u),2,3);store_le(PSX_ADDR(glyph+62u+j*8u),2,j==24?0x2018u:0x201cu);}store_le(PSX_ADDR(glyph+ng*8u),2,0xffffu);memset(PSX_ADDR(state),0x5a,256);unsigned masks[]={1,2,5,10,4096u,4294967295u},mask=masks[k],first=0;while(!(mask&(1u<<first)))first++;uint32_t ids[]={id,id2,id3,id4,id5,id6,id7,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48},raw=0x4080u+first,mapped=ids[first];for(unsigned i=0;i<ng;i++)if(mask&(1u<<i)){store_le(PSX_ADDR(0x80160180u+i),1,ids[i]);store_le(PSX_ADDR(glyph+8u*i),2,0x4080u+i);}memcpy(initial,g_PsxRam,sizeof initial);initial_gte();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=glyph_bridge;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[5]=0x55550000u+(k&1);cpu.gpr[6]=0xaabb0000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080758u;PcPortMipsCpu host=cpu;host.bus.bridge=runtime_bridge;
assert(PcPortMipsRun(&cpu,0x8008d598u,0x80080758u,100000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);initial_gte();runtime.initialized=1;g_ActiveBattleRuntime=&runtime;runtime.bridge_cpu=&host;PcPortMipsCpu before=host;
assert(one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x80160100u+(raw&255u)),1,255);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x80160100u+(raw&255u)),1,mapped);if(ng==32){store_le(PSX_ADDR(glyph+256u),2,49);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(glyph+256u),2,0xffffu);}if(k==2){store_le(PSX_ADDR(table+0x100u),2,42);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(table+0x100u),2,first_count);}if(k==5){store_le(PSX_ADDR(0x800c3ea4u),4,state+0xd0u-0x641cu-(k&1)*0xfa0u-(k&1)*40u-3920u);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x800c3ea4u),4,0x80130000u);}if(k==0){store_le(PSX_ADDR(table+0x8000u),2,51);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(table+0x8000u),2,first_count);}uint32_t early=load_le(PSX_ADDR(glyph+56u),2);store_le(PSX_ADDR(glyph+56u),2,0xffffu);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(glyph+56u),2,early);if(k==0){for(unsigned i=0;i<5;i++){store_le(PSX_ADDR(glyph+8u*i),2,0x4080u+i);store_le(PSX_ADDR(0x80160180u+i),1,ids[i]);}
assert(one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));memcpy(g_PsxRam,initial,sizeof initial);}uint32_t packet=0x8013641cu+(k&1)*0xfa0u+(k&1)*40u;store_le(PSX_ADDR(0x800c3eacu),4,packet-0x100u-0x1cu-(style5&15u)*2u);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x800c3eacu),4,packet-0x100u-0x1cu-(style6&15u)*2u);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x800c3eacu),4,packet-0x100u-0x1cu-(style7&15u)*2u);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);uint32_t mapbase=state+0xd0u-0x2c0u-(raw&3u);store_le(PSX_ADDR(0x800c3eacu),4,mapbase);store_le(PSX_ADDR(mapbase+0x100u+(raw&255u)),1,mapped);
assert(!one_glyph_preflight_capture(&runtime,&host,0x8008d598u,NULL,22));store_le(PSX_ADDR(0x800c3eacu),4,0x80160000u);*(uint8_t *)PSX_ADDR(mapbase+0x100u+(raw&255u))=initial[(uint8_t *)PSX_ADDR(mapbase+0x100u+(raw&255u))-g_PsxRam];pump_calls=0;mutate_at=0;
assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));
assert(PcPortMipsRun(&host,0x8008d598u,0x80080758u,100000)==PC_PORT_MIPS_HALTED);unsetenv("XENO_DISABLE_CPU_INTERPRETER");
assert(pump_calls==(15+2*(ng-7))+5*(first_count+second_count+third_count+fourth_count+fifth_count+sixth_count+seventh_count+extra_sprites)+2*first_count+2*second_count*(style2==2u||(k&1)!=0)+2*third_count+2*fourth_count+2*fifth_count+2*sixth_count+2*seventh_count+2*extra_sprites);for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}
assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));
assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));if(k==0){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=248;mutate_address=table+0x8000u+4u+49u*28u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==248);mutate_at=0;}if(k==3){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=105;mutate_address=table+0x2000u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==105);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=0x8016012cu;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==1);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=34;mutate_address=table+0x700u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==34);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=0x8016011cu+(style7&15u)*2u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==1);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=25;mutate_address=table+0x600u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==25);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=0x8016011cu+(style6&15u)*2u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==1);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=16;mutate_address=table+0x500u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==16);memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=1;mutate_address=0x8016011cu+(style5&15u)*2u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==1);mutate_at=0;}if(k==5){for(unsigned z=0;z<4;z++){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=z==2?19:z==3?413:1;mutate_address=z==0?0x8016019fu:0x801602c3u;
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==mutate_at);}mutate_at=0;}if(k<4){const unsigned when[]={1,3,1,3};for(unsigned z=0;z<2;z++){memcpy(g_PsxRam,initial,sizeof initial);host=before;runtime.bridge_cpu=&host;pump_calls=0;mutate_at=z?when[k]:1;mutate_address=z?0x801602c0u+(raw&3u):0x80160100u+(raw&255u);
assert(many_glyph_try_host(&runtime,&host,0x8008d598u)==-1);
assert(one_glyph_active==NULL&&pump_calls==mutate_at);}mutate_at=0;}}puts("up to 50 primitives PASS six cases");return 0;}
