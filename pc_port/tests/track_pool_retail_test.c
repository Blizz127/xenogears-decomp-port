#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u8* func_801DF5F4(u8*,s32) __attribute__((weak));
extern void func_801DF668(u8*) __attribute__((weak));
extern void func_801DF6A8(u8*) __attribute__((weak));
extern u8* func_801DF6F0(u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 header[2];u8 entries[65535*20];} fixture,initial,expected;
static u32 calls[4][4],expectedCalls[4][4];static unsigned nCalls;static int failAllocation;
void HeapChangeCurrentUser(u32 tag,char** names){assert(nCalls<4);calls[nCalls][0]=1;calls[nCalls][1]=tag;calls[nCalls++][2]=(u32)(uintptr_t)names;}
void* HeapAlloc(u32 size,u32 flags){assert(nCalls<4);calls[nCalls][0]=2;calls[nCalls][1]=size;calls[nCalls++][2]=flags;return failAllocation?NULL:fixture.entries;}
u_int HeapFree(void*p){assert(nCalls<4);calls[nCalls][0]=3;calls[nCalls][1]=(u32)(uintptr_t)p;calls[nCalls][2]=fixture.header[0];calls[nCalls++][3]=fixture.header[1];return 0;}
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
    else if(t==0x80031bdc)c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);
    else if(t==0x800320e8)c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);
    else return 0;return 1;
}
static int compare(unsigned kind,s32 count){
    initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
    const u32 entries[]={0x801df5f4,0x801df668,0x801df6a8,0x801df6f0};
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)(uintptr_t)fixture.header;cpu.gpr[5]=(u32)count;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,entries[kind],0xfffffffcu,2000000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TRACK POOL FAIL oracle %s\n",cpu.error);return 1;}
    expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));u8*result=NULL;
    switch(kind){
    case 0:result=func_801DF5F4((u8*)fixture.header,count);break;
    case 1:func_801DF668((u8*)fixture.header);break;
    case 2:func_801DF6A8((u8*)fixture.header);break;
    case 3:result=func_801DF6F0((u8*)fixture.header);break;
    }
    if(((kind==0||kind==3)&&(u32)(uintptr_t)result!=cpu.gpr[2])||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"TRACK POOL FAIL kind=%u count=%d\n",kind,count);return 1;}
    return 0;
}
int main(void){
    if(sizeof(D_801E86A8)!=8){fputs("TRACK POOL FAIL native header extent is not retail eight bytes\n",stderr);return 1;}
    if(!func_801DF5F4||!func_801DF668||!func_801DF6A8||!func_801DF6F0){fputs("TRACK POOL FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s32 sizes[]={(-2147483647-1),-1,0,1,2,8,65535,65536,65537,0x7fffffff};unsigned cases=0;
    for(unsigned i=0;i<10;++i)for(unsigned fail=0;fail<2;++fail){memset(&fixture,0xa5,sizeof(fixture));failAllocation=fail;if(compare(0,sizes[i]))return 1;++cases;}
    for(unsigned kind=1;kind<=2;++kind)for(unsigned null=0;null<2;++null)for(unsigned count=0;count<9;++count){
        memset(&fixture,0xa5,sizeof(fixture));fixture.header[0]=null?0:(u32)(uintptr_t)fixture.entries;
        fixture.header[1]=(count<<16)|7;if(compare(kind,count))return 1;++cases;
    }
    for(unsigned bits=0;bits<256;++bits)for(unsigned count=0;count<=8;++count)for(unsigned next=0;next<=9;++next){
        memset(&fixture,0xa5,sizeof(fixture));fixture.header[0]=(u32)(uintptr_t)fixture.entries;
        fixture.header[1]=(count<<16)|next;
        for(unsigned i=0;i<8;++i)fixture.entries[i*20]=(bits>>i)&1;
        if(compare(3,count))return 1;++cases;
    }
    printf("TRACK POOL PASS %u cases: create/reset/claim/destroy, full memory and heap boundaries\n",cases);
}
