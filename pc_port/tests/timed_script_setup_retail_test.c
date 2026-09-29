#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E5C74(u8*,u8*,s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E5C74){fputs("TIMED SCRIPT SETUP FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;const s32 loops[]={0,1,(s32)0x80000000};
 for(unsigned count=0;count<65536;++count)for(unsigned alias=0;alias<3;++alias)for(unsigned loop=0;loop<3;++loop){
  memset(fixture,0xa5,sizeof(fixture));u8*obj=(u8*)fixture;u8*data=obj+(alias==0?0x100:alias==1?0x84:0x88);
  *(u16*)(data+2)=(u16)(count^0x55aa);*(u16*)(data+0x12)=count;*(u32*)(data+0x14)=0xfedc0000u+count*37;
  memcpy(initial,fixture,sizeof(fixture));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=(u32)(uintptr_t)data;cpu.gpr[6]=loops[loop];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e5c74,0xfffffffcu,100)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TIMED SCRIPT SETUP FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));func_801E5C74(obj,data,loops[loop]);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"TIMED SCRIPT SETUP FAIL count=%u alias=%u loop=%u\n",count,alias,loop);return 1;}++cases;
 }
 printf("TIMED SCRIPT SETUP PASS %u complete-memory comparisons, retail body without calls\n",cases);
}
