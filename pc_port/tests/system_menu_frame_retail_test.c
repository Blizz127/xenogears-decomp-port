/* Actual menu frame vs retail 8001C074..8001C1A8. GPU/input/heap boundaries
 * are spies; menu and debug pointer rebinding is adversarial fixture behavior. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#define RAM_SIZE 0x200000
#define ENTRY 0x8001c074u
#define END 0x8001c1a8u
#define MENU0 0x80070000u
#define MENU1 0x80073000u
#define GLOBAL 0x800625a0u
#define DEBUG_GLOBAL 0x8005917cu
#define DEBUG0 0x80060000u
#define HALT 0xfffffffcu
static unsigned char ram[RAM_SIZE], retail[END-ENTRY];
static SystemMenu menus[2], before[2];
SystemMenu *g_Menu;
static int32_t debug_values[2];
s32 *D_8005917C;
static unsigned native_mode, mutation, cases, checks, seen[(END-ENTRY)/4];
static PcPortMipsCpu *active_cpu;
typedef struct { uint32_t id,a,b,c,d; } Event;
typedef struct { Event events[16]; unsigned count; uint32_t env[2],page[2],menu,debug; } Result;
static Result result;
extern void func_8001C074(void);
static void require(int ok,const char *why) {
 checks++; if(!ok) { fprintf(stderr,"SYSTEM MENU FRAME FAIL case=%u native=%u mutation=%u %s\n",cases,native_mode,mutation,why); exit(1); }
}
static unsigned char *ptr(uint32_t a) { return ram+(a&0x1fffffu); }
static uint32_t r32(uint32_t a) { uint32_t v; memcpy(&v,ptr(a),4); return v; }
static void w32(uint32_t a,uint32_t v) { memcpy(ptr(a),&v,4); }
static uint32_t menuaddr(unsigned m) { return m?MENU1:MENU0; }
static uint32_t pointer(const void *p) {
 for(unsigned m=0;m<2;m++) {
  if(p==&menus[m]) return menuaddr(m);
  for(unsigned e=0;e<2;e++) {
   GfxEnvironment *g=&menus[m].gfxEnvs[e]; uint32_t a=menuaddr(m)+0x6c+e*0xb4;
   if(p==g || p==&g->drawEnv) return a;
   if(p==&g->dispEnv) return a+0x5c;
   if(p==g->ot) return a+0x70;
   if(p==&g->ot[15]) return a+0xac;
  }
 }
 require(0,"unrecognized native pointer (PSX offset used on host?)"); return 0;
}
static void event(uint32_t id,uint32_t a,uint32_t b,uint32_t c,uint32_t d) {
 require(result.count<16,"trace overflow"); result.events[result.count++]=(Event){id,a,b,c,d};
 if(mutation==1 && id==1) {
  if(native_mode) g_Menu=&menus[1]; else w32(GLOBAL,MENU1);
 }
 if(mutation==2 && id==3) {
  if(native_mode) { D_8005917C=&debug_values[1]; } else w32(DEBUG_GLOBAL,DEBUG0+4);
 }
 if(mutation==3) {
  unsigned m=result.count%2;
  if(native_mode) g_Menu=&menus[m]; else w32(GLOBAL,menuaddr(m));
 }
}
void MenuProcessControllerInput(void) { event(1,0,0,0,0); }
unsigned long *ClearOTagR(unsigned long *ot,int n) { event(2,pointer(ot),(uint32_t)n,0,0); return ot; }
void HeapDebugDump(u_int a,u_int b,int c,u_int d) { event(3,a,b,(uint32_t)c,d); }
void FontDrawLetters(void *ot) { event(4,pointer(ot),0,0,0); }
int DrawSync(int a) { event(5,(uint32_t)a,0,0,0); return 0; }
int Vsync(int a) { event(6,(uint32_t)a,0,0,0); return 0; }
DRAWENV *PutDrawEnv(DRAWENV *p) { event(7,pointer(p),0,0,0); return p; }
DISPENV *PutDispEnv(DISPENV *p) { event(8,pointer(p),0,0,0); return p; }
void DrawOTag(unsigned long *p) { event(9,pointer(p),0,0,0); }
static int read_bus(void *o,uint32_t a,unsigned width,uint32_t *v) {
 (void)o; if(a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
 *v=0;for(unsigned i=0;i<width;i++) *v|=(uint32_t)ptr(a)[i]<<(i*8);
 if(active_cpu && width==4 && a==active_cpu->pc && a>=ENTRY && a<END) seen[(a-ENTRY)/4]++;
 return 0;
}
static int write_bus(void *o,uint32_t a,unsigned width,uint32_t v) {
 (void)o;if(a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
 for(unsigned i=0;i<width;i++)ptr(a)[i]=(unsigned char)(v>>(i*8));return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
 (void)o;unsigned id;
 switch(t) {
 case 0x8001bf38:id=1;break;case 0x80044ad8:id=2;break;
 case 0x8003278c:id=3;break;case 0x80037324:id=4;break;
 case 0x800445d0:id=5;break;case 0x8004b54c:id=6;break;
 case 0x80044c44:id=7;break;case 0x80044e9c:id=8;break;
 case 0x80044bd0:id=9;break;default:return 0;
 }
 event(id,id==1?0:c->gpr[4],id==2||id==3?c->gpr[5]:0,id==3?c->gpr[6]:0,id==3?c->gpr[7]:0);
 c->gpr[2]=0;return 1;
}
static Result run(unsigned native,unsigned initial,int32_t page,int32_t debug,unsigned flag) {
 native_mode=native;memset(&result,0,sizeof(result));memset(ram,0,sizeof(ram));memset(menus,0,sizeof(menus));
 for(unsigned m=0;m<2;m++) {
  unsigned env=(initial+m)%3;uint32_t a=menuaddr(m);
  menus[m].pGfxEnv=env==2?NULL:&menus[m].gfxEnvs[env];
  menus[m].renderContext=page;menus[m].unk1E94=flag;menus[m].unk1E95=(u8)(17+m*201);
  w32(a+0x1d4,env==2?0:a+0x6c+env*0xb4);w32(a+0x308,(uint32_t)page);
  ptr(a)[0x1e94]=flag;ptr(a)[0x1e95]=(unsigned char)(17+m*201);
 }
 /* Rebinding targets must have valid submitted environments even if the entry
  * starts with NULL; retail replaces any non-first environment at entry. */
 if(initial==2) {menus[1].pGfxEnv=&menus[1].gfxEnvs[0];w32(MENU1+0x1d4,MENU1+0x6c);}
 memcpy(before,menus,sizeof(menus));
 g_Menu=&menus[0];D_8005917C=&debug_values[0];debug_values[0]=debug;debug_values[1]=-1;
 w32(GLOBAL,MENU0);w32(DEBUG_GLOBAL,DEBUG0);w32(DEBUG0,(uint32_t)debug);w32(DEBUG0+4,0xffffffffu);
 if(native) func_8001C074();
 else {
  memcpy(ptr(ENTRY),retail,sizeof(retail));
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
  PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000;cpu.gpr[31]=HALT;active_cpu=&cpu;
  int rc=PcPortMipsRun(&cpu,ENTRY,HALT,1000);active_cpu=NULL;
  require(rc==PC_PORT_MIPS_HALTED,"retail execution");
 }
 for(unsigned m=0;m<2;m++) {
  result.env[m]=native?(menus[m].pGfxEnv?pointer(menus[m].pGfxEnv):0):r32(menuaddr(m)+0x1d4);
  result.page[m]=native?(uint32_t)menus[m].renderContext:r32(menuaddr(m)+0x308);
  if(native) {
   before[m].pGfxEnv=menus[m].pGfxEnv;before[m].renderContext=menus[m].renderContext;
   require(memcmp(&before[m],&menus[m],sizeof(SystemMenu))==0,"unrelated native menu bytes");
  }
 }
 result.menu=native?pointer(g_Menu):r32(GLOBAL);
 result.debug=native?(D_8005917C==&debug_values[1]?1:0):(r32(DEBUG_GLOBAL)==DEBUG0+4?1:0);
 return result;
}
int main(int argc,char **argv) {
 require(argc==2,"retail SLUS argument");FILE *f=fopen(argv[1],"rb");require(f!=NULL,"open retail");
 require(fseek(f,0xc874,SEEK_SET)==0 && fread(retail,1,sizeof(retail),f)==sizeof(retail),"read retail");fclose(f);
 int32_t pages[]={0,1,-1,2,INT32_MIN},debugs[]={-1,0,123};
 for(unsigned e=0;e<3;e++)for(unsigned p=0;p<5;p++)for(unsigned d=0;d<3;d++)for(unsigned flag=0;flag<2;flag++)for(mutation=0;mutation<4;mutation++) {
  /* A rebind after input must target an initialized environment. */
  if(mutation==3 && e!=0) continue;
  Result raw=run(0,e,pages[p],debugs[d],flag),native=run(1,e,pages[p],debugs[d],flag);
  require(memcmp(&raw,&native,sizeof(raw))==0,"native vs retail state/helper trace");cases++;
 }
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++)require(seen[i]>0,"retail instruction coverage");
 printf("SYSTEM MENU FRAME PASS cases=%u checks=%u instructions=%zu\n",cases,checks,sizeof(seen)/sizeof(seen[0]));return 0;
}
