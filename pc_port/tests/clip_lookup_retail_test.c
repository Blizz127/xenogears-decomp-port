#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 func_801E6910(u8*,s32,u32*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 object[0x134/4],primary[256],secondary[256],out;} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E6910){fputs("CLIP LOOKUP FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned index=0;index<256;++index)for(unsigned selector=0;selector<256;++selector)for(unsigned alias=0;alias<3;++alias){
  memset(&fixture,0xa5,sizeof(fixture));fixture.object[5]=(u32)(uintptr_t)fixture.primary;fixture.object[6]=(u32)(uintptr_t)fixture.secondary;((u8*)fixture.object)[0x2a]=selector;
  for(unsigned i=0;i<256;++i){fixture.primary[i]=0x12345600u+i*123;fixture.secondary[i]=0xaabbcc00u+i*789;}
  u32*out=alias==0?&fixture.out:alias==1?fixture.object+10:fixture.primary+1;
  s32 argument=(s32)(0xa55a0000u|index);initial=fixture;PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)fixture.object;cpu.gpr[5]=argument;cpu.gpr[6]=(u32)(uintptr_t)out;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e6910,0xfffffffcu,100)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP LOOKUP FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;u32 result=cpu.gpr[2];fixture=initial;
  u32 actual=func_801E6910((u8*)fixture.object,argument,out);
  if(actual!=result||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"CLIP LOOKUP FAIL index=%u selector=%u alias=%u result=%x/%x\n",index,selector,alias,actual,result);return 1;}++cases;
 }
 printf("CLIP LOOKUP PASS %u cases, complete retail body and memory, no call boundaries\n",cases);
}
