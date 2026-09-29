#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 func_801DF7F4(u8*,u8*,u8*,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 pool[2],nodes[5*31],tracks[32*5];u16 pose[128];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801DF7F4){fputs("STREAM TRACK BIND FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const unsigned counts[]={1,2,5},limits[]={0,1,5},frames[]={0,1,65535};unsigned cases=0;
 for(unsigned n=0;n<3;++n)for(unsigned flags=0;flags<4;++flags)for(unsigned limit=0;limit<3;++limit)for(unsigned gate=0;gate<9;++gate)for(unsigned pattern=0;pattern<4;++pattern)for(unsigned frame=0;frame<3;++frame)for(unsigned loop=0;loop<2;++loop)for(unsigned availability=0;availability<3;++availability)for(unsigned immediate=0;immediate<2;++immediate){
  memset(&fixture,0xa5,sizeof(fixture));fixture.pool[0]=(u32)(uintptr_t)fixture.tracks;*(u16*)((u8*)fixture.pool+4)=availability==0?16:availability==1?32:0;*(u16*)((u8*)fixture.pool+6)=32;for(unsigned i=0;i<32;++i)((u8*)fixture.tracks)[i*20]=i<16?1:0;
  u8*root=(u8*)fixture.nodes;*(u16*)(root+0xa)=counts[n];for(unsigned i=0;i<5;++i)for(unsigned ch=0;ch<2;++ch){unsigned kind=(gate/(ch?3:1))%3;u8*track=(u8*)fixture.tracks+(i*2+ch)*20;track[3]=kind==2?0xff:0xfe;*(u32*)(root+i*124+0x70+ch*4)=kind?(u32)(uintptr_t)track:0;}
  for(unsigned i=0;i<5;++i)*(u32*)(root+i*124+0x78)=0;
  fixture.pose[1]=frames[frame];fixture.pose[2]=flags;fixture.pose[3]=immediate;fixture.pose[6]=limits[limit];fixture.pose[7]=limits[2-limit];for(unsigned i=0;i<6;++i){fixture.pose[12+i*3]=(pattern&1)?0xffff:(u16)(i*13);fixture.pose[13+i*3]=(pattern&2)?0xffff:(u16)(0xfffe - i*17);fixture.pose[14+i*3]=(u16)(0xf123+i*37);}
  initial=fixture;PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)fixture.pool;cpu.gpr[5]=(u32)(uintptr_t)root;cpu.gpr[6]=(u32)(uintptr_t)fixture.pose;cpu.gpr[7]=loop+2;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;s32 tag=(s32)(0x12340000u+gate*31);assert(!wr(NULL,cpu.gpr[29]+16,4,tag));
  if(PcPortMipsRun(&cpu,0x801df7f4,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"STREAM TRACK BIND FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;u32 result=cpu.gpr[2];fixture=initial;u32 actual=func_801DF7F4((u8*)fixture.pool,root,(u8*)fixture.pose,loop+2,tag);
  if(actual!=result||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"STREAM TRACK BIND FAIL n=%u flags=%u limit=%u gate=%u pattern=%u frame=%u loop=%u availability=%u immediate=%u result=%x/%x\n",counts[n],flags,limit,gate,pattern,frame,loop,availability,immediate,actual,result);return 1;}++cases;
 }
 printf("STREAM TRACK BIND PASS %u cases; real pose/pool/release and full memory/return\n",cases);
}
