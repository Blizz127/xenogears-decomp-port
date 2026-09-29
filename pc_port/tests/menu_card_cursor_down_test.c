#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
#if defined(CURSOR_BACKWARD) || defined(CURSOR_SINGLE)
u32 D_801E981C[32];
#ifdef CURSOR_SINGLE
u32 D_801E9818[32], D_801E9820[32];
#endif
#else
u32 D_801E9820[32];
#endif
static SystemMenu menu;
static MenuUnk2 card,expected;
static uint8_t ram[0x200000];
#ifdef CURSOR_SINGLE
#include "cursor_single.inc"
#elif defined(CURSOR_BACKWARD)
#include "cursor_up.inc"
#else
#include "cursor_down.inc"
#endif
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
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
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x248a0);fclose(f);
#if defined(CURSOR_BACKWARD) || defined(CURSOR_SINGLE)
    memcpy(D_801E981C,ram+0x1e981c,sizeof D_801E981C);
#ifdef CURSOR_SINGLE
    memcpy(D_801E9818,ram+0x1e9818,sizeof D_801E9818);
    memcpy(D_801E9820,ram+0x1e9820,sizeof D_801E9820);
#endif
#else
    memcpy(D_801E9820,ram+0x1e9820,sizeof D_801E9820);
#endif
    g_Menu=&menu;menu.unk32C=&card;assert((uintptr_t)&card>UINT32_MAX);
    put32(ram+0x625a0,0x80080000);put32(ram+0x8032c,0x80100000);
    unsigned cases=0;
    for(int mode=-1;mode<=3;++mode)for(int selected=0;selected<30;++selected)
    for(int current=0;current<30;++current)for(unsigned pattern=0;pattern<100;++pattern) {
        memset(&card,0xa5,sizeof card);card.unk4F7C=current;
        for(unsigned i=0;i<32;++i) {
            unsigned occupied=pattern<64?(i==pattern%32):((i*13+pattern*7)%5!=0);
            if(pattern==98)occupied=0;if(pattern==99)occupied=1;
            card.unk4F80[0x2e + i]=occupied?(u8)i:0xff;
            card.unk4F80[0x0e + i]=pattern<32?1:pattern<64?0:(u8)((i+pattern)%3);
        }
        for(unsigned p=0;p<3;++p)card.unk4F80[0x64+p]=(pattern&(1u<<p))?0x80:0;
        memcpy(&expected,&card,sizeof card);
        memcpy(ram+0x104f80,card.unk4F80,sizeof card.unk4F80);put32(ram+0x104f7c,current);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=(uint32_t)mode;c.gpr[5]=selected;c.gpr[31]=0xfffffffc;
#ifdef CURSOR_SINGLE
        const s32 limits[]={-1,0,29,INT32_MAX};
        s32 limit=limits[pattern%4];c.gpr[6]=(uint32_t)limit;
#ifdef CURSOR_BACKWARD
        assert(PcPortMipsRun(&c,0x801ca5f0,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801CA5F0(mode,selected,limit);
#else
        assert(PcPortMipsRun(&c,0x801ca480,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801CA480(mode,selected,limit);
#endif
#elif defined(CURSOR_BACKWARD)
        assert(PcPortMipsRun(&c,0x801ca1d4,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801CA1D4(mode,selected);
#else
        assert(PcPortMipsRun(&c,0x801c9ef4,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801C9EF4(mode,selected);
#endif
        memcpy(&expected.unk4F7C,ram+0x104f7c,4);
        assert(!memcmp(&card,&expected,sizeof card));++cases;
    }
#ifdef CURSOR_SINGLE
    printf("PASS %u retail/native single-position cursor fixtures\n",cases);
#elif defined(CURSOR_BACKWARD)
    printf("PASS %u retail/native card cursor-retreat fixtures\n",cases);
#else
    printf("PASS %u retail/native card cursor-down fixtures\n",cases);
#endif
}
