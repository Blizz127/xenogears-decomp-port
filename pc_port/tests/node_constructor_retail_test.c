#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u8* func_801DC2D0(u8*,u16*,s32,s32,s16,s16,s16,s16) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 table[2],ptrs[4];u16 list[18];u32 nodes[9*124/4];u8 models[4][56],packets[8][2][64];} fixture,initial,expected;
static u32 calls[128][5],expectedCalls[128][5];static unsigned nCalls,buildCalls;static int allocationFail,packetFail;
static void call(u32 id,u32 a,u32 b,u32 c,u32 d){assert(nCalls<128);u32*p=calls[nCalls++];p[0]=id;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
void HeapChangeCurrentUser(u32 tag,char**names){call(1,tag,(u32)(uintptr_t)names,0,0);}
void* HeapAlloc(u32 size,u32 flags){call(2,size,flags,0,0);return allocationFail?NULL:fixture.nodes;}
u_int HeapFree(void*p){call(3,(u32)(uintptr_t)p,*(u16*)((u8*)fixture.nodes+10),0,0);return 0;}
void func_8002CB54(u8*model,u32*a,u32*b){
    unsigned index=buildCalls++;assert(index<8);call(4,(u32)(uintptr_t)model,(u32)(uintptr_t)a,(u32)(uintptr_t)b,index);
    *a=(int)index==packetFail?0:(u32)(uintptr_t)fixture.packets[index][0];*b=(u32)(uintptr_t)fixture.packets[index][1];
}
void func_8002CC10(u16 x,u16 y){call(5,(u32)(s32)(s16)x,(u32)(s32)(s16)y,0,0);}
void func_8002CC74(u16 x,u16 y){call(6,(u32)(s32)(s16)x,(u32)(s32)(s16)y,0,0);}
void func_8002C8CC(u8*model,void*p,s32 mode){call(7,(u32)(uintptr_t)model,(u32)(uintptr_t)p,mode,0);((u8*)p)[3]^=(u8)mode;}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){
    (void)u;u32*a=c->gpr+4;
    switch(t){
    case 0x80032498:HeapChangeCurrentUser(a[0],(char**)(uintptr_t)a[1]);break;
    case 0x80031bdc:c->gpr[2]=(u32)(uintptr_t)HeapAlloc(a[0],a[1]);break;
    case 0x800320e8:c->gpr[2]=HeapFree((void*)(uintptr_t)a[0]);break;
    case 0x8002cb54:func_8002CB54((u8*)(uintptr_t)a[0],(u32*)(uintptr_t)a[1],(u32*)(uintptr_t)a[2]);break;
    case 0x8002cc10:func_8002CC10(a[0],a[1]);break;
    case 0x8002cc74:func_8002CC74(a[0],a[1]);break;
    case 0x8002c8cc:func_8002C8CC((u8*)(uintptr_t)a[0],(void*)(uintptr_t)a[1],a[2]);break;
    case 0x8003f968:assert(address(a[0],a[2])&&address(a[1],a[2]));memcpy((void*)(uintptr_t)a[0],(void*)(uintptr_t)a[1],a[2]);break;
    default:return 0;
    }return 1;
}
static int compare(unsigned records,unsigned pattern,s32 mode,s32 setup){
    memset(&fixture,0xa5,sizeof(fixture));fixture.table[0]=(u32)(uintptr_t)fixture.ptrs;fixture.table[1]=4;
    for(unsigned i=0;i<4;++i){fixture.ptrs[i]=(u32)(uintptr_t)fixture.models[i];*(u32*)(fixture.models[i]+52)=32+i*4;*(u16*)(fixture.models[i]+6)=i%2?3:0;}
    for(unsigned i=0;i<records;++i){fixture.list[i*2]=(pattern>>i)&1?0xffff:i%4;fixture.list[i*2+1]=(i+pattern)%3==0?0xffff:(u16)(i?i-1:0);}
    fixture.list[records*2]=4;fixture.list[records*2+1]=0;
    initial=fixture;nCalls=buildCalls=0;memset(calls,0,sizeof(calls));
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)(uintptr_t)fixture.table;cpu.gpr[5]=(u32)(uintptr_t)fixture.list;cpu.gpr[6]=mode;cpu.gpr[7]=setup;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    const s16 xy[4]={-32768,32767,-1,513};for(unsigned i=0;i<4;++i)wr(NULL,0x801ff010+i*4,4,(u32)(s32)xy[i]);
    if(PcPortMipsRun(&cpu,0x801dc2d0,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"NODE CONSTRUCTOR FAIL oracle %s\n",cpu.error);return 1;}
    expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    fixture=initial;nCalls=buildCalls=0;memset(calls,0,sizeof(calls));
    u8*result=func_801DC2D0((u8*)fixture.table,fixture.list,mode,setup,xy[0],xy[1],xy[2],xy[3]);
    if((u32)(uintptr_t)result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"NODE CONSTRUCTOR FAIL records=%u pattern=%x mode=%d setup=%d allocfail=%d packetfail=%d\n",records,pattern,mode,setup,allocationFail,packetFail);return 1;}
    fixture=initial;nCalls=buildCalls=0;memset(calls,0,sizeof(calls));
    OvlyPtrTab host={fixture.ptrs,4};
    result=OvlyBuildNodes(&host,fixture.list,mode,setup,xy[0],xy[1],xy[2],xy[3]);
    if((u32)(uintptr_t)result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fputs("NODE CONSTRUCTOR FAIL host adapter\n",stderr);return 1;}
    if(result)for(unsigned i=0;i<records;++i){u8*node=result+(i+1)*124;u16 modelIndex=fixture.list[i*2];
        /* A poisoned track pointer must never supply the drawing model. */
        *(u32*)(node+0x70)=0xdeadbeef;
        u8*want=modelIndex==0xffff?NULL:fixture.models[modelIndex];
        if(OvlyNodeModel(&host,node)!=want){fputs("NODE CONSTRUCTOR FAIL model lookup\n",stderr);return 1;}
    }
    return 0;
}
int main(void){
    if(!func_801DC2D0){fputs("NODE CONSTRUCTOR FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;
    for(unsigned records=0;records<=8;++records)for(unsigned pattern=0;pattern<256;pattern+=17)for(s32 mode=0;mode<=2;mode+=2)for(s32 setup=0;setup<=1;++setup)for(int failure=-2;failure<8;++failure){
        allocationFail=failure==-2;packetFail=failure;if(compare(records,pattern,mode,setup))return 1;++cases;
    }
    printf("NODE CONSTRUCTOR PASS %u cases: complete retail constructor and cleanup, controlled model/heap boundaries\n",cases);
}
