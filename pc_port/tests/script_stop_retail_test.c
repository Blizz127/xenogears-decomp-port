#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern s32 func_801E0844(u8*,s32) __attribute__((weak));
extern s32 func_801E632C(u8*) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E0844||!func_801E632C){fputs("SCRIPT STOP FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned value=0;value<65536;++value)for(unsigned kind=0;kind<2;++kind){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)fixture[i]=value*8191u+i*37u;
  u8*object=(u8*)fixture+0x140;*(u16*)(object+(kind?0x98:0))=value;memcpy(initial,fixture,sizeof(fixture));
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)object;cpu.gpr[5]=0xa55a0000u|value;cpu.gpr[31]=0xfffffffcu;
  assert(PcPortMipsRun(&cpu,kind?0x801e632c:0x801e0844,0xfffffffcu,100)==PC_PORT_MIPS_HALTED);
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));
  s32 got=kind?func_801E632C(object):func_801E0844(object,(s32)(0xa55a0000u|value));
  if((u32)got!=cpu.gpr[2]||memcmp(expected,fixture,sizeof(fixture))){fprintf(stderr,"SCRIPT STOP FAIL value=%u kind=%u\n",value,kind);return 1;}++cases;
 }
 printf("SCRIPT STOP PASS %u complete-memory/return comparisons; both retail leaf bodies\n",cases);
}

