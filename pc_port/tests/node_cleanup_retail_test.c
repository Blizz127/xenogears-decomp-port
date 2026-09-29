#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801DCD8C(u8*) __attribute__((weak));
static u8 ram[0x200000];
static u32 nodes[(65535*124+128)/4],initial[(65535*124+128)/4],expected[(65535*124+128)/4];
static u32 calls[65536][2],expectedCalls[65536][2];static unsigned nCalls,used;
u_int HeapFree(void*p){assert(nCalls<65536);calls[nCalls][0]=(u32)(uintptr_t)p;calls[nCalls++][1]=*(u16*)((u8*)nodes+10);return 0;}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)nodes;if(a>=lo&&(uint64_t)a+w<=lo+used)return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t!=0x800320e8)return 0;c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);return 1;}
static int compare(unsigned count,unsigned bits,int nullRoot,int hostAdapter){
    used=(count?count:1)*124+4;memset(nodes,0xa5,used);
    *(u16*)((u8*)nodes+10)=count;
    for(unsigned i=0;i<count;++i){u8*n=(u8*)nodes+i*124;*(u32*)(n+0x68)=(bits>>(i%8))&1?0x10000+i*16:0;}
    memcpy(initial,nodes,used);nCalls=0;memset(calls,0,sizeof(calls));
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=nullRoot?0:(u32)(uintptr_t)nodes;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x801dcd8c,0xfffffffcu,3000000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE CLEANUP FAIL oracle %s\n",cpu.error);return 1;}
    memcpy(expected,nodes,used);memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    memcpy(nodes,initial,used);nCalls=0;memset(calls,0,sizeof(calls));
    if(hostAdapter)OvlyFreeOwnedNodes(nullRoot?NULL:(u8*)nodes);
    else func_801DCD8C(nullRoot?NULL:(u8*)nodes);
    if(memcmp(nodes,expected,used)||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"NODE CLEANUP FAIL count=%u bits=%x null=%d adapter=%d\n",count,bits,nullRoot,hostAdapter);return 1;}
    return 0;
}
int main(int argc,char**argv){
    (void)argv;int hostAdapter=argc>1;
    if(!func_801DCD8C){fputs("NODE CLEANUP FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;
    for(unsigned count=0;count<=8;++count)for(unsigned bits=0;bits<256;++bits){if(compare(count,bits,0,hostAdapter))return 1;++cases;}
    const unsigned large[]={1023,1024,32768,65535};
    for(unsigned i=0;i<4;++i)for(unsigned bits=0;bits<=255;bits+=255){if(compare(large[i],bits,0,hostAdapter))return 1;++cases;}
    if(compare(8,255,1,hostAdapter))return 1;++cases;
    printf("NODE CLEANUP PASS %u cases adapter=%d: full memory and ordered frees, root count at each free\n",cases,hostAdapter);
}
