#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E8510(u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 parent[0x134/4];u32 entries[255*0x70/4];} fixture,initial,expected;
static u32 allocCalls,allocSize,allocFlags,allocOwner[0x134/4],mutation;
void* HeapAlloc(u32 size,u32 flags){++allocCalls;allocSize=size;allocFlags=flags;memcpy(allocOwner,fixture.parent,sizeof(allocOwner));if(mutation==1)((u8*)fixture.parent)[0x10c]=0;else if(mutation==2)--((u8*)fixture.parent)[0x10c];return fixture.entries;}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t!=0x80031bdc)return 0;c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);return 1;}
static void resetCalls(void){allocCalls=allocSize=allocFlags=0;memset(allocOwner,0,sizeof(allocOwner));}
int main(void){
 if(!func_801E8510){fputs("TRAIL INIT FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned n=0;n<256;++n)for(unsigned seed=0;seed<4;++seed)for(mutation=0;mutation<3;++mutation){
  for(unsigned i=0;i<sizeof(fixture);++i)((u8*)&fixture)[i]=(u8)(i*37+seed*67);
  ((u8*)fixture.parent)[0x10c]=n;initial=fixture;resetCalls();PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)fixture.parent;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e8510,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TRAIL INIT FAIL retail %s\n",cpu.error);return 1;}
  expected=fixture;u32 wantedCalls=allocCalls,wantedSize=allocSize,wantedFlags=allocFlags,wantedOwner[0x134/4];memcpy(wantedOwner,allocOwner,sizeof(wantedOwner));
  fixture=initial;resetCalls();func_801E8510((u8*)fixture.parent);
  if(memcmp(&fixture,&expected,sizeof(fixture))||allocCalls!=wantedCalls||allocSize!=wantedSize||allocFlags!=wantedFlags||memcmp(allocOwner,wantedOwner,sizeof(wantedOwner))){fprintf(stderr,"TRAIL INIT FAIL count=%u seed=%u mutation=%u\n",n,seed,mutation);return 1;}++cases;
 }
 printf("TRAIL INIT PASS %u cases, full memory and allocation boundary; controlled allocator\n",cases);
}
