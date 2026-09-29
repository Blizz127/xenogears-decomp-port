/* Three real native bodies versus three retail bodies. No call bridges. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void func_80084A7C(uint32_t);
struct BattleCommandContext;
struct BattleCommandContext *D_800C3EAC;
uint8_t D_800D3274, D_800C3E90[12];
uint8_t D_800D2DCC[256], D_800C3EB4[256][28], D_800C3EB7[256][28];
uint8_t D_800D32A1[256][8], *D_800D3364;
uint16_t D_800CCD34[256][184], D_800CCD64[256][184], D_800CCE08[256][184];
enum { CONTEXT=0x100000, MATRIX=0x110000, SIZE=0x4100 };
static uint8_t ram[0x200000], matrix[0x5000];
static _Alignas(4) uint8_t context[SIZE];
static uint8_t expected_context[SIZE];
static unsigned cases;
static void fail(const char *s) {
    fprintf(stderr, "TARGET FULL CHAIN FAIL %s case=%u\n", s, cases); exit(1);
}
static int read_bus(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p; a &= 0x1fffffff;
    if ((uint64_t)a+w > sizeof(ram)) return -1;
    *v=0;
    for (unsigned i=0; i<w; ++i) *v |= (uint32_t)ram[a+i] << (8*i);
    return 0;
}
static int write_bus(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p; a &= 0x1fffffff;
    if ((uint64_t)a+w > sizeof(ram)) return -1;
    for (unsigned i=0; i<w; ++i) ram[a+i]=(uint8_t)(v >> (8*i));
    return 0;
}
static void put(unsigned a, unsigned w, unsigned v) { write_bus(NULL,a,w,v); }
int main(void) {
    FILE *f=fopen("disc/battle.bin", "rb");
    if (!f) fail("retail open");
    const unsigned offsets[]={0x14504,0x146f0,0x14f8c};
    const unsigned addresses[]={0x83ff4,0x841e0,0x84a7c};
    const unsigned sizes[]={0x114,0x368,0xc4};
    for (unsigned i=0;i<3;++i)
        if (fseek(f,offsets[i],SEEK_SET) || fread(ram+addresses[i],1,sizes[i],f)!=sizes[i])
            fail("retail load");
    fclose(f);
    const unsigned actors[]={0,2,3,255,0x101};
    for (unsigned ai=0;ai<5;++ai)
    for (unsigned bits=0;bits<256;++bits)
    for (unsigned mode=0;mode<6;++mode)
    for (unsigned group_mode=0;group_mode<4;++group_mode)
    for (unsigned pattern=0;pattern<4;++pattern) {
        unsigned actor=actors[ai]&255, group[256], eligible[11];
        uint16_t ranks[11];
        memset(matrix,0,sizeof(matrix));
        for (unsigned i=0;i<256;++i) {
            /* Vary membership independently of rank, including a prefix of
             * at least three matching candidates to distinguish swapping
             * only slot zero from sorting the whole matching prefix. */
            group[i]=group_mode==0?(i+pattern)%4:group_mode==1?0:
                     group_mode==2?(i/3)%4:((i*i+bits+i*(bits>>4))>>2)&3;
            D_800C3EB4[i][0]=(uint8_t)group[i];
            ram[0xc3eb4+i*28]=(uint8_t)group[i];
        }
        /* In matrix mode deny some groups; alternate targets must bypass it. */
        if (mode==4 || mode==5)
            for (unsigned g=0;g<4;++g)
                matrix[0x140+group[actor]*64+g*8]=(uint8_t)((bits>>g)&1);
        memcpy(ram+MATRIX,matrix,sizeof(matrix));
        D_800D3364=matrix; put(0xd3364,4,0x80000000u+MATRIX);
        unsigned first=actor<3?3:0, end=actor<3?11:3;
        for (unsigned i=0;i<11;++i) {
            unsigned bit=(bits>>(i%8))&1;
            unsigned present=mode==0?bit:1;
            unsigned blocked=mode==1?!bit:0;
            unsigned alt=mode==3?1:mode==5?(i&1):0;
            const unsigned status_bits[]={1,0x4000,0x8000,0xc001};
            unsigned normal=mode==2?(bit?0:status_bits[pattern]):0;
            unsigned alternate=mode==3?(bit?0:status_bits[pattern]):0;
            unsigned deny=matrix[0x140+group[actor]*64+group[i]*8];
            eligible[i]=present && !blocked && (alt ? !(alternate&0xc001) : !deny && !(normal&0xc001));
            ranks[i]=pattern==0?(uint16_t)(i*6000):pattern==1?(uint16_t)(65535-i*6000):pattern==2?0x8000:(i&1?0xffff:0);
            D_800D2DCC[i]=(uint8_t)present; ram[0xd2dcc+i]=(uint8_t)present;
            D_800C3EB7[i][0]=(uint8_t)blocked; ram[0xc3eb7+i*28]=(uint8_t)blocked;
            D_800D32A1[i][0]=(uint8_t)alt; ram[0xd32a1+i*8]=(uint8_t)alt;
            D_800CCD34[i][0]=ranks[i]; put(0xccd34+i*0x170,2,ranks[i]);
            D_800CCD64[i][0]=(uint16_t)normal; put(0xccd64+i*0x170,2,normal);
            D_800CCE08[i][0]=(uint16_t)alternate; put(0xcce08+i*0x170,2,alternate);
        }
        uint8_t list[12]; memset(list,255,sizeof(list));
        unsigned count=0, same=0;
        for (unsigned pass=0;pass<2;++pass)
            for (unsigned i=first;i<end;++i)
                if (eligible[i] && ((group[i]==group[actor])==(pass==0))) {
                    list[count++]=(uint8_t)i; if (!pass) ++same;
                }
        for (unsigned i=1;i<(same?same:count);++i)
            if (ranks[list[i]]<ranks[list[0]]) { uint8_t t=list[0];list[0]=list[i];list[i]=t; }
        unsigned seed=pattern&1 && count?list[count-1]:bits;
        unsigned selected=list[0];
        for (unsigned i=0;i<count;++i) if (list[i]==seed) selected=seed;
        /* Uniform padding would hide copying four bytes instead of one. */
        for (unsigned j=0;j<SIZE;++j) context[j]=(uint8_t)(j*37+j/256+bits);
        context[actor*64+0x3c]=(uint8_t)seed;
        memcpy(ram+CONTEXT,context,SIZE); memcpy(expected_context,context,SIZE);
        expected_context[0x2e8]=(uint8_t)selected;
        D_800C3EAC=(struct BattleCommandContext *)context;
        put(0xc3eac,4,0x80000000u+CONTEXT);
        memset(D_800C3E90,0x5a,12); memset(ram+0xc3e90,0x5a,12);
        D_800D3274=ram[0xd3274]=0xa5;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus};
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=actors[ai];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if (PcPortMipsRun(&cpu,0x80084a7c,0xfffffffcu,10000)) fail(cpu.error);
        if (ram[0xd3274]!=count || memcmp(ram+0xc3e90,list,12) ||
            memcmp(ram+CONTEXT,expected_context,SIZE)) fail("retail contract");
        func_80084A7C(actors[ai]);
        if (D_800D3274!=count || memcmp(D_800C3E90,list,12) ||
            memcmp(context,ram+CONTEXT,SIZE) || memcmp(matrix,ram+MATRIX,sizeof(matrix)) ||
            D_800C3EAC!=(struct BattleCommandContext *)context)
            fail("native differs from retail");
        ++cases;
    }
    printf("TARGET FULL CHAIN PASS %u cases, no callee replacements\n",cases);
    return 0;
}
