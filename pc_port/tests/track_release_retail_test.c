#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern s32 func_801DF7A8(u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
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
    if(!func_801DF7A8){fputs("TRACK RELEASE FAIL missing native owner\n",stderr);return 1;}
    FILE* f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){
        assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));
        assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);
    }
    assert(!fclose(f));
    const u32 deltas[]={0,19,20,21,1310680,1310700,1310720,0x7fffffff,0x80000000,0xffffffff};
    unsigned cases=0;
    for(unsigned mode=0;mode<11;++mode)for(unsigned height=0;height<65536;++height){
        memset(&fixture,0xa5,sizeof(fixture));
        fixture.object[0]=(u32)(uintptr_t)fixture.root-deltas[mode%10];
        *(u16*)((u8*)fixture.object+4)=height;
        u8* header=mode==10?NULL:(u8*)fixture.object;
        u8* entry=mode==10?NULL:(u8*)fixture.root;
        initial=fixture;
        PcPortMipsBus bus={.read=rd,.write=wr};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)header;cpu.gpr[5]=(u32)(uintptr_t)entry;
        cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801df7a8,0xfffffffcu,100)!=PC_PORT_MIPS_HALTED){
            fprintf(stderr,"TRACK RELEASE FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;fixture=initial;
        s32 result=func_801DF7A8(header,entry);
        if((u32)result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))){
            fprintf(stderr,"TRACK RELEASE FAIL mode=%u height=%u\n",mode,height);return 1;
        }
        ++cases;
    }
    printf("TRACK RELEASE PASS %u cases: all free-index halfwords, wrapped offsets, null, memory, return\n",cases);
}
