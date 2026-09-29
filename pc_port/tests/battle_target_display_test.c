/* Actual BC404 bodies, simulated BC2F0/BC460 side effects. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

extern void func_800BC404(uint32_t mask);
uint8_t D_800C37C8[2];
uint16_t D_800C3CDC[2], D_80059454[2];
static uint8_t ram[0x200000];
static uint32_t argument;
static uint16_t initial_source, initial_dest;
static unsigned calls, guest_writes, native_active;
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static void put(unsigned at, unsigned width, uint32_t value) {
    for (unsigned i=0; i<width; i++) ram[at+i]=(uint8_t)(value>>(8*i));
}
static void callback(uint32_t fn, uint32_t a) {
    assert(calls<2);
    assert(fn==(calls ? 0x800BC460u : 0x800BC2F0u));
    assert(a==(calls ? argument : 1));
    unsigned source=initial_source^(calls ? 0x1111 : 0);
    if (native_active) {
        assert(D_80059454[0]==initial_dest && D_800C3CDC[0]==source);
        D_800C37C8[0]=255;
        D_800C3CDC[0]^=calls ? 0x2222 : 0x1111;
    } else {
        assert(get(0x59454,2)==initial_dest && get(0xC3CDC,2)==source);
        put(0xC37C8,1,255);
        put(0xC3CDC,2,source^(calls ? 0x2222 : 0x1111));
    }
    calls++;
}
void func_800BC2F0(uint32_t a) { callback(0x800BC2F0,a); }
void func_800BC460(uint32_t a) { callback(0x800BC460,a); }
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram));
    *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    if (at==0x59454) {
        assert(width==2 && guest_writes==0);
        assert(value==(initial_source^(calls ? 0x3333 : 0)));
        guest_writes++;
    } else assert(width==4 && (at==0x1FEFF8 || at==0x1FEFFC));
    put(at,width,value); return 0;
}
static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target) {
    (void)opaque;
    if (target!=0x800BC2F0 && target!=0x800BC460) return 0;
    callback(target,cpu->gpr[4]);
    for (unsigned r=2; r<=15; r++) cpu->gpr[r]=0xBEEF0000+r;
    return 1;
}
static void run(uint32_t mask, unsigned flag) {
    argument=mask; initial_source=(uint16_t)(mask^0x5A5A); initial_dest=initial_source^0xFFFF;
    calls=guest_writes=native_active=0;
    put(0xC37C8,2,flag|0xAB00); put(0xC3CDC,4,initial_source|0xCDEF0000u);
    put(0x59454,4,initial_dest|0x12340000u);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16; r<=23; r++) cpu.gpr[r]=0xCAFE0000+r;
    cpu.gpr[4]=mask; cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    assert(PcPortMipsRun(&cpu,0x800BC404,0xFFFFFFFC,100)==0);
    assert(calls==(flag ? 0u : 2u) && guest_writes==1);
    for (unsigned r=16; r<=23; r++) assert(cpu.gpr[r]==0xCAFE0000+r);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
    D_800C37C8[0]=(uint8_t)flag; D_800C37C8[1]=0xAB;
    D_800C3CDC[0]=initial_source; D_800C3CDC[1]=0xCDEF;
    D_80059454[0]=initial_dest; D_80059454[1]=0x1234;
    calls=0; native_active=1;
    func_800BC404(mask);
    native_active=0;
    assert(calls==(flag ? 0u : 2u));
    assert(D_800C37C8[0]==get(0xC37C8,1) && D_800C37C8[1]==get(0xC37C9,1));
    assert(D_800C3CDC[0]==get(0xC3CDC,2) && D_800C3CDC[1]==get(0xC3CDE,2));
    assert(D_80059454[0]==get(0x59454,2) && D_80059454[1]==get(0x59456,2));
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C914,SEEK_SET));
    assert(fread(ram+0xBC404,1,0x50,f)==0x50); fclose(f);
    const uint32_t high[]={0,0xABCD0000,0xFFFF0000,0x80000000};
    const unsigned flags[]={0,1,255};
    unsigned cases=0;
    for (unsigned h=0; h<4; h++) for (unsigned low=0; low<65536; low++)
    for (unsigned fidx=0; fidx<3; fidx++) { run(high[h]|low,flags[fidx]); cases++; }
    assert(cases==786432);
    printf("TARGET DISPLAY BC404 native/retail PASS %u cases, simulated downstream calls\n",cases);
}
