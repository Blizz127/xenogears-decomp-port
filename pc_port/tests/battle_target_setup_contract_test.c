/* BC2F0 retail body; only task callback bodies are simulated. */
#include "battle_mips_adapter.h"
#ifdef XBT_SETUP_NATIVE
#include "battle_target_setup.h"
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x200000];
static unsigned calls, replace_roots, first_present, second_present, writes;
static unsigned native_active;
static uint32_t handles[3];
static uint32_t mode;
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0;i<width;i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static void put(unsigned at, unsigned width, uint32_t value) {
    for (unsigned i=0;i<width;i++) ram[at+i]=(uint8_t)(value>>(8*i));
}
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    for (unsigned i=0;i<3;i++) if (address==handles[i]+12) {
        assert(width==4); *value=0x80010000+i*4; return 0;
    }
    /* Task words must resolve through their exact handle, not RAM masking. */
    assert(address>=0x80000000u && address<0x80200000u);
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram)); *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque; unsigned at=address&0x1FFFFFFF;
    assert(width==4 && !(at&3));
    assert(at==0x1FEFF8 || at==0x1FEFFC || at==0xC3CC0 || at==0xC3CBC ||
        at==0xC3680 || at==0xC3684 || (at>=0x6F99C && at<=0x6F9A4) ||
        (at>=0x6F9AC && at<=0x6F9B4));
    put(at,width,value); writes++; return 0;
}
static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target) {
    (void)opaque;
    if (target<0x80010000 || target>0x80010008 || (target&3)) return 0;
    assert(mode!=2 && mode!=4 && calls<2);
    unsigned task=(target-0x80010000)/4;
    assert(cpu->gpr[4]==handles[task]);
    assert(get(0xC3CC0,4)==mode && get(0xC3CBC,4)==1);
    if (!calls && first_present) {
        assert(task==0 && get(0xC3680,4)==handles[0]);
        if (replace_roots) {
            put(0xC3680,4,handles[2]);
            put(0xC3684,4,handles[2]);
        }
    } else {
        assert(get(0xC3680,4)==0);
        assert(task==(first_present && replace_roots ? 2u : 1u));
        assert(get(0xC3684,4)==cpu->gpr[4]);
        if (replace_roots) put(0xC3684,4,handles[0]);
    }
    calls++;
    for (unsigned r=2;r<=15;r++) cpu->gpr[r]=0xBAD00000+r;
    return 1;
}
#ifdef XBT_SETUP_NATIVE
static int invoke(void *opaque, uint32_t target, uint32_t task) {
    PcPortMipsCpu cpu = {0};
    cpu.gpr[4] = task;
    return bridge(opaque, &cpu, target) == 1 ? 0 : -1;
}
#endif
static void run(uint32_t input_mode, unsigned seed, unsigned roots, unsigned replacing) {
    mode=input_mode; first_present=roots&1; second_present=!!(roots&2);
    replace_roots=replacing; calls=writes=0;
    put(0xC3680,4,first_present ? handles[0] : 0);
    put(0xC3684,4,second_present ? handles[1] : 0);
    put(0xC3CC0,4,0xAABBCCDD); put(0xC3CBC,4,0xDDCCBBAA);
    const unsigned offsets[]={0,2,4,8,10,12};
    uint16_t values[6];
    memset(ram+0xD30A0,0x5A,16);
    memset(ram+0x6F99C,0xA5,32);
    for (unsigned i=0;i<6;i++) {
        values[i]=(uint16_t)(seed+i*0x3333);
        put(0xD30A0+offsets[i],2,values[i]);
    }
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16;r<=23;r++) cpu.gpr[r]=0xCAFE0000+r;
    cpu.gpr[4]=mode; cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
#ifdef XBT_SETUP_NATIVE
    if (native_active) assert(PcPortBattleTargetSetup(&bus, mode, invoke, NULL)==0);
    else
#endif
    assert(PcPortMipsRun(&cpu,0x800BC2F0,0xFFFFFFFC,200)==0);
    assert(get(0xC3CC0,4)==mode && get(0xC3CBC,4)==(mode==4 ? 5u : 1u));
    if (mode==2 || mode==4) {
        assert(!calls);
        assert(get(0xC3680,4)==(first_present ? handles[0] : 0));
        assert(get(0xC3684,4)==(second_present ? handles[1] : 0));
        assert(writes==(mode==2 ? 10u : 5u)-2*native_active);
    } else {
        unsigned expected_calls=first_present+!!(second_present || (first_present && replace_roots));
        assert(calls==expected_calls && writes==4+calls-2*native_active);
        assert(get(0xC3680,4)==0 && get(0xC3684,4)==0);
    }
    for (unsigned i=0;i<6;i++) {
        unsigned dest=0x6F99C+(i/3)*16+(i%3)*4;
        uint32_t expected=mode==2 ? (uint32_t)values[i]*65536u : 0xA5A5A5A5;
        assert(get(dest,4)==expected && get(0xD30A0+offsets[i],2)==values[i]);
    }
    assert(get(0x6F9A8,4)==0xA5A5A5A5 && get(0x6F9B8,4)==0xA5A5A5A5);
    for (unsigned r=16;r<=23;r++) assert(cpu.gpr[r]==0xCAFE0000+r);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C800,SEEK_SET));
    assert(fread(ram+0xBC2F0,1,0x108,f)==0x108); fclose(f);
#ifdef XBT_SETUP_NO_FIRST_CLEAR
    put(0xBC3B8,4,0); /* Isolated instruction-copy mutant. */
#endif
    unsigned cases=0;
#ifdef XBT_SETUP_NATIVE
    for (native_active=0; native_active<2; native_active++) {
#endif
    const uint32_t domains[3][3]={{0x80100000,0x80100020,0x80100040},
        {0xA0100000,0xA0100020,0xA0100040},
        {0x41001000,0x80100020,0x41003000}};
    for (unsigned domain=0;domain<3;domain++) {
    memcpy(handles,domains[domain],sizeof(handles));
    for (unsigned seed=0;seed<65536;seed++) { run(2,seed,3,1); cases++; }
    const uint32_t modes[]={0,1,2,4,0xFFFFFFFF,0x80000002,0x10004};
    for (unsigned m=0;m<7;m++) for (unsigned roots=0;roots<4;roots++)
    for (unsigned replacing=0;replacing<2;replacing++) {
        run(modes[m],0x8000,roots,replacing); cases++;
    }
    }
#ifdef XBT_SETUP_NATIVE
    }
    assert(cases==393552);
#else
    assert(cases==196776);
#endif
    printf("TARGET SETUP BC2F0 contract PASS %u executions, native=%u, simulated task resolver/callbacks\n",cases,
#ifdef XBT_SETUP_NATIVE
           1u
#else
           0u
#endif
    );
}
