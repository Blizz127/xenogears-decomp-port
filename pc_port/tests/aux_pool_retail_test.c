#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u8* func_801E0064(u8*,s32) __attribute__((weak));
extern void func_801E00DC(u8*) __attribute__((weak));
extern void func_801E011C(u8*) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 header[2];u8 entries[32768*124+16];} fixture,initial,expected;
static u32 calls[262150][5],expectedCalls[262150][5];
static unsigned nCalls;static int failAllocation;
static int splitPools;static unsigned allocationCalls,allocationFailureMask;
static void call(u32 id,u32 a,u32 b,u32 c,u32 d){assert(nCalls<262150);u32*p=calls[nCalls++];p[0]=id;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
void HeapChangeCurrentUser(u32 tag,char** names){call(1,tag,(u32)(uintptr_t)names,fixture.header[0],fixture.header[1]);}
void* HeapAlloc(u32 size,u32 flags){
    call(2,size,flags,fixture.header[0],fixture.header[1]);
#ifdef XENO_TEST_SPLIT_POOLS
    if(splitPools){unsigned index=allocationCalls++;assert(index<2);return allocationFailureMask&(1u<<index)?NULL:fixture.entries+index*0x10000;}
#endif
    return failAllocation?NULL:fixture.entries;
}
u_int HeapFree(void*p){call(3,(u32)(uintptr_t)p,0,fixture.header[0],fixture.header[1]);return 0;}
/* Controlled SDK boundary: distinctive writes/returns expose wrong offsets,
 * call order, or arguments; this fixture does not prove SDK packet semantics. */
void SetPolyFT4(POLY_FT4*p){call(4,(u32)(uintptr_t)p,0,0,0);((u8*)p)[3]=0x79;((u8*)p)[7]=0x53;}
void SetSemiTrans(void*p,int abe){call(5,(u32)(uintptr_t)p,abe,0,0);((u8*)p)[7]^=(u8)abe;}
u_short GetClut(int x,int y){call(6,x,y,0,0);return 0x9347;}
u_short GetTPage(int tp,int abr,int x,int y){call(7,tp,abr,x,y);return 0x29b1;}
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
    case 0x80043cb0:SetPolyFT4((POLY_FT4*)(uintptr_t)a[0]);break;
    case 0x80043bfc:SetSemiTrans((void*)(uintptr_t)a[0],a[1]);break;
    case 0x80043a58:c->gpr[2]=GetClut(a[0],a[1]);break;
    case 0x80043a1c:c->gpr[2]=GetTPage(a[0],a[1],a[2],a[3]);break;
    default:return 0;
    }return 1;
}
static int compare(unsigned kind,s32 count){
    initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
    const u32 entries[]={0x801e0064,0x801e00dc,0x801e011c};
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)(uintptr_t)fixture.header;cpu.gpr[5]=(u32)count;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,entries[kind],0xfffffffcu,10000000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"AUX POOL FAIL oracle %s\n",cpu.error);return 1;}
    expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));u8*result=NULL;
    if(kind==0)result=func_801E0064((u8*)fixture.header,count);
    else if(kind==1)func_801E00DC((u8*)fixture.header);
    else func_801E011C((u8*)fixture.header);
    if((kind==0&&(u32)(uintptr_t)result!=cpu.gpr[2])||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){
        fprintf(stderr,"AUX POOL FAIL kind=%u count=%d calls=%u/%u return=%x/%x\n",kind,count,nCalls,expectedCount,(u32)(uintptr_t)result,cpu.gpr[2]);
        for(unsigned i=0;i<sizeof(fixture);++i)if(((u8*)&fixture)[i]!=((u8*)&expected)[i]){fprintf(stderr,"byte %x: %x/%x\n",i,((u8*)&fixture)[i],((u8*)&expected)[i]);break;}
        for(unsigned i=0;i<nCalls&&i<expectedCount;++i)if(memcmp(calls[i],expectedCalls[i],sizeof(calls[i]))){fprintf(stderr,"call %u: %x %x %x %x %x / %x %x %x %x %x\n",i,calls[i][0],calls[i][1],calls[i][2],calls[i][3],calls[i][4],expectedCalls[i][0],expectedCalls[i][1],expectedCalls[i][2],expectedCalls[i][3],expectedCalls[i][4]);break;}
        return 1;
    }
    return 0;
}
int main(void){
    if(!func_801E0064||!func_801E00DC||!func_801E011C){fputs("AUX POOL FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s32 sizes[]={(-2147483647-1),-32769,-32768,-1,0,1,2,16,32767,32768,65535,65536,65537,0x7fffffff};unsigned cases=0;
    for(unsigned i=0;i<sizeof(sizes)/sizeof(*sizes);++i)for(unsigned fail=0;fail<2;++fail){
        memset(&fixture,0xa5,sizeof(fixture));failAllocation=fail;if(compare(0,sizes[i]))return 1;++cases;
    }
    for(unsigned kind=1;kind<=2;++kind)for(unsigned null=0;null<2;++null)for(unsigned count=0;count<32;++count){
        /* NULL reset is valid only when the signed capacity disables the loop. */
        memset(&fixture,0xa5,sizeof(fixture));fixture.header[0]=null?0:(u32)(uintptr_t)fixture.entries;
        fixture.header[1]=(0x9753u<<16)|(null&&kind==2?0xffff:count);
        if(compare(kind,count))return 1;++cases;
    }
    printf("AUX POOL PASS %u cases: constructor/reset/destructor, full memory and SDK/heap boundaries\n",cases);
}
