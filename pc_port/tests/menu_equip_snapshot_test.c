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
GameState g_GameState;
static SystemMenu menu;
static MenuManager manager;
static MenuUnk6 resources;
static u8 work[0x400],ram[0x200000];
extern void func_801DF5D0(s32,s32);
extern void func_801DF890(s32,s32);
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&(a==0x800625a0||a==0x80100330||a==0x8010033c||a==0x80100360)){
  if(write)return -1;
  *v=a==0x800625a0?0x80100000:a==0x80100330?0x80101000:a==0x8010033c?0x80102000:0x80103000;return 0;
 }
 if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof g_GameState)p=(u8*)&g_GameState+a-0x8006d634;
 else if(a>=0x801010b8&&(uint64_t)a+w<=0x801010ca)p=(u8*)&resources.unkB8+a-0x801010b8;
 else if(a>=0x80102030&&(uint64_t)a+w<=0x80102033)p=manager.currentCharacterIDs+a-0x80102030;
 else if(a>=0x80103000&&(uint64_t)a+w<=0x80103400)p=work+a-0x80103000;
 else if(a>=0x801c5000&&(uint64_t)a+w<=0x80200000&&!write)p=ram+(a&0x1fffff);
 if(!p)return -1;
 if(write){for(unsigned i=0;i<w;i++)p[i]=*v>>(8*i);}else{*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(8*i);}return 0;
}
static int rd(void*u,u32 a,unsigned w,u32*v){return memory(a,w,v,0);}
static int wr(void*u,u32 a,unsigned w,u32 v){return memory(a,w,&v,1);}
static void pattern(void*p,size_t n,unsigned seed){u8*b=p;for(size_t i=0;i<n;i++){seed=seed*1664525+1013904223;b[i]=seed>>24;}}
static void init(unsigned seed,unsigned ch,unsigned gear,unsigned slot){
 pattern(&g_GameState,sizeof g_GameState,seed);pattern(work,sizeof work,seed+1);pattern(&resources,sizeof resources,seed+2);
 memset(&menu,0,sizeof menu);memset(&manager,0,sizeof manager);g_Menu=&menu;menu.pManager=&manager;menu.unk330=&resources;
 u32 ptr=(u32)(uintptr_t)work;memcpy(&menu.unk358[8],&ptr,4); /* retail g_Menu+0x360 == native unk358[8] */
 manager.currentCharacterIDs[slot]=ch;((u8*)&g_GameState)[0x30c+ch*0xa4]=gear;
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1ab68)return 2;
 unsigned cases=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned ch=0;ch<MAX_GAME_CHARACTERS;ch++)for(unsigned gear=0;gear<MAX_GAME_GEARS;gear++)for(unsigned slot=0;slot<3;slot++)for(unsigned mode=0;mode<2;mode++)for(unsigned cancel=0;cancel<2;cancel++){
  init(seed,ch,gear,slot);PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
  u32 upper=seed*0x55555500, a=upper|slot,b=upper|(mode?(seed*84+1):0);
  c.gpr[4]=a;c.gpr[5]=b;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  if(PcPortMipsRun(&c,cancel?0x801df890:0x801df5d0,0xfffffffc,3000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  GameState expected=g_GameState;u8 ew[sizeof work];memcpy(ew,work,sizeof work);MenuUnk6 er=resources;SystemMenu em=menu;MenuManager en=manager;
  init(seed,ch,gear,slot);if(cancel)func_801DF890(a,b);else func_801DF5D0(a,b);
  if(memcmp(&expected,&g_GameState,sizeof expected)||memcmp(ew,work,sizeof work)||memcmp(&er,&resources,sizeof er)||memcmp(&em,&menu,sizeof em)||memcmp(&en,&manager,sizeof en)){
   fprintf(stderr,"FAIL equip snapshot seed=%u ch=%u gear=%u slot=%u mode=%u cancel=%u\n",seed,ch,gear,slot,mode,cancel);return 1;
  }cases++;
 }
 printf("EQUIP SNAPSHOT PASS cases=%u\n",cases);return 0;
}
