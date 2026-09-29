/* Execute the disc's 800250E0 against mixed-address deferred-free lists. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
void *g_GfxWorkBuffers, *g_GfxWorkBuffer2;
void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
uintptr_t D_80059524;
uint32_t D_80059300, D_80059304;
int g_GfxCurContext, g_GfxWorkBufferSize;
extern void func_800250E0(int);
static uintptr_t calls[4], expected_calls[4];
static unsigned call_count;
static uint8_t guards[0x100];

static uint8_t *address(uint32_t p) {
    if (p >= (uintptr_t)g_PsxRam && p < (uintptr_t)g_PsxRam + PSX_RAM_SIZE)
        return (uint8_t *)(uintptr_t)p;
    assert((p & 0xffe00000u) == 0x80000000u ||
           (p & 0xffe00000u) == 0xa0000000u);
    return PSX_ADDR(p);
}
static uint32_t get(uint32_t p) { uint32_t v; memcpy(&v,address(p),4);return v; }
static void put(uint32_t p,uint32_t v) { memcpy(address(p),&v,4); }
static int rd(void *o,uint32_t p,unsigned w,uint32_t *v) {
    (void)o;*v=0;for(unsigned i=0;i<w;i++)*v|=(uint32_t)address(p)[i]<<(i*8);return 0;
}
static int wr(void *o,uint32_t p,unsigned w,uint32_t v) {
    (void)o;for(unsigned i=0;i<w;i++)address(p)[i]=(uint8_t)(v>>(i*8));return 0;
}
void HeapFree(void *p) {
    assert(call_count < 4);
    calls[call_count++] = (uintptr_t)p;
}
static int bridge(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    (void)o;
    if(target != 0x800320e8u)return 0;
    HeapFree(address(cpu->gpr[4]));return 1;
}
static uint32_t domain(uint32_t guest,unsigned mode) {
    if(mode==0)return (uint32_t)(uintptr_t)address(guest);
    return (guest&0x1fffffu)|(mode==1?0x80000000u:0xa0000000u);
}
int main(void) {
    assert((uintptr_t)g_PsxRam+PSX_RAM_SIZE < UINT32_MAX);
    FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
    assert(fseek(f,0x158e0,SEEK_SET)==0);
    assert(fread(PSX_ADDR(0x800250e0u),1,160,f)==160);fclose(f);
    unsigned cases=0;
    for(unsigned context=0;context<2;context++)
    for(unsigned count=0;count<=2;count++)
    for(unsigned mask=0;mask<243;mask++) {
        unsigned m=mask, d[5];
        for(unsigned i=0;i<5;i++){d[i]=m%3;m/=3;}
        memset(PSX_ADDR(0x80100000u),0xa5,sizeof(guards));
        put(0x80100000u,domain(0x80101000u,d[1]));
        put(0x80100004u,count==2?domain(0x80100010u,d[2]):0);
        put(0x80100010u,domain(0x80102000u,d[3]));
        put(0x80100014u,0);
        uint32_t head=count?domain(0x80100000u,d[0]):0;
        /* Keep the other context nonempty to detect clearing both lists. */
        uint32_t other=domain(0x80100080u,d[4]);
        put(0x80059300u,context?other:head);
        put(0x80059304u,context?head:other);
        put(0x800594b4u,0x80110000u);put(0x800594b8u,0x80112000u);
        put(0x800592fcu,0x1000);
        memcpy(guards,PSX_ADDR(0x80100000u),sizeof(guards));
        call_count=0;
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=context;cpu.gpr[28]=0x80059170u;
        cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0xfffffffcu;
        int rc=PcPortMipsRun(&cpu,0x800250e0u,0xfffffffcu,1000);
        if(rc){fprintf(stderr,"retail: %s\n",cpu.error);return 1;}
        assert(call_count==count);memcpy(expected_calls,calls,sizeof(calls));
        g_GfxWorkBuffers=PSX_ADDR(0x80110000u);g_GfxWorkBuffer2=PSX_ADDR(0x80112000u);
        g_GfxWorkBufferSize=0x1000;g_GfxCurContext=-1;
        D_80059300=context?other:head;D_80059304=context?head:other;
        call_count=0;func_800250E0((int)context);
        assert(call_count==count);
        assert(memcmp(calls,expected_calls,count*sizeof(*calls))==0);
        assert(D_80059300==get(0x80059300u)&&D_80059304==get(0x80059304u));
        assert(g_GfxCurContext==(int)get(0x800592f8u));
        assert(g_GfxCurWorkBuffer==address(get(0x80059580u)));
        assert(g_GfxCurWorkBufferEnd==address(get(0x80059534u)));
        assert(D_80059524==(uintptr_t)address(get(0x80059524u)));
        assert(memcmp(guards,PSX_ADDR(0x80100000u),sizeof(guards))==0);
        ++cases;
    }
    printf("GFX DEFERRED FREE PASS %u cases: contexts, empty/multiple nodes, mixed native/KSEG0/KSEG1 pointers, ordered frees and guards\n",cases);
}
