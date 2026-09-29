#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"
#include "psx/gtereg.h"
#include "battle_mips_adapter.h"
extern void func_801E22F8(u8*,s16*,MATRIX*,u32*,s32,s32,s32) __attribute__((weak));
u8 g_PsxScratchpad[4096];
s32 D_80050100;
static u8 ram[0x200000];
static struct Fixture {
    u32 owner[9],chains[2],vertices[4][6],constraints[2][4],faces[2][22],ot[4096];
    MATRIX matrix;s16 offset[4];
} fixture,initial,expected;
static struct Call {u32 kind;u8 input[32];} calls[64],expectedCalls[64];
static unsigned nCalls,projections,rejected;
static void record(u32 kind,const void*p,unsigned n){assert(nCalls<64);calls[nCalls].kind=kind;memcpy(calls[nCalls++].input,p,n);}
/* Controlled SDK boundaries, deliberately not production normal/sqrt math.
 * GTE instructions and register transfers use the actual PsyCross backend. */
int SquareRoot0(int value){record(1,&value,4);u32 n=(u32)value,lo=0,hi=65536;while(lo+1<hi){u32 m=(lo+hi)/2;if((uint64_t)m*m<=n)lo=m;else hi=m;}return (int)lo;}
long VectorNormal(VECTOR*a,VECTOR*b){record(2,a,12);s32 x=a->vx,y=a->vy,z=a->vz;b->vx=x/3;b->vy=y/3;b->vz=z/3;return 0;}
void SetRotMatrix(MATRIX*m){record(3,m,32);for(unsigned i=0;i<5;++i){u32 v;memcpy(&v,(u8*)m+i*4,4);CTC2(v,i);}}
void SetTransMatrix(MATRIX*m){record(4,m,32);for(unsigned i=0;i<3;++i)CTC2(m->t[i],5+i);}
static u8* address(u32 a,unsigned w){
    if(a==0x80050100u&&w==4)return(u8*)&D_80050100;
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
    if(t==0x80048c4c)c->gpr[2]=SquareRoot0(c->gpr[4]);
    else if(t==0x80048d7c)c->gpr[2]=VectorNormal((VECTOR*)address(c->gpr[4],16),(VECTOR*)address(c->gpr[5],16));
    else if(t==0x80049efc)SetRotMatrix((MATRIX*)address(c->gpr[4],32));
    else if(t==0x80049f8c)SetTransMatrix((MATRIX*)address(c->gpr[4],32));
    else return 0;
    return 1;
}
static u32 copRead(void*u,int control,unsigned reg){(void)u;return control?CFC2(reg):MFC2(reg);}
static void copWrite(void*u,int control,unsigned reg,u32 value){(void)u;if(control)CTC2(value,reg);else MTC2(value,reg);}
static int copCommand(void*u,u32 ins){(void)u;doCOP2(ins&0x1ffffffu);if((ins&0x3f)==0x30){++projections;if(CFC2(31)&0x40000u)++rejected;}return 0;}
static void reset(void){memset(&gteRegs,0,sizeof(gteRegs));CTC2(160u<<16,24);CTC2(112u<<16,25);CTC2(256,26);CTC2(0x555,29);CTC2(0x10001000,8);CTC2(0x10001000,16);CTC2(0x400,13);CTC2(0x600,14);CTC2(0x800,15);}
int main(void){
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;const s32 scales[]={0,4096,-4096,8192};
    for(unsigned enabled=0;enabled<2;++enabled)for(unsigned motion=0;motion<2;++motion)for(unsigned constraint=0;constraint<3;++constraint)
    for(unsigned buffer=0;buffer<2;++buffer)for(unsigned variant=0;variant<8;++variant)for(unsigned scale=0;scale<4;++scale){
        memset(&fixture,0,sizeof(fixture));u8*o=(u8*)fixture.owner;
        fixture.owner[5]=enabled?1:0;fixture.owner[6]=(u32)(uintptr_t)fixture.constraints;
        fixture.owner[7]=(u32)(uintptr_t)fixture.chains;fixture.owner[8]=(u32)(uintptr_t)fixture.faces;
        *(s16*)(o+4)=2;*(s16*)(o+6)=2;*(s16*)(o+8)=4;*(s16*)(o+0xa)=constraint;
        for(unsigned i=0;i<6;++i)o[0xc+i]=32+i*17;
        fixture.chains[0]=(u32)(uintptr_t)fixture.vertices[0];fixture.chains[1]=(u32)(uintptr_t)fixture.vertices[2];
        for(unsigned i=0;i<4;++i){s16*v=(s16*)fixture.vertices[i];v[0]=(motion&&!(i&1))?40:0;v[1]=variant-3;v[2]=(i&1)?32:-32;v[3]=(i&2)?32:-32;v[4]=variant==7?0:128;}
        for(unsigned i=0;i<2;++i){s16*c=(s16*)fixture.constraints[i];c[4]=i*16;c[5]=i*8;c[6]=128;c[7]=variant&1?100:1;}
        const s16 indices[2][3]={{0,1,2},{1,3,2}};
        for(unsigned i=0;i<2;++i){u8*p=(u8*)fixture.faces[i];memcpy(p,indices[i],6);if(variant&1){s16 t=*(s16*)p;*(s16*)p=*(s16*)(p+2);*(s16*)(p+2)=t;}for(unsigned k=0;k<2;++k){u8*q=p+8+k*40;memset(q,0xa5,40);q[3]=9;q[7]=0x34;}}
        for(unsigned i=0;i<4096;++i)fixture.ot[i]=0x12000000u|((i*4+0x60000)&0xffffff);
        for(unsigned i=0;i<3;++i)fixture.matrix.m[i][i]=4096;
        fixture.matrix.t[2]=variant==6?-1024:variant==7?0:512;
        fixture.offset[0]=variant*3;fixture.offset[1]=1;fixture.offset[2]=-2;D_80050100=variant&3;
        initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));reset();
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge,.cop2_read=copRead,.cop2_write=copWrite,.cop2_command=copCommand};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)o;cpu.gpr[5]=(u32)(uintptr_t)fixture.offset;cpu.gpr[6]=(u32)(uintptr_t)&fixture.matrix;cpu.gpr[7]=(u32)(uintptr_t)fixture.ot;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        wr(NULL,0x801ff010,4,buffer);wr(NULL,0x801ff014,4,scales[scale]);wr(NULL,0x801ff018,4,variant&1?0:100);
        if(PcPortMipsRun(&cpu,0x801e22f8,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"LINKED GEOMETRY FAIL oracle variant=%u: %s\n",variant,cpu.error);return 1;}
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;GTERegisters expectedGte=gteRegs;
        if(func_801E22F8){fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));reset();func_801E22F8(o,fixture.offset,&fixture.matrix,fixture.ot,buffer,scales[scale],variant&1?0:100);
            if(memcmp(&fixture,&expected,sizeof(fixture))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))||memcmp(&gteRegs,&expectedGte,sizeof(gteRegs))){fprintf(stderr,"LINKED GEOMETRY FAIL enabled=%u motion=%u constraints=%u buffer=%u variant=%u scale=%u\n",enabled,motion,constraint,buffer,variant,scale);return 1;}}
        ++cases;
    }
    printf("LINKED GEOMETRY retail fixtures %u, RTPT=%u rejected=%u; controlled SDK and shared GTE backend\n",cases,projections,rejected);
    if(!func_801E22F8){fputs("LINKED GEOMETRY FAIL missing native owner\n",stderr);return 1;}
    puts("LINKED GEOMETRY PASS complete body/memory/GTE comparisons");
}
