#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
/* Isolate unused PsyQ typedefs from the host process API. */
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include OBJECT_OVERLAY_SOURCE
#undef uid_t
#undef gid_t
#include "battle_mips_adapter.h"
extern u32 func_801DDBF8(u8*,u8*,u32,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 nodes[4][0x7c/4],pool[2],tracks[12][5],streams[12][12];} fixture,initial,expected;
static u32 calls[64][3],expectedCalls[64][3];static unsigned nCalls;
static MATRIX matrices[64],expectedMatrices[64];
static s16 vectors[64][3],expectedVectors[64][3];
VECTOR* ApplyMatrix(MATRIX*m,SVECTOR*v,VECTOR*out){
    assert(nCalls<64);matrices[nCalls]=*m;memcpy(vectors[nCalls],v,6);
    calls[nCalls++][0]=0x80049cec;
    out->vx=(u32)m->t[0]+(u32)(s32)v->vx*17u;
    out->vy=(u32)m->t[1]+(u32)(s32)v->vy*31u;
    out->vz=(u32)m->t[2]+(u32)(s32)v->vz*63u;
    return out;
}
int SquareRoot0(int a){assert(nCalls<64);calls[nCalls][0]=0x80048c4c;calls[nCalls][1]=a;calls[nCalls++][2]=0;return ((u32)a>>17)+3;}
int ratan2(int y,int x){assert(nCalls<64);calls[nCalls][0]=0x8004b32c;calls[nCalls][1]=y;calls[nCalls++][2]=x;return ((u32)y+3u*(u32)x)&0xfff;}
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
    if(t==0x80048c4c){c->gpr[2]=SquareRoot0(c->gpr[4]);return 1;}
    if(t==0x8004b32c){c->gpr[2]=ratan2(c->gpr[4],c->gpr[5]);return 1;}
    if(t!=0x80049cec)return 0;
    ApplyMatrix((MATRIX*)address(c->gpr[4],32),(SVECTOR*)address(c->gpr[5],6),(VECTOR*)address(c->gpr[6],12));
    c->gpr[2]=c->gpr[6];
    return 1;
}

int main(void){
    if(!func_801DDBF8){fputs("TRACK DECODER FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s16 frames[]={-1,0,1,2,32767,-32768,31999,32000};
    const s32 scales[]={0,1,-1,4096,32767,-32768,0x7fffffff,(-2147483647-1)};
    unsigned cases=0;
    for(unsigned modes=0;modes<4096;++modes)for(unsigned variant=0;variant<8;++variant){
        memset(&fixture,0xa5,sizeof(fixture));
        for(unsigned i=0;i<sizeof(fixture.nodes)/4;++i)((u32*)fixture.nodes)[i]=0x1001001u*(i+1);
        for(unsigned nodeIndex=0;nodeIndex<4;++nodeIndex){
            u8*node=(u8*)fixture.nodes[nodeIndex];
            for(unsigned stage=0;stage<3;++stage){
                unsigned slot=nodeIndex*3+stage;
                u8*track=(u8*)fixture.tracks[slot];
                unsigned mode=((modes>>(stage*4))+nodeIndex)&15;
                *(u32*)(node+0x70+stage*4)=variant==7&&nodeIndex==stage?0:(u32)(uintptr_t)track;
                track[0]=1;track[1]=(variant+stage)&1;track[2]=mode|(((variant+nodeIndex)&7)<<4);track[3]=(variant+stage)&2?0x23:0x24;
                *(s16*)(track+0x10)=frames[variant];*(s16*)(track+0x12)=variant&4?-3:3;
                if(mode<3&&stage<2){
                    fixture.tracks[slot][1]=(u32)(uintptr_t)fixture.streams[slot];
                    fixture.tracks[slot][2]=(u32)(uintptr_t)fixture.streams[slot];
                }else{
                    *(s16*)(track+4)=4096;*(s16*)(track+6)=-17;*(s16*)(track+8)=31;
                    *(s16*)(track+0xa)=variant&1?-32768:32767;
                    *(s16*)(track+0xc)=variant&2?-1:0;
                    *(s16*)(track+0xe)=variant&1?17:-31;
                }
                for(unsigned i=0;i<sizeof(fixture.streams[slot]);++i)((u8*)fixture.streams[slot])[i]=variant&1?0x80:(u8)(i+slot);
            }
        }
        *(u16*)((u8*)fixture.nodes+0xa)=variant%5;
        fixture.pool[0]=(u32)(uintptr_t)fixture.tracks;fixture.pool[1]=0x1234000c;
        for(unsigned tick=0;tick<2;++tick){
            initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));memset(matrices,0,sizeof(matrices));memset(vectors,0,sizeof(vectors));
            PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
            cpu.gpr[4]=(u32)(uintptr_t)fixture.pool;cpu.gpr[5]=(u32)(uintptr_t)fixture.nodes;
            cpu.gpr[6]=0x23;cpu.gpr[7]=scales[variant];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
            if(PcPortMipsRun(&cpu,0x801ddbf8,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TRACK DECODER FAIL oracle %s modes=%u variant=%u tick=%u\n",cpu.error,modes,variant,tick);return 1;}
            expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));memcpy(expectedMatrices,matrices,sizeof(matrices));memcpy(expectedVectors,vectors,sizeof(vectors));unsigned expectedCount=nCalls;
            fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));memset(matrices,0,sizeof(matrices));memset(vectors,0,sizeof(vectors));
            u32 result=func_801DDBF8((u8*)fixture.pool,(u8*)fixture.nodes,0x23,scales[variant]);
            if(result!=cpu.gpr[2]||memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))||memcmp(matrices,expectedMatrices,sizeof(matrices))||memcmp(vectors,expectedVectors,sizeof(vectors))){
                fprintf(stderr,"TRACK DECODER FAIL modes=%u variant=%u tick=%u\n",modes,variant,tick);return 1;
            }
            ++cases;
        }
    }
    const u32 breakAddresses[]={0x801ddee8,0x801de758,0x801deb58,0x801de860};
    for(unsigned kind=0;kind<4;++kind){
        memset(&fixture,0,sizeof(fixture));nCalls=0;
        u8*node=(u8*)fixture.nodes;u8*track=(u8*)fixture.tracks;
        *(u16*)(node+0xa)=1;
        unsigned stage=kind==3?1:kind;
        *(u32*)(node+0x70+stage*4)=(u32)(uintptr_t)track;
        track[0]=1;track[2]=kind==3?4:3;
        if(kind==3){*(u32*)(node+0x5c)=0x80000000;*(s16*)(track+0x12)=-1;}
        initial=fixture;
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.pool;cpu.gpr[5]=(u32)(uintptr_t)node;
        cpu.gpr[6]=0x23;cpu.gpr[7]=4096;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        int result=PcPortMipsRun(&cpu,0x801ddbf8,0xfffffffcu,100000);
        char expectedError[64];snprintf(expectedError,sizeof(expectedError),"break at pc=0x%08x",breakAddresses[kind]);
        if(result!=PC_PORT_MIPS_UNSUPPORTED||strcmp(cpu.error,expectedError)){
            fprintf(stderr,"TRACK DECODER FAIL exception oracle kind=%u: %s\n",kind,cpu.error);return 1;
        }
        fixture=initial;nCalls=0;
        pid_t child=fork();assert(child>=0);
        if(!child){
            struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));
            func_801DDBF8((u8*)fixture.pool,node,0x23,4096);_exit(0);
        }
        int status;assert(waitpid(child,&status,0)==child);
        if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGILL){
            fprintf(stderr,"TRACK DECODER FAIL exception native kind=%u status=%d\n",kind,status);return 1;
        }
    }
    puts("TRACK DECODER exceptions PASS: three zero-divisor stages and signed overflow");
    printf("TRACK DECODER PASS %u cases: complete retail function, nodes0..4, mixed stages, two ticks\n",cases);
}
