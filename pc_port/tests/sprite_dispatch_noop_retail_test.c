#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
extern int AnimationScriptOpcodeIsNoop(uint32_t) __attribute__((weak));
extern void func_8001FBE4(void*,uint32_t,void*);
/* Link guards only: no no-op may call a dependency or dereference its input.
 * These are not replacement implementations and never enter the port. */
#define GUARD(name) void name(void){fputs("SPRITE NOOP FAIL unexpected callee " #name "\n",stderr);abort();}
GUARD(func_800B2AEC)
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV) GUARD(func_80039E60)
/* Guest-RAM backing for PSX_ADDR reads (opcode 0xB0 selector); never touched. */
uint8_t g_PsxRam[0x300000];
/* Shared dispatcher data dependency; no-op paths must leave it untouched. */
int32_t g_WorkListCurTimer __attribute__((weak));
void MTC2(unsigned int value, int reg)
{ (void)value; (void)reg; fputs("SPRITE NOOP FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg)
{ (void)reg; fputs("SPRITE NOOP FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command)
{ (void)command; fputs("SPRITE NOOP FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE NOOP FAIL unexpected angle helper\n",stderr); abort(); }
uint32_t D_80018644,D_80059198,D_800592E4;
uint8_t D_800591AD,D_800591B0,D_800591B3;
int16_t D_800592E8,D_800592EA;
uint32_t D_8004FBB8[8],D_8006F99C[4],D_8006F9AC[4],D_800C3EB0[4];
static uint8_t ram[0x200000];
static int rd(void* u,uint32_t a,unsigned w,uint32_t* v)
{
    (void)u;if(a<0x80000000u||(uint64_t)a+w>0x80200000u)return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)ram[(a&0x1fffff)+i]<<(8*i);
    return 0;
}
static int wr(void* u,uint32_t a,unsigned w,uint32_t v)
{
    (void)u;if(a<0x801ff000u||(uint64_t)a+w>0x80200000u)return -1;
    for(unsigned i=0;i<w;++i)ram[(a&0x1fffff)+i]=(uint8_t)(v>>(8*i));
    return 0;
}
int main(void)
{
    if(!AnimationScriptOpcodeIsNoop){fputs("SPRITE NOOP FAIL missing predicate\n",stderr);return 1;}
    FILE* f=fopen("disc/SLUS_006.64","rb");assert(f);
    assert(!fseek(f,0x800,SEEK_SET));
    size_t n=fread(ram+0x10000,1,sizeof(ram)-0x10000,f);assert(n>0x12000);assert(!fclose(f));
    unsigned count=0;
    const uint32_t high[]={0,0x100,0x80000000u,0xffffff00u};
    for(unsigned k=0;k<4;++k)for(unsigned opcode=0;opcode<256;++opcode){
        uint32_t entry=0;int expected=0;
        if(opcode>=0x8a&&opcode<=0xfc){assert(!rd(NULL,0x800183d8+(opcode-0x8a)*4,4,&entry));expected=entry==0x80021ab8;}
        if(AnimationScriptOpcodeIsNoop(high[k]|opcode)!=expected){fprintf(stderr,"SPRITE NOOP FAIL opcode=%08x target=%08x\n",high[k]|opcode,entry);return 1;}
        if(expected){
            func_8001FBE4(NULL,high[k]|opcode,NULL);
            PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;
            PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=0;cpu.gpr[5]=high[k]|opcode;cpu.gpr[6]=0;cpu.gpr[29]=0x801fff00;cpu.gpr[31]=0xfffffffcu;
            if(PcPortMipsRun(&cpu,0x8001fbe4,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE NOOP FAIL retail %s\n",cpu.error);return 1;}
            ++count;
        }
    }
    printf("SPRITE NOOP PASS 1024 classifications, %u native and actual retail no-op executions\n",count);
}

/* Shared packed sprite globals; newly used BD table pointer lives at+0x10. */
uint8_t D_8006BE10[32] __attribute__((weak));

__attribute__((weak)) GUARD(func_8001D4E8)

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
