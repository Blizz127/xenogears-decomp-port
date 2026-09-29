#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "battle_mips_adapter.h"
u8 D_801EA8C0;
u16 D_801EA5D0[128];
static u8 ram[0x200000];
static u32 expected_code,expected_flag;
static uintptr_t provider_result;
static unsigned calls,gcalls;
static const uint8_t *PcPortKromFont(unsigned code,size_t length) {
    assert(length==32);
    assert(calls++==0 && code==expected_code && D_801EA8C0==expected_flag);
    return (const uint8_t*)provider_result;
}
/* Old SDK-width transport kept only as a regression witness. */
static int Krom2RawAdd(unsigned code) {return (int)(uintptr_t)PcPortKromFont(code,32);}
#include "convert.inc"
static u8 *ptr(u32 a,unsigned w) {a&=0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a;}
static int read_bus(void *o,u32 a,unsigned w,u32 *v) {
    (void)o;u8*p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,u32 a,unsigned w,u32 v) {
    (void)o;u8*p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,u32 target) {
    (void)o;if(target!=0x800405c4)return 0;
    assert(gcalls++==0);expected_code=c->gpr[4];expected_flag=ram[0x1ea8c0];
    c->gpr[2]=(u32)provider_result;return 1;
}
int main(void) {
    FILE*f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(D_801EA5D0,ram+0x1ea5d0,sizeof D_801EA5D0);
    unsigned cases=0;
    assert((uintptr_t)&provider_result>UINT32_MAX);
    for(unsigned code=0;code<65536;++code)for(unsigned failure=0;failure<3;++failure) {
        u8 input[]={(u8)(code>>8),(u8)code};
        memcpy(ram+0x100000,input,2);D_801EA8C0=ram[0x1ea8c0]=0xa5;
        calls=gcalls=0;provider_result=failure==1?UINTPTR_MAX:failure==2?(uintptr_t)&provider_result:0x12345678;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=0x80100000;cpu.gpr[29]=0x801b0000;cpu.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&cpu,0x801e65e4,0xfffffffc,1000)==PC_PORT_MIPS_HALTED);
        void *result=func_801E65E4(input);
        assert(calls==1 && gcalls==1 && (u32)(uintptr_t)result==cpu.gpr[2]);
        assert((uintptr_t)result==provider_result);
        ++cases;
    }
    printf("PASS %u retail/native title code fixtures; font provider intercepted\n",cases);
}
