#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E6D94(u8*,u8*,s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E6D94){fputs("NODE VISIBILITY FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;const u32 values[]={0,0xffff,0x8000,0x7fffffff};
 for(unsigned flags=0;flags<256;++flags)for(unsigned count=1;count<=5;++count)for(unsigned selected=0;selected<count;++selected)for(unsigned shape=0;shape<3;++shape)for(unsigned seed=0;seed<4;++seed){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)fixture[i]=0x12481248u*(i+seed);
  u8*obj=(u8*)fixture;u8*root=obj+0x140;*(u32*)(obj+4)=(u32)(uintptr_t)root;
  for(unsigned i=0;i<count;++i){u8*node=root+i*124;unsigned parent=shape==0?0:shape==1?(i?i-1:0):i/2;*(u32*)node=i?(u32)(uintptr_t)(root+parent*124):0;*(u16*)(node+10)=i?i:count;}
  s32 args[3]={(s32)values[seed],(s32)(values[(seed+1)%4]^0xa55a0000u),(s32)(values[(seed+2)%4]^0x80000000u)};s32 mode=(s32)(0xa55a0000u|flags);u8*node=root+selected*124;
  memcpy(initial,fixture,sizeof(fixture));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=(u32)(uintptr_t)node;cpu.gpr[6]=mode;cpu.gpr[7]=args[0];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;assert(!wr(NULL,0x801ff010,4,args[1]));assert(!wr(NULL,0x801ff014,4,args[2]));
  if(PcPortMipsRun(&cpu,0x801e6d94,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE VISIBILITY FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));func_801E6D94(obj,node,mode);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"NODE VISIBILITY FAIL flags=%u count=%u selected=%u shape=%u seed=%u\n",flags,count,selected,shape,seed);return 1;}++cases;
 }
 printf("NODE VISIBILITY PASS %u complete-memory comparisons, real recursive retail calls\n",cases);
}
