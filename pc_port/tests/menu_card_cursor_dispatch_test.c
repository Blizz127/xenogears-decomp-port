#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u32 D_801E981C[32];
static SystemMenu menus[3];
static MenuUnk2 cards[3],expected[3];
static uint8_t ram[0x200000];
static unsigned input,selected,replace,step,gstep,moves,gmoves,refreshes,grefreshes;
static s32 mode,limit;
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
#ifdef INTEGRATED_NAVIGATION
u32 D_801E9820[32];
#include "navigation.inc"
#else
static void move(unsigned key,s32 m,s32 s,s32 l) {
    assert(moves++==0 && key==input && m==mode && s==(s32)selected);
    if(key==1 || key==3)assert(l==limit);
    ++step;if(replace)g_Menu=&menus[step];
    g_Menu->unk32C->unk4F7C=(selected+7)%30;
}
static void func_801C9EF4(s32 m,s32 s) { move(0,m,s,0); }
static void func_801CA1D4(s32 m,s32 s) { move(2,m,s,0); }
static void func_801CA480(s32 m,s32 s,s32 l) { move(1,m,s,l); }
static void func_801CA5F0(s32 m,s32 s,s32 l) { move(3,m,s,l); }
#endif
static void func_801E781C(s32 a,u8 b) {
    assert(refreshes++==0);MenuUnk2 *c=g_Menu->unk32C;
    u32 slot=D_801E981C[c->unk4F7C];
    assert(a==c->unk4F80[0x2E + slot] && b==c->unk4F80[0x0E + slot]);
    ++step;if(replace)g_Menu=&menus[step];
    g_Menu->unk32C->unk4F7C=(selected+11)%30;
}
#include "dispatch.inc"
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
    (void)o;const uint32_t targets[]={0x801c9ef4,0x801ca480,0x801ca1d4,0x801ca5f0};
    if(t==0x801e781c) {
        assert(grefreshes++==0);unsigned owner=replace?gstep:0;
        uint32_t pos;memcpy(&pos,ram+0x104f7c+owner*0x10000,4);
        unsigned slot=D_801E981C[pos];
        assert(c->gpr[4]==ram[0x104fae + owner*0x10000+slot]);
        assert(c->gpr[5]==ram[0x104f8e + owner*0x10000+slot]);
        ++gstep;owner=replace?gstep:0;
        put32(ram+0x104f7c+owner*0x10000,(selected+11)%30);
    } else {
#ifdef INTEGRATED_NAVIGATION
        /* Execute all four original movement routines in the MIPS engine. */
        return 0;
#else
        unsigned key;for(key=0;key<4 && t!=targets[key];++key){}
        if(key==4)return 0;
        assert(gmoves++==0 && key==input && c->gpr[4]==(uint32_t)mode && c->gpr[5]==selected);
        if(key==1 || key==3)assert(c->gpr[6]==(uint32_t)limit);
        ++gstep;put32(ram+0x104f7c+(replace?gstep:0)*0x10000,(selected+7)%30);
#endif
    }
    put32(ram+0x625a0,0x80080000+(replace?gstep:0)*0x1000);c->gpr[2]=0;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x2489c);fclose(f);
    memcpy(D_801E981C,ram+0x1e981c,sizeof D_801E981C);
#ifdef INTEGRATED_NAVIGATION
    memcpy(D_801E9820,ram+0x1e9820,sizeof D_801E9820);
#endif
    const s32 limits[]={-1,0,29,INT32_MAX};unsigned cases=0;
    assert((uintptr_t)cards>UINT32_MAX);
    for(input=0;input<256;++input)for(selected=0;selected<30;++selected)
    for(unsigned same=0;same<2;++same)for(replace=0;replace<2;++replace)
    for(mode=0;mode<3;++mode)for(unsigned l=0;l<4;++l) {
        limit=limits[l];memset(menus,0,sizeof menus);memset(cards,0xa5,sizeof cards);
        step=gstep=moves=gmoves=refreshes=grefreshes=0;
        for(unsigned n=0;n<3;++n) {
            menus[n].unk32C=&cards[n];menus[n].input=input;
            cards[n].unk4F7C=(selected+n)%30;
            put32(cards[n].unk4F80,same?(uint32_t)cards[n].unk4F7C:0xff);
            for(unsigned i=0;i<32;++i) {
                cards[n].unk4F80[0x2E + i]=(u8)(i+n*32);
                cards[n].unk4F80[0x0E + i]=(u8)(255-i-n*32);
#ifdef INTEGRATED_NAVIGATION
                unsigned occupied=l==1 || (l==2 && i==(selected+7)%32) ||
                    (l==3 && (i+selected)%3!=0);
                if(!occupied)cards[n].unk4F80[0x2E + i]=0xff;
                if((i+selected)%2==0)cards[n].unk4F80[0x0E + i]=0;
#endif
            }
#ifdef INTEGRATED_NAVIGATION
            for(unsigned i=0;i<3;++i)
                cards[n].unk4F80[0x64+i]=(selected & (1u<<i))?0x80:0;
#endif
            memcpy(ram+0x104f80+n*0x10000,cards[n].unk4F80,0xb4);
            put32(ram+0x104f7c+n*0x10000,cards[n].unk4F7C);
            put32(ram+0x8032c+n*0x1000,0x80100000+n*0x10000);
            ram[0x80325+n*0x1000]=input;
        }
        memcpy(expected,cards,sizeof cards);g_Menu=&menus[0];put32(ram+0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=mode;c.gpr[5]=0x12345678;c.gpr[6]=(uint32_t)limit;
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801ca750,0xfffffffc,1000)==PC_PORT_MIPS_HALTED);
        assert((uint32_t)func_801CA750(mode,0x12345678,limit)==c.gpr[2]);
        assert(moves==gmoves && refreshes==grefreshes && step==gstep);
        for(unsigned n=0;n<3;++n) {
            memcpy(&expected[n].unk4F7C,ram+0x104f7c+n*0x10000,4);
            memcpy(expected[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
        }
        assert(!memcmp(cards,expected,sizeof cards));++cases;
    }
#ifdef INTEGRATED_NAVIGATION
    printf("PASS %u retail/native integrated navigation fixtures\n",cases);
#else
    printf("PASS %u retail/native card cursor dispatch fixtures\n",cases);
#endif
}
