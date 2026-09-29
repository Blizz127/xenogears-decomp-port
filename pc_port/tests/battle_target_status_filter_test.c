#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern uint32_t func_80084108(uint32_t, uint32_t);
uint8_t D_800D2DCC[256], D_800C3EB7[256][28], D_800D32A1[256][8];
uint16_t D_800CCD64[256][184], D_800CCE08[256][184];
static uint8_t ram[0x200000];
static unsigned nreads, cases;
static uint32_t reads[4];
static unsigned widths[4];
static int read_bus(void *p, uint32_t address, unsigned width, uint32_t *value) {
    (void)p; unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram)) return -1;
    if (!(at>=0x84108 && at<0x841E0)) {
        assert(nreads<4); reads[nreads]=at; widths[nreads++]=width;
    }
    *value=0;
    for (unsigned i=0; i<width; i++) *value|=(uint32_t)ram[at+i]<<(8*i);
    return 0;
}
static int write_bus(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p; (void)a; (void)w; (void)v; assert(0); return -1;
}
static void check(unsigned argument, unsigned flag, unsigned present,
                  unsigned block, unsigned alternate, unsigned status) {
    unsigned t=argument&255;
    D_800D2DCC[t]=ram[0xD2DCC+t]=present;
    D_800C3EB7[t][0]=ram[0xC3EB7+t*28]=block;
    D_800D32A1[t][0]=ram[0xD32A1+t*8]=alternate;
    unsigned inverse=(status&0xC001) ? 0 : 1;
    D_800CCD64[t][0]=alternate ? inverse : status;
    D_800CCE08[t][0]=alternate ? status : inverse;
    unsigned addresses[]={0xCCD64+t*0x170,0xCCE08+t*0x170};
    unsigned values[]={D_800CCD64[t][0],D_800CCE08[t][0]};
    for (unsigned i=0; i<2; i++) {
        ram[addresses[i]]=values[i]; ram[addresses[i]+1]=values[i]>>8;
    }
    unsigned count=1, expected=0;
    uint32_t expected_reads[]={0xD2DCC+t,0xC3EB7+t*28,0xD32A1+t*8,
                              addresses[alternate!=0]};
    if (present) {
        count++;
        if (!block) {
            count++;
            if (!(flag&255)) count++;
            expected=(flag&255) || !(status&0xC001);
        }
    }
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=argument; cpu.gpr[5]=flag; cpu.gpr[31]=0xFFFFFFFC; nreads=0;
    assert(PcPortMipsRun(&cpu,0x80084108,0xFFFFFFFC,200)==0);
    assert(cpu.gpr[2]==expected && nreads==count);
    assert(memcmp(reads,expected_reads,count*sizeof(*reads))==0);
    for (unsigned i=0; i<count; i++) assert(widths[i]==(i==3 ? 2u : 1u));
    assert(func_80084108(argument,flag)==expected);
    assert(D_800D2DCC[t]==present && D_800C3EB7[t][0]==block && D_800D32A1[t][0]==alternate);
    assert(D_800CCD64[t][0]==values[0] && D_800CCE08[t][0]==values[1]);
    cases++;
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x14618,SEEK_SET));
    assert(fread(ram+0x84108,1,0xD8,f)==0xD8); fclose(f);
    const unsigned flags[]={0,1,0x100,0xFFFFFFFF};
    for (unsigned status=0; status<65536; status++)
    for (unsigned alt=0; alt<2; alt++) for (unsigned flag=0; flag<4; flag++)
        check(0x101,flags[flag],255,0,alt ? 255 : 0,status);
    for (unsigned t=0; t<256; t++) for (unsigned p=0; p<2; p++)
    for (unsigned b=0; b<2; b++) for (unsigned a=0; a<2; a++)
    for (unsigned flag=0; flag<4; flag++) check(0xABCDE000u|t,flags[flag],p,b,a,0xC001);
    printf("STATUS FILTER retail/native and retail read-trace PASS %u cases\n",cases);
}
