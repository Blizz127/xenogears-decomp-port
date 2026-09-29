#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
extern void func_8001FBE4(void*,uint32_t,void*);
/* Link guards only: these data handlers must not call a dependency.
 * These are not replacement implementations and never enter the port. */
#define GUARD(name) void name(void){fputs("SPRITE C5 FAIL unexpected callee " #name "\n",stderr);abort();}
GUARD(func_800B2AEC) GUARD(func_8001D4E8)
uint32_t g_WorkListCurTimer, D_8006BE10;
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE C5 FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE C5 FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE C5 FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE C5 FAIL unexpected angle helper\n",stderr); abort(); }
uint32_t D_80018644,D_80059198,D_800592E4;
uint8_t D_800591AD,D_800591B0,D_800591B3;
int16_t D_800592E8,D_800592EA;
uint32_t D_8004FBB8[8],D_8006F99C[4],D_8006F9AC[4],D_800C3EB0[4];

/* C5 boundary proof: execute the complete retail dispatcher and bridge only
 * the motion callee. The spy changes both inputs after each call, proving
 * operand/global sampling, argument identity, call count and unchanged guards.
 * This does not claim equivalence of the motion implementation itself. */
static uint8_t ram[0x200000];
static struct { uint32_t before[8], sprite[64], ops[4], after[8]; } fixture, initial, expected;
static unsigned calls, cases;
static uint8_t *active_ops;
void func_80022CDC(void *sprite) {
    assert(sprite == fixture.sprite);
    ++calls;
    ((uint8_t*)sprite)[0] += 13;
    active_ops[0] ^= 0xff;
    D_80059198 ^= 0x12345678u;
}
static uint8_t *address(uint32_t a, unsigned w) {
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture)) return (uint8_t*)(uintptr_t)a;
    if(a==0x80059198u && w==4) return (uint8_t*)&D_80059198;
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u) return ram+(a&0x1fffff);
    return NULL;
}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v) {
    (void)u; uint8_t*p=address(a,w); if(!p)return -1;
    *v=0; for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v) {
    (void)u; uint8_t*p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;
}
static int bridge(void*u,PcPortMipsCpu*c,uint32_t target) {
    (void)u;
    if(target!=0x80022cdcu)return 0;
    func_80022CDC((void*)(uintptr_t)c->gpr[4]);return 1;
}
static void compare(unsigned byte,uint32_t timing,unsigned alias) {
    for(unsigned i=0;i<sizeof(fixture);++i)((uint8_t*)&fixture)[i]=(uint8_t)(i*37u+byte);
    active_ops=alias?(uint8_t*)fixture.sprite:(uint8_t*)fixture.ops;
    active_ops[0]=(uint8_t)byte; initial=fixture; D_80059198=timing;calls=0;
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(uint32_t)(uintptr_t)fixture.sprite;
    cpu.gpr[5]=alias?0xdeadbec5u:0xc5u;
    cpu.gpr[6]=(uint32_t)(uintptr_t)active_ops;
    cpu.gpr[29]=0x801fff00u;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x8001fbe4u,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"SPRITE C5 oracle error %s\n",cpu.error);exit(2);
    }
    unsigned expected_calls=calls;uint32_t expected_timing=D_80059198;
    expected=fixture;fixture=initial;D_80059198=timing;calls=0;
    func_8001FBE4(fixture.sprite,cpu.gpr[5],active_ops);
    if(calls!=expected_calls||D_80059198!=expected_timing||memcmp(&fixture,&expected,sizeof(fixture))) {
        fprintf(stderr,"SPRITE C5 FAIL case=%u byte=%u timing=%08x alias=%u calls=%u expected=%u\n",cases,byte,timing,alias,calls,expected_calls);exit(1);
    }
    ++cases;
}
int main(void) {
    assert((uintptr_t)&fixture+sizeof(fixture)<=UINT32_MAX);
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));
    assert(fread(ram+0x10000,1,sizeof(ram)-0x10000,f)>0x48000);assert(!fclose(f));
    compare(8,0,0);
    for(unsigned b=0;b<256;++b)for(unsigned t=0;t<256;++t)for(unsigned a=0;a<2;++a)compare(b,t,a);
    const uint32_t edges[]={256,65535,0x7ffffffe,0x7fffffff,0x80000000,0xfffffeff};
    for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);++i)for(unsigned b=0;b<256;++b)for(unsigned a=0;a<2;++a)compare(b,edges[i],a);
    printf("SPRITE C5 PASS %u bounded cases: all byte operands/timing0..255, signed denominator edges, aliased operands, callee mutations\n",cases);
}
