#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include OBJECT_OVERLAY_SOURCE
#undef uid_t
#undef gid_t
#undef rcos
#include "battle_mips_adapter.h"
extern s32 func_801E1258(u8*,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct Fixture {u32 effect[12],linked[12];u16 a[512],b[512],c[512],matrix[512],out[512];} fixture,initial,expected;
static struct Call {u32 kind,args[5],owner[12];} calls[16],expectedCalls[16];
static unsigned nCalls;
static void record(u32 kind,u32 a,u32 b,u32 c,u32 d,u32 e){
    assert(nCalls<16);struct Call*r=&calls[nCalls++];r->kind=kind;
    r->args[0]=a;r->args[1]=b;r->args[2]=c;r->args[3]=d;r->args[4]=e;
    memcpy(r->owner,fixture.effect,sizeof(r->owner));
}
int rcos(int angle){record(1,angle,0,0,0,0);return (angle&8191)-4096;}
u_int HeapFree(void*p){record(2,(u32)(uintptr_t)p,0,0,0,0);return 0;}
int LoadImage(RECT16*r,u_long*p){record(3,(u32)(uintptr_t)r,(u32)(uintptr_t)p,0,0,0);return 0;}
void func_80026F44(s32 n,s32 f,u16*d,const u16*a){
    record(4,n,f,(u32)(uintptr_t)d,(u32)(uintptr_t)a,0);
    for(s32 i=0;i<n;++i)d[i]=(u16)(a[i]+f);
}
void func_80026FE8(s32 n,s32 f,u16*d,const u16*a,const u16*b){
    record(5,n,f,(u32)(uintptr_t)d,(u32)(uintptr_t)a,(u32)(uintptr_t)b);
    for(s32 i=0;i<n;++i)d[i]=(u16)(a[i]-b[i]+f);
}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
    if(t==0x8003f8cc)c->gpr[2]=rcos(c->gpr[4]);
    else if(t==0x800320e8)c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);
    else if(t==0x80044894)c->gpr[2]=LoadImage((RECT16*)(uintptr_t)c->gpr[4],(u_long*)(uintptr_t)c->gpr[5]);
    else if(t==0x80026f44)func_80026F44(c->gpr[4],c->gpr[5],(u16*)(uintptr_t)c->gpr[6],(u16*)(uintptr_t)c->gpr[7]);
    else if(t==0x80026fe8){u32 a;assert(!rd(NULL,c->gpr[29]+0x10,4,&a));func_80026FE8(c->gpr[4],c->gpr[5],(u16*)(uintptr_t)c->gpr[6],(u16*)(uintptr_t)c->gpr[7],(u16*)(uintptr_t)a);}
    else return 0;return 1;
}
int main(void){
    if(!func_801E1258){fputs("EFFECT UPDATE FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const u32 callbacks[]={0x801e0850,0x801e08d4,0x801e0938,0x801e0988};
    const s32 ticks[]={-1,0,1,2147483647,(-2147483647-1)};unsigned cases=0;
    for(unsigned cb=0;cb<4;++cb)for(unsigned type=0;type<8;++type)for(unsigned link=0;link<3;++link)
    for(unsigned state=0;state<6;++state)for(unsigned tick=0;tick<5;++tick)for(int layout=-3;layout<=5;++layout){
        memset(&fixture,0,sizeof(fixture));
        for(unsigned i=0;i<512;++i){fixture.a[i]=i*47;fixture.b[i]=65535-i*91;fixture.c[i]=i*193;fixture.matrix[i]=i*57;fixture.out[i]=0xa55a;}
        u8*e=(u8*)fixture.effect;u8*l=(u8*)fixture.linked;
        fixture.effect[0]=link?(u32)(uintptr_t)l:0;
        fixture.effect[1]=(u32)(uintptr_t)(fixture.a+128);fixture.effect[2]=(u32)(uintptr_t)(fixture.b+128);
        fixture.effect[3]=(u32)(uintptr_t)(state==5?fixture.out+129:fixture.c+128);
        fixture.effect[7]=(u32)(uintptr_t)(fixture.matrix+128);fixture.effect[9]=callbacks[cb];
        e[0x10]=type;*(u16*)(e+0x12)=4;*(u16*)(e+0x14)=state==4?65530:state*8;
        *(u16*)(e+0x16)=state==5?65535:2;*(u16*)(e+0x18)=state==1?4:0xffff;
        *(u16*)(e+0x1a)=state!=0;*(s16*)(e+0x20)=2;*(s16*)(e+0x22)=state==3?-32:0;
        *(s16*)(e+0x28)=layout;*(s16*)(e+0x2a)=-layout;*(s16*)(e+0x2c)=3;*(s16*)(e+0x2e)=2;
        fixture.linked[1]=(u32)(uintptr_t)(fixture.out+128);l[0x10]=state&1?4:1;*(u16*)(l+0x1a)=link==2;
        *(s16*)(l+0x28)=-layout;*(s16*)(l+0x2a)=layout;*(s16*)(l+0x2c)=5;*(s16*)(l+0x2e)=4;
        if(layout>=4){
            l[0x10]=1;
            *(s16*)(e+0x28)=*(s16*)(e+0x2a)=layout==4?32767:-32768;
            *(s16*)(l+0x28)=*(s16*)(l+0x2a)=layout==4?-32768:32767;
            /* Matrix writes use absolute positions; supply a compensating
             * address so their wrapped sums remain inside this fixture. */
            fixture.effect[7]-=(u32)(s32)*(s16*)(e+0x28)*8u;
        }
        initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)e;cpu.gpr[5]=ticks[tick];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e1258,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"EFFECT UPDATE FAIL oracle %s\n",cpu.error);return 1;}
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
        fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));s32 result=func_801E1258(e,ticks[tick]);
        if((u32)result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){
            fprintf(stderr,"EFFECT UPDATE FAIL cb=%u type=%u link=%u state=%u tick=%u layout=%d\n",cb,type,link,state,tick,layout);return 1;
        }++cases;
    }
    /* Unknown callback addresses have no invented host fallback. */
    fixture=initial;fixture.effect[9]=0x12345678;*(u16*)((u8*)fixture.effect+0x1a)=1;
    pid_t child=fork();assert(child>=0);
    if(!child){struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));func_801E1258((u8*)fixture.effect,0);_exit(0);}
    int status;assert(waitpid(child,&status,0)==child);
    if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGILL){fputs("EFFECT UPDATE FAIL unknown callback fallback\n",stderr);return 1;}
    printf("EFFECT UPDATE PASS %u cases and unknown callback trap: full updater and callback/cleanup/matrix bodies; controlled GTE blend, trig, upload and heap boundaries\n",cases);
}
