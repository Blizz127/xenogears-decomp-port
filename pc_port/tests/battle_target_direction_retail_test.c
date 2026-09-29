/* Retail84854 contract; executes real ratan2 and its retail lookup table. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t ram[0x200000];
static unsigned scanned, calls, returns, writes;
static unsigned negative_winners, retained_ties, mode2_upper_boundary;
static uint32_t arguments[3][2];
static int expected_angles[3];
#ifdef XBT_NATIVE_DIRECTION
typedef struct { uint16_t v; uint8_t pad[26]; } Row310;
Row310 D_800C3EBE[256], D_800C3EC0[256];
static Row310 before_x[256], before_y[256];
uint8_t D_800C3E90[12];
extern uint32_t func_80084854(uint32_t, uint32_t);
#endif
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static void put(unsigned at, unsigned width, uint32_t value) {
    for (unsigned i=0; i<width; i++) ram[at+i]=value>>(8*i);
}
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    PcPortMipsCpu *cpu=opaque;
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram));
    if (address==0x8004B32C) {
        assert(width==4 && calls<3);
        assert(cpu->gpr[4]==arguments[calls][0] && cpu->gpr[5]==arguments[calls][1]);
        calls++;
    }
    if (address==0x80084900) {
        assert(width==4 && returns<3 && calls==returns+1);
        assert(cpu->gpr[2]==(uint32_t)expected_angles[returns]);
        returns++;
    }
    if (at>=0xC3E90 && at<=0xC3E9B) {
        assert(width==1 && at!=0xC3E9B);
        scanned|=1u<<(at-0xC3E90);
    }
    *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert(width==4 && at>=0x1FEFE0 && at<=0x1FEFFC && !(at&3));
    put(at,width,value); writes++; return 0;
}
#ifdef XBT_NATIVE_DIRECTION
/* Observe either the real native callee or the explicit retail hybrid boundary. */
#ifdef XBT_REAL_NATIVE_ATAN
extern int PcPortDirectionNativeAtan(int, int);
extern short ratan_tbl[1025];
#endif
int ratan2(int dy, int dx) {
#ifdef XBT_REAL_NATIVE_ATAN
    assert(calls<3 && returns==calls);
    assert((uint32_t)dy==arguments[calls][0] && (uint32_t)dx==arguments[calls][1]);
    int result=PcPortDirectionNativeAtan(dy,dx);
    assert(result==expected_angles[returns]);
    calls++; returns++;
    return result;
#else
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.opaque=&cpu,.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    assert(dy>=-65535 && dy<=65535 && dx>=-65535 && dx<=65535);
    cpu.gpr[4]=(uint32_t)dy; cpu.gpr[5]=(uint32_t)dx;
    cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    assert(PcPortMipsRun(&cpu,0x8004B32C,0xFFFFFFFC,1000)==0);
    assert(returns<3 && calls==returns+1 && cpu.gpr[2]==(uint32_t)expected_angles[returns]);
    returns++;
    return (int32_t)cpu.gpr[2];
#endif
}
#endif
/* Independent integer expression using the authoritative table, not libm. */
static int angle(int dy, int dx) {
    unsigned y=dy<0 ? -dy : dy, x=dx<0 ? -dx : dx;
    if (!(x|y)) return 0;
    int result=y<x ? (int)get(0x57030+2*((y<<10)/x),2) :
                     0x400-(int)get(0x57030+2*((x<<10)/y),2);
    if (dx<0) result=0x800-result;
    return dy<0 ? -result : result;
}
static int accepts(unsigned mode, int value) {
    uint32_t a=(uint32_t)value;
    switch (mode&255) {
    case 0: return ((a+0x200)&0xFFFF)<0x400;
    case 1: return ((a+0x600)&0xFFFF)<0x400;
    case 2: return ((a+0x800)&0xFFFF)<0x200 || ((a-0x600)&0xFFFF)<=0x200;
    case 3: return ((a-0x200)&0xFFFF)<0x400;
    default: return 0;
    }
}
static void coordinates(unsigned actor, unsigned y, unsigned x) {
    put(0xC3EC0+actor*28,2,y); put(0xC3EBE +actor*28,2,x);
}
static void run(unsigned actor_word, unsigned mode, unsigned ay, unsigned ax,
                unsigned ty, unsigned tx, int last_near) {
    unsigned actor=actor_word&255, selected=actor;
    int64_t best=0xFFFFFF;
    coordinates(actor,ay,ax);
    coordinates(5,ty,tx); coordinates(6,65535-ty,65535-tx);
    coordinates(7,ty,tx); coordinates(8,ay,ax);
    if (last_near) {
        assert(ax<65535);
        coordinates(7,ay,ax+1);
    }
    memset(ram+0xC3E90,255,12);
    ram[0xC3E90]=actor; ram[0xC3E92]=5; ram[0xC3E93]=6;
    ram[0xC3E9A]=7; ram[0xC3E9B]=8;
    ram[0xD3274]=0; /* Count is not this routine's termination condition. */
    for (unsigned j=0; j<3; j++) {
        unsigned target=5+j;
        int dy=(int)get(0xC3EC0+target*28,2)-(int)ay;
        int dx=(int)get(0xC3EBE +target*28,2)-(int)ax;
        arguments[j][0]=(uint32_t)dy; arguments[j][1]=(uint32_t)dx;
        uint32_t bits=(uint32_t)((int64_t)dy*dy+(int64_t)dx*dx);
        int64_t distance=bits&0x80000000u ? (int64_t)bits-0x100000000LL : bits;
        expected_angles[j]=angle(dy,dx);
        if ((mode&255)==2 && expected_angles[j]==0x800) mode2_upper_boundary++;
        if (accepts(mode,expected_angles[j]) && distance==best) retained_ties++;
        if (accepts(mode,expected_angles[j]) && distance<best) {
            best=distance; selected=target;
        }
    }
    if (best<0) negative_winners++;
    if (last_near) assert(selected==7);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.opaque=&cpu,.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16; r<=23; r++) cpu.gpr[r]=0x12345600+r;
    cpu.gpr[4]=actor_word; cpu.gpr[5]=mode;
    cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    scanned=calls=returns=writes=0;
    assert(PcPortMipsRun(&cpu,0x80084854,0xFFFFFFFC,10000)==0);
    assert(cpu.gpr[2]==selected && scanned==0x7FF && calls==3 && returns==3 && writes==8);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
    for (unsigned r=16; r<=23; r++) assert(cpu.gpr[r]==0x12345600+r);
#ifdef XBT_NATIVE_DIRECTION
    memcpy(D_800C3E90,ram+0xC3E90,12);
    memset(D_800C3EBE,0xA5,sizeof(D_800C3EBE));
    memset(D_800C3EC0,0x5A,sizeof(D_800C3EC0));
    for (unsigned i=0; i<256; i++) {
        D_800C3EBE[i].v=get(0xC3EBE +i*28,2);
        D_800C3EC0[i].v=get(0xC3EC0+i*28,2);
    }
    memcpy(before_x,D_800C3EBE,sizeof(before_x));
    memcpy(before_y,D_800C3EC0,sizeof(before_y));
    scanned=calls=returns=writes=0;
    assert(func_80084854(actor_word,mode)==selected);
    assert(calls==3 && returns==3 && writes==0);
    assert(memcmp(D_800C3E90,ram+0xC3E90,12)==0);
    assert(memcmp(D_800C3EBE,before_x,sizeof(before_x))==0);
    assert(memcmp(D_800C3EC0,before_y,sizeof(before_y))==0);
#endif
}
int main(void) {
    FILE *f=fopen("disc/SLUS_006.64","rb"); assert(f);
    assert(!fseek(f,0x800,SEEK_SET));
    assert(fread(ram+0x10000,1,0x49800,f)==0x49800); fclose(f);
    f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x14D64,SEEK_SET));
    assert(fread(ram+0x84854,1,0x228,f)==0x228); fclose(f);
#ifdef XBT_REAL_NATIVE_ATAN
    for (unsigned i=0; i<1025; i++) assert(ratan_tbl[i]==(int16_t)get(0x57030+i*2,2));
#endif
    const unsigned values[]={0,1,2,17,1024,32767,32768,46340,46341,65534,65535};
    const unsigned actors[]={0xABCDE000u,0xABCDE003u,0xFFFFFFFFu};
    const unsigned origins[][2]={{0,65535},{32768,32768},{65535,0}};
    const unsigned modes[]={0,1,2,3,4,255,0x100,0x101};
    unsigned cases=0;
    for (unsigned a=0; a<3; a++) for (unsigned o=0; o<3; o++)
    for (unsigned y=0; y<11; y++) for (unsigned x=0; x<11; x++)
    for (unsigned m=0; m<8; m++) {
        run(actors[a],modes[m],origins[o][0],origins[o][1],values[y],values[x],0);
        cases++;
    }
    run(0,0,0,0,0,20,1); /* slot10 wins; slot11's still closer target is ignored. */
    cases++;
    assert(negative_winners && retained_ties && mode2_upper_boundary);
    printf("TARGET DIRECTION retail+real-ratan2 contract PASS %u cases\n",cases);
    printf("TARGET DIRECTION witnesses negative-winners=%u ties=%u mode2-upper=%u\n",
           negative_winners,retained_ties,mode2_upper_boundary);
#ifdef XBT_NATIVE_DIRECTION
#ifdef XBT_REAL_NATIVE_ATAN
    printf("TARGET DIRECTION native-picker/native-atan differential PASS %u cases\n",cases);
#else
    printf("TARGET DIRECTION native-picker/retail-atan differential PASS %u cases\n",cases);
#endif
#endif
}
