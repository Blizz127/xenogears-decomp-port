#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u8* func_801DC22C(u8*,u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 header[2];u32 pointers[1025];u8 model[64];} fixture,initial,expected;
static u32 calls[3][5],expectedCalls[3][5];static unsigned nCalls;static s32 modelCount;static int failAllocation;
static void call(u32 op,u32 a,u32 b){assert(nCalls<3);u32*p=calls[nCalls++];p[0]=op;p[1]=a;p[2]=b;p[3]=fixture.header[0];p[4]=fixture.header[1];}
void HeapChangeCurrentUser(u32 tag,char** names){call(1,tag,(u32)(uintptr_t)names);}
int func_8002C3E8(u8*p){call(2,(u32)(uintptr_t)p,0);p[7]^=0x61;return modelCount;}
void* HeapAlloc(u32 size,u32 flags){call(3,size,flags);return failAllocation?NULL:fixture.pointers;}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){
    (void)u;
    if(t==0x80032498)HeapChangeCurrentUser(c->gpr[4],(char**)(uintptr_t)c->gpr[5]);
    else if(t==0x8002c3e8)c->gpr[2]=func_8002C3E8((u8*)(uintptr_t)c->gpr[4]);
    else if(t==0x80031bdc)c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);
    else return 0;return 1;
}
static int compare(void){
    initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)(uintptr_t)fixture.model;cpu.gpr[5]=(u32)(uintptr_t)fixture.header;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x801dc22c,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"MODEL REGISTRY FAIL oracle %s\n",cpu.error);return 1;}
    expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));
    u8*result=func_801DC22C(fixture.model,(u8*)fixture.header);
    if((u32)(uintptr_t)result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"MODEL REGISTRY FAIL count=%d failure=%d\n",modelCount,failAllocation);return 1;}
    fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));
    OvlyPtrTab host;
    OvlyPtrTab* returned=OvlyBuildPtrTab(fixture.model,&host);
    if(returned!=&host||(u32)(uintptr_t)host.ptrs!=expected.header[0]||(u32)host.count!=expected.header[1]||
       memcmp(fixture.pointers,expected.pointers,sizeof(fixture.pointers))||memcmp(fixture.model,expected.model,sizeof(fixture.model))||
       nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"MODEL REGISTRY FAIL host adapter count=%d failure=%d\n",modelCount,failAllocation);return 1;}
    return 0;
}
int main(void){
    if(!func_801DC22C){fputs("MODEL REGISTRY FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;
    for(modelCount=0;modelCount<=1024;++modelCount)for(failAllocation=0;failAllocation<2;++failAllocation){memset(&fixture,0xa5,sizeof(fixture));if(compare())return 1;++cases;}
    const s32 extremes[]={-1,(-2147483647-1),0x7fffffff};failAllocation=1;
    for(unsigned i=0;i<3;++i){modelCount=extremes[i];memset(&fixture,0xa5,sizeof(fixture));if(compare())return 1;++cases;}
    printf("MODEL REGISTRY PASS %u cases: full memory, return, relocation and heap boundaries\n",cases);
}
