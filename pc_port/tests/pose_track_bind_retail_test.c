#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 func_801DF0B4(u8*,u8*,u8*,s32,s32,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 pool[2],nodes[5*31],tracks[32*5];u16 pose[128];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801DF0B4){fputs("POSE TRACK BIND FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const unsigned counts[]={1,2,5},limits[4][2]={{0,5},{1,1},{5,0},{5,5}};const s32 durations[]={0,65536,-1};unsigned cases=0;
 for(unsigned n=0;n<3;++n)for(unsigned flags=0;flags<4;++flags)for(unsigned limit=0;limit<4;++limit)for(unsigned gate=0;gate<9;++gate)for(unsigned equal=0;equal<2;++equal)for(unsigned d=0;d<3;++d)for(unsigned mode=0;mode<2;++mode)for(unsigned availability=0;availability<3;++availability)for(unsigned prefix=0;prefix<2;++prefix){
  memset(&fixture,0xa5,sizeof(fixture));fixture.pool[0]=(u32)(uintptr_t)fixture.tracks;*(u16*)((u8*)fixture.pool+4)=availability==0?16:availability==1?32:0;*(u16*)((u8*)fixture.pool+6)=32;
  for(unsigned i=0;i<32;++i)((u8*)fixture.tracks)[i*20]=i<16?1:0;
  u8*root=(u8*)fixture.nodes;*(u16*)(root+0xa)=counts[n];
  for(unsigned i=0;i<5;++i)for(unsigned ch=0;ch<2;++ch){u8*node=root+i*124;unsigned kind=(gate/(ch?3:1))%3;u8*track=(u8*)fixture.tracks+(i*2+ch)*20;track[3]=kind==2?0xff:0xfe;*(u32*)(node+0x70+ch*4)=kind?(u32)(uintptr_t)track:0;for(unsigned axis=0;axis<3;++axis){if(ch)*(s32*)(node+0x5c+axis*4)=equal?-32768:(s32)(0x12340000u+i*313+axis*7901);else *(s16*)(node+0x54+axis*2)=equal?-32768:(s16)(i*1111+axis*2048);}}
  fixture.pose[2]=flags;fixture.pose[3]=prefix?0:1;fixture.pose[6]=limits[limit][0];fixture.pose[7]=limits[limit][1];for(unsigned i=12;i<128;++i)fixture.pose[i]=equal?0x8000:(u16)(i*7919u+0x8765u);
  initial=fixture;PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)fixture.pool;cpu.gpr[5]=(u32)(uintptr_t)root;cpu.gpr[6]=(u32)(uintptr_t)fixture.pose;cpu.gpr[7]=durations[d];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  s32 absolute=mode?3:2,loop=mode?2:3,tag=(s32)(0xabc00000u|gate*31u);assert(!wr(NULL,cpu.gpr[29]+16,4,absolute));assert(!wr(NULL,cpu.gpr[29]+20,4,loop));assert(!wr(NULL,cpu.gpr[29]+24,4,tag));
  if(PcPortMipsRun(&cpu,0x801df0b4,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"POSE TRACK BIND FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;u32 result=cpu.gpr[2];fixture=initial;u32 actual=func_801DF0B4((u8*)fixture.pool,root,(u8*)fixture.pose,durations[d],absolute,loop,tag);
  if(actual!=result||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"POSE TRACK BIND FAIL n=%u flags=%u limit=%u gate=%u equal=%u duration=%u mode=%u available=%u prefix=%u result=%x/%x\n",counts[n],flags,limit,gate,equal,d,mode,availability,prefix,actual,result);return 1;}++cases;
 }
 printf("POSE TRACK BIND PASS %u cases; real pool claim/release and full memory/return\n",cases);
}
