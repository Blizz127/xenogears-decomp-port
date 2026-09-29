#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern s32 func_801E8480(s32) __attribute__((weak));
static u8 ram[0x200000];
static u32 fixture[124*132/4], initial[124*132/4], expected[124*132/4];
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
int main(void){
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 if(!func_801E8480){fputs("OBJECT SCALE FAIL missing native body\n",stderr);return 1;}
 unsigned cases=0;const unsigned slots[]={0,1,9};
 for(unsigned value=0;value<65536;++value)for(unsigned flags=0;flags<2;++flags)for(unsigned k=0;k<3;++k){
  memset(fixture,0xa5,sizeof(fixture));u8*obj=(u8*)fixture;u8*node=obj+0x140;
  *(u32*)(obj+4)=(u32)(uintptr_t)node;*(u16*)(obj+0x4a)=(u16)(0xfff7u|(flags<<3));*(u16*)(obj+0x1c)=value;
  *(u16*)(obj+0x26)=(u16)(value*37+0x8000);*(u16*)(obj+0x28)=(u16)(~value);
  *(u16*)(node+0x4c)=(u16)(value^0x55aa);*(u16*)(node+0x50)=(u16)(value*199);
  D_801E8670[slots[k]]=(u32)(uintptr_t)obj;assert(!wr(NULL,0x801e8670+slots[k]*4,4,(u32)(uintptr_t)obj));
  memcpy(initial,fixture,sizeof(fixture));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=slots[k];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e8480,0xfffffffcu,100)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"OBJECT SCALE FAIL oracle %s\n",cpu.error);return 1;}
  s32 got=func_801E8480(slots[k]);
  if((u32)got!=cpu.gpr[2]||memcmp(fixture,initial,sizeof(fixture))){fprintf(stderr,"OBJECT SCALE FAIL value=%u flags=%u slot=%u\n",value,flags,slots[k]);return 1;}++cases;
 }
 for(unsigned slot=0;slot<10;++slot){D_801E8670[slot]=0;assert(!wr(NULL,0x801e8670+slot*4,4,0));PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=slot;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;assert(PcPortMipsRun(&cpu,0x801e8480,0xfffffffcu,100)==PC_PORT_MIPS_HALTED);if((u32)func_801E8480(slot)!=cpu.gpr[2]){fputs("OBJECT SCALE FAIL null slot\n",stderr);return 1;}}
 printf("OBJECT SCALE PASS %u arithmetic cases and 10 null slots; full retail body\n",cases);
}
