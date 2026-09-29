#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E1708(u8*,s32) __attribute__((weak));
extern void func_801E17B8(u8*,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 owner[16];u16 a[256],b[256],destination[256];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
int main(void){
    if(!func_801E1708||!func_801E17B8){fputs("EFFECT BLEND FAIL missing native owners\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s32 factors[]={0,1,-1,31,32,33,32767,-32768,65535,65536};unsigned cases=0;
    for(unsigned kind=0;kind<2;++kind)for(s32 width=-1;width<=4;++width)for(s32 height=-1;height<=4;++height)
    for(s32 origin=-3;origin<=3;++origin)for(unsigned alias=0;alias<3;++alias)for(unsigned variant=0;variant<10;++variant){
        memset(&fixture,0xa5,sizeof(fixture));
        for(unsigned i=0;i<256;++i){fixture.a[i]=(u16)(i*1297+variant);fixture.b[i]=(u16)(65535-i*313-variant);}
        fixture.a[64]=0;fixture.b[64]=65535;fixture.a[65]=65535;fixture.b[65]=0;
        fixture.owner[1]=(u32)(uintptr_t)(fixture.a+64);fixture.owner[2]=(u32)(uintptr_t)(fixture.b+64);
        fixture.owner[7]=(u32)(uintptr_t)((alias==0?fixture.destination:alias==1?fixture.a:fixture.b)+64);
        s16*rect=(s16*)((u8*)fixture.owner+0x28);rect[0]=origin;rect[1]=-origin;rect[2]=width;rect[3]=height;initial=fixture;
        PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.owner;cpu.gpr[5]=(u32)factors[variant];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,kind?0x801e17b8:0x801e1708,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"EFFECT BLEND FAIL oracle %s\n",cpu.error);return 1;}
        expected=fixture;fixture=initial;
        if(kind)func_801E17B8((u8*)fixture.owner,factors[variant]);else func_801E1708((u8*)fixture.owner,factors[variant]);
        if(memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"EFFECT BLEND FAIL kind=%u size=%d/%d origin=%d alias=%u factor=%d\n",kind,width,height,origin,alias,factors[variant]);return 1;}
        ++cases;
    }
    printf("EFFECT BLEND PASS %u cases: full retail bodies and memory, signed factors, overlapping destinations\n",cases);
}
