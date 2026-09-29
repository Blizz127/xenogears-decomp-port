#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801DF52C(u8*,u8*,s32,s32) __attribute__((weak));
extern void func_801DFE8C(u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 pool[2],nodes[67*31],tracks[12*5];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801DF52C||!func_801DFE8C){fputs("NODE TRACK RELEASE FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const unsigned counts[]={0,1,2,5,65};const s32 indices[]={-1,0,1,4,63,64,65,0x7fffffff};unsigned cases=0;
 for(unsigned kind=0;kind<2;++kind)for(unsigned n=0;n<5;++n)for(unsigned k=0;k<(kind?1:8);++k)for(unsigned mask=0;mask<(kind?1:16);++mask)for(unsigned pattern=0;pattern<27;++pattern)for(unsigned freeIndex=0;freeIndex<3;++freeIndex){
  memset(&fixture,0xa5,sizeof(fixture));fixture.pool[0]=(u32)(uintptr_t)fixture.tracks;*(u16*)((u8*)fixture.pool+4)=freeIndex==0?0:freeIndex==1?7:0xffff;
  for(unsigned i=0;i<12;++i)fixture.tracks[i*5]=0xfeabcc01u;
  for(unsigned i=0;i<67;++i)for(unsigned channel=0;channel<3;++channel){unsigned digit=(pattern/(channel==0?1:channel==1?3:9))%3;unsigned track=(i%4)*3+channel;fixture.tracks[track*5]=(digit==2?0xffabcc01u:0xfeabcc01u);fixture.nodes[i*31+28+channel]=digit?(u32)(uintptr_t)(fixture.tracks+track*5):0;}
  u8*root=(u8*)(fixture.nodes+31);*(u16*)(root+0xa)=counts[n];initial=fixture;PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)fixture.pool;cpu.gpr[5]=(u32)(uintptr_t)root;cpu.gpr[6]=indices[k];cpu.gpr[7]=mask;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,kind?0x801dfe8c:0x801df52c,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE TRACK RELEASE FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;fixture=initial;
  if(kind)func_801DFE8C((u8*)fixture.pool,root);else func_801DF52C((u8*)fixture.pool,root,indices[k],mask);
  if(memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"NODE TRACK RELEASE FAIL kind=%u count=%u index=%d mask=%u pattern=%u free=%u\n",kind,counts[n],indices[k],mask,pattern,freeIndex);return 1;}++cases;
 }
 printf("NODE TRACK RELEASE PASS %u cases, full memory and real pool release on both sides\n",cases);
}
