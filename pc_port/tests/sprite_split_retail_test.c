/* Execute pinned retail leaves; only projection is a recording boundary.
 * Including the real TU also permits checking its persistent scratch vectors. */
#define strlen retail_strlen
#ifndef SPRITE_RENDER_SOURCE
#define SPRITE_RENDER_SOURCE "../../src/slus_006.64/system/rendering.c"
#endif
#include SPRITE_RENDER_SOURCE
#undef strlen
#include "battle_mips_adapter.h"
#include <stdio.h>
#include <string.h>
extern void func_8001EE88(void*,void*,s16) __attribute__((weak));
extern void func_8001F1D4(void*,void*,s16) __attribute__((weak));
void* g_GfxCurWorkBuffer;
void* g_GfxCurWorkBufferEnd;
static u8 ram[0x200000];
static struct {
    u32 sprite[48],base[20],prims[63*6],packets[63*10+8],ot;
} fixture, initial, expected;
static SVECTOR calls[64][4],expectedCalls[64][4];
static unsigned nCalls,projectionMode;
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);
    return 0;
}
static int wr(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));
    return 0;
}
static u32 read32(u32 a) {u32 v;assert(!rd(NULL,a,4,&v));return v;}
static u32 projected(const SVECTOR* v) {
    s32 x=projectionMode ? -(s32)v->vx-17 : (s32)v->vx+23;
#ifdef SPRITE_TEST_SHADOW_PROJECTION
    /* Shadow fixtures vary projected Y across corners to test signed,
     * halfword-wrapped averaging. This is a recording boundary only. */
    s32 y=projectionMode ? v->vz*3+v->vx/2-7 : v->vz*2+v->vx+32767;
    return (u16)x|((u32)(u16)y<<16);
#else
    return (u16)x|((u32)(u16)(v->vy+41)<<16);
#endif
}
int RotTransPers4(SVECTOR* a,SVECTOR* b,SVECTOR* c,SVECTOR* d,
                 long* x0,long* x1,long* x2,long* x3,long* p,long* f) {
    SVECTOR* v[4]={a,b,c,d};long* out[4]={x0,x1,x2,x3};
    assert(nCalls<64);
    for(unsigned i=0;i<4;++i) {
        calls[nCalls][i]=*v[i];
        u32 xy=projected(v[i]);memcpy(out[i],&xy,4);
    }
    ++nCalls;*p=123;*f=0;return 99;
}
static int bridge(void* u,PcPortMipsCpu* c,u32 target) {
    (void)u;if(target!=0x8004a7bc)return 0;
    assert(nCalls<64);
    for(unsigned i=0;i<4;++i) {
        u8* v=address(c->gpr[4+i],8);assert(v);
        memcpy(&calls[nCalls][i],v,8);
        assert(!wr(NULL,read32(c->gpr[29]+16+4*i),4,projected(&calls[nCalls][i])));
    }
    ++nCalls;
    assert(!wr(NULL,read32(c->gpr[29]+32),4,123));
    assert(!wr(NULL,read32(c->gpr[29]+36),4,0));c->gpr[2]=99;return 1;
}
int main(void) {
    if(!func_8001EE88 || !func_8001F1D4) {
        fputs("SPRITE SPLIT FAIL missing native leaf\n",stderr);return 1;
    }
    FILE* file=fopen("disc/SLUS_006.64","rb");assert(file);
    assert(!fseek(file,0x800+0xee88,SEEK_SET));
    assert(fread(ram+0x1ee88,1,0x6a8,file)==0x6a8);assert(!fclose(file));
    static const s16 cuts[]={-32768,-33,-16,-1,0,1,8,16,32,63,32767};
    static const unsigned counts[]={0,1,3,63},shifts[]={0,1,4,15,31};
    unsigned cases=0;
    for(unsigned side=0;side<2;++side)
    for(unsigned variant=0;variant<16;++variant)
    for(unsigned ci=0;ci<4;++ci)
    for(unsigned si=0;si<5;++si)
    for(unsigned cut=0;cut<11;++cut)
    for(unsigned capacity=0;capacity<3;++capacity) {
        unsigned count=counts[ci];projectionMode=variant&1;
        memset(&fixture,0xa5,sizeof(fixture));
        fixture.sprite[0x20/4]=(u32)(uintptr_t)fixture.base;
        fixture.sprite[0x40/4]=(count<<2)|(shifts[si]<<8);
        fixture.sprite[0x3c/4]=(variant&3)<<3;
        fixture.base[0x30/4]=(u32)(uintptr_t)fixture.prims;
        fixture.ot=0xca123456;
        for(unsigned i=0;i<63;++i) {
            u8* p=(u8*)fixture.prims+i*24;
            s16 x=(i&1)?-13:0,y=(i%3)*11-16;
            memcpy(p,&x,2);memcpy(p+2,&y,2);
            p[4]=(i&1)?255:0;p[5]=250;p[6]=16;p[7]=32;
            p[8]=(variant&4)?0xec:3;p[9]=(variant&8)?0xd8:0;
            p[10]=0x1a;p[11]=0x82;p[12]=0x7c;p[13]=0x19;
            u32 color=0x2e9abcde,flags=(i&3)<<4;
            memcpy(p+16,&color,4);memcpy(p+20,&flags,4);
        }
        initial=fixture;
        memset(s_QuadWork8004FB98,0x37,sizeof(s_QuadWork8004FB98));
        memcpy(ram+0x4fb98,s_QuadWork8004FB98,32);
        u32 work=(u32)(uintptr_t)fixture.packets;
        u32 end=work+count*40+(capacity==0?0:capacity==1?1:(u32)-1);
        assert(!wr(NULL,0x80059580,4,work));assert(!wr(NULL,0x80059534,4,end));
        nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.sprite;
        cpu.gpr[5]=(u32)(uintptr_t)&fixture.ot;cpu.gpr[6]=(s32)cuts[cut];
        cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,side?0x8001f1d4:0x8001ee88,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"SPRITE SPLIT FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));
        unsigned expectedCount=nCalls;fixture=initial;
        g_GfxCurWorkBuffer=(void*)(uintptr_t)work;
        g_GfxCurWorkBufferEnd=(void*)(uintptr_t)end;
        nCalls=0;memset(calls,0,sizeof(calls));
        if(side)func_8001F1D4(fixture.sprite,&fixture.ot,cuts[cut]);
        else func_8001EE88(fixture.sprite,&fixture.ot,cuts[cut]);
        if(memcmp(&fixture,&expected,sizeof(fixture)) ||
           memcmp(s_QuadWork8004FB98,ram+0x4fb98,32) ||
           memcmp(calls,expectedCalls,sizeof(calls)) || nCalls!=expectedCount ||
           (uintptr_t)g_GfxCurWorkBuffer!=read32(0x80059580)) {
            fprintf(stderr,"SPRITE SPLIT FAIL side=%u variant=%u count=%u shift=%u cut=%d cap=%u calls=%u/%u\n",
                    side,variant,count,shifts[si],cuts[cut],capacity,nCalls,expectedCount);return 1;
        }
        ++cases;
    }
    printf("SPRITE SPLIT PASS %u cases: packets/scratch/projection inputs/OT/work cursor\n",cases);
}
