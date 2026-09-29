/* Retail state consumer, with directory enumeration at a controlled boundary. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
uint8_t g_PsxRam[0x200000];
static SystemMenu menus[3], expectedMenus[3];
static MenuUnk2 cards[3], expectedCards[3];
static uint8_t ram[0x200000];
static unsigned calls,gcalls,ports[2],gports[2],replace;
static s32 results[2];
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static s32 func_801C8D78(u8 port) {
    assert(calls<2 && port<2);ports[calls++]=port;
    if(replace) g_Menu=&menus[calls];
    /* State mutations must not change the already-selected branch. */
    g_Menu->unk32C->unk4F80[0x66]^=3;
    return results[port];
}
#include "scan_state.inc"
static uint8_t *ptr(uint32_t a,unsigned w) {
    a &= 0x1fffffff; assert((uint64_t)a+w<=sizeof ram);return ram+a;
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;uint8_t *p=ptr(a,w);*v=0;
    for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(i*8);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;uint8_t *p=ptr(a,w);
    for(unsigned i=0;i<w;++i)p[i]=v>>(i*8);return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;
    if(t!=0x801c8d78)return 0;
    unsigned p=c->gpr[4];assert(gcalls<2 && p<2);gports[gcalls++]=p;
    unsigned owner=replace?gcalls:0;
    put32(ram+0x625a0,0x80080000+owner*0x1000);
    ram[0x104fe6+owner*0x10000]^=3;
    c->gpr[2]=(uint32_t)results[p];return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x4038);fclose(f);
    assert((uintptr_t)cards>UINT32_MAX);
    const s32 values[]={0,1,255,256,-256,-1};unsigned cases=0;
    for(unsigned state=0;state<256;++state) for(unsigned ready=0;ready<4;++ready)
    for(replace=0;replace<2;++replace) for(unsigned a=0;a<6;++a) for(unsigned b=0;b<6;++b) {
        memset(cards,0xa5,sizeof cards);memset(menus,0x5a,sizeof menus);
        results[0]=values[a];results[1]=values[b];calls=gcalls=0;
        for(unsigned n=0;n<3;++n) {
            menus[n].unk32C=&cards[n];
            cards[n].unk4F80[0x66]=state;
            cards[n].unk4F80[8]=((ready>>0)&1)?0xff:0;
            cards[n].unk4F80[9]=((ready>>1)&1)?0x80:0;
            /* Different later owners detect snapshots across callbacks. */
            if(n==1) cards[n].unk4F80[9]^=0x80;
            memcpy(ram+0x104f80+n*0x10000,cards[n].unk4F80,0xb4);
            put32(ram+0x8032c+n*0x1000,0x80100000+n*0x10000);
            ram[0x80334+n*0x1000]=menus[n].unk334;
        }
        memcpy(expectedCards,cards,sizeof cards);memcpy(expectedMenus,menus,sizeof menus);
        g_Menu=&menus[0];put32(ram+0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801c8ee8,0xfffffffc,1000)==PC_PORT_MIPS_HALTED);
        func_801C8EE8();
        assert(calls==gcalls && !memcmp(ports,gports,calls*sizeof *ports));
        for(unsigned n=0;n<3;++n) {
            memcpy(expectedCards[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
            expectedMenus[n].unk334=ram[0x80334+n*0x1000];
        }
        assert(!memcmp(cards,expectedCards,sizeof cards));
        assert(!memcmp(menus,expectedMenus,sizeof menus));++cases;
    }
    printf("PASS %u retail/native card scan-state fixtures\n",cases);
}
