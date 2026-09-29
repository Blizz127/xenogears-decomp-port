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
extern void func_801DFE2C(u8);
static MenuUnk6 extra;
static u32 events[2][3];
static unsigned calls,selected,mutate;
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&(a==0x800625a0||a==0x80100330||a==0x8010033c||a==0x80100360)){
  if(write)return -1;
  *v=a==0x800625a0?0x80100000:a==0x80100330?(menu.unk330==&resources?0x80101000:0x80104000):a==0x8010033c?0x80102000:0x80103000;return 0;
 }
 if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof g_GameState)p=(u8*)&g_GameState+a-0x8006d634;
 else if(a>=0x80101020&&(uint64_t)a+w<=0x801010b8)p=resources.unk20+a-0x80101020;
 else if(a>=0x80104020&&(uint64_t)a+w<=0x801040b8)p=extra.unk20+a-0x80104020;
 else if(a>=0x801040b8&&(uint64_t)a+w<=0x801040ca)p=(u8*)&extra.unkB8+a-0x801040b8;
 else if(a>=0x801010b8&&(uint64_t)a+w<=0x801010ca)p=(u8*)&resources.unkB8+a-0x801010b8;
 else if(a>=0x80102030&&(uint64_t)a+w<=0x80102033)p=manager.currentCharacterIDs+a-0x80102030;
 else if(a>=0x80103000&&(uint64_t)a+w<=0x80103400)p=work+a-0x80103000;
 else if(a>=0x801c5000&&(uint64_t)a+w<=0x80200000 )p=ram+(a&0x1fffff);
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
static void callback(unsigned which,void*p,u8 gear){
 if(calls>=2)abort();events[calls][0]=which;events[calls][1]=p==&resources?0x80101000:p==&extra?0x80104000:0;events[calls++][2]=gear;
 if(mutate){
  if(which==0){manager.currentCharacterIDs[selected]=(manager.currentCharacterIDs[selected]+1)%MAX_GAME_CHARACTERS;menu.unk330=&extra;}
  else{menu.unk330=&resources;resources.unk20[0x90]^=0x7b;}
 }
}
void func_801E3ECC(void*p,u8 gear){callback(0,p,gear);}
void func_801E3C2C(void*p,u8 gear){callback(1,p,gear);}
static int bridge(void*u,PcPortMipsCpu*c,u32 target){
 if(target!=0x801e3ecc&&target!=0x801e3c2c)return 0;
 void*p=c->gpr[4]==0x80101000?&resources:c->gpr[4]==0x80104000?&extra:NULL;
 callback(target==0x801e3ecc?0:1,p,c->gpr[5]);return 1;
}
static void prepare(unsigned seed,unsigned ch){
 init(seed,ch,0,selected);pattern(&extra,sizeof extra,seed+9);calls=0;memset(events,0,sizeof events);
 for(unsigned i=0;i<MAX_GAME_CHARACTERS;i++)((u8*)&g_GameState)[0x30c+i*0xa4]=(seed+i)%MAX_GAME_GEARS;
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1af5c)return 2;
 unsigned cases=0;
 for(unsigned seed=0;seed<256;seed++)for(unsigned ch=0;ch<MAX_GAME_CHARACTERS;ch++)for(selected=0;selected<3;selected++)for(mutate=0;mutate<2;mutate++){
  prepare(seed,ch);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);c.gpr[4]=0xffffff00|selected;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  if(PcPortMipsRun(&c,0x801dfe2c,0xfffffffc,1000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  MenuUnk6 er=resources,ee=extra;GameState eg=g_GameState;MenuManager en=manager;SystemMenu em=menu;u32 ev[2][3];memcpy(ev,events,sizeof ev);unsigned ec=calls;
  prepare(seed,ch);func_801DFE2C(selected);
  if(calls!=ec||memcmp(ev,events,sizeof ev)||memcmp(&er,&resources,sizeof er)||memcmp(&ee,&extra,sizeof ee)||memcmp(&eg,&g_GameState,sizeof eg)||memcmp(&en,&manager,sizeof en)||memcmp(&em,&menu,sizeof em)){
   fprintf(stderr,"FAIL gear caller seed=%u ch=%u slot=%u mutate=%u\n",seed,ch,selected,mutate);return 1;
  }cases++;
 }
 printf("GEAR CALLER PASS cases=%u\n",cases);return 0;
}
