#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E6974(u8*,u8*,u8*,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void) {
 if(!func_801E6974){fputs("TRACK BIND FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;const u32 modes[]={0,1,2,255};const u32 values[]={0,0xffff,0x8000,0x7fffffff};
 for(unsigned flags=0;flags<256;++flags)for(unsigned mode=0;mode<4;++mode)for(unsigned state=0;state<4;++state)for(unsigned shape=0;shape<3;++shape)for(unsigned seed=0;seed<4;++seed){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)fixture[i]=0x12481248u*(i+seed);
  u8*obj=(u8*)fixture;u8*pool=obj+0x120;u8*root=obj+0x140;u8*entries=obj+0x1000;
  *(u32*)(obj+4)=(u32)(uintptr_t)root;*(u32*)pool=(u32)(uintptr_t)entries;*(u16*)(pool+4)=state==2?8:0;*(u16*)(pool+6)=8;
  for(unsigned i=0;i<8;++i)entries[i*20]=state==3?1:0;
  for(unsigned i=0;i<5;++i){
   u8*node=root+i*124;unsigned parent=shape==0?0:shape==1?(i?i-1:0):i/2;
   *(u32*)node=i?(u32)(uintptr_t)(root+parent*124):0;*(u16*)(node+10)=i?i:5;
   for(unsigned c=0;c<3;++c)*(u32*)(node+0x70+c*4)=0;
   /* Children retain existing protected-tag records even in exhausted pools. */
   if(state==1 || (i && state>=2)){
    unsigned c=(flags&7)==0?0:(flags&7)==1?1:2;
    *(u32*)(node+0x70+c*4)=(u32)(uintptr_t)(entries+i*20);entries[i*20]=1;entries[i*20+3]=255;
   }
  }
  u32 args[10]={0xa5000000u|modes[mode],0xabcd00ffu,0x12340000u|seed,
   values[seed],values[(seed+1)%4],values[(seed+2)%4],
   values[(seed+3)%4],values[seed]^0xa55a0000u,values[(seed+1)%4]^0x80000000u,values[seed]};
  memcpy(initial,fixture,sizeof(fixture));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=(u32)(uintptr_t)pool;cpu.gpr[6]=(u32)(uintptr_t)root;cpu.gpr[7]=0xa55a0000u|flags;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  for(unsigned i=0;i<10;++i)assert(!wr(NULL,0x801ff010+i*4,4,args[i]));
  if(PcPortMipsRun(&cpu,0x801e6974,0xfffffffcu,30000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TRACK BIND FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));
  func_801E6974(obj,pool,root,(s32)(0xa55a0000u|flags),args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"TRACK BIND FAIL flags=%u mode=%u state=%u shape=%u seed=%u\n",flags,mode,state,shape,seed);return 1;}++cases;
 }
 printf("TRACK BIND PASS %u complete-memory comparisons, retail recursion and allocator\n",cases);
}

