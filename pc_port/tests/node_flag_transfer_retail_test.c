#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E6668(u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E6668){fputs("NODE FLAG TRANSFER FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;unsigned counts[]={0,1,2,5,65};
 for(unsigned n=0;n<5;++n)for(unsigned flag=0;flag<256;++flag)for(unsigned layout=0;layout<4;++layout)for(unsigned pattern=0;pattern<3;++pattern){
  memset(fixture,0xa5,sizeof(fixture));u8*src=(u8*)fixture+124;u8*dst=layout==0?src+66*124:layout==1?src:layout==2?src+124:src-124;
  for(unsigned i=0;i<66;++i)src[i*124+7]=(u8)(pattern==0?flag:pattern==1?(i&1?flag:0):(flag+i));
  *(u16*)(src+10)=counts[n];memcpy(initial,fixture,sizeof(fixture));
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)src;cpu.gpr[5]=(u32)(uintptr_t)dst;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e6668,0xfffffffcu,2000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE FLAG TRANSFER FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));func_801E6668(src,dst);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"NODE FLAG TRANSFER FAIL count=%u flag=%u layout=%u pattern=%u\n",counts[n],flag,layout,pattern);return 1;}++cases;
 }
 printf("NODE FLAG TRANSFER PASS %u full-memory retail comparisons\n",cases);
}
