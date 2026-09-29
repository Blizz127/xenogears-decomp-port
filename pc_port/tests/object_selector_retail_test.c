#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 func_801E67F8(void) __attribute__((weak));
extern u32 func_801E6830(u8*,s32,u16*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 object[16];u32 output;} fixture,initial,expected;
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static u32 oracle(u32 entry,u32 a,u32 b,u32 c){PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=c;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;if(PcPortMipsRun(&cpu,entry,0xfffffffcu,300)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"OBJECT SELECTOR FAIL oracle %s\n",cpu.error);exit(1);}return cpu.gpr[2];}
int main(void){
 if(!func_801E67F8||!func_801E6830){fputs("OBJECT SELECTOR FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 for(unsigned bits=0;bits<65536;++bits){D_801E863C=(s16)bits;assert(!wr(NULL,0x801e863c,2,bits));if(func_801E67F8()!=oracle(0x801e67f8,0,0,0)){fputs("OBJECT SELECTOR FAIL first-bit scan\n",stderr);return 1;}}
 unsigned cases=0;
 for(unsigned selector=0;selector<256;++selector)for(unsigned id=0;id<256;++id)for(unsigned mode=0;mode<3;++mode)for(unsigned alias=0;alias<2;++alias){
  memset(&fixture,0xa5,sizeof(fixture));u8*obj=(u8*)fixture.object;obj[0x20]=id;obj[0x21]=(u8)(255-id);D_801E86B0=(s16)(0xab00u|((id*37)&255));D_801E863C=(s16)(mode==0?0:mode==1?0x8000:1u<<(id%8));assert(!wr(NULL,0x801e86b0,2,(u16)D_801E86B0));assert(!wr(NULL,0x801e863c,2,(u16)D_801E863C));u16*out=alias?(u16*)(obj+0x20):(u16*)&fixture.output;
  s32 arg=(s32)(0xa55a0000u|selector);initial=fixture;u32 want=oracle(0x801e6830,(u32)(uintptr_t)obj,arg,(u32)(uintptr_t)out);expected=fixture;fixture=initial;u32 got=func_801E6830(obj,arg,out);
  if(got!=want||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"OBJECT SELECTOR FAIL selector=%u id=%u mode=%u alias=%u result=%x/%x\n",selector,id,mode,alias,got,want);return 1;}++cases;
 }
 printf("OBJECT SELECTOR PASS 65536 mask scans and %u selectors; full retail body/memory/return\n",cases);
}
