/* Guest-frame alias parity. Callback frame edits are explicit simulations;
 * corrupted return targets are halt sentinels, never claimed runnable code. */
#include "battle_target_setup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x200000], expected_ram[0x200000], body[0x108];
static uint32_t stack, saved_s0;
static unsigned edits, calls, writes;
static uint32_t get(unsigned at,unsigned width) {
    uint32_t v=0;
    for (unsigned i=0;i<width;i++) v|=(uint32_t)ram[at+i]<<(8*i);
    return v;
}
static void put(unsigned at,unsigned width,uint32_t v) {
    for (unsigned i=0;i<width;i++) ram[at+i]=(uint8_t)(v>>(8*i));
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o; assert(a>=0x80000000u && a<=0x80200000u-w);
    *v=get(a-0x80000000u,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; assert(w==4 && !(a&3));
    assert(a==stack-4 || a==stack-8 || a==0x800C3CC0 || a==0x800C3CBC ||
        a==0x800C3680 || a==0x800C3684 ||
        (a>=0x8006F99C && a<=0x8006F9A4) || (a>=0x8006F9AC && a<=0x8006F9B4));
    put(a-0x80000000u,w,v); writes++; return 0;
}
static int callback(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    (void)o;
    if (target!=0x80010000) return 0;
    assert(edits && !calls && cpu->gpr[4]==0x80100000);
    assert(cpu->gpr[29]==stack-0x18 && cpu->gpr[31]==0x800BC3B8);
    assert(get(stack-0x80000000u-4,4)==0xFFFFFFFCu);
    assert(get(stack-0x80000000u-8,4)==saved_s0);
    if (edits&1) write_bus(NULL,stack-8,4,0x12345678);
    if (edits&2) write_bus(NULL,stack-4,4,0x80010010);
    calls++;
    for (unsigned r=2;r<=15;r++) cpu->gpr[r]=0xBAD00000+r;
    return 1;
}
static int invoke(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    return callback(o,cpu,target)==1?0:-1;
}
static uint32_t vector_address(unsigned i) { return 0x8006F99Cu+(i/3)*16+(i%3)*4; }
static uint32_t source_value(unsigned seed,unsigned i) {
    uint32_t a=0x800D30A0u+(i/3)*8+(i%3)*2;
    if ((a&~3u)==stack-4) return (0xFFFFFFFCu>>((a&2)*8))&0xFFFFu;
    if ((a&~3u)==stack-8) return (saved_s0>>((a&2)*8))&0xFFFFu;
    return (seed+i*0x3333)&0xFFFFu;
}
static uint32_t expected_return(unsigned mode,unsigned seed) {
    if (edits&2) return 0x80010010;
    if (mode==4 && stack-4==0x800C3CBC) return 5;
    if (mode==2) for (unsigned i=0;i<6;i++)
        if (vector_address(i)==stack-4) return source_value(seed,i)<<16;
    return 0xFFFFFFFC;
}
static void reset(PcPortMipsCpu *cpu,unsigned mode,unsigned seed) {
    memset(ram,0xA5,sizeof(ram)); memcpy(ram+0xBC2F0,body,sizeof(body));
    put(0xC3680,4,edits?0x80100000:0); put(0xC3684,4,0);
    put(0x10000C,4,0x80010000);
    for (unsigned i=0;i<6;i++) put(0xD30A0+(i/3)*8+(i%3)*2,2,(seed+i*0x3333)&0xFFFF);
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=callback};
    PcPortMipsCpuInit(cpu,&bus);
    for (unsigned r=16;r<=23;r++) cpu->gpr[r]=0xCAFE0000u+seed+r;
    saved_s0=cpu->gpr[16];
    cpu->gpr[4]=mode; cpu->gpr[29]=stack; cpu->gpr[31]=0xFFFFFFFC;
    calls=writes=0;
}
static void run(unsigned mode,uint32_t sp,unsigned seed,unsigned change) {
    stack=sp; edits=change;
    PcPortMipsCpu retail,native;
    reset(&retail,mode,seed);
    uint32_t stop=expected_return(mode,seed);
    assert(PcPortMipsRun(&retail,0x800BC2F0,stop,200)==0);
    unsigned expected_writes=writes, expected_calls=calls;
    memcpy(expected_ram,ram,sizeof(ram));
    reset(&native,mode,seed);
    assert(PcPortBattleTargetSetupFrame(&native,invoke,NULL)==0);
    assert(native.gpr[31]==stop && retail.gpr[31]==stop);
    assert(native.gpr[29]==sp && retail.gpr[29]==sp);
    for (unsigned r=16;r<=23;r++) assert(native.gpr[r]==retail.gpr[r]);
    uint32_t expected_s0=(edits&1)?0x12345678:saved_s0;
    if (mode==2) for (unsigned i=0;i<6;i++)
        if (vector_address(i)==sp-8) expected_s0=source_value(seed,i)<<16;
    assert(native.gpr[16]==expected_s0);
    assert(writes==expected_writes && calls==expected_calls && calls==!!edits);
    assert(writes==4u+(mode==2?6u:mode==4?1u:(unsigned)!!edits)+
        (unsigned)!!(edits&1)+(unsigned)!!(edits&2));
    assert(!memcmp(expected_ram,ram,sizeof(ram)));
    for (unsigned i=0;i<6;i++) {
        uint32_t a=vector_address(i), value=0xA5A5A5A5;
        if (a==sp-4) value=0xFFFFFFFC;
        if (a==sp-8) value=saved_s0;
        if (mode==2) value=source_value(seed,i)<<16;
        assert(get(a-0x80000000u,4)==value);
    }
    assert(get(0xC3CC0,4)==(sp-8==0x800C3CC0u?saved_s0:mode));
    assert(get(0xC3CBC,4)==(mode==4?5u:sp-4==0x800C3CBCu?0xFFFFFFFCu:1u));
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C800,SEEK_SET));
    assert(fread(body,1,sizeof(body),f)==sizeof(body)); fclose(f);
    const uint32_t stacks[]={0x801FF000,0x800C3CC0,0x800C3CC8,0x800D30A8,0x800D30B0,
        0x8006F9A0,0x8006F9A8,0x8006F9B0,0x8006F9B8};
    const unsigned modes[]={0,2,4};
    unsigned cases=0;
    for (unsigned s=0;s<9;s++) for (unsigned m=0;m<3;m++) for (unsigned seed=0;seed<16;seed++) {
        run(modes[m],stacks[s],seed*0x1111,0); cases++;
    }
    for (unsigned change=1;change<4;change++) for (unsigned seed=0;seed<16;seed++) {
        run(1,0x801FF000,seed*0x1111,change); cases++;
    }
    assert(cases==480);
    puts("TARGET SETUP frame aliases PASS 480 retail/native pairs, simulated callback edits");
}
