#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E6578(u8*,s32,u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E6578){fputs("NODE TRANSFER FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned count=1;count<=8;++count)for(unsigned selected=0;selected<count;++selected)for(unsigned shape=0;shape<3;++shape)for(unsigned slots=0;slots<8;++slots)for(unsigned alias=0;alias<2;++alias)for(unsigned cursor=0;cursor<3;++cursor){
  memset(fixture,0xa5,sizeof(fixture));u8*src=(u8*)fixture;u8*dst=alias?src:src+16*124;u8*pool=src+32*124;u8*tracks=pool+16;
  *(u32*)pool=(u32)(uintptr_t)tracks;*(u16*)(pool+4)=cursor==0?0:cursor==1?7:65535;
  for(unsigned i=0;i<count;++i){
   u8*node=src+i*124;unsigned parent=shape==0?0:shape==1?(i?i-1:0):i/2;
   *(u32*)node=i?(u32)(uintptr_t)(src+parent*124):0;*(u16*)(node+10)=i?i:count;
   for(unsigned c=0;c<3;++c){u8*t=tracks+(i*3+c)*20;t[0]=1;t[3]=0xff;*(u32*)(node+0x70+c*4)=(slots&(1u<<c))?(u32)(uintptr_t)t:0;}
  }
  memcpy(initial,fixture,sizeof(fixture));
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)pool;cpu.gpr[5]=selected;cpu.gpr[6]=(u32)(uintptr_t)src;cpu.gpr[7]=(u32)(uintptr_t)dst;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e6578,0xfffffffcu,20000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE TRANSFER FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));func_801E6578(pool,selected,src,dst);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"NODE TRANSFER FAIL count=%u selected=%u shape=%u slots=%u alias=%u cursor=%u\n",count,selected,shape,slots,alias,cursor);return 1;}++cases;
 }
 printf("NODE TRANSFER PASS %u full-memory retail comparisons, real release and recursive calls\n",cases);
}
