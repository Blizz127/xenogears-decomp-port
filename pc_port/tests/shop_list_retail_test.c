/* Native row builder versus retail execution; graphics/text helpers are
 * observed at their call boundaries with deterministic shared side effects. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#define ENTRY 0x801CDD14u
#define END 0x801CE480u
#define BASE 0x80000000u
#define MENU 0x80070000u
#define SHOP 0x80080000u
#define RES 0x80090000u
#define WEAPON 0x800A0000u
#define ACCESSORY 0x800A1000u
#define ITEM 0x800A2000u
#define FLAGS 0x800B0000u
#define WORK 0x800B1000u
#define NAMES 0x800C0000u
#define ATLAS 0x800D0000u
#define HALT 0xFFFFFFFCu
static u8 ram[0x200000], code[END-ENTRY];
static SystemMenu menu;
static MenuShop shop;
static MenuUnk6 resources;
static MenuShopWeapon weapons[256];
static MenuShopAccessory accessories[256];
static MenuShopItem items[256];
static u8 flags[8], work[0x3F6], names[3][256][8];
SystemMenu* g_Menu=&menu;
static unsigned cases,checks,seen[(END-ENTRY)/4];
static PcPortMipsCpu* active_cpu;
extern void func_801CDD14(s32,s32,u8*);
static void require(int ok,const char* why) {
 ++checks;if(!ok){fprintf(stderr,"SHOP LIST FAIL case=%u %s\n",cases,why);exit(1);}
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

typedef struct {uint32_t kind,a[8];u8 bytes[16];} Event;
static Event events[256], expected[256];static unsigned count;
static Event* event(unsigned kind) {require(count<256,"event bound");Event* e=&events[count++];memset(e,0,sizeof(*e));e->kind=kind;return e;}
static uint32_t address(const void* p) {
 for(unsigned i=0;i<8;i++) {
  if(p==&shop.strings3C30[i]) return SHOP+0x3C30+i*0x80;
  if(p==&shop.strings4030[i]) return SHOP+0x4030+i*0x80;
  if(p==shop.strings3C30[i].vertices) return SHOP+0x3C80+i*0x80;
  if(p==shop.strings4030[i].vertices) return SHOP+0x4080+i*0x80;
 }
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)shop.unk1DB0;
 if(a>=b && a<b+sizeof(shop.unk1DB0)) return SHOP+0x1DB0+(uint32_t)(a-b);
 require(0,"native pointer map");return 0;
}
void* HeapAlloc(u_int size,u_int f) {Event* e=event(1);e->a[0]=size;e->a[1]=f;return work;}
u_int HeapFree(void* p) {require(p==work,"free pointer");event(2);return 0;}
static void* name(unsigned type,s32 id) {Event* e=event(3);e->a[0]=type;e->a[1]=id;return names[type][id];}
void* GetWeaponName(s32 id){return name(0,id);}
void* GetAccessoryName(s32 id){return name(1,id);}
void* GetItemName(s32 id){return name(2,id);}
static unsigned render(const u8* bytes,unsigned height,unsigned plane) {
 Event* e=event(4);e->a[0]=height;e->a[1]=plane;memcpy(e->bytes,bytes,plane?10:8);
 return (bytes[0]+bytes[2]+17u)&255;
}
s32 SystemRenderStringEntry(void* str,void* buffer,s32 height,s32 plane) {require(buffer==work,"render work");return render(str,height,plane);}
static void convert(const u16* src,u8* dst,s32 n) {
 Event* e=event(5);e->a[0]=n;memcpy(e->bytes,src,n*2);memcpy(dst,src,n*2);
}
void func_80033B34(u16* src,u8* dst,s32 n){convert(src,dst,n);}
int LoadImage(RECT* rect,u_long* data){require((void*)data==work,"upload work");Event* e=event(6);memcpy(e->bytes,rect,8);return 0;}
int DrawSync(int mode){Event* e=event(7);e->a[0]=mode;return 0;}
void func_801C5A7C(MenuString* p,s32 index,s32 off,u8 f){Event* e=event(8);e->a[0]=address(p);e->a[1]=index;e->a[2]=off;e->a[3]=f;}
void ShopMenuSetVertices(SVECTOR* p,u16 x,u16 y,u16 w,u16 h){Event* e=event(9);e->a[0]=address(p);e->a[1]=x;e->a[2]=y;e->a[3]=w;e->a[4]=h;}
s32 func_8002675C(void* atlas,s32 id,POLY_FT4* p,s32 ctx,s32 x,s32 y,s32 scale){
 require(atlas==(void*)ATLAS,"atlas pointer");Event* e=event(10);e->a[0]=id;e->a[1]=address(p);e->a[2]=ctx;e->a[3]=x;e->a[4]=y;e->a[5]=scale;return 1;
}
static uint32_t get(uint32_t a){uint32_t v;memcpy(&v,ptr(a),4);return v;}
static int bridge(void* o,PcPortMipsCpu* c,uint32_t target) {
 (void)o;uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6],f=c->gpr[7],sp=c->gpr[29];Event* e;
 switch(target){
 case 0x80031BDCu:e=event(1);e->a[0]=a;e->a[1]=b;c->gpr[2]=WORK;break;
 case 0x800320E8u:require(a==WORK,"retail free");event(2);break;
 case 0x8003F8E8u:memset(ptr(a),0,b);break;
 case 0x80033848u:case 0x800337E8u:case 0x80033818u:{
  unsigned type=target==0x80033848u?0:target==0x800337E8u?1:2;
  e=event(3);e->a[0]=type;e->a[1]=a;c->gpr[2]=NAMES+type*2048+a*8;break;}
 case 0x80034EACu:require(b==WORK,"retail render work");c->gpr[2]=render(ptr(a),d,f);break;
 case 0x80033B34u:convert((u16*)ptr(a),ptr(b),d);break;
 case 0x80044894u:require(b==WORK,"retail upload work");e=event(6);memcpy(e->bytes,ptr(a),8);break;
 case 0x800445D0u:e=event(7);e->a[0]=a;break;
 case 0x801C5A7Cu:e=event(8);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;break;
 case 0x801C6E90u:e=event(9);e->a[0]=a;e->a[1]=b;e->a[2]=d;e->a[3]=f;e->a[4]=get(sp+16);break;
 case 0x8002675Cu:require(a==ATLAS,"retail atlas");e=event(10);e->a[0]=b;e->a[1]=d;e->a[2]=f;e->a[3]=get(sp+16);e->a[4]=get(sp+20);e->a[5]=get(sp+24);c->gpr[2]=1;break;
 default:return 0;
 }
 return 1;
}
int main(int argc,char** argv){
 require(argc==2,"disc argument");FILE* f=fopen(argv[1],"rb");require(f!=NULL,"disc open");fseek(f,ENTRY-0x801C5000u,SEEK_SET);require(fread(code,1,sizeof(code),f)==sizeof(code),"disc read");fclose(f);
 const s32 prices[]={0,1,9,10,99,100,999,1000,9999,10000,65535};
 const s32 money[]={-1,0,9,100,10000,65535};const unsigned starts[]={0,1,40};
 for(unsigned seed=0;seed<11;seed++) for(unsigned gold=0;gold<6;gold++)
 for(unsigned start=0;start<3;start++) for(unsigned ctx=0;ctx<2;ctx++) {
  ++cases;memset(ram,0,sizeof(ram));memcpy(ptr(ENTRY),code,sizeof(code));
  memset(&shop,0xA5,sizeof(shop));memset(ptr(SHOP),0xA5,0x4788);memset(flags,0xA5,8);memset(ptr(FLAGS),0xA5,8);
  menu.pShop=&shop;menu.unk330=&resources;menu.renderContext=ctx;menu.unk2DC=(void*)ATLAS;
  resources.pWeaponsData=weapons;resources.pAccessoriesData=accessories;resources.pItemsData=items;
  put(0x800625A0u,MENU);put(MENU+0x450,SHOP);put(MENU+0x330,RES);put(MENU+0x308,ctx);put(MENU+0x2DC,ATLAS);
  put(RES,WEAPON);put(RES+4,ACCESSORY);put(RES+0x1C,ITEM);
  for(unsigned i=0;i<256;i++) {
   weapons[i].price=accessories[i].price=items[i].price=prices[(i+seed)%11];
   memcpy(ptr(WEAPON+i*16),&weapons[i],16);memcpy(ptr(ACCESSORY+i*16),&accessories[i],16);memcpy(ptr(ITEM+i*16),&items[i],16);
   for(unsigned t=0;t<3;t++){memset(names[t][i],0,8);names[t][i][0]=i;names[t][i][2]=t;memcpy(ptr(NAMES+t*2048+i*8),names[t][i],8);}
  }
  for(unsigned i=0;i<48;i++){
   u8 id=(i+seed)%4==0?0:(u8)((i*17+seed)%255+1),type=(i+seed)%3,quantity=(u8)((i+seed)%4==0?0:(i+seed)%4==1?1:(i+seed)%4==2?9:99);
   menu.shopItemIDs[i]=ptr(MENU+0x1E30)[i]=id;menu.shopItemTypes[i]=ptr(MENU+0x1E60)[i]=type;
   shop.curItemQuantities[i]=ptr(SHOP+0x4654)[i]=quantity;
  }
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=starts[start];cpu.gpr[5]=money[gold];cpu.gpr[6]=FLAGS;cpu.gpr[29]=0x801FF000u;cpu.gpr[31]=HALT;
  count=0;active_cpu=&cpu;require(PcPortMipsRun(&cpu,ENTRY,HALT,100000)==PC_PORT_MIPS_HALTED,"MIPS execution");active_cpu=NULL;
  unsigned expected_count=count;memcpy(expected,events,count*sizeof(Event));count=0;
  func_801CDD14(starts[start],money[gold],flags);
  require(count==expected_count,"call count");require(memcmp(events,expected,count*sizeof(Event))==0,"ordered calls and payloads");
  require(memcmp(flags,ptr(FLAGS),8)==0,"affordability");
  for(unsigned i=0;i<8;i++){
   require(shop.strings3C30[i].width==ptr(SHOP+0x3CAE + i*0x80)[0],"name width");
   require(shop.strings4030[i].width==ptr(SHOP+0x40AE + i*0x80)[0],"price width");
   require(shop.strings3C30[i].renderContext==ptr(SHOP+0x3CAD+i*0x80)[0],"name context");
   require(shop.strings4030[i].renderContext==ptr(SHOP+0x40AD+i*0x80)[0],"price context preserved");
   require(shop.unk4684[i]==ptr(SHOP+0x4684)[i],"row active");require(shop.unk468C[i]==ptr(SHOP+0x468C)[i],"quantity length");require(shop.unk4694[i]==ptr(SHOP+0x4694)[i],"quantity context");
  }
 }
 unsigned covered=0;
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) {
  unsigned pc=ENTRY+4*i;
  int invalid_type_path=pc==0x801CDE2Cu || pc==0x801CDE30u || pc==0x801CDE40u || pc==0x801CDE44u;
  require(seen[i] || invalid_type_path,"all valid-type instructions covered");
  if(seen[i]) ++covered;
 }
 printf("SHOP LIST PASS cases=%u checks=%u instructions=%u/%zu\n",cases,checks,covered,sizeof(seen)/sizeof(seen[0]));return 0;
}
