#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
/* PsyQ's unused 16-bit user/group typedefs differ from the host process API. */
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include OBJECT_OVERLAY_SOURCE
#undef uid_t
#undef gid_t
#include "battle_mips_adapter.h"
#ifdef OBJECT_TREE_SCALED
#define func_801DC5C0 func_801DC848
#define TREE_ENTRY 0x801dc848
#else
#define TREE_ENTRY 0x801dc5c0
#endif
extern s32 func_801DC5C0(u8*,s32) __attribute__((weak));
u8 g_PsxScratchpad[4096];
static u8 ram[0x200000], guestScratch[4096];
static u32 nodes[5][0x7c/4], initial[5][0x7c/4], expected[5][0x7c/4];
static struct Record {u32 kind;u8 a[32],b[32];} records[32],expectedRecords[32];
static unsigned nRecords;
static void record(u32 kind,void* a,unsigned size,void* b) {
    assert(nRecords<32);struct Record* r=&records[nRecords++];
    r->kind=kind;memset(r->a,0,32);memcpy(r->a,a,size);
    if(b)memcpy(r->b,b,32);else memset(r->b,0,32);
}
static MATRIX* rotation(u32 kind,SVECTOR* r,MATRIX* m) {
    record(kind,r,6,NULL);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        m->m[i][j]=(u16)((s16*)r)[i]+j*37+(kind&255);
    return m;
}
MATRIX* RotMatrix(SVECTOR* r,MATRIX* m){return rotation(0x8003f738,r,m);}
MATRIX* RotMatrixYXZ(SVECTOR* r,MATRIX* m){return rotation(0x8004a92c,r,m);}
static MATRIX* product(u32 kind,MATRIX* a,MATRIX* b,MATRIX* out) {
    MATRIX x=*a,y=*b;record(kind,a,32,b);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        out->m[i][j]=(u16)x.m[i][j]+(u16)y.m[j][i]+i*7+j;
    if(kind==0x8004931c)for(unsigned i=0;i<3;++i)out->t[i]=(u32)x.t[i]+(u32)y.t[i]+i;
    return out;
}
MATRIX* MulMatrix0(MATRIX* a,MATRIX* b,MATRIX* out){return product(0x8004920c,a,b,out);}
MATRIX* CompMatrix(MATRIX* a,MATRIX* b,MATRIX* out){return product(0x8004931c,a,b,out);}
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    if(a>=0x1f800000u && (uint64_t)a+w<=0x1f801000u)return guestScratch+(a-0x1f800000u);
    uintptr_t lo=(uintptr_t)nodes;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(nodes))return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int wr(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;
}
static int bridge(void* u,PcPortMipsCpu* c,u32 t) {
    (void)u;u32* a=c->gpr+4;
    switch(t) {
    case 0x8003f738:RotMatrix((SVECTOR*)address(a[0],6),(MATRIX*)address(a[1],32));break;
    case 0x8004a92c:RotMatrixYXZ((SVECTOR*)address(a[0],6),(MATRIX*)address(a[1],32));break;
    case 0x8004920c:MulMatrix0((MATRIX*)address(a[0],32),(MATRIX*)address(a[1],32),(MATRIX*)address(a[2],32));break;
    case 0x8004931c:CompMatrix((MATRIX*)address(a[0],32),(MATRIX*)address(a[1],32),(MATRIX*)address(a[2],32));break;
    default:return 0;
    }
    c->gpr[2]=a[t==0x8004920c||t==0x8004931c?2:1];return 1;
}
int main(void) {
    if(!func_801DC5C0){fputs("OBJECT TREE FAIL missing native owner\n",stderr);return 1;}
    FILE* f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s32 scales[]={0,1,-1,4096,32767,-32768,0x7fffffff,(-2147483647-1)};
    unsigned cases=0;
    for(unsigned count=0;count<=5;++count)for(unsigned flags=0;flags<256;++flags)
    for(unsigned topology=0;topology<3;++topology)for(unsigned scale=0;scale<8;++scale){
        for(unsigned i=0;i<sizeof(nodes)/4;++i)((u32*)nodes)[i]=0x1001001u*(i+1);
        for(unsigned i=0;i<5;++i){
            u8* n=(u8*)nodes[i];
            nodes[i][0]=(!i||!topology)?0:(u32)(uintptr_t)nodes[topology==1?0:i-1];
            n[4]=(flags>>(i%4))&3;n[5]=(flags>>(i%4+2))&1;n[6]=(flags>>(i%4+3))&1;
#ifdef OBJECT_TREE_SCALED
            const s16 localScales[]={-32768,-4096,-3,-1,1,3,4096,32767};
            for(unsigned axis=0;axis<3;++axis)*(s16*)(n+0x4c+axis*2)=localScales[(scale+i+axis)%8];
#endif
        }
        *(u16*)((u8*)nodes+0xa)=count;memcpy(initial,nodes,sizeof(nodes));
        memset(g_PsxScratchpad,0x36,4096);memcpy(guestScratch,g_PsxScratchpad,4096);
        nRecords=0;memset(records,0,sizeof(records));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)nodes;cpu.gpr[5]=(u32)scales[scale];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,TREE_ENTRY,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"OBJECT TREE FAIL oracle %s\n",cpu.error);return 1;}
        memcpy(expected,nodes,sizeof(nodes));memcpy(expectedRecords,records,sizeof(records));unsigned expectedCount=nRecords;
        memcpy(nodes,initial,sizeof(nodes));nRecords=0;memset(records,0,sizeof(records));
        s32 result=func_801DC5C0((u8*)nodes,scales[scale]);
        if((u32)result!=cpu.gpr[2]||memcmp(nodes,expected,sizeof(nodes))||nRecords!=expectedCount||
           memcmp(records,expectedRecords,sizeof(records))||memcmp(guestScratch,g_PsxScratchpad,4096)){
            fprintf(stderr,"OBJECT TREE FAIL count=%u flags=%u topology=%u scale=%d\n",count,flags,topology,scales[scale]);return 1;
        }
        ++cases;
    }
#ifdef OBJECT_TREE_SCALED
    for(unsigned axis=0;axis<3;++axis){
        memset(nodes,0,sizeof(nodes));
        *(u16*)((u8*)nodes+0xa)=2;nodes[1][0]=(u32)(uintptr_t)nodes[0];
        ((u8*)nodes[1])[5]=1;
        for(unsigned a=0;a<3;++a){
            *(s16*)((u8*)nodes[0]+0x4c+a*2)=a==axis?0:4096;
            *(s16*)((u8*)nodes[1]+0x4c+a*2)=4096;
        }
        memcpy(initial,nodes,sizeof(nodes));nRecords=0;
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)nodes;cpu.gpr[5]=4096;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        int runResult=PcPortMipsRun(&cpu,TREE_ENTRY,0xfffffffcu,10000);
        const u32 breaks[]={0x801dca68,0x801dcab4,0x801dcb00};
        char expectedError[64];snprintf(expectedError,sizeof(expectedError),"break at pc=0x%08x",breaks[axis]);
        if(runResult!=PC_PORT_MIPS_UNSUPPORTED||strcmp(cpu.error,expectedError)){
            fprintf(stderr,"OBJECT TREE FAIL zero divisor oracle axis=%u: %s\n",axis,cpu.error);return 1;
        }
        memcpy(nodes,initial,sizeof(nodes));nRecords=0;
        pid_t child=fork();assert(child>=0);
        if(!child){
            struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));
            func_801DC5C0((u8*)nodes,4096);_exit(0);
        }
        int status;assert(waitpid(child,&status,0)==child);
        if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGILL){
            fprintf(stderr,"OBJECT TREE FAIL zero divisor native axis=%u status=%d\n",axis,status);return 1;
        }
    }
    puts("OBJECT TREE zero-divisor PASS: retail BREAK7 and host trap, all three axes");
#endif
    printf("OBJECT TREE PASS %u cases: hierarchy, flags, scale, scratch, matrix boundaries, return\n",cases);
}
