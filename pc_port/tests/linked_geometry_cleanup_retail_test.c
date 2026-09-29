#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E3438(u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 owner[9],tables[2][2];} fixture,initial,expected;
static struct Call {u32 pointer,owner[9];} calls[5],expectedCalls[5];
static unsigned nCalls,mutate;
u_int HeapFree(void*p){
    assert(nCalls<5);calls[nCalls].pointer=(u32)(uintptr_t)p;
    memcpy(calls[nCalls].owner,fixture.owner,36);++nCalls;
    if(mutate&&nCalls==1)fixture.owner[7]=(u32)(uintptr_t)fixture.tables[1];
    return 0;
}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t!=0x800320e8)return 0;c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);return 1;}
int main(void){
    if(!func_801E3438){fputs("GEOMETRY CLEANUP FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;
    for(unsigned active=0;active<2;++active)for(unsigned mask=0;mask<8;++mask)for(mutate=0;mutate<2;++mutate)for(unsigned seed=0;seed<16;++seed){
        memset(&fixture,0xa5,sizeof(fixture));fixture.owner[5]=active?0x10000+seed*16:0;
        fixture.owner[6]=mask&1?0x20000+seed*16:0;fixture.owner[7]=(u32)(uintptr_t)fixture.tables[0];fixture.owner[8]=mask&2?0x30000+seed*16:0;
        fixture.tables[0][0]=mask&4?0x40000+seed*16:0;fixture.tables[1][0]=0x50000+seed*16;
        initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.owner;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e3438,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"GEOMETRY CLEANUP FAIL oracle %s\n",cpu.error);return 1;}
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
        fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));func_801E3438((u8*)fixture.owner);
        if(memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"GEOMETRY CLEANUP FAIL active=%u mask=%u mutate=%u seed=%u\n",active,mask,mutate,seed);return 1;}
        ++cases;
    }
    printf("GEOMETRY CLEANUP PASS %u cases: full retail body, memory and ordered frees; controlled heap boundary\n",cases);
}
