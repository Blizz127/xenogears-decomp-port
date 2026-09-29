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
extern s32 func_801DF0D4(s32,s32,s32,s32);
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&(a==0x800625a0||a==0x80100330||a==0x8010033c||a==0x80100360)){
  if(write)return -1;
  *v=a==0x800625a0?0x80100000:a==0x80100330?0x80101000:a==0x8010033c?0x80102000:0x80103000;return 0;
 }
 if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof stateStorage)p=stateStorage+a-0x8006d634;
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
 pattern(stateStorage,sizeof stateStorage,seed);pattern(work,sizeof work,seed+1);pattern(&resources,sizeof resources,seed+2);
 memset(&menu,0,sizeof menu);memset(&manager,0,sizeof manager);g_Menu=&menu;menu.pManager=&manager;menu.unk330=&resources;
 u32 ptr=(u32)(uintptr_t)work;memcpy(&menu.unk358[8],&ptr,4); /* retail g_Menu+0x360 == native unk358[8] */
 manager.currentCharacterIDs[slot]=ch;(stateStorage)[0x30c+ch*0xa4]=gear;
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1a5d0)return 2;
 unsigned cases=0;
 for(unsigned seed=0;seed<512;seed++)for(unsigned cat=0;cat<5;cat++)for(unsigned mode=0;mode<2;mode++)for(unsigned group=0;group<2;group++){
  unsigned ch=seed%MAX_GAME_CHARACTERS,gear=(seed/3)%MAX_GAME_GEARS,slot=seed%3;
  init(seed,ch,gear,slot);
  u8*st=stateStorage;unsigned base=(mode?gear:ch)*0xa4;
  unsigned off=group?(mode?0x97c:0x2db)+cat:cat?(mode?0x980:0x2df)+cat:(mode?0x984:0x2d6);
  unsigned wo=group?0x2a1+cat:cat?0x2a5+cat:0x29c;
  unsigned ids=mode?0x2120:0x1d9c,limit=100,counts=ids-100;
  if(!group&&cat){ids=mode?0x221a:0x1ec8;counts=mode?0x2184:0x1e00;limit=mode?150:200;}
  u8 current=seed%8?17:0,previous=seed%7?29:0;
  st[base+off]=current;work[wo]=previous;
  /* Target first/last/missing/duplicate matches and empty/full tables. */
  if(seed&1){memset(st+ids,45,limit);memset(st+counts,seed,limit);}
  if(seed&2)st[ids+(seed%limit)]=current;
  if(seed&4)st[ids+limit-1]=previous;
  if(seed&8)st[ids+(seed*3%limit)]=0;
  if(seed&16){st[ids]=current;st[ids+1]=current;}
  if(group)st[(mode?0x22b6:0x2286)+current]=(u8[]){0,99,100,255}[seed%4];
  u8 initial[sizeof stateStorage];memcpy(initial,stateStorage,sizeof initial);u8 iw[sizeof work];memcpy(iw,work,sizeof work);
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
  c.gpr[4]=0xffffff00|slot;c.gpr[5]=0xffffff00|cat;c.gpr[6]=0xffffff00|group;c.gpr[7]=0xffffff00|mode;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  if(PcPortMipsRun(&c,0x801df0d4,0xfffffffc,20000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  u8 expected[sizeof stateStorage];memcpy(expected,stateStorage,sizeof expected);u8 ew[sizeof work];memcpy(ew,work,sizeof work);MenuUnk6 er=resources;SystemMenu em=menu;MenuManager en=manager;
  memcpy(stateStorage,initial,sizeof initial);memcpy(work,iw,sizeof work);s32 result=func_801DF0D4(0xffffff00|slot,0xffffff00|cat,0xffffff00|group,0xffffff00|mode);
  if((u32)result!=c.gpr[2]||memcmp(&expected,stateStorage,sizeof expected)||memcmp(ew,work,sizeof work)||memcmp(&er,&resources,sizeof er)||memcmp(&em,&menu,sizeof em)||memcmp(&en,&manager,sizeof en)){
   fprintf(stderr,"FAIL equip commit seed=%u cat=%u mode=%u group=%u result=%d/%u\n",seed,cat,mode,group,result,c.gpr[2]);return 1;
  }cases++;
 }
 printf("EQUIP COMMIT PASS cases=%u\n",cases);return 0;
}
