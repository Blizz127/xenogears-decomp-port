#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801DEF10(u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 nodes[65*124/4];u32 tracks[2];u16 pose[1024];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801DEF10){fputs("POSE APPLY FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const unsigned nodeCounts[]={1,2,4,65},limits[]={0,1,2,70};const s16 triple[]={-32768,32767,-1};unsigned cases=0;
 for(unsigned n=0;n<4;++n)for(unsigned flags=0;flags<4;++flags)for(unsigned r=0;r<4;++r)for(unsigned t=0;t<4;++t)for(unsigned gate=0;gate<9;++gate)for(unsigned equal=0;equal<4;++equal)for(unsigned skip=0;skip<2;++skip)for(unsigned pattern=0;pattern<2;++pattern){
  memset(&fixture,0xa5,sizeof(fixture));u8*root=(u8*)fixture.nodes;*(u16*)(root+0xa)=nodeCounts[n];fixture.tracks[0]=(gate%3==2?0xff000000:0xfe000000);fixture.tracks[1]=(gate/3==2?0xff000000:0xfe000000);
  for(unsigned i=1;i<65;++i){u8*node=root+i*124;*(u32*)(node+0x70)=gate%3?(u32)(uintptr_t)&fixture.tracks[0]:0;*(u32*)(node+0x74)=gate/3?(u32)(uintptr_t)&fixture.tracks[1]:0;for(unsigned axis=0;axis<3;++axis){*(s16*)(node+0x54+axis*2)=equal&1?triple[axis]:0;*(s32*)(node+0x5c+axis*4)=equal&2?triple[axis]:0;}}
  fixture.pose[2]=flags;fixture.pose[3]=skip?0:0xffff;fixture.pose[6]=limits[r];fixture.pose[7]=limits[t];for(unsigned i=12;i<1024;++i)fixture.pose[i]=pattern?(u16)((i-12)*7919u+0x8765u):(u16)triple[(i-12)%3];
  initial=fixture;PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)root;cpu.gpr[5]=(u32)(uintptr_t)fixture.pose;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801def10,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"POSE APPLY FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;fixture=initial;func_801DEF10(root,(u8*)fixture.pose);
  if(memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"POSE APPLY FAIL nodes=%u flags=%u limits=%u/%u gate=%u equal=%u skip=%u\n",nodeCounts[n],flags,limits[r],limits[t],gate,equal,skip);return 1;}++cases;
 }
 printf("POSE APPLY PASS %u cases, full retail memory comparison, no call boundaries\n",cases);
}
