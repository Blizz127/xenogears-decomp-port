#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
uint8_t g_PsxRam[0x200000]; /* MENU_GUEST_PTR maps NULL to guest address 0 */
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u8 D_801E9779;
static SystemMenu menus[3],expected[3];
static MenuUnk2 cards[3],before[3];
static uint8_t ram[0x200000];
static unsigned calls,gcalls,replace;
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
#ifdef INTEGRATED_STATUS
/* Adjacent in retail and in the port data blob; bodies index them as a pair. */
s32 D_801EA90x[2];
#define D_801EA900 (D_801EA90x[0])
#define D_801EA904 (D_801EA90x[1])
static s32 response[2];
static s32 func_801C891C(s32 channel) {
    assert(calls<2 && channel==(s32)(calls*16));
    assert(g_Menu->unk32C->unk4F80[0x64+calls]==1);
    unsigned port=calls++;
    if(replace)g_Menu=&menus[calls];
    return response[port];
}
#include "status.inc"
#else
static s32 func_801C8A10(u8 port) {
    assert(calls<2 && port==calls);++calls;
    if(replace)g_Menu=&menus[calls];
    return -123; /* caller must ignore the return */
}
#endif
#include "poller.inc"
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
#ifdef INTEGRATED_STATUS
    (void)o;if(t!=0x801c891c)return 0;
    assert(gcalls<2 && c->gpr[4]==gcalls*16);
    assert(ram[0x104fe4+(replace?gcalls:0)*0x10000+gcalls]==1);
    unsigned port=gcalls++;
    put32(ram+0x625a0,0x80080000+(replace?gcalls:0)*0x1000);
    c->gpr[2]=(uint32_t)response[port];return 1;
#else
    (void)o;if(t!=0x801c8a10)return 0;
    assert(gcalls<2 && c->gpr[4]==gcalls);++gcalls;
    put32(ram+0x625a0,0x80080000+(replace?gcalls:0)*0x1000);
    c->gpr[2]=(uint32_t)-123;return 1;
#endif
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x3ca4);fclose(f);
    assert((uintptr_t)cards>UINT32_MAX);unsigned cases=0;
#ifdef INTEGRATED_STATUS
    const unsigned thresholdStep=17;
    const s32 responses[]={0,1,-1,-2};
#else
    const unsigned thresholdStep=1;
#endif
    for(unsigned active=0;active<2;++active)for(unsigned count=0;count<256;++count)
    for(unsigned threshold=0;threshold<256;threshold+=thresholdStep)for(replace=0;replace<2;++replace) {
        memset(menus,0x5a,sizeof menus);memset(cards,0xa5,sizeof cards);
        calls=gcalls=0;D_801E9779=threshold;ram[0x1e9779]=threshold;
        unsigned mask=(count^threshold)&3;
#ifdef INTEGRATED_STATUS
        response[0]=responses[count%4];response[1]=responses[(count/4+threshold)%4];
        D_801EA900=0x12345678;D_801EA904=0x76543210;
        put32(ram+0x1ea900,D_801EA900);put32(ram+0x1ea904,D_801EA904);
#endif
        for(unsigned n=0;n<3;++n) {
            menus[n].unk32C=&cards[n];menus[n].unk326=(u8)(count+n);
            cards[n].unk4F80[0x66]=n?0:(active?(count&1?1:255):0);
            for(unsigned p=0;p<2;++p) {
                s32 value=((mask^n)&(1u<<p))?-1:(p?-2:0);
                put32(cards[n].unk4C94+0x2e0+p*4,(uint32_t)value);
                put32(ram+0x104f74+n*0x10000+p*4,(uint32_t)value);
            }
            ram[0x104fe6+n*0x10000]=cards[n].unk4F80[0x66];
            put32(ram+0x8032c+n*0x1000,0x80100000+n*0x10000);
            ram[0x80326+n*0x1000]=menus[n].unk326;
            ram[0x80334+n*0x1000]=menus[n].unk334;
#ifdef INTEGRATED_STATUS
            memcpy(ram+0x100000+n*0x10000,cards[n].unk0,0xb80);
            memcpy(ram+0x104c94+n*0x10000,cards[n].unk4C94,0x2e8);
            memcpy(ram+0x104f80+n*0x10000,cards[n].unk4F80,0xb4);
#endif
        }
        memcpy(expected,menus,sizeof menus);memcpy(before,cards,sizeof cards);
        g_Menu=&menus[0];put32(ram+0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801c8bec,0xfffffffc,3000)==PC_PORT_MIPS_HALTED);
        func_801C8BEC();
        assert(calls==gcalls && calls==((active && (u8)(count+1)>threshold)?2:0));
        for(unsigned n=0;n<3;++n) {
            expected[n].unk326=ram[0x80326+n*0x1000];
            expected[n].unk334=ram[0x80334+n*0x1000];
#ifdef INTEGRATED_STATUS
            memcpy(before[n].unk0,ram+0x100000+n*0x10000,0xb80);
            memcpy(before[n].unk4C94,ram+0x104c94+n*0x10000,0x2e8);
            memcpy(before[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
#endif
        }
#ifdef INTEGRATED_STATUS
        assert(!memcmp(&D_801EA900,ram+0x1ea900,4));
        assert(!memcmp(&D_801EA904,ram+0x1ea904,4));
#endif
        assert(!memcmp(menus,expected,sizeof menus));
        assert(!memcmp(cards,before,sizeof cards));++cases;
    }
#ifdef INTEGRATED_STATUS
    printf("PASS %u integrated retail/native poller + status fixtures; card-info boundary\n",cases);
#else
    printf("PASS %u retail/native periodic card poll fixtures\n",cases);
#endif
}
