#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
SystemMenu* g_Menu;
GameState g_GameState;
u8 D_801EA730[512];
static SystemMenu menu;
static MenuManager manager;
static u8 ram[0x200000];
extern void func_801DFB68(s32,s32,s32,s32,s32,s32);
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&(a==0x800625a0||a==0x8010033c)){if(write)return -1;*v=a==0x800625a0?0x80100000:0x80102000;return 0;}
 if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof g_GameState)p=(u8*)&g_GameState+a-0x8006d634;
 else if(a>=0x80102030&&(uint64_t)a+w<=0x80102033)p=manager.currentCharacterIDs+a-0x80102030;
 else if(a>=0x801ea730&&(uint64_t)a+w<=0x801ea930)p=D_801EA730+a-0x801ea730;
 else if(a>=0x801c5000&&(uint64_t)a+w<=0x80200000)p=ram+(a&0x1fffff);
 if(!p)return -1;
 if(write){for(unsigned i=0;i<w;i++)p[i]=*v>>(8*i);}else{*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(8*i);}return 0;
}
static int rd(void*u,u32 a,unsigned w,u32*v){return memory(a,w,v,0);}
static int wr(void*u,u32 a,unsigned w,u32 v){return memory(a,w,&v,1);}
static void pattern(void*p,size_t n,unsigned seed){u8*b=p;for(size_t i=0;i<n;i++){seed=seed*1664525+1013904223;b[i]=seed>>24;}}
static void init(unsigned seed,unsigned ch,unsigned gear,unsigned slot){
 pattern(&g_GameState,sizeof g_GameState,seed);pattern(D_801EA730,sizeof D_801EA730,seed+1);memset(&menu,0,sizeof menu);memset(&manager,0,sizeof manager);g_Menu=&menu;menu.pManager=&manager;
 manager.currentCharacterIDs[slot]=ch;((u8*)&g_GameState)[0x30c+ch*0xa4]=gear;
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1ae2c)return 2;
 unsigned cases=0;
 for(unsigned seed=0;seed<512;seed++)for(int cat=-1;cat<=5;cat++)for(unsigned mode=0;mode<2;mode++)for(unsigned group=0;group<2;group++){
  unsigned ch=seed%MAX_GAME_CHARACTERS,gear=(seed/3)%MAX_GAME_GEARS,slot=seed%3;init(seed,ch,gear,slot);
  u32 args[]={0xffffff00|slot,(u32)cat,(u32)(seed-17),17,0xffffff00|group,0x12345600|mode};
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);for(unsigned i=0;i<4;i++)c.gpr[4+i]=args[i];
  c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;wr(NULL,0x801ff010,4,args[4]);wr(NULL,0x801ff014,4,args[5]);
  if(PcPortMipsRun(&c,0x801dfb68,0xfffffffc,1000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  GameState expected=g_GameState;u8 el[512];memcpy(el,D_801EA730,sizeof el);SystemMenu em=menu;MenuManager en=manager;
  init(seed,ch,gear,slot);func_801DFB68(args[0],args[1],args[2],args[3],args[4],args[5]);
  if(memcmp(&expected,&g_GameState,sizeof expected)||memcmp(el,D_801EA730,sizeof el)||memcmp(&em,&menu,sizeof em)||memcmp(&en,&manager,sizeof en)){
   fprintf(stderr,"FAIL equip preview seed=%u cat=%d mode=%u group=%u\n",seed,cat,mode,group);return 1;
  }cases++;
 }
 printf("EQUIP PREVIEW PASS cases=%u\n",cases);return 0;
}
