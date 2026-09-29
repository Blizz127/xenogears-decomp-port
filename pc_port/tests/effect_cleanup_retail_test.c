#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E165C(u8*) __attribute__((weak));
static u8 ram[0x200000];static u32 effect[16],initial[16],expected[16];
static struct Call {u32 kind,args[2],owner[16];u8 rect[8];} calls[4],expectedCalls[4];static unsigned nCalls;static int mutateUpload;
static struct Call* record(u32 kind,u32 a,u32 b){assert(nCalls<4);struct Call*r=&calls[nCalls++];r->kind=kind;r->args[0]=a;r->args[1]=b;memcpy(r->owner,effect,sizeof(effect));return r;}
u_int HeapFree(void*p){record(1,(u32)(uintptr_t)p,0);return 0;}
int LoadImage(RECT16*r,u_long*p){struct Call*c=record(2,(u32)(uintptr_t)r,(u32)(uintptr_t)p);memcpy(c->rect,r,8);if(mutateUpload)effect[1]=0x70000;return 0;}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)effect;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(effect))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
    if(t==0x800320e8)c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);
    else if(t==0x80044894)c->gpr[2]=LoadImage((RECT16*)(uintptr_t)c->gpr[4],(u_long*)(uintptr_t)c->gpr[5]);
    else return 0;return 1;
}
int main(void){
    if(!func_801E165C){fputs("EFFECT CLEANUP FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;const u16 enabled[]={0,1,0xffff};
    for(unsigned type=0;type<256;++type)for(unsigned active=0;active<3;++active)for(unsigned mask=0;mask<8;++mask)for(mutateUpload=0;mutateUpload<2;++mutateUpload){
        memset(effect,0xa5,sizeof(effect));((u8*)effect)[0x10]=type;*(u16*)((u8*)effect+0x1a)=enabled[active];
        for(unsigned i=0;i<3;++i)effect[1+i]=(mask>>i)&1?0x10000+i*256:0;
        memcpy(initial,effect,sizeof(effect));nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)effect;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e165c,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"EFFECT CLEANUP FAIL oracle %s\n",cpu.error);return 1;}
        memcpy(expected,effect,sizeof(effect));memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
        memcpy(effect,initial,sizeof(effect));nCalls=0;memset(calls,0,sizeof(calls));func_801E165C((u8*)effect);
        if(memcmp(effect,expected,sizeof(effect))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"EFFECT CLEANUP FAIL type=%u active=%u mask=%u mutate=%d\n",type,active,mask,mutateUpload);return 1;}
        ++cases;
    }
    printf("EFFECT CLEANUP PASS %u cases: full memory, image restore and free ordering\n",cases);
}
