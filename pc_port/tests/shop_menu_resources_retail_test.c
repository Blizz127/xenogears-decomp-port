/* Actual full-TU loader vs raw retail; archive, heap, atlas and GPU boundaries
 * are explicit spies. This does not execute decompression, CD, audio or GPU. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#define ENTRY 0x801c54b4u
#define END 0x801c58f4u
#define HALT 0xfffffffcu
#define STACK 0x801ff000u
#define GLOBAL 0x800625a0u
#define RESGLOBAL 0x8005945cu
#define SEDSGLOBAL 0x8006259cu
#define DEBUGGLOBAL 0x80059178u
#define RES0 0x80060000u
#define RES1 0x80060100u
#define MENU0 0x80070000u
#define WORK0 0x80080000u
#define MANAGER0 0x80090000u
#define ICON 0x800a0000u
#define TEXTURES 0x800a2000u
#define ATLAS0 0x800a4000u
#define ATLAS1 0x800a6000u
#define PORTRAITS 0x800b0000u
#define PORTRAIT_BYTES (0xb20u*255u)
#define SEDS 0x80170000u
#define ENTRIES 0x80172000u
#define OLDSEDS 0x80173000u
#define WORK_TAIL_SIZE 0x4a0u
static uint8_t ram[0x200000],retail[END-ENTRY],retail_data[16];
static SystemMenu *menus,before_menus[2];
static MenuUnk2 *works,before_works[2];
static MenuManager *managers;
SystemMenu *g_Menu;
void *D_8005945C,*D_8006259C;
u8 g_MenuDebugEnabled;
static uint32_t resources[2][8];
static unsigned native_mode,mutation,cases,checks,seen[(END-ENTRY)/4],pattern,atlas_count;
static PcPortMipsCpu *active_cpu;
static uint32_t opened;
static unsigned active_portraits;
typedef struct {uint32_t guest,size;uint8_t *host;} Region;
static Region regions[]={{ICON,0x1000,0},{TEXTURES,0x1000,0},{ATLAS0,0x1000,0},{ATLAS1,0x1000,0},{PORTRAITS,PORTRAIT_BYTES,0},{SEDS,0x1000,0},{ENTRIES,0x1000,0},{OLDSEDS,0x1000,0}};
typedef struct {uint32_t id,args[8],payload[4];} Event;
typedef struct {Event events[64];unsigned count;uint32_t menu,resglobal,sedsglobal,destinations[2][4],resources[2][8];uint8_t tail[2][WORK_TAIL_SIZE];} Result;
static Result result;
extern void ShopMenuLoadResources(void);
extern const char D_801C5000[16];
static void require(int ok,const char *why){checks++;if(!ok){fprintf(stderr,"SHOP RESOURCES FAIL case=%u native=%u mutation=%u %s\n",cases,native_mode,mutation,why);exit(1);}}
static uint8_t *ptr(uint32_t a){return ram+(a&0x1fffffu);}
static uint32_t r32(uint32_t a){uint32_t v;memcpy(&v,ptr(a),4);return v;}
static void w32(uint32_t a,uint32_t v){memcpy(ptr(a),&v,4);}
static uint32_t menuaddr(unsigned m){return MENU0+m*0x3000;}
static uint32_t workaddr(unsigned m){return WORK0+m*0x6000;}
static uint32_t manageraddr(unsigned m){return MANAGER0+m*0x200;}
static void *host(uint32_t a){for(unsigned i=0;i<sizeof(regions)/sizeof(regions[0]);i++)if(a>=regions[i].guest&&a-regions[i].guest<regions[i].size)return regions[i].host+(a-regions[i].guest);require(0,"unknown region guest pointer");return 0;}
static uint32_t normal(const void *p){
 uintptr_t a=(uintptr_t)p;
 for(unsigned i=0;i<sizeof(regions)/sizeof(regions[0]);i++)if(a>=(uintptr_t)regions[i].host&&a-(uintptr_t)regions[i].host<regions[i].size)return regions[i].guest+(uint32_t)(a-(uintptr_t)regions[i].host);
 for(unsigned m=0;m<2;m++){
  if(p==&menus[m])return menuaddr(m);if(p==&works[m].tim)return workaddr(m)+0xb80;
  if(p==resources[m])return RES0+m*0x100;
  if(p==works[m].unk4B98)return workaddr(m)+0x4b98;
  if(p==works[m].unk4BF4)return workaddr(m)+0x4bf4;
  if(p==works[m].unk4C14)return workaddr(m)+0x4c14;
 }
 if(a<=UINT32_MAX)return (uint32_t)a;
 fprintf(stderr,"UNKNOWN POINTER %p events=%u lastid=%u opened=%08x\n",p,result.count,result.count?result.events[result.count-1].id:0,opened);require(0,"unknown high native pointer");return 0;
}
static void rebind(unsigned n){
 if(mutation==1 || (mutation==2 && (n==3||n==14||n==18))){unsigned m=n&1;if(native_mode)g_Menu=&menus[m];else w32(GLOBAL,menuaddr(m));}
 if(mutation==3 && n==1){if(native_mode)D_8005945C=resources[1];else w32(RESGLOBAL,RES1);}
}
static Event *event(unsigned id){require(result.count<64,"trace overflow");Event *e=&result.events[result.count++];e->id=id;return e;}
static void done(void){rebind(result.count);}
static uint32_t decode_return(uint32_t input){require(input>=0x81100001u&&input<=0x81100007u,"packed u32 resource slot");switch(input-0x81100000u){case 1:return ICON;case 2:return TEXTURES;case 3:return ATLAS0;case 4:return ATLAS1;case 5:return PORTRAITS;case 7:return ENTRIES;default:require(0,"unexpected archive slot");return 0;}}
unsigned int ResolveArchiveEntryPointers(u32 *p){Event *e=event(1);e->args[0]=normal(p);require(p==resources[0],"initial resource table captured");for(unsigned i=1;i<8;i++){e->args[i]=p[i];p[i]=0x81100000u+i;}done();return p[0];}
void *LZSSHeapDecompress(void *p,int flags){Event *e=event(2);e->args[0]=normal(p);e->args[1]=(uint32_t)flags;uint32_t r=decode_return(e->args[0]);e->args[2]=r;done();return host(r);}
int OpenTIM(u_long *p){Event *e=event(3);e->args[0]=normal(p);opened=e->args[0];done();return 0;}
static void tim_fields(uint32_t *v){v[0]=8;v[1]=opened+0x10;v[2]=opened+0x40;v[3]=opened+0x18;v[4]=opened+0x80;}
TIM_IMAGE *ReadTIM(TIM_IMAGE *t){Event *e=event(4);uint32_t dest=0;for(unsigned m=0;m<2;m++)if(t==&works[m].tim)dest=workaddr(m)+0xb80;e->args[0]=dest?dest:0xffff0000u;uint32_t v[5];tim_fields(v);t->mode=v[0];t->crect=host(v[1]);t->caddr=host(v[2]);t->prect=host(v[3]);t->paddr=host(v[4]);done();return t;}
/* Source memmove/bzero resolve to these explicit spies; memcpy remains its
 * real compiler/libc implementation so the 13-byte inline copy is observed. */
void bzero(void *p,size_t n){Event *e=event(5);e->args[0]=normal(p);e->args[1]=(uint32_t)n;memset(p,0,n);done();}
void *memmove(void *d,const void *s,size_t n){Event *e=event(6);e->args[0]=normal(d);e->args[1]=normal(s);e->args[2]=(uint32_t)n;uint8_t tmp[0x100];require(n<=sizeof(tmp),"bounded memmove");memcpy(tmp,s,n);memcpy(d,tmp,n);done();return d;}
u_int HeapFree(void *p){Event *e=event(7);e->args[0]=normal(p);done();return 0;}
void func_8002DD20(u32 *p){Event *e=event(8);e->args[0]=normal(p);done();}
static void atlas_values(uint32_t *v){for(unsigned i=0;i<6;i++)v[i]=(pattern?0x7ffffff8u:0x12348000u)+(atlas_count*0x10001u)+i;atlas_count++;}
void func_80026338(u8 *p,s32 index,u32 *a,s32 *b,s32 *c,s32 *d,s32 *f,s32 *g){Event *e=event(9);e->args[0]=normal(p);e->args[1]=(uint32_t)index;void *outs[6]={a,b,c,d,f,g};uint32_t v[6];atlas_values(v);for(unsigned i=0;i<6;i++){e->args[i+2]=(uint32_t)((uintptr_t)outs[i]-(uintptr_t)a);memcpy(outs[i],&v[i],4);}done();}
int LoadImage(RECT *r,u_long *p){Event *e=event(10);e->args[0]=normal(r);e->args[1]=normal(p);memcpy(e->payload,r,8);memcpy(e->payload+2,p,8);active_portraits++;done();return 0;}
int DrawSync(int mode){Event *e=event(11);e->args[0]=(uint32_t)mode;done();return 0;}
int ArchiveSetIndex(int a,int b){Event *e=event(12);e->args[0]=(uint32_t)a;e->args[1]=(uint32_t)b;done();return 0;}
int ArchiveDecodeAlignedSize(unsigned int a){Event *e=event(13);e->args[0]=a;done();return 0x1000;}
void *HeapAlloc(u_int n,u_int flags){Event *e=event(14);e->args[0]=n;e->args[1]=flags;done();return host(SEDS);}
s32 ArchiveReadFileToBuffer(s32 index,void *p,u32 a,u32 flags){Event *e=event(15);e->args[0]=(uint32_t)index;e->args[1]=normal(p);e->args[2]=a;e->args[3]=flags;done();return 0;}
void ArchiveCdDataSync(int a){Event *e=event(16);e->args[0]=(uint32_t)a;done();}
void SoundAddSedsEntry(SoundFile *p){Event *e=event(17);e->args[0]=normal(p);done();}
static int read_bus(void *o,uint32_t a,unsigned width,uint32_t *v){(void)o;if(a<0x80000000u||(uint64_t)a+width>0x80200000u)return -1;*v=0;for(unsigned i=0;i<width;i++)*v|=(uint32_t)ptr(a)[i]<<(8*i);if(active_cpu&&width==4&&a==active_cpu->pc&&a>=ENTRY&&a<END)seen[(a-ENTRY)/4]++;return 0;}
static int write_bus(void *o,uint32_t a,unsigned width,uint32_t v){(void)o;if(a<0x80000000u||(uint64_t)a+width>0x80200000u)return -1;for(unsigned i=0;i<width;i++)ptr(a)[i]=(uint8_t)(v>>(8*i));return 0;}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t){
 (void)o;unsigned id;switch(t){case 0x8003342c:id=1;break;case 0x80032e88:id=2;break;case 0x800471b4:id=3;break;case 0x800471c4:id=4;break;case 0x8003f8e8:id=5;break;case 0x8003f99c:id=6;break;case 0x800320e8:id=7;break;case 0x8002dd20:id=8;break;case 0x80026338:id=9;break;case 0x80044894:id=10;break;case 0x800445d0:id=11;break;case 0x80028470:id=12;break;case 0x800288ec:id=13;break;case 0x80031bdc:id=14;break;case 0x800295d8:id=15;break;case 0x80028a60:id=16;break;case 0x80038428:id=17;break;default:return 0;}
 Event *e=event(id);uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6];e->args[0]=a;c->gpr[2]=0;
 switch(id){
 case 1:for(unsigned i=1;i<8;i++){e->args[i]=r32(a+4*i);w32(a+4*i,0x81100000u+i);}c->gpr[2]=7;break;
 case 2:e->args[1]=b;c->gpr[2]=decode_return(a);e->args[2]=c->gpr[2];break;
 case 3:opened=a;break;
 case 4:{uint32_t v[5];tim_fields(v);for(unsigned i=0;i<5;i++)w32(a+4*i,v[i]);if(a==STACK-0x98+0x20)e->args[0]=0xffff0000u;c->gpr[2]=a;break;}
 case 5:e->args[1]=b;memset(ptr(a),0,b);break;
 case 6:e->args[1]=b;e->args[2]=d;{uint8_t tmp[0x100];require(d<=sizeof(tmp),"raw bounded memmove");memcpy(tmp,ptr(b),d);memcpy(ptr(a),tmp,d);}c->gpr[2]=a;break;
 case 9:{e->args[1]=b;uint32_t outs[6]={d,c->gpr[7],r32(c->gpr[29]+16),r32(c->gpr[29]+20),r32(c->gpr[29]+24),r32(c->gpr[29]+28)},v[6];atlas_values(v);for(unsigned i=0;i<6;i++){e->args[i+2]=outs[i]-d;w32(outs[i],v[i]);}break;}
 case 10:e->args[1]=b;memcpy(e->payload,ptr(a),8);memcpy(e->payload+2,ptr(b),8);active_portraits++;break;
 case 12:e->args[1]=b;break;
 case 13:c->gpr[2]=0x1000;break;
 case 14:e->args[1]=b;c->gpr[2]=SEDS;break;
 case 15:e->args[1]=b;e->args[2]=d;e->args[3]=c->gpr[7];break;
 }
 done();return 1;
}
static Result run(unsigned native,const uint8_t ids[3],unsigned debug){
 native_mode=native;memset(&result,0,sizeof(result));memset(ram,0x9a,sizeof(ram));memset(menus,0x9a,2*sizeof(*menus));memset(works,0x9a,2*sizeof(*works));memset(managers,0x9a,2*sizeof(*managers));atlas_count=0;active_portraits=0;opened=0;
 for(unsigned k=0;k<sizeof(regions)/sizeof(regions[0]);k++){memset(regions[k].host,pattern?0x35:0xa6,regions[k].size);memset(ptr(regions[k].guest),pattern?0x35:0xa6,regions[k].size);}
 for(unsigned m=0;m<2;m++){
  menus[m].unk32C=&works[m];menus[m].pManager=&managers[m];menus[m].unk2DC=host(ATLAS0);menus[m].unk2E0=host(ATLAS1);menus[m].unk2E4=NULL;menus[m].pShopEntries=NULL;
  works[m].tim.mode=8;works[m].tim.crect=host(ICON+0x10);works[m].tim.caddr=host(ICON+0x40);works[m].tim.prect=host(ICON+0x18);works[m].tim.paddr=host(ICON+0x80);uint32_t tv[5]={8,ICON+0x10,ICON+0x40,ICON+0x18,ICON+0x80};for(unsigned j=0;j<5;j++)w32(workaddr(m)+0xb80+j*4,tv[j]);
  for(unsigned i=0;i<3;i++){managers[m].currentCharacterIDs[i]=ids[(i+m)%3];ptr(manageraddr(m)+0x30)[i]=ids[(i+m)%3];}
  w32(menuaddr(m)+0x32c,workaddr(m));w32(menuaddr(m)+0x33c,manageraddr(m));w32(menuaddr(m)+0x2dc,ATLAS0);w32(menuaddr(m)+0x2e0,ATLAS1);w32(menuaddr(m)+0x2e4,0);w32(menuaddr(m)+0x1e2c,0);
  for(unsigned i=0;i<8;i++){resources[m][i]=i?i*0x101:7;w32(RES0+m*0x100+i*4,resources[m][i]);}
 }
 memcpy(before_menus,menus,2*sizeof(*menus));memcpy(before_works,works,2*sizeof(*works));g_Menu=&menus[0];D_8005945C=resources[0];D_8006259C=host(OLDSEDS);g_MenuDebugEnabled=debug;w32(GLOBAL,MENU0);w32(RESGLOBAL,RES0);w32(SEDSGLOBAL,OLDSEDS);ptr(DEBUGGLOBAL)[0]=debug;
 if(native)ShopMenuLoadResources();else{
  memcpy(ptr(ENTRY),retail,sizeof(retail));memcpy(ptr(0x801c5000),retail_data,16);PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;active_cpu=&cpu;int rc=PcPortMipsRun(&cpu,ENTRY,HALT,3000);active_cpu=NULL;require(rc==PC_PORT_MIPS_HALTED,"retail execution");require(cpu.gpr[29]==STACK,"stack restored");
 }
 result.menu=native?normal(g_Menu):r32(GLOBAL);result.resglobal=native?normal(D_8005945C):r32(RESGLOBAL);result.sedsglobal=native?normal(D_8006259C):r32(SEDSGLOBAL);
 for(unsigned m=0;m<2;m++){
  if(native){result.destinations[m][0]=normal(menus[m].unk2DC);result.destinations[m][1]=normal(menus[m].unk2E0);result.destinations[m][2]=normal(menus[m].unk2E4);result.destinations[m][3]=normal(menus[m].pShopEntries);memcpy(result.resources[m],resources[m],32);memcpy(result.tail[m],&works[m].unk4B94,WORK_TAIL_SIZE);
   before_menus[m].unk2DC=menus[m].unk2DC;before_menus[m].unk2E0=menus[m].unk2E0;before_menus[m].unk2E4=menus[m].unk2E4;before_menus[m].pShopEntries=menus[m].pShopEntries;require(memcmp(&before_menus[m],&menus[m],sizeof(SystemMenu))==0,"unrelated menu bytes");
   before_works[m].tim=works[m].tim;memcpy(&before_works[m].unk4B94,&works[m].unk4B94,WORK_TAIL_SIZE);require(memcmp(&before_works[m],&works[m],sizeof(MenuUnk2))==0,"unrelated work prefix and padding");
  }else{for(unsigned j=0;j<3;j++)result.destinations[m][j]=r32(menuaddr(m)+0x2dc+j*4);result.destinations[m][3]=r32(menuaddr(m)+0x1e2c);memcpy(result.resources[m],ptr(RES0+m*0x100),32);memcpy(result.tail[m],ptr(workaddr(m)+0x4b94),WORK_TAIL_SIZE);}
 }
 return result;
}
int main(int argc,char **argv){
 require(argc==2,"retail module argument");FILE *f=fopen(argv[1],"rb");require(f!=NULL,"retail open");require(fread(retail_data,1,16,f)==16,"rodata read");require(fseek(f,0x4b4,SEEK_SET)==0&&fread(retail,1,sizeof(retail),f)==sizeof(retail),"function read");fclose(f);require(memcmp(D_801C5000,retail_data,16)==0,"actual C data16");
 for(unsigned i=0;i<sizeof(regions)/sizeof(regions[0]);i++){regions[i].host=mmap(NULL,regions[i].size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);require(regions[i].host!=MAP_FAILED,"host allocate");require((uintptr_t)regions[i].host>UINT32_MAX,"high pointer fixture");}
 menus=mmap(NULL,2*sizeof(*menus),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);works=mmap(NULL,2*sizeof(*works),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);managers=mmap(NULL,2*sizeof(*managers),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);require(menus!=MAP_FAILED&&works!=MAP_FAILED&&managers!=MAP_FAILED,"native object maps");require((uintptr_t)menus>UINT32_MAX&&(uintptr_t)works>UINT32_MAX&&(uintptr_t)managers>UINT32_MAX,"high menu/work/manager objects");
 uint8_t *portrait_expected=malloc(PORTRAIT_BYTES);require(portrait_expected!=NULL,"expected allocation");
 const uint8_t ids[][3]={{0,1,2},{255,255,255},{254,255,1},{255,0,255},{10,11,12},{1,1,1},{127,128,253},{0,255,254}};
 for(unsigned id=0;id<8;id++)for(unsigned debug=0;debug<2;debug++)for(mutation=0;mutation<4;mutation++)for(pattern=0;pattern<2;pattern++){
  Result raw=run(0,ids[id],debug);memcpy(portrait_expected,ptr(PORTRAITS),PORTRAIT_BYTES);Result native=run(1,ids[id],debug);
  require(memcmp(&raw,&native,sizeof(raw))==0,"native/raw events and complete destinations/work tail");require(memcmp(portrait_expected,host(PORTRAITS),PORTRAIT_BYTES)==0,"entire portrait blob including untouched bytes");cases++;
 }
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++)require(seen[i]>0,"retail slot coverage");printf("SHOP RESOURCES PASS cases=%u checks=%u instructions=%zu\n",cases,checks,sizeof(seen)/sizeof(seen[0]));return 0;
}
