#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E59D4(u8*,u8*,s32,s32,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 if(!func_801E59D4){fputs("ROTATION BIND FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const s32 durations[]={-1,0,1,2,65535,65536,0x7fffffff};
 const s32 values[]={0,1,-1,2047,2048,4095,4096,-32768,32767,65536,(s32)0x80000000,0x7fffffff};
 unsigned cases=0;
 for(unsigned d=0;d<7;++d)for(unsigned x=0;x<12;++x)for(unsigned y=0;y<12;++y)for(unsigned state=0;state<4;++state)for(unsigned equal=0;equal<2;++equal){
  memset(fixture,0xa5,sizeof(fixture));u8*node=(u8*)fixture;u8*pool=node+128;u8*tracks=pool+16;
  *(u32*)pool=(u32)(uintptr_t)tracks;*(u16*)(pool+4)=state==2?4:0;*(u16*)(pool+6)=4;
  for(unsigned i=0;i<4;++i)tracks[i*20]=state==3?1:0;
  tracks[3]=0xff;*(u32*)(node+0x70)=state==1?(u32)(uintptr_t)tracks:0;
  s32 args[3]={values[x],values[y],values[(x+y)%12]};
  for(unsigned i=0;i<3;++i)*(s16*)(node+0x54+i*2)=equal?(s16)args[i]:(s16)values[(x+y+i+3)%12];
  memcpy(initial,fixture,sizeof(fixture));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)pool;cpu.gpr[5]=(u32)(uintptr_t)node;cpu.gpr[6]=durations[d];cpu.gpr[7]=args[0];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;assert(!wr(NULL,0x801ff010,4,args[1]));assert(!wr(NULL,0x801ff014,4,args[2]));
  if(PcPortMipsRun(&cpu,0x801e59d4,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"ROTATION BIND FAIL oracle %s\n",cpu.error);return 1;}
  memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));func_801E59D4(pool,node,durations[d],args[0],args[1],args[2]);
  if(memcmp(fixture,expected,sizeof(fixture))){fprintf(stderr,"ROTATION BIND FAIL duration=%d x=%u y=%u state=%u equal=%u\n",durations[d],x,y,state,equal);return 1;}++cases;
 }
 printf("ROTATION BIND PASS %u complete-memory comparisons, real retail pool claim\n",cases);
}
