#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E5B50(u8*,u8*,s32,s32,s32,s32,s32,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static u32 rootInput;static unsigned rootCalls;
static u8 rootSnapshot[sizeof(fixture)],wantSnapshot[sizeof(fixture)];
int SquareRoot0(int a){rootInput=(u32)a;++rootCalls;memcpy(rootSnapshot,fixture,sizeof(fixture));return (s32)((u32)a^0x12345678u);}
static int bridge(void*u,PcPortMipsCpu*c,u32 target){(void)u;if(target!=0x80048c4c)return 0;c->gpr[2]=SquareRoot0(c->gpr[4]);return 1;}
int main(void){
 if(!func_801E5B50){fputs("DISTANCE TRACK BIND FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;u32 seed=0xa55a1234;
 for(unsigned mode=0;mode<256;++mode)for(unsigned trial=0;trial<32;++trial)for(unsigned state=0;state<4;++state){
  memset(fixture,0xa5,sizeof(fixture));u8*node=(u8*)fixture;u8*pool=node+128;u8*tracks=pool+16;
  *(u32*)pool=(u32)(uintptr_t)tracks;*(u16*)(pool+4)=state==2?4:0;*(u16*)(pool+6)=4;
  for(unsigned i=0;i<4;++i)tracks[i*20]=state==3?1:0;
  tracks[3]=0xff;*(u32*)(node+0x70)=state==1?(u32)(uintptr_t)tracks:0;
  s32 args[7];args[0]=(s32)(0xa55a0000u|mode);for(unsigned i=1;i<7;++i){seed=seed*1664525u+1013904223u;args[i]=(s32)seed;}
  for(unsigned i=0;i<3;++i){seed=seed*1664525u+1013904223u;*(u32*)(node+0x5c+i*4)=seed;}
  memcpy(initial,fixture,sizeof(fixture));rootCalls=0;rootInput=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)pool;cpu.gpr[5]=(u32)(uintptr_t)node;cpu.gpr[6]=args[0];cpu.gpr[7]=args[1];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;for(unsigned i=2;i<7;++i)assert(!wr(NULL,0x801ff010+(i-2)*4,4,args[i]));
  if(PcPortMipsRun(&cpu,0x801e5b50,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"DISTANCE TRACK BIND FAIL oracle %s\n",cpu.error);return 1;}
  u32 wantInput=rootInput;unsigned wantCalls=rootCalls;memcpy(wantSnapshot,rootSnapshot,sizeof(fixture));memcpy(expected,fixture,sizeof(fixture));memcpy(fixture,initial,sizeof(fixture));rootCalls=0;rootInput=0;func_801E5B50(pool,node,args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
  if(memcmp(fixture,expected,sizeof(fixture))||rootCalls!=wantCalls||rootInput!=wantInput||(rootCalls&&memcmp(rootSnapshot,wantSnapshot,sizeof(fixture)))){fprintf(stderr,"DISTANCE TRACK BIND FAIL mode=%u trial=%u state=%u\n",mode,trial,state);return 1;}++cases;
 }
 printf("DISTANCE TRACK BIND PASS %u memory/call comparisons; real pool claim, controlled square root\n",cases);
}
