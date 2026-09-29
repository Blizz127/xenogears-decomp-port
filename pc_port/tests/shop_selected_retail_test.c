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
#define ENTRY 0x801CEB3Cu
#define END 0x801CF2A0u
#define BASE 0x80000000u
#define MENU 0x80070000u
#define SHOP 0x80080000u
#define GAME 0x8006D634u
#define WORK 0x800B1000u
#define HALT 0xFFFFFFFCu
static u8 ram[0x200000], code[END-LOOKUP], work[0x618];
static SystemMenu menu;
static MenuShop shop;
SystemMenu* g_Menu=&menu;
GameState g_GameState;
u16 D_801D2260;
static unsigned cases,checks,seen[(END-ENTRY)/4];
static PcPortMipsCpu* active_cpu;
extern s32 func_801CEB3C(s32,s32,u8*);
static MenuUnk6 resources;
static MenuShopWeapon weapons[256];
static MenuShopAccessory accessories[256];
static MenuShopItem items[256];
s32 D_801D21CC[9];
static u16 equipped_flags;
static unsigned pattern;
#define RES 0x80090000u
#define WEAPONS 0x800A0000u
#define ACCESSORIES 0x800A1000u
#define ITEMS 0x800A2000u
#define STRINGS 0x800C0000u
#define ATLAS 0x800D0000u
static u8 text_data[8];
static unsigned current_alloc;

static void require(int ok,const char* why) {
 ++checks;if(!ok){fprintf(stderr,"SHOP SELECTED FAIL case=%u %s\n",cases,why);exit(1);}
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
static Event events[256],expected[256];static unsigned count;
static Event* event(unsigned kind) {require(count<256,"event bound");Event* e=&events[count++];memset(e,0,sizeof(*e));e->kind=kind;return e;}
void* HeapAlloc(u_int size,u_int f) {Event* e=event(1);e->a[0]=size;e->a[1]=f;current_alloc=size;memset(work,0xA5,sizeof(work));return work;}
u_int HeapFree(void* p) {require(p==work,"free pointer");event(2);return 0;}
static void convert(const u16* src,u8* dst,s32 n) {
 Event* e=event(3);e->a[0]=n;require(n==2,"two digits");memcpy(e->bytes,src,4);memcpy(dst,src,4);
}
void func_80033B34(u16* src,u8* dst,s32 n) {convert(src,dst,n);}
static s32 render(void* text,s32 width,s32 plane) {
 Event* e=event(4);e->a[0]=width;e->a[1]=plane;memcpy(e->bytes,text,4);
 return 0x100 + ((e->bytes[0]+e->bytes[2])&31);
}
s32 SystemRenderStringEntry(void* text,void* buffer,s32 width,s32 plane) {require(buffer==work,"render buffer");if(current_alloc==0x618) for(unsigned i=0;i<current_alloc;i++) require(work[i]==0,"native cleared work");return render(text,width,plane);}
int LoadImage(RECT* r,u_long* data) {require((void*)data==work,"upload buffer");memcpy(event(5)->bytes,r,8);return 0;}
int DrawSync(int mode) {event(6)->a[0]=mode;return 0;}
void func_801C5A7C(MenuString* str,s32 index,s32 offset,u8 flags) {
 require(str==&shop.str45B0 || str==&shop.strItemDesc,"typed string");Event* e=event(7);e->a[0]=str==&shop.str45B0?SHOP+0x45B0:SHOP+0x4430;e->a[1]=index;e->a[2]=offset;e->a[3]=flags;
}
void ShopMenuSetVertices(SVECTOR* v,u16 x,u16 y,u16 w,u16 h) {
 require(v==shop.str45B0.vertices || v==shop.strItemDesc.vertices,"typed vertices");Event* e=event(8);e->a[0]=v==shop.str45B0.vertices?SHOP+0x4600:SHOP+0x4480;e->a[1]=x;e->a[2]=y;e->a[3]=w;e->a[4]=h;
}
static uint32_t address(const void* p) {
 if(p==&shop.strItemDesc.polys[0]) return SHOP+0x4430;
 if(p==&shop.strItemDesc.polys[1]) return SHOP+0x4458;
 const void* bases[]={shop.polys2D0,shop.polys27B0,shop.polys3020};
 unsigned sizes[]={sizeof(shop.polys2D0),sizeof(shop.polys27B0),sizeof(shop.polys3020)};
 unsigned offsets[]={0x2D0,0x27B0,0x3020};
 for(unsigned i=0;i<3;i++) {
  uintptr_t a=(uintptr_t)p,b=(uintptr_t)bases[i];
  if(a>=b && a<b+sizes[i]) return SHOP+offsets[i]+(uint32_t)(a-b);
 }
 require(0,"polygon address");return 0;
}
void* GetStringEntry(void* bundle,s32 id) {
 Event* e=event(9);e->a[0]=(uintptr_t)bundle;e->a[1]=id;
 memset(text_data,0,sizeof(text_data));text_data[0]=(uintptr_t)bundle/256; text_data[2]=id;
 return text_data;
}
u_short ShopMenuGetCharacterEquippedItemFlags(u_char id,u_char type) {
 Event* e=event(10);e->a[0]=id;e->a[1]=type;return equipped_flags;
}
u_short ShopMenuIsCharacterFlagSet(u_short flags,u_char character) {
 Event* e=event(11);e->a[0]=flags;e->a[1]=character;return flags & (1u<<character);
}
void func_801C5040(POLY_FT4* p,short x,short y,u_char u,u_char v,short w,short h) {
 Event* e=event(12);e->a[0]=address(p);e->a[1]=x;e->a[2]=y;e->a[3]=u;e->a[4]=v;
 memcpy(e->bytes,&w,2);memcpy(e->bytes+2,&h,2);
}
s32 func_8002675C(void* atlas,s32 id,POLY_FT4* p,s32 ctx,s32 x,s32 y,s32 scale) {
 require(atlas==(void*)ATLAS,"atlas");Event* e=event(13);e->a[0]=id;e->a[1]=address(p);e->a[2]=ctx;e->a[3]=x;e->a[4]=y;memcpy(e->bytes,&scale,4);return 1;
}
static void change_values(s32* changes,u8* colors,u8 id,u8 type,u8 character) {
 require(changes[0]==0 && changes[1]==0,"changes zero before helper");
 Event* e=event(14);e->a[0]=id;e->a[1]=type;e->a[2]=character;
 static const s32 values[]={0,1,9,10,99,100,999,1000};
 changes[0]=values[(pattern+character)%8];changes[1]=values[(pattern+character+3)%8];
 colors[0]=(pattern+character)%2;colors[1]=1-colors[0];
}
void func_801CE480(s32* changes,u8* colors,u8 id,u8 type,u8 character) {change_values(changes,colors,id,type,character);}
static void parse(unsigned n,u8* digits) {
 event(15)->a[0]=n;memset(digits,0xFF,9);
 for(int i=8;i>=0;i--) {digits[i]=n%10;n/=10;if(!n) break;}
}
void ShopMenuParseNumberToString(unsigned n) {parse(n,menu.digits);}
void ShopMenuSetStatChangeColor(int n,POLY_FT4* p,u_char color) {
 Event* e=event(16);e->a[0]=n;e->a[1]=address(p);e->a[2]=color;
}
static uint32_t get(uint32_t a) {uint32_t v;memcpy(&v,ptr(a),4);return v;}
static int bridge(void* o,PcPortMipsCpu* c,uint32_t target) {
 (void)o;uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6],f=c->gpr[7];Event* e;
 switch(target) {
 case 0x80031BDCu:e=event(1);e->a[0]=a;e->a[1]=b;current_alloc=a;memset(ptr(WORK),0xA5,sizeof(work));c->gpr[2]=WORK;break;
 case 0x800320E8u:require(a==WORK,"retail free");event(2);break;
 case 0x80033B34u:convert((u16*)ptr(a),ptr(b),d);break;
 case 0x80034EACu:require(b==WORK,"retail render buffer");if(current_alloc==0x618) for(unsigned i=0;i<current_alloc;i++) require(ptr(WORK)[i]==0,"retail cleared work");c->gpr[2]=render(ptr(a),d,f);break;
 case 0x80044894u:require(b==WORK,"retail upload buffer");memcpy(event(5)->bytes,ptr(a),8);break;
 case 0x800445D0u:event(6)->a[0]=a;break;
 case 0x801C5A7Cu:e=event(7);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;break;
 case 0x801C6E90u:e=event(8);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;e->a[4]=get(c->gpr[29]+16);break;
 case 0x8003F8E8u:require(a==WORK && b==0x618,"clear work");memset(ptr(a),0,b);break;
 case 0x80033728u: {
  void* p=GetStringEntry((void*)(uintptr_t)a,b);memcpy(ptr(STRINGS),p,8);c->gpr[2]=STRINGS;break;
 }
 case 0x801CDBA0u:c->gpr[2]=ShopMenuGetCharacterEquippedItemFlags(a,b);break;
 case 0x801C50B0u:c->gpr[2]=ShopMenuIsCharacterFlagSet(a,b);break;
 case 0x801C5040u: {
  e=event(12);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;e->a[4]=get(c->gpr[29]+16);
  u16 w=get(c->gpr[29]+20),h=get(c->gpr[29]+24);memcpy(e->bytes,&w,2);memcpy(e->bytes+2,&h,2);break;
 }
 case 0x8002675Cu: {
  require(a==ATLAS,"retail atlas");e=event(13);e->a[0]=b;e->a[1]=d;e->a[2]=f;e->a[3]=get(c->gpr[29]+16);e->a[4]=get(c->gpr[29]+20);
  unsigned scale=get(c->gpr[29]+24);memcpy(e->bytes,&scale,4);c->gpr[2]=1;break;
 }
 case 0x801CE480u:change_values((s32*)ptr(a),ptr(b),d,f,get(c->gpr[29]+16));break;
 case 0x801C50E8u:parse(a,ptr(MENU+0x31C));break;
 case 0x801CD404u:e=event(16);e->a[0]=a;e->a[1]=b;e->a[2]=d;break;
 default:return 0;
 }
 return 1;
}
static void compare_byte(u8 native,unsigned offset) {require(native==ptr(SHOP+offset)[0],"shop output byte");}
int main(int argc,char** argv) {
 require(argc==2,"disc path");FILE* file=fopen(argv[1],"rb");require(file!=NULL,"open disc");
 require(fseek(file,LOOKUP-0x801C5000u,SEEK_SET)==0,"seek");require(fread(code,1,sizeof(code),file)==sizeof(code),"read");fclose(file);
 for(unsigned type=0;type<3;type++) for(unsigned ctx=0;ctx<2;ctx++)
 for(pattern=0;pattern<16;pattern++) for(unsigned selection=0;selection<4;selection++) for(unsigned party=0;party<4;party++) {
  ++cases;memset(ram,0,sizeof(ram));memcpy(ptr(LOOKUP),code,sizeof(code));memset(&g_GameState,0,sizeof(g_GameState));
  memset(&shop,0xA5,sizeof(shop));memset(ptr(SHOP),0xA5,0x4788);memset(&menu,0,sizeof(menu));
  menu.pShop=&shop;menu.renderContext=ctx;menu.unk330=&resources;menu.unk2DC=(void*)ATLAS;
  put(0x800625A0u,MENU);put(MENU+0x450,SHOP);put(MENU+0x308,ctx);put(MENU+0x330,RES);put(MENU+0x2DC,ATLAS);
  resources.pWeaponsData=weapons;resources.pAccessoriesData=accessories;resources.pItemsData=items;
  put(RES,WEAPONS);put(RES+4,ACCESSORIES);put(RES+0x1C,ITEMS);
  shop.pWeaponDescriptions=(void*)0x800E0000u;shop.pAccessoryDescriptions=(void*)0x800E1000u;shop.pItemDescriptions=(void*)0x800E2000u;
  put(SHOP+0x4634,0x800E0000u);put(SHOP+0x4638,0x800E1000u);put(SHOP+0x4630,0x800E2000u);
  unsigned row=selection==0?0:selection==1?7:selection==2?3:7, scroll=selection==3?40:selection==2?12:0;
  unsigned id=pattern==0?0:pattern==15?255:pattern*13;
  menu.shopItemIDs[row+scroll]=ptr(MENU+0x1E30+row+scroll)[0]=id;
  menu.shopItemTypes[row+scroll]=ptr(MENU+0x1E60+row+scroll)[0]=type;
  u16 equip=pattern%4==0?0:pattern%4==1?0xFFFF:pattern%4==2?0x5555:0xAAAA;
  equipped_flags=pattern%3==0?0:pattern%3==1?0xFFFF:0xA55A;
  weapons[id].equipFlags=accessories[id].equipFlags=equip;
  weapons[id].price=pattern*4097u;accessories[id].price=65535u-pattern*4097u;items[id].price=pattern*31u;
  memcpy(ptr(WEAPONS+id*16),&weapons[id],16);memcpy(ptr(ACCESSORIES+id*16),&accessories[id],16);memcpy(ptr(ITEMS+id*16),&items[id],16);
  /* At most nine visible portraits fit retail's nine-slot arrays. Include high
   * character IDs and holes, not just a prefix of the availability table. */
  for(unsigned i=0;i<16;i++) {
   unsigned active=party==0?0:party==1?i==15:party==2?i%2==0:i<9;
   menu.availableCharacters[i]=ptr(MENU+0x30C+i)[0]=active;
  }
  for(unsigned i=0;i<9;i++) {D_801D21CC[i]=30+i*26;put(0x801D21CC+i*4,D_801D21CC[i]);}
  unsigned ids_offset=type==0?0x1D9C:type==1?0x1EC8:0x2026, n=type==0?100:type==1?200:150;
  u8* ids=type==0?g_GameState.weaponIDs:type==1?g_GameState.accessoryIDs:g_GameState.itemIDs;
  u8* values=type==0?g_GameState.weaponQuantities:type==1?g_GameState.accessoryQuantities:g_GameState.itemQuantities;
  memset(ids,(u8)(id+1),n);ids[n-1]=id;values[n-1]=pattern*17;
  memcpy(ptr(0x8006D634+ids_offset),ids,n);memcpy(ptr(0x8006D634+ids_offset-n),values,n);
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=row;cpu.gpr[5]=scroll;cpu.gpr[6]=0;cpu.gpr[29]=0x801FF000u;cpu.gpr[31]=HALT;
  count=0;active_cpu=&cpu;require(PcPortMipsRun(&cpu,ENTRY,HALT,100000)==PC_PORT_MIPS_HALTED,"retail execution");active_cpu=NULL;
  unsigned expected_count=count;memcpy(expected,events,count*sizeof(Event));count=0;
  s32 price=func_801CEB3C(row,scroll,NULL);
  require((uint32_t)price==cpu.gpr[2],"returned retail price");
  require(count==expected_count,"helper count");require(memcmp(events,expected,count*sizeof(Event))==0,"ordered calls and data");
  require(D_801D2260==(get(0x801D2260u)&65535),"stored quantity");
  compare_byte(shop.strItemDesc.width,0x44AE);compare_byte(shop.strItemDesc.renderContext,0x44AD);
  compare_byte(shop.str45B0.width,0x462E);compare_byte(shop.str45B0.renderContext,0x462D);
  compare_byte(shop.unk46A7,0x46A7);compare_byte(shop.unk46A8,0x46A8);compare_byte(shop.unk46A9,0x46A9);compare_byte(shop.unk4785,0x4785);
  for(unsigned i=0;i<9;i++) {
   compare_byte(shop.unk469C[i],0x469C+i);compare_byte(shop.unk46BC[i],0x46BC+i);compare_byte(shop.unk46C5[i],0x46C5+i);
   compare_byte(shop.unk46CE[i],0x46CE + i);compare_byte(shop.unk46D7[i],0x46D7+i);
  }
 }
 unsigned covered=0;
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) {
  unsigned pc=ENTRY+i*4;
  int invalid=pc==0x801CEBC8u || pc==0x801CEBCCu || pc==0x801CEBDCu || pc==0x801CEBE0u;
  require(seen[i] || invalid,"all valid-type instruction sites");if(seen[i]) ++covered;
 }
 printf("SHOP SELECTED PASS cases=%u checks=%u instructions=%u/%zu (plus stored/lookup)\n",cases,checks,covered,sizeof(seen)/sizeof(seen[0]));return 0;
}
