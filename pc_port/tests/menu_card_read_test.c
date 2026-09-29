/* Retail 801C90B0 differential at the file-read boundary, not card I/O proof. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"

SystemMenu *g_Menu;
static SystemMenu menus[2];
static MenuUnk2 cards[2], before[2];
static uint8_t ram[0x200000], overlay[0x3b000];
char D_801C50A8[6], D_801C50B0[6];
char D_801C50AC, D_801C50B4;
static unsigned port, slot, count, initial, replace, calls, guest_calls;
static int result;
static char wanted[72];
static uint32_t get32(const void *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static int func_801C9038(char *path,void *dest) {
    assert(++calls==1 && !strcmp(path,wanted));
    assert(dest==&cards[0].unkB94[(port*16+slot)*512]);
    g_Menu=&menus[replace];
    return result;
}
static void func_801C8CA4(u8 p) { (void)p; assert(!"unexpected retry"); }
#include "card_read.inc"

static uint8_t *ptr(uint32_t a,unsigned w) {
    a &= 0x1fffffff;
    assert((uint64_t)a+w<=sizeof ram); return ram+a;
}
static int bus_read(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o; uint8_t *p=ptr(a,w); *v=0;
    for(unsigned i=0;i<w;++i) *v|=(uint32_t)p[i]<<(i*8);
    return 0;
}
static int bus_write(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; uint8_t *p=ptr(a,w);
    for(unsigned i=0;i<w;++i) p[i]=v>>(i*8);
    return 0;
}
static int bridge(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    (void)o;
    if(target==0x8003fb84 || target==0x8003fa78) {
        char *dst=(char*)ptr(cpu->gpr[4],1);
        char *src=(char*)ptr(cpu->gpr[5],1);
        if(target==0x8003fb84) strcpy(dst,src); else strcat(dst,src);
        cpu->gpr[2]=cpu->gpr[4]; return 1;
    }
    if(target==0x801c9038) {
        assert(++guest_calls==1 && !strcmp((char*)ptr(cpu->gpr[4],1),wanted));
        assert(cpu->gpr[5]==0x80100000+0xb94+(port*16+slot)*512);
        put32(ram+0x625a0,0x80080000+replace*0x1000);
        cpu->gpr[2]=(uint32_t)result; return 1;
    }
    assert(target!=0x801c8ca4); return 0;
}
static void to_guest(unsigned n) {
    uint8_t *dst=ram+0x100000+n*0x10000;
    memcpy(dst,cards[n].unk0,0xb80);
    memcpy(dst+0xb94,cards[n].unkB94,0x4000);
    memcpy(dst+0x4f80,cards[n].unk4F80,0xb4);
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb"); assert(f);
    size_t size=fread(overlay,1,sizeof overlay,f); fclose(f); assert(size>0x4270);
    memcpy(ram+0x1c5000,overlay,size);
    memcpy(D_801C50A8,overlay+0xa8,6); D_801C50AC=overlay[0xac];
    memcpy(D_801C50B0,overlay+0xb0,6); D_801C50B4=overlay[0xb4];
    assert((uintptr_t)cards>UINT32_MAX);
    unsigned fixtures=0;
    for(port=0;port<2;++port) for(slot=0;slot<16;++slot)
    for(count=0;count<=15;++count) for(initial=0;initial+count<=16;++initial)
    for(replace=0;replace<2;++replace) for(result=-1;result<=1;++result) {
        memset(cards,0xa5,sizeof cards); memset(menus,0,sizeof menus);
        unsigned index=port*16+slot;
        char name[32]; snprintf(name,sizeof name,"BISLPS-01234%02u",slot);
        snprintf(wanted,sizeof wanted,"%s%s",port?D_801C50B0:D_801C50A8,name);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk32C=&cards[n];
            strcpy((char*)&cards[n].unk0[index*0x5c+0x18],name);
            cards[n].unkB94[index*512+3]=(n==replace)?count:0;
            put32(cards[n].unk4F80+4,initial);
            to_guest(n);
            put32(ram+0x80000+n*0x1000+0x32c,0x80100000+n*0x10000);
        }
        memcpy(before,cards,sizeof cards);
        put32(ram+0x625a0,0x80080000); g_Menu=&menus[0]; calls=guest_calls=0;
        PcPortMipsBus bus={.read=bus_read,.write=bus_write,.bridge=bridge};
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=port; cpu.gpr[5]=slot; cpu.gpr[29]=0x801b0000; cpu.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&cpu,0x801c90b0,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801C90B0(port,slot);
        assert(calls==1 && guest_calls==1);
        for(unsigned n=0;n<2;++n) {
            uint8_t *guest=ram+0x100000+n*0x10000;
            assert(!memcmp(cards[n].unk4F80,guest+0x4f80,0xb4));
            /* All native bytes outside the two retail-written regions stay intact. */
            memcpy(before[n].unk4F80,guest+0x4f80,0xb4);
            assert(!memcmp(&cards[n],&before[n],sizeof cards[n]));
        }
        assert(get32(cards[replace].unk4F80+4)==initial+count);
        ++fixtures;
    }
    printf("PASS %u retail/native card-read fixtures: both ports, all slots, 0..15 blocks, failure/success, menu replacement\n",fixtures);
}
