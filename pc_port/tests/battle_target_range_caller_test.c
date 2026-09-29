/* Retail caller prefix only: stop at the selected callee, never bridge it. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t ram[0x200000];
static unsigned writes;
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram));
    *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert(width==4 && !(at&3) && at>=0x1FEFE0 && at<=0x1FEFFC);
    for (unsigned i=0; i<width; i++) ram[at+i]=(uint8_t)(value>>(8*i));
    writes++; return 0;
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x152F4,SEEK_SET));
    assert(fread(ram+0x84DE4,1,0xE8,f)==0xE8); fclose(f);
#ifdef XBT_CALLER_BAD_MODE
    /* Change mode2 setup to mode3 in the isolated instruction copy. */
    ram[0x84EA4]=3;
#endif
    const unsigned controls[]={0,1,2,3,255};
    const unsigned alternates[]={0,1,255};
    unsigned cases=0, range_calls=0, actor_calls=0, modes[3]={0};
    for (unsigned flags=0; flags<65536; flags++)
    for (unsigned c=0; c<sizeof(controls)/sizeof(controls[0]); c++)
    for (unsigned a=0; a<sizeof(alternates)/sizeof(alternates[0]); a++) {
        PcPortMipsCpu cpu;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus};
        PcPortMipsCpuInit(&cpu,&bus);
        for (unsigned r=1; r<32; r++) cpu.gpr[r]=0xBEEF0000+r;
        cpu.gpr[4]=0xABCD0000|flags;
        cpu.gpr[5]=0x12340000|(flags^0xFFFF);
        cpu.gpr[6]=0x987600FF;
        cpu.gpr[7]=0x76540000|controls[c];
        cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
        memset(ram+0x1FEFD0,0xA5,0x30);
        /* Fifth argument is a byte at entry SP+16; surrounding bytes differ. */
        memset(ram+0x1FF010,0x5A,4);
        ram[0x1FF010]=(uint8_t)alternates[a];
        writes=0;
        uint32_t target=alternates[a] ? 0x80084750 : 0x80084548;
        assert(PcPortMipsRun(&cpu,0x80084DE4,target,100)==0);
        assert(cpu.pc==target && writes==8 && cpu.gpr[29]==0x801FEFD0);
        if (alternates[a]) {
            assert(cpu.gpr[4]==255 && cpu.gpr[31]==0x80084EBC);
            actor_calls++;
        } else {
            unsigned effective=controls[c]==0 ? (flags^0xFFFF)
                : controls[c]==1 ? 0x3000 : controls[c]==2 ? 0x2001 : flags;
            unsigned mode=(effective&0x2000) ? 2 : (effective&0x1000) ? 0 : 1;
            assert(cpu.gpr[4]==mode && cpu.gpr[4]<=2);
            assert(cpu.gpr[5]==!!(effective&0x8000));
            assert(cpu.gpr[6]==!(effective&0x1000));
            assert(cpu.gpr[31]==0x80084ECC);
            modes[mode]++; range_calls++;
        }
        cases++;
    }
    assert(cases==983040 && range_calls==327680 && actor_calls==655360);
    assert(modes[0] && modes[1] && modes[2]);
    printf("TARGET RANGE caller-prefix PASS %u cases, range=%u actor=%u modes=%u/%u/%u\n",
        cases,range_calls,actor_calls,modes[0],modes[1],modes[2]);
}
