/* Production inventory lookup + Stored renderer versus the disc MIPS pair. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#define LOOKUP 0x801CE8D8u
#define ENTRY 0x801CE91Cu
#define END 0x801CEB3Cu
#define BASE 0x80000000u
#define MENU 0x80070000u
#define SHOP 0x80080000u
#define GAME 0x8006D634u
#define WORK 0x800B1000u
#define HALT 0xFFFFFFFCu
static u8 ram[0x200000], code[END-LOOKUP], work[0x3F6];
static SystemMenu menu;
static MenuShop shop;
SystemMenu* g_Menu=&menu;
GameState g_GameState;
u16 D_801D2260;
static unsigned cases,checks,seen[(END-ENTRY)/4];
static PcPortMipsCpu* active_cpu;
extern void func_801CE91C(u8,s32);
static void require(int ok,const char* why) {
 ++checks;if(!ok){fprintf(stderr,"SHOP STORED FAIL case=%u %s\n",cases,why);exit(1);}
}
static u8* ptr(uint32_t a) { return ram+(a-BASE); }
static void put(uint32_t a,uint32_t v) { memcpy(ptr(a),&v,4); }
static int read_bus(void* o,uint32_t a,unsigned w,uint32_t* v) {
 (void)o;if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 if(active_cpu && w==4 && a==active_cpu->pc && a>=ENTRY && a<END) ++seen[(a-ENTRY)/4];
 *v=0;for(unsigned i=0;i<w;i++) *v|=(uint32_t)ptr(a)[i]<<(8*i);return 0;
}
static int write_bus(void* o,uint32_t a,unsigned w,uint32_t v) {
 (void)o;if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 for(unsigned i=0;i<w;i++) { ptr(a)[i]=(u8)(v>>(8*i)); } return 0;
}
typedef struct {uint32_t kind,a[5];u8 bytes[8];} Event;
static Event events[16],expected[16];static unsigned count;
static Event* event(unsigned kind) {require(count<16,"event bound");Event* e=&events[count++];memset(e,0,sizeof(*e));e->kind=kind;return e;}
void* HeapAlloc(u_int size,u_int f) {Event* e=event(1);e->a[0]=size;e->a[1]=f;return work;}
u_int HeapFree(void* p) {require(p==work,"free pointer");event(2);return 0;}
static void convert(const u16* src,u8* dst,s32 n) {
 Event* e=event(3);e->a[0]=n;require(n==2,"two digits");memcpy(e->bytes,src,4);memcpy(dst,src,4);
}
void func_80033B34(u16* src,u8* dst,s32 n) {convert(src,dst,n);}
static s32 render(void* text,s32 width,s32 plane) {
 Event* e=event(4);e->a[0]=width;e->a[1]=plane;memcpy(e->bytes,text,4);
 return 0x100 + ((e->bytes[0]+e->bytes[2])&31);
}
s32 SystemRenderStringEntry(void* text,void* buffer,s32 width,s32 plane) {require(buffer==work,"render buffer");return render(text,width,plane);}
int LoadImage(RECT* r,u_long* data) {require((void*)data==work,"upload buffer");memcpy(event(5)->bytes,r,8);return 0;}
int DrawSync(int mode) {event(6)->a[0]=mode;return 0;}
void func_801C5A7C(MenuString* str,s32 index,s32 offset,u8 flags) {
 require(str==&shop.str45B0,"typed string");Event* e=event(7);e->a[0]=SHOP+0x45B0;e->a[1]=index;e->a[2]=offset;e->a[3]=flags;
}
void ShopMenuSetVertices(SVECTOR* v,u16 x,u16 y,u16 w,u16 h) {
 require(v==shop.str45B0.vertices,"typed vertices");Event* e=event(8);e->a[0]=SHOP+0x4600;e->a[1]=x;e->a[2]=y;e->a[3]=w;e->a[4]=h;
}
static uint32_t get(uint32_t a) {uint32_t v;memcpy(&v,ptr(a),4);return v;}
static int bridge(void* o,PcPortMipsCpu* c,uint32_t target) {
 (void)o;uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6],f=c->gpr[7];Event* e;
 switch(target) {
 case 0x80031BDCu:e=event(1);e->a[0]=a;e->a[1]=b;c->gpr[2]=WORK;break;
 case 0x800320E8u:require(a==WORK,"retail free");event(2);break;
 case 0x80033B34u:convert((u16*)ptr(a),ptr(b),d);break;
 case 0x80034EACu:require(b==WORK,"retail render buffer");c->gpr[2]=render(ptr(a),d,f);break;
 case 0x80044894u:require(b==WORK,"retail upload buffer");memcpy(event(5)->bytes,ptr(a),8);break;
 case 0x800445D0u:event(6)->a[0]=a;break;
 case 0x801C5A7Cu:e=event(7);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;break;
 case 0x801C6E90u:e=event(8);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;e->a[4]=get(c->gpr[29]+16);break;
 default:return 0;
 }
 return 1;
}
int main(int argc,char** argv) {
 require(argc==2,"disc path");FILE* file=fopen(argv[1],"rb");require(file!=NULL,"open disc");
 require(fseek(file,LOOKUP-0x801C5000u,SEEK_SET)==0,"seek");require(fread(code,1,sizeof(code),file)==sizeof(code),"read");fclose(file);
 for(unsigned type=0;type<3;type++) for(unsigned ctx=0;ctx<2;ctx++)
 for(unsigned quantity=0;quantity<256;quantity++) for(unsigned pattern=0;pattern<5;pattern++) for(unsigned high=0;high<2;high++) {
  ++cases;memset(ram,0,sizeof(ram));memcpy(ptr(LOOKUP),code,sizeof(code));memset(&g_GameState,0,sizeof(g_GameState));
  memset(&shop,0xA5,sizeof(shop));memset(ptr(SHOP),0xA5,0x4788);
  menu.pShop=&shop;menu.renderContext=ctx;put(0x800625A0u,MENU);put(MENU+0x450,SHOP);put(MENU+0x308,ctx);
  unsigned n=type==0?100:type==1?200:150, target=quantity%3==0?0:quantity%3==1?1:255;
  u8* ids=type==0?g_GameState.weaponIDs:type==1?g_GameState.accessoryIDs:g_GameState.itemIDs;
  u8* values=type==0?g_GameState.weaponQuantities:type==1?g_GameState.accessoryQuantities:g_GameState.itemQuantities;
  unsigned ids_offset=type==0?0x1D9C:type==1?0x1EC8:0x2026;
  memset(ids,(u8)(target+1),n);memset(values,0x79,n);
  unsigned slot=pattern==0?0:pattern==1?n/2:n-1;
  if(pattern!=3) {ids[slot]=target;values[slot]=quantity;}
  if(pattern==4) {ids[0]=target;values[0]=(u8)(quantity+1);}
  memcpy(ptr(GAME+ids_offset),ids,n);memcpy(ptr(GAME+ids_offset-n),values,n);
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=type+high*256;cpu.gpr[5]=target+high*0x123400;cpu.gpr[29]=0x801FF000u;cpu.gpr[31]=HALT;
  count=0;active_cpu=&cpu;require(PcPortMipsRun(&cpu,ENTRY,HALT,10000)==PC_PORT_MIPS_HALTED,"retail execution");active_cpu=NULL;
  unsigned expected_count=count;memcpy(expected,events,count*sizeof(Event));count=0;
  func_801CE91C((u8)(type+high*256),target+high*0x123400);
  require(count==expected_count,"helper count");require(memcmp(events,expected,count*sizeof(Event))==0,"ordered calls and data");
  require(D_801D2260==(get(0x801D2260u)&65535),"stored inventory quantity");
  require(shop.str45B0.width==ptr(SHOP+0x462E)[0],"width truncation");
  require(shop.str45B0.renderContext==ptr(SHOP+0x462D)[0],"context");
  require(shop.unk4785==ptr(SHOP+0x4785)[0],"render enabled");
  require(memcmp(ids,ptr(GAME+ids_offset),n)==0 && memcmp(values,ptr(GAME+ids_offset-n),n)==0,"inventory unchanged");
 }
 unsigned covered=0;
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) {
  unsigned pc=ENTRY+i*4;
  int invalid=pc==0x801CE950u || pc==0x801CE954u || pc==0x801CE964u || pc==0x801CE968u;
  require(seen[i] || invalid,"all valid-type instruction sites");if(seen[i]) ++covered;
 }
 printf("SHOP STORED PASS cases=%u checks=%u instructions=%u/%zu (plus real lookup)\n",cases,checks,covered,sizeof(seen)/sizeof(seen[0]));return 0;
}
