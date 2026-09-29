#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern s32 func_801E66BC(VECTOR*,VECTOR*,VECTOR*,s32) __attribute__((weak));
static u8 ram[0x200000];
static VECTOR vectors[3], crossValue;
static u32 rootInput, arguments[6];
static unsigned crossCalls,rootCalls;
void OuterProduct12(VECTOR*a,VECTOR*b,VECTOR*out){memcpy(arguments,a,12);memcpy(arguments+3,b,12);++crossCalls;memcpy(out,&crossValue,12);}
int SquareRoot0(int a){++rootCalls;rootInput=(u32)a;return ((u32)a>>17)+1;}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)vectors;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(vectors))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t==0x80048c4c){c->gpr[2]=SquareRoot0(c->gpr[4]);return 1;}if(t==0x8004a480){VECTOR out;OuterProduct12((VECTOR*)address(c->gpr[4],12),(VECTOR*)address(c->gpr[5],12),&out);for(unsigned i=0;i<3;++i)assert(!wr(NULL,c->gpr[6]+i*4,4,((u32*)&out)[i]));c->gpr[2]=c->gpr[6];return 1;}return 0;}
int main(void){
 if(!func_801E66BC){fputs("ORIENTATION SCALAR FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 u32 seed=0xdeadbeef;unsigned cases=0;const s32 divisors[]={1,2,-2,4096,-4096,0x7fffffff,(s32)0x80000000};
 for(unsigned trial=0;trial<8192;++trial)for(unsigned d=0;d<7;++d){
  for(unsigned i=0;i<12;++i){seed=seed*1664525u+1013904223u;((u32*)vectors)[i]=seed;}for(unsigned i=0;i<3;++i){seed=seed*1664525u+1013904223u;((u32*)&crossValue)[i]=seed;}
  crossCalls=rootCalls=0;PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);for(unsigned i=0;i<3;++i)cpu.gpr[4+i]=(u32)(uintptr_t)&vectors[i];cpu.gpr[7]=divisors[d];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e66bc,0xfffffffcu,200)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"ORIENTATION SCALAR FAIL oracle %s\n",cpu.error);return 1;}
  u32 wantRoot=rootInput,wantArgs[6];memcpy(wantArgs,arguments,24);assert(crossCalls==1&&rootCalls==1);crossCalls=rootCalls=0;
  s32 got=func_801E66BC(&vectors[0],&vectors[1],&vectors[2],divisors[d]);
  if((u32)got!=cpu.gpr[2]||rootInput!=wantRoot||memcmp(wantArgs,arguments,24)||crossCalls!=1||rootCalls!=1){fprintf(stderr,"ORIENTATION SCALAR FAIL trial=%u divisor=%d\n",trial,divisors[d]);return 1;}++cases;
 }
 printf("ORIENTATION SCALAR PASS %u cases; controlled SDK boundaries, retail arithmetic and return\n",cases);
}
