#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 func_801E5CD8(u8*,s32) __attribute__((weak));
u8 g_PsxRam[PSX_RAM_SIZE];
#define ram g_PsxRam
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if((a<0x200000u || (a&0xffe00000u)==0x80000000u || (a&0xffe00000u)==0xa0000000u) && (a&0x1fffff)+w<=0x200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E5CD8){fputs("SOUND BANK FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned id=0;id<65536;++id)for(unsigned scheme=0;scheme<4;++scheme)for(unsigned selector=0;selector<3;++selector){
  u8*obj=(u8*)fixture;memset(fixture,0xa5,sizeof(fixture));
  for(unsigned bank=0;bank<3;++bank){
   u32 p=scheme==0?(u32)(uintptr_t)(obj+0x400+bank*64):(scheme==1?0:scheme==2?0x80000000u:0xa0000000u)+0x10000+bank*64;
   assert(!wr(NULL,p+0x14,2,(u16)(id+bank*199)));
   if(bank==0)assert(!wr(NULL,0x8005919c,4,p));
   else{u32 package=scheme==0?(u32)(uintptr_t)(obj+0x200+bank*32):(scheme==1?0:scheme==2?0x80000000u:0xa0000000u)+0x20000+bank*32;assert(!wr(NULL,package+8,4,p));*(u32*)(obj+0xac+bank*4)=package;}
  }
  memcpy(initial,fixture,sizeof(fixture));
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=selector;cpu.gpr[31]=0xfffffffcu;
  assert(PcPortMipsRun(&cpu,0x801e5cd8,0xfffffffcu,100)==PC_PORT_MIPS_HALTED);
  u32 result=func_801E5CD8(obj,selector);
  if(result!=cpu.gpr[2]||memcmp(initial,fixture,sizeof(fixture))){fprintf(stderr,"SOUND BANK FAIL id=%u scheme=%u selector=%u\n",id,scheme,selector);return 1;}++cases;
 }
 const u32 others[]={3,255,256,257,258,65536,0x80000000,0xffffffff};
 for(unsigned i=0;i<sizeof(others)/sizeof(*others);++i){
  PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=0;cpu.gpr[5]=others[i];cpu.gpr[31]=0xfffffffcu;
  assert(PcPortMipsRun(&cpu,0x801e5cd8,0xfffffffcu,100)==PC_PORT_MIPS_HALTED);
  if(func_801E5CD8(NULL,others[i])!=cpu.gpr[2]){fputs("SOUND BANK FAIL fullword selector\n",stderr);return 1;}
 }
 printf("SOUND BANK PASS %u bank IDs/routes/aliases and 8 other selectors; owner lifetime not proven\n",cases);
}
