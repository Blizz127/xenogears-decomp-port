#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
static SystemMenu menus[3];
static MenuUnk2 cards[3],expected[3];
static uint8_t ram[0x200000];
static unsigned kind,port,replace,calls,gcalls;
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static void callback(unsigned which) {
    assert(which==kind && calls<59);
    if(!calls && !kind) assert(cards[0].unk4F80[0x66]==2);
    ++calls;unsigned owner=replace?calls%3:0;g_Menu=&menus[owner];
    cards[owner].unk4F80[0x66]=(u8)(calls+0x10);
    put32(cards[owner].unk4C94+0x2e0+port*4,(uint32_t)-2);
}
static void func_801C7BF4(void) { callback(0); }
int Vsync(int mode) { assert(mode==0);callback(1);return 0; }
#include "waits.inc"
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
    (void)o;if(t!=0x801c7bf4 && t!=0x8004b54c)return 0;
    assert(t==(kind?0x8004b54c:0x801c7bf4) && gcalls<59);
    if(kind) assert(c->gpr[4]==0);
    if(!gcalls && !kind)assert(ram[0x104fe6]==2);
    ++gcalls;unsigned owner=replace?gcalls%3:0;
    put32(ram+0x625a0,0x80080000+owner*0x1000);
    ram[0x104fe6+owner*0x10000]=(uint8_t)(gcalls+0x10);
    put32(ram+0x104f74+owner*0x10000+port*4,(uint32_t)-2);
    c->gpr[2]=0;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x3d78);fclose(f);
    const s32 values[]={-2,-1,0,1,2,INT32_MIN,INT32_MAX,0xfffe};
    unsigned cases=0;assert((uintptr_t)cards>UINT32_MAX);
    for(kind=0;kind<2;++kind)for(port=0;port<2;++port)for(replace=0;replace<2;++replace)
    for(unsigned a=0;a<8;++a)for(unsigned state=0;state<256;++state) {
        memset(cards,state,sizeof cards);memset(menus,0,sizeof menus);calls=gcalls=0;
        for(unsigned n=0;n<3;++n) {
            menus[n].unk32C=&cards[n];
            put32(cards[n].unk4C94+0x2e0+port*4,(uint32_t)values[a]);
            put32(cards[n].unk4C94+0x2e0+(1-port)*4,(uint32_t)(values[a]==-2?0:-2));
            memcpy(ram+0x104c94+n*0x10000,cards[n].unk4C94,0x2e8);
            memcpy(ram+0x104f80+n*0x10000,cards[n].unk4F80,0xb4);
            put32(ram+0x8032c+n*0x1000,0x80100000+n*0x10000);
        }
        memcpy(expected,cards,sizeof cards);g_Menu=&menus[0];put32(ram+0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[4]=port;c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,kind?0x801c8d1c:0x801c8ca4,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        if(kind)func_801C8D1C(port);else func_801C8CA4(port);
        assert(calls==gcalls && calls==(values[a]==-2?0:59));
        for(unsigned n=0;n<3;++n) {
            memcpy(expected[n].unk4C94,ram+0x104c94+n*0x10000,0x2e8);
            memcpy(expected[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
        }
        assert(!memcmp(cards,expected,sizeof cards));++cases;
    }
    printf("PASS %u retail/native card wait fixtures\n",cases);
}
