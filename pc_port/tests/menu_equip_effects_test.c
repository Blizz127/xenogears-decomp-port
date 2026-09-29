/* Original retail MIPS versus the full production translation unit. */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
GameState g_GameState;
static MenuUnk6 resources;
static u8 weapons[4096],accessories[4096],ram[0x200000];
extern void func_801E36D4(void*,u8);
static int memory(u32 a,unsigned w,u32*v,int write){
 u8*p=NULL;
 if(w==4&&(a==0x80101000||a==0x80101004)){if(write)return -1;*v=a==0x80101000?0x80102000:0x80103000;return 0;}
 if(a>=0x8006d634&&(uint64_t)a+w<=0x8006d634+sizeof g_GameState)p=(u8*)&g_GameState+a-0x8006d634;
 else if(a>=0x80102000&&(uint64_t)a+w<=0x80103000)p=weapons+a-0x80102000;
 else if(a>=0x80103000&&(uint64_t)a+w<=0x80104000)p=accessories+a-0x80103000;
 else if(a>=0x801c5000&&(uint64_t)a+w<=0x80200000&&!write)p=ram+(a&0x1fffff);
 if(!p)return -1;
 if(write){for(unsigned i=0;i<w;i++)p[i]=*v>>(8*i);}else{*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(8*i);}return 0;
}
static int rd(void*u,u32 a,unsigned w,u32*v){return memory(a,w,v,0);}
static int wr(void*u,u32 a,unsigned w,u32 v){return memory(a,w,&v,1);}
static void pattern(void*p,size_t n,unsigned seed){u8*b=p;for(size_t i=0;i<n;i++){seed=seed*1664525+1013904223;b[i]=seed>>24;}}
static void init(unsigned seed,unsigned ch,unsigned type,unsigned flags,unsigned special){
 pattern(&g_GameState,sizeof g_GameState,seed);pattern(weapons,sizeof weapons,seed+1);pattern(accessories,sizeof accessories,seed+2);memset(&resources,0,sizeof resources);
 resources.pWeaponsData=(void*)weapons;resources.pAccessoriesData=(void*)accessories;
 u8*c=(u8*)&g_GameState.characters[ch];c[0x56]=special&1?4:3;
 for(unsigned i=0;i<256;i++){
  if(type < 256){accessories[i*16+9]=type;accessories[i*16+13]=flags;}
  /* Both conditional branches, with arbitrary 16-bit mask and bonus values. */
  weapons[i*16+11]=special&2?100:99;
 }
}
int main(void){
 FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;size_t n=fread(ram+0x1c5000,1,0x3b000,f);fclose(f);if(n<0x1ea80)return 2;
 unsigned cases=0;
 for(unsigned type=0;type<257;type++)for(unsigned flags=0;flags<256;flags++)for(unsigned special=0;special<4;special++){
  unsigned seed=type*1024+flags*4+special,ch=seed%MAX_GAME_CHARACTERS;
  init(seed,ch,type,flags,special);PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
  c.gpr[4]=0x80101000;c.gpr[5]=0xffffff00|ch;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  if(PcPortMipsRun(&c,0x801e36d4,0xfffffffc,3000)){fprintf(stderr,"FAIL retail %s\n",c.error);return 1;}
  GameState expected=g_GameState;u8 ew[4096],ea[4096];memcpy(ew,weapons,sizeof ew);memcpy(ea,accessories,sizeof ea);MenuUnk6 er=resources;
  init(seed,ch,type,flags,special);func_801E36D4(&resources,ch);
  if(memcmp(&expected,&g_GameState,sizeof expected)||memcmp(ew,weapons,sizeof ew)||memcmp(ea,accessories,sizeof ea)||memcmp(&er,&resources,sizeof er)){
   fprintf(stderr,"FAIL equip effects type=%u flags=%u special=%u ch=%u\n",type,flags,special,ch);return 1;
  }cases++;
 }
 printf("EQUIP EFFECTS PASS cases=%u\n",cases);return 0;
}
