#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern u32 FieldDecodeScaleTrack(u8*,u8*,u32,u32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 node[0x7c/4],pool[2],track[5],stream[12];} fixture,initial,expected;
static u32 calls[8][3],expectedCalls[8][3];static unsigned nCalls;
int SquareRoot0(int a){assert(nCalls<8);calls[nCalls][0]=0x80048c4c;calls[nCalls][1]=a;calls[nCalls++][2]=0;return ((u32)a>>17)+3;}
int ratan2(int y,int x){assert(nCalls<8);calls[nCalls][0]=0x8004b32c;calls[nCalls][1]=y;calls[nCalls++][2]=x;return ((u32)y+3u*(u32)x)&0xfff;}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v){(void)u;u8*p=address(a,w);if(!p)return -1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void* u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return -1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void* u,PcPortMipsCpu*c,u32 t){
    (void)u;
    if(t==0x80048c4c)c->gpr[2]=SquareRoot0(c->gpr[4]);
    else if(t==0x8004b32c)c->gpr[2]=ratan2(c->gpr[4],c->gpr[5]);
    else return 0;
    return 1;
}
int main(void){
    if(!FieldDecodeScaleTrack){fputs("SCALE TRACK FAIL missing native stage\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s16 times[]={-32768,-1,0,1,2,31999,32000,32767};
    unsigned cases=0;
    for(unsigned flags=0;flags<256;++flags)for(unsigned behavior=0;behavior<8;++behavior)
    for(unsigned time=0;time<8;++time)for(unsigned data=0;data<5;++data){
        memset(&fixture,0xa5,sizeof(fixture));u8*node=(u8*)fixture.node;u8*track=(u8*)fixture.track;
        *(u32*)(node+0x78)=behavior==7?0:(u32)(uintptr_t)track;
        fixture.pool[0]=(u32)(uintptr_t)track;fixture.pool[1]=0x12340010;
        track[0]=1;track[1]=behavior&1;track[2]=flags;track[3]=behavior&2?0x23:0x24;
        *(s16*)(track+0x10)=times[time];*(s16*)(track+0x12)=behavior&4?-3:3;
        if((flags&15)<3){
            fixture.track[1]=(u32)(uintptr_t)fixture.stream;fixture.track[2]=(u32)(uintptr_t)fixture.stream;
        }else{
            *(s16*)(track+4)=data&1?-1:4096;*(s16*)(track+6)=data&2?-32768:11;
            *(s16*)(track+8)=data&1?32767:-7;
            *(s16*)(track+0xa)=data&1?-32768:17;*(s16*)(track+0xc)=data&2?32767:-31;
            *(s16*)(track+0xe)=data&1?-1:0;
        }
        for(unsigned i=0;i<sizeof(fixture.stream);++i)((u8*)fixture.stream)[i]=data==0?0x80:data==1?0x7f:data==2?0xff:(u8)i;
        *(u16*)(node+0x4c)=data==0?17:0x8000;*(u16*)(node+0x4e)=data==0?0xfff:0x7fff;*(u16*)(node+0x50)=data==0?-31:0xffff;
        if(data==4&&(flags&15)==4)memcpy(track+4,track+0xa,6);
        initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[17]=(u32)(uintptr_t)node+4;cpu.gpr[21]=0x80000000;cpu.gpr[29]=0x801ff000;
        u32 pool=(u32)(uintptr_t)fixture.pool;memcpy(ram+0x1ff028,&pool,4);u32 tag=0x23;memcpy(ram+0x1ff038,&tag,4);
        if(PcPortMipsRun(&cpu,0x801dead0,0x801deeb0,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SCALE TRACK FAIL oracle %s flags=%u\n",cpu.error,flags);return 1;}
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
        fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));
        u32 result=FieldDecodeScaleTrack(node,(u8*)fixture.pool,0x23,0x80000000);
        if(result!=cpu.gpr[21]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){
            fprintf(stderr,"SCALE TRACK FAIL flags=%u behavior=%u time=%d data=%u\n",flags,behavior,times[time],data);return 1;
        }
        ++cases;
    }
    printf("SCALE TRACK PASS %u cases: all modes, masks, lifecycle, stream, SDK boundaries\n",cases);
}
