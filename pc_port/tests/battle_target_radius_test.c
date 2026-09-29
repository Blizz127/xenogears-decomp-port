#include "battle_mips_adapter.h"
#include "battle_target_bounds.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x200000];
static unsigned stores;
static uint32_t get(unsigned a,unsigned w) {
    uint32_t v=0; for (unsigned i=0;i<w;i++) v|=(uint32_t)ram[a+i]<<(8*i); return v;
}
static void put(unsigned a,unsigned w,uint32_t v) {
    for (unsigned i=0;i<w;i++) ram[a+i]=(uint8_t)(v>>(8*i));
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o; assert(a>=0x80000000u && a<=0x80200000u-w);
    *v=get(a-0x80000000u,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; assert(w==2 && (a==0x801FF068 || a==0x801FF06A));
    put(a-0x80000000u,w,v); stores++; return 0;
}
static void run(uint16_t x,uint16_t y,uint32_t maximum) {
    put(0x1FF068,2,x); put(0x1FF06A,2,y);
    stores=0;
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[21]=maximum; cpu.gpr[29]=0x801FF000;
    assert(PcPortMipsRun(&cpu,0x800BC7FC,0x800BC85C,100)==0);
    uint32_t actual=PcPortBattleAccumulateTargetRadius(maximum,x,y);
    assert(actual==cpu.gpr[21]);
    assert(stores==4 && cpu.gpr[29]==0x801FF000);
    assert(get(0x1FF068,2)==(uint16_t)(((uint32_t)x-160u)*4u));
    assert(get(0x1FF06A,2)==(uint16_t)(((uint32_t)y-164u)*4u));
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4CD0C,SEEK_SET));
    assert(fread(ram+0xBC7FC,1,0x60,f)==0x60); fclose(f);
#ifdef XBT_RADIUS_UNSIGNED_RETAIL
    assert(get(0xBC84C,4)==0x02A5102A);
    put(0xBC84C,4,0x02A5102B);
#endif
    const uint32_t maxima[]={0,17,0x40000000u,0x80000000u};
    unsigned cases=0;
    for (unsigned x=0;x<65536;x++) for (unsigned pattern=0;pattern<3;pattern++)
    for (unsigned m=0;m<4;m++) {
        uint16_t y=pattern==0?0:pattern==1?0xFFFF:(uint16_t)(x+4);
        run((uint16_t)x,y,maxima[m]); cases++;
    }
    /* Two -32768 scaled offsets square to 0x80000000: signed max from0
     * must ignore this negative wrapped sum. No replacement GTE is involved. */
    assert(PcPortBattleAccumulateTargetRadius(0,8352,8356)==0);
    assert(PcPortBattleAccumulateTargetRadius(0,160,164)==0);
    assert(PcPortBattleAccumulateTargetRadius(0,161,164)==16);
    assert(cases==786432);
    puts("TARGET RADIUS native/retail PASS 786432 arithmetic-fragment cases; no geometry calls");
}
