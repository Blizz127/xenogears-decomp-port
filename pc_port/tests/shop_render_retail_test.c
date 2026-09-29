/* Compare the complete draw-call trace with execution of the disc MIPS. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#define ENTRY 0x801CCFF4u
#define END 0x801CD404u
#define BASE 0x80000000u
#define MENU 0x80070000u
#define SHOP 0x80080000u
#define MANAGER 0x80090000u
#define GFX 0x800A0000u
#define HALT 0xFFFFFFFCu
static u8 ram[0x200000], code[END-ENTRY];
static SystemMenu menu;
static MenuShop shop;
static MenuManager manager;
static GfxEnvironment gfx;
SystemMenu* g_Menu=&menu;
static unsigned cases, checks, seen[(END-ENTRY)/4];
static PcPortMipsCpu* active_cpu;
extern void func_801CCFF4(void);
static void require(int ok,const char* why) {
 ++checks; if(!ok) {fprintf(stderr,"SHOP RENDER FAIL case=%u %s\n",cases,why);exit(1);}
}
static u8* ptr(uint32_t a) { return ram+(a-BASE); }
static void put(uint32_t a, uint32_t v) { memcpy(ptr(a),&v,4); }
static int read_bus(void* o,uint32_t a,unsigned w,uint32_t* v) {
 (void)o; if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 if (active_cpu && w == 4 && a == active_cpu->pc && a >= ENTRY && a < END) ++seen[(a-ENTRY)/4];
 *v=0; for(unsigned i=0;i<w;i++) *v|=(uint32_t)ptr(a)[i]<<(8*i); return 0;
}
static int write_bus(void* o,uint32_t a,unsigned w,uint32_t v) {
 (void)o; if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 for(unsigned i=0;i<w;i++) { ptr(a)[i]=(u8)(v>>(8*i)); } return 0;
}
typedef struct {uint32_t kind,a,b,c,d;} Event;
static Event events[128], expected[128];
static unsigned count;
static void emit(uint32_t kind,uint32_t a,uint32_t b,uint32_t c,uint32_t d) {
 require(count<128,"event bound");events[count++]=(Event){kind,a,b,c,d};
}
typedef struct {const u8* p; unsigned size; uint32_t guest;} Mapping;
static Mapping maps[128];static unsigned nmap;
static void map(const void* p,unsigned size,uint32_t guest) {maps[nmap++]=(Mapping){p,size,guest};}
static uint32_t guest(const void* p) {
 uintptr_t a=(uintptr_t)p;
 for(unsigned i=0;i<nmap;i++) if(a>=(uintptr_t)maps[i].p && a<(uintptr_t)maps[i].p+maps[i].size)
  return maps[i].guest+(uint32_t)(a-(uintptr_t)maps[i].p);
 require(0,"unmapped native pointer");return 0;
}
void AddPrim(void* ot,void* p) {emit(1,guest(ot),guest(p),0,0);}
void ShopMenuRenderString(int length,POLY_FT4* p,int ctx) {emit(2,length,guest(p),ctx,0);}
void ShopMenuRenderPolygons(int n,SVECTOR* vertices,POLY_FT4* p,int ctx) {emit(3,n,guest(vertices),guest(p),ctx);}
static int bridge(void* o,PcPortMipsCpu* c,uint32_t target) {
 (void)o;uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6],e=c->gpr[7];
 if(target==0x80043B48u) emit(1,a,b,0,0);
 else if(target==0x801C8D58u) emit(2,a,b,d,0);
 else if(target==0x801C8C3Cu) emit(3,a,b,d,e);
 else return 0;
 return 1;
}
static void set_byte(u8* p,unsigned offset,unsigned v) {*p=(u8)v;ptr(SHOP+offset)[0]=(u8)v;}
static void setup(unsigned pattern,unsigned ctx) {
 memset(&shop,0,sizeof(shop));memset(ram,0,sizeof(ram));memcpy(ptr(ENTRY),code,sizeof(code));
 menu.pShop=&shop;menu.pManager=&manager;menu.pGfxEnv=&gfx;menu.renderContext=ctx;
 put(0x800625A0u,MENU);put(MENU+0x450,SHOP);put(MENU+0x33C,MANAGER);put(MENU+0x1D4,GFX);put(MENU+0x308,ctx);
 nmap=0;map(&gfx.ot[4],sizeof(gfx.ot[4]),GFX+0x80);
 map(shop.polysCharacterPortraits,sizeof(shop.polysCharacterPortraits),SHOP+0x0);
 map(shop.polys2D0,sizeof(shop.polys2D0),SHOP+0x2d0);
 map(shop.polysExplanations,sizeof(shop.polysExplanations),SHOP+0x5a0);
 map(shop.polysGoldBefore,sizeof(shop.polysGoldBefore),SHOP+0xc80);
 map(shop.polysTotalPrice,sizeof(shop.polysTotalPrice),SHOP+0xf50);
 map(shop.polysFinalPrice,sizeof(shop.polysFinalPrice),SHOP+0x1220);
 map(shop.polysGoldAfter,sizeof(shop.polysGoldAfter),SHOP+0x1ae0);
 map(shop.unk1DB0,sizeof(shop.unk1DB0),SHOP+0x1db0);
 map(shop.polys27B0,sizeof(shop.polys27B0),SHOP+0x27b0);
 map(shop.polys3020,sizeof(shop.polys3020),SHOP+0x3020);
 map(shop.linesPortraitHighlight1,sizeof(shop.linesPortraitHighlight1),SHOP+0x3890);
 map(shop.linesPortraitHighlight2,sizeof(shop.linesPortraitHighlight2),SHOP+0x3a40);
 map(shop.lines3BF0,sizeof(shop.lines3BF0),SHOP+0x3bf0);
 for(unsigned i=0;i<8;i++) {
  map(shop.strings3C30[i].polys,0x70,SHOP+0x3c30+0x80*i);
  map(shop.strings4030[i].polys,0x70,SHOP+0x4030+0x80*i);
  set_byte(&shop.strings3C30[i].renderContext,0x3cad+0x80*i,(i+ctx)%2);
  set_byte(&shop.strings4030[i].renderContext,0x40ad+0x80*i,(i+ctx+1)%2);
 }
 map(shop.strItemDesc.polys,0x70,SHOP+0x4430);set_byte(&shop.strItemDesc.renderContext,0x44ad,ctx);
 map(shop.str44B0.polys,0x70,SHOP+0x44b0);set_byte(&shop.str44B0.renderContext,0x452d,ctx);
 map(shop.str45B0.polys,0x70,SHOP+0x45b0);set_byte(&shop.str45B0.renderContext,0x462d,ctx);
 set_byte(&shop.numPortraits,0x46a5,(pattern+0)%10);
 set_byte(&shop.portraitsRenderCtx,0x46a6,(pattern+1)%2);
 set_byte(&shop.unk46A8,0x46a8,(pattern+2)%2);
 set_byte(&shop.unk46A9,0x46a9,(pattern+3)%10);
 set_byte(&shop.explanationsRenderCtx,0x46aa,(pattern+4)%2);
 set_byte(&shop.explanationsLen,0x46ab,(pattern+5)%10);
 set_byte(&shop.goldBeforeRenderCtx,0x46ac,(pattern+6)%2);
 set_byte(&shop.goldBeforeStrLen,0x46ad,(pattern+7)%10);
 set_byte(&shop.totalPriceRenderCtx,0x46ae,(pattern+8)%2);
 set_byte(&shop.totalPriceStrLen,0x46af,(pattern+9)%10);
 set_byte(&shop.goldAfterRenderCtx,0x46b0,(pattern+10)%2);
 set_byte(&shop.goldAfterStrLen,0x46b1,(pattern+11)%10);
 set_byte(&shop.finalPriceRenderCtx,0x46b3,(pattern+12)%2);
 set_byte(&shop.finalPriceStrLen,0x46b4,(pattern+13)%10);
 unsigned bit=0;
#define FLAG(member,offset) set_byte(&shop.member,offset,pattern==0 ? 0 : pattern==1 ? 1 : pattern==2 ? 255 : ((pattern-3)==bit ? 1 : 0)); ++bit
 for(unsigned i=0;i<9;i++) {FLAG(unk469C[i],0x469c+i);}
 for(unsigned i=0;i<8;i++) {FLAG(unk4684[i],0x4684+i);}
 FLAG(unk46A7,0x46a7);FLAG(unk46B5,0x46b5);FLAG(unk4785,0x4785);FLAG(unk46B2,0x46b2);
#undef FLAG
 for(unsigned i=0;i<8;i++) {set_byte(&shop.unk468C[i],0x468c+i,(pattern+i)%5);set_byte(&shop.unk4694[i],0x4694+i,(i+ctx)%2);}
 for(unsigned i=0;i<9;i++) {
  set_byte(&shop.unk46BC[i],0x46bc+i,(pattern+i)%4);set_byte(&shop.unk46C5[i],0x46c5+i,(pattern+i+1)%4);
  set_byte(&shop.unk46CE[i],0x46ce + i,(i+ctx)%2);set_byte(&shop.unk46D7[i],0x46d7+i,(i+ctx+1)%2);
 }
}
int main(int argc,char** argv) {
 require(argc==2,"disc argument");FILE* f=fopen(argv[1],"rb");require(f!=NULL,"disc open");
 fseek(f,ENTRY-0x801C5000u,SEEK_SET);require(fread(code,1,sizeof(code),f)==sizeof(code),"disc read");fclose(f);
 const unsigned gates[]={0,1,2,255};
 for(unsigned pattern=0;pattern<24;pattern++) for(unsigned a=0;a<4;a++)
 for(unsigned b=0;b<4;b++) for(unsigned ctx=0;ctx<2;ctx++) {
  ++cases;setup(pattern,ctx);manager.unk5A=gates[a];manager.unk5B=gates[b];
  ptr(MANAGER)[0x5a]=gates[a];ptr(MANAGER)[0x5b]=gates[b];
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
  PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801FF000u;cpu.gpr[31]=HALT;
  count=0;active_cpu=&cpu;require(PcPortMipsRun(&cpu,ENTRY,HALT,10000)==PC_PORT_MIPS_HALTED,"MIPS execution");active_cpu=NULL;
  unsigned expected_count=count;memcpy(expected,events,count*sizeof(Event));count=0;
  MenuShop before=shop;MenuManager before_manager=manager;
  func_801CCFF4();require(count==expected_count,"draw count");
  require(memcmp(expected,events,count*sizeof(Event))==0,"ordered draw trace");
  require(memcmp(&before,&shop,sizeof(shop))==0,"shop unchanged");
  require(memcmp(&before_manager,&manager,sizeof(manager))==0,"manager unchanged");
 }
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) require(seen[i]!=0,"full instruction coverage");
 printf("SHOP RENDER PASS cases=%u checks=%u\n",cases,checks);return 0;
}
