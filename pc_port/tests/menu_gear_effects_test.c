/* Compare the production Equip snapshot/cancel helpers to original retail MIPS. */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
SystemMenu* g_Menu;
u8 stateStorage[0x4600] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");
static SystemMenu menu;
static MenuManager manager;
static MenuUnk6 resources;
static u8 work[0x400],ram[0x200000];
extern void func_801E433C(void*,u8);
static u8 items[256*28];
static unsigned callCount,callId,callbackValue,callbackMutation;
u8 D_801E9808[256];
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&a==0x80101014){if(write)return -1;*v=0x80110000;return 0;}
 if(w==4&&(a==0x800625a0||a==0x80100330||a==0x8010033c||a==0x80100360)){
  if(write)return -1;
  *v=a==0x800625a0?0x80100000:a==0x80100330?0x80101000:a==0x8010033c?0x80102000:0x80103000;return 0;
 }
 if(a>=0x80110000&&(uint64_t)a+w<=0x80111c00)p=items+a-0x80110000;
 else if(a>=0x801e9808&&(uint64_t)a+w<=0x801e9908)p=D_801E9808+a-0x801e9808;
 else if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof stateStorage)p=stateStorage+a-0x8006d634;
 else if(a>=0x8010109c&&(uint64_t)a+w<=0x801010b7)p=resources.unk20+0x7c+a-0x8010109c;
 else if(a>=0x80102030&&(uint64_t)a+w<=0x80102033)p=manager.currentCharacterIDs+a-0x80102030;
 else if(a>=0x80103000&&(uint64_t)a+w<=0x80103400)p=work+a-0x80103000;
 else if(a>=0x801c5000&&(uint64_t)a+w<=0x80200000)p=ram+(a&0x1fffff);
 if(!p)return -1;
 if(write){for(unsigned i=0;i<w;i++)p[i]=*v>>(8*i);}else{*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(8*i);}return 0;
}
static int rd(void*u,u32 a,unsigned w,u32*v){return memory(a,w,v,0);}
static int wr(void*u,u32 a,unsigned w,u32 v){return memory(a,w,&v,1);}
static void pattern(void*p,size_t n,unsigned seed){u8*b=p;for(size_t i=0;i<n;i++){seed=seed*1664525+1013904223;b[i]=seed>>24;}}
static void init(unsigned seed,unsigned ch,unsigned gear,unsigned slot){
 pattern(stateStorage,sizeof stateStorage,seed);pattern(work,sizeof work,seed+1);pattern(&resources,sizeof resources,seed+2);
 memset(&menu,0,sizeof menu);memset(&manager,0,sizeof manager);g_Menu=&menu;menu.pManager=&manager;menu.unk330=&resources;
 u32 ptr=(u32)(uintptr_t)work;memcpy(&menu.unk358[8],&ptr,4); /* retail g_Menu+0x360 == native unk358[8] */
 manager.currentCharacterIDs[slot]=ch;(stateStorage)[0x30c+ch*0xa4]=gear;
}
u8 func_801E4928(u8 id){callCount++;callId=id; if(callbackMutation){D_801E9808[id]=(D_801E9808[id]+1)%MAX_GAME_CHARACTERS;stateStorage[0x30c+D_801E9808[id]*0xa4]=id;} return callbackValue;}
static int bridge(void*u,PcPortMipsCpu*c,u32 target){if(target!=0x801e4928)return 0;c->gpr[2]=func_801E4928(c->gpr[4]);return 1;}
static void prepare(unsigned seed,unsigned type,unsigned id){
 init(seed,0,0,0);pattern(items,sizeof items,seed+type*64);
 for(unsigned i=0;i<256;i++)D_801E9808[i]=(seed+i)%MAX_GAME_CHARACTERS;
 u32 ptr=(u32)(uintptr_t)items;memcpy(resources.unk8+0xc,&ptr,4);
 for(unsigned i=0;i<256;i++){
  if(type<256)items[i*28+0x15]=type;
  if(seed<16){u16 mask=1u<<seed;memcpy(items+i*28+0x16,&mask,2);}
 }
 u8*gear=stateStorage+0x978+id*0xa4;unsigned ch=D_801E9808[id];
 /* Exhaust both prerequisites independently for the three conditional effects. */
 u16 f=((seed&1)?0x1000:0)|((seed&2)?0x800:0)|((seed&4)?0x200:0)|((seed&8)?0x100:0)|((seed&16)?0x40:0)|((seed&32)?0x20:0)|0x490;
 memcpy(stateStorage+0x16c4+ch*32,&f,2);
 stateStorage[0x30c+ch*0xa4]=seed&1?id:(id+1)%MAX_GAME_GEARS;
 callbackMutation=seed>=64;callbackValue=seed*13+type;callCount=callId=0;
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1f754)return 2;
 unsigned cases=0;
 for(unsigned type=0;type<257;type++)for(unsigned seed=0;seed<128;seed++){
  unsigned id=(type+seed)%MAX_GAME_GEARS;prepare(seed,type,id);
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);c.gpr[4]=0x80101000;c.gpr[5]=0xffffff00|id;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  if(PcPortMipsRun(&c,0x801e433c,0xfffffffc,3000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  u8 expected[sizeof stateStorage],ei[sizeof items],em[256];memcpy(expected,stateStorage,sizeof expected);memcpy(ei,items,sizeof ei);memcpy(em,D_801E9808,256);MenuUnk6 er=resources;unsigned ec=callCount,eg=callId;
  prepare(seed,type,id);func_801E433C(&resources,id);
  if(callCount!=ec||callId!=eg||memcmp(expected,stateStorage,sizeof expected)||memcmp(ei,items,sizeof ei)||memcmp(em,D_801E9808,256)||memcmp(&er,&resources,sizeof er)){
   fprintf(stderr,"FAIL gear effects type=%u seed=%u id=%u\n",type,seed,id);return 1;
  }cases++;
 }
 printf("GEAR EFFECTS PASS cases=%u\n",cases);return 0;
}
