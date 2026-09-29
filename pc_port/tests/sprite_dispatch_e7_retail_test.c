#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
extern void func_8001FBE4(void*,uint32_t,void*);
/* Link guards only: these data handlers must not call a dependency.
 * These are not replacement implementations and never enter the port. */
#define GUARD(name) void name(void){fputs("SPRITE E7 FAIL unexpected callee " #name "\n",stderr);abort();}
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE E7 FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE E7 FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE E7 FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE E7 FAIL unexpected angle helper\n",stderr); abort(); }
uint32_t D_80018644,D_80059198,D_800592E4;
uint8_t D_800591AD,D_800591B0,D_800591B3;
int16_t D_800592E8,D_800592EA;
uint32_t D_8004FBB8[8],D_8006F99C[4],D_8006F9AC[4],D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static struct {uint32_t sprite[64],base[16],aux[8];uint8_t ops[4];} fixture,initial,expected;
static uint8_t* address(uint32_t a,unsigned w)
{
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return (uint8_t*)(uintptr_t)a;
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    return NULL;
}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v)
{
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v)
{
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;
}
static unsigned cases;
static void compare(unsigned value, unsigned scale, unsigned layout, unsigned alias) {
    uint8_t* p = (uint8_t*)fixture.sprite;
    for (unsigned i=0;i<sizeof(fixture);++i) ((uint8_t*)&fixture)[i]=(uint8_t)(i*37u+value*13u+scale);
    uint8_t* bases[]={(uint8_t*)fixture.base,NULL,p+0x24,p+0x34,p+0x38};
    uint8_t* base=bases[layout];
    uint8_t* ops=alias?p+0x2b:fixture.ops;
    uint32_t pointer=(uint32_t)(uintptr_t)base;
    uint16_t initial_scale=(uint16_t)scale;
    memcpy(p+0x20,&pointer,4);memcpy(p+0x2c,&initial_scale,2);
    ops[0]=(uint8_t)value;ops[1]=(uint8_t)(value>>8);
    initial=fixture;
    PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    uint32_t opcode=alias?0xdeadbee7u:0xe7u;
    cpu.gpr[4]=(uint32_t)(uintptr_t)p;cpu.gpr[5]=opcode;cpu.gpr[6]=(uint32_t)(uintptr_t)ops;
    cpu.gpr[29]=0x801fff00;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x8001fbe4u,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE E7 oracle failure %s\n",cpu.error);exit(2);}
    expected=fixture;fixture=initial;func_8001FBE4(p,opcode,ops);
    if(memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"SPRITE E7 FAIL case=%u operand=%04x scale=%04x layout=%u alias=%u\n",cases,value,scale,layout,alias);exit(1);}
    ++cases;
}
int main(void) {
    assert((uintptr_t)&fixture+sizeof(fixture)<=UINT32_MAX);
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(ram+0x10000,1,sizeof(ram)-0x10000,f)>0x48000);assert(!fclose(f));
    compare(200,0x1000,0,0);
    for(unsigned v=0;v<65536;++v) compare(v,0x1000,0,0);
    for(unsigned v=0;v<65536;++v) compare(200,v,0,0);
    const unsigned edges[]={0,1,0x3fff,0x4000,0x7fff,0x8000,0xbfff,0xc000,0xfffe,0xffff};
    for(unsigned v=0;v<10;++v)for(unsigned scale=0;scale<10;++scale)
        for(unsigned layout=0;layout<5;++layout)for(unsigned alias=0;alias<2;++alias)
            compare(edges[v],edges[scale],layout,alias);
    printf("SPRITE E7 PASS %u cases: complete operand and initial-scale scalar domains, finite edge pairs, null/overlapping transforms, operand alias and high opcode prefix\n",cases);
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
