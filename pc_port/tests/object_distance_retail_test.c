#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern s32 func_801E6338(u8*) __attribute__((weak));
static u8 ram[0x200000];
static u32 squareInput;static unsigned squareCalls;
int SquareRoot0(int a){squareInput=(u32)a;++squareCalls;return (s32)((u32)a^0x12345678u);}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){
    (void)u;if(t!=0x80048c4c)return 0;
    c->gpr[2]=SquareRoot0(c->gpr[4]);return 1;
}
static struct { u32 object[0x134/4], root[0x7c/4]; } fixture, initial, expected;
static u8* address(u32 a, unsigned w) {
    if (a >= 0x80000000u && (uint64_t)a+w <= 0x80200000u)
        return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;
    if (a >= lo && (uint64_t)a+w <= lo+sizeof(fixture)) return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v) {
    (void)u; u8* p=address(a,w); if(!p)return -1; *v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);
    return 0;
}
static int wr(void* u,u32 a,unsigned w,u32 v) {
    (void)u; u8* p=address(a,w); if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));
    return 0;
}
int main(void) {
    if(!func_801E6338){fputs("OBJECT DISTANCE FAIL missing native owner\n",stderr);return 1;}
    FILE* f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){
        assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));
        assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);
    }
    assert(!fclose(f));
    const u32 modes[]={0,0xffffffff,0x7fffffff,0x80000000};
    unsigned cases=0;
    for(unsigned mode=0;mode<4;++mode)for(unsigned height=0;height<65536;++height){
        memset(&fixture,0xa5,sizeof(fixture));
        fixture.object[1]=(u32)(uintptr_t)fixture.root;
        *(u16*)((u8*)fixture.object+0x88)=height;
        *(u16*)((u8*)fixture.object+0x8a)=height^0x55aa;
        *(u16*)((u8*)fixture.object+0x8c)=~height;
        fixture.root[0x5c/4]=modes[mode];
        fixture.root[0x60/4]=modes[(mode+1)%4];
        fixture.root[0x64/4]=modes[(mode+2)%4];
        initial=fixture;
        squareCalls=0;
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.object;
        cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e6338,0xfffffffcu,100)!=PC_PORT_MIPS_HALTED){
            fprintf(stderr,"OBJECT DISTANCE FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;fixture=initial;u32 expectedInput=squareInput;unsigned expectedCalls=squareCalls;squareCalls=0;
        s32 result=func_801E6338((u8*)fixture.object);
        if((u32)result!=cpu.gpr[2]||squareInput!=expectedInput||squareCalls!=expectedCalls||squareCalls!=1||memcmp(&fixture,&expected,sizeof(fixture))){
            fprintf(stderr,"OBJECT DISTANCE FAIL mode=%u height=%u\n",modes[mode],height);return 1;
        }
        ++cases;
    }
    printf("OBJECT DISTANCE PASS %u cases: signed targets, wrapped differences and squares, SDK input, return\n",cases);
}
