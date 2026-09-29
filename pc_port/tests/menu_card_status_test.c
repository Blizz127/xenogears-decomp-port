#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
/* Retail (and the port data blob) keep these two words adjacent; the body
 * indexes them as ((s32*)&D_801EA900)[port]. */
s32 D_801EA90x[2];
#define D_801EA900 (D_801EA90x[0])
#define D_801EA904 (D_801EA90x[1])
static SystemMenu menus[2];
static MenuUnk2 cards[2],expected[2];
static uint8_t ram[0x200000];
static unsigned port,replace,calls,gcalls;
static s32 response;
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static s32 func_801C891C(s32 channel) {
    assert(++calls==1 && channel==(s32)(port*16));
    assert(cards[0].unk4F80[0x64+port]==1);
    if(replace) g_Menu=&menus[1];
    return response;
}
#include "status.inc"
static uint8_t *ptr(uint32_t a,unsigned w) {
    a &= 0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a;
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;uint8_t *p=ptr(a,w);*v=0;
    for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;uint8_t *p=ptr(a,w);
    for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;if(t!=0x801c891c)return 0;
    assert(++gcalls==1 && c->gpr[4]==port*16);
    assert(ram[0x104fe4+port]==1);
    put32(ram+0x625a0,0x80080000+replace*0x1000);
    c->gpr[2]=(uint32_t)response;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x3bec);fclose(f);
    const s32 values[]={-2,-1,0,1,2,INT32_MIN};unsigned cases=0;
    assert((uintptr_t)cards>UINT32_MAX);
    for(port=0;port<2;++port) for(replace=0;replace<2;++replace)
    for(unsigned a=0;a<6;++a) for(unsigned b=0;b<6;++b) for(unsigned seen=0;seen<256;++seen) {
        memset(cards,seen,sizeof cards);memset(menus,0,sizeof menus);
        response=values[a];calls=gcalls=0;
        D_801EA900=0x12345678;D_801EA904=0x76543210;
        put32(ram+0x1ea900,D_801EA900);put32(ram+0x1ea904,D_801EA904);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk32C=&cards[n];
            put32(cards[n].unk4C94+0x2e0+port*4,(uint32_t)(n==replace?values[b]:0x12345678));
            cards[n].unk4F80[0x64+port]=(u8)(seen%3);
            cards[n].unk4F80[0x68+port]=(u8)seen;
            memcpy(ram+0x100000+n*0x10000,cards[n].unk0,0xb80);
            memcpy(ram+0x104c94+n*0x10000,cards[n].unk4C94,0x2e8);
            memcpy(ram+0x104f80+n*0x10000,cards[n].unk4F80,0xb4);
            put32(ram+0x8032c+n*0x1000,0x80100000+n*0x10000);
        }
        memcpy(expected,cards,sizeof cards);g_Menu=&menus[0];put32(ram+0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[4]=port;c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801c8a10,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        assert((uint32_t)func_801C8A10(port)==c.gpr[2]);
        assert(calls==1 && gcalls==1);
        for(unsigned n=0;n<2;++n) {
            memcpy(expected[n].unk0,ram+0x100000+n*0x10000,0xb80);
            memcpy(expected[n].unk4C94,ram+0x104c94+n*0x10000,0x2e8);
            memcpy(expected[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
        }
        assert(!memcmp(cards,expected,sizeof cards));
        assert(!memcmp(&D_801EA900,ram+0x1ea900,4));
        assert(!memcmp(&D_801EA904,ram+0x1ea904,4));++cases;
    }
    printf("PASS %u retail/native card status transitions\n",cases);
}
