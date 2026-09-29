#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include <psx/kernel.h>
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
static SystemMenu menus[2];
static MenuUnk2 cards[2], expected[2];
u8 D_801EA6D0[32];
s32 D_801EA900, D_801EA904;
char D_801C50A8[6], D_801C50B0[6], D_801C50AC, D_801C50B4;
static uint8_t ram[0x200000];
static unsigned port,entries,replace,foreign,pattern,step,gstep;
static struct DIRENTRY other;
static void put32(void *p,uint32_t v) { memcpy(p,&v,4); }
static void name(char *p,unsigned i) { snprintf(p,20,"BASLUS-00664%02u",i); }
static struct DIRENTRY *entry(struct DIRENTRY *p) {
    assert(step<=entries);
    unsigned i=step++;
    if(replace) g_Menu=&menus[step&1];
    /* Fill the full native entry, including its expanded pointer field. */
    memset(p,0x6b,sizeof *p); name(p->name,i);
    return i<entries?p:(foreign?&other:NULL);
}
struct DIRENTRY *firstfile(char *path,struct DIRENTRY *p) {
    assert(step==0 && !strcmp(path,port?D_801C50B0:D_801C50A8));
    return entry(p);
}
struct DIRENTRY *nextfile(struct DIRENTRY *p) { assert(step>0);return entry(p); }
#include "directory.inc"
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
    if(t==0x8003fb84) {
        strcpy((char*)ptr(c->gpr[4],1),(char*)ptr(c->gpr[5],1));
        c->gpr[2]=c->gpr[4];return 1;
    }
    if(t==0x80040584 || t==0x80040594) {
        assert(gstep<=entries);
        if(t==0x80040584) assert(gstep==0 && !strcmp((char*)ptr(c->gpr[4],1),port?D_801C50B0:D_801C50A8));
        else assert(gstep>0);
        uint32_t out=c->gpr[t==0x80040584?5:4];
        unsigned i=gstep++;
        if(replace) put32(ram+0x625a0,0x80080000+(gstep&1)*0x1000);
        memset(ptr(out,40),0x6b,40);name((char*)ptr(out,20),i);
        c->gpr[2]=i<entries?out:(foreign?0x80070000:0);return 1;
    }
    return 0;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x3ee8);fclose(f);
    memcpy(D_801C50A8,ram+0x1c50a8,6);memcpy(D_801C50B0,ram+0x1c50b0,6);
    D_801C50AC=D_801C50A8[4];D_801C50B4=D_801C50B0[4];
    assert((uintptr_t)cards>UINT32_MAX);
    unsigned cases=0;
    for(port=0;port<2;++port) for(entries=0;entries<=16;++entries)
    for(replace=0;replace<2;++replace) for(foreign=0;foreign<2;++foreign)
    for(pattern=0;pattern<256;pattern+=17) {
        memset(cards,pattern,sizeof cards);memset(menus,0,sizeof menus);
        memset(D_801EA6D0,pattern,sizeof D_801EA6D0);
        D_801EA900=0x12345678;D_801EA904=0x76543210;
        put32(ram+0x1ea900,D_801EA900);put32(ram+0x1ea904,D_801EA904);
        memcpy(ram+0x1ea6d0,D_801EA6D0,32);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk32C=&cards[n];
            uint8_t *p=ram+0x100000+n*0x10000;
            memcpy(p,cards[n].unk0,0xb80);memcpy(p+0x4f80,cards[n].unk4F80,0xb4);
            put32(ram+0x80000+n*0x1000+0x32c,0x80100000+n*0x10000);
        }
        memcpy(expected,cards,sizeof cards);put32(ram+0x625a0,0x80080000);
        g_Menu=&menus[0];step=gstep=0;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[4]=port;c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801c8d78,0xfffffffc,4000)==PC_PORT_MIPS_HALTED);
        assert(func_801C8D78(port)==c.gpr[2] && c.gpr[2]==entries);
        assert(step==entries+1 && gstep==step);
        assert(!memcmp(D_801EA6D0,ram+0x1ea6d0,32));
        assert(!memcmp(&D_801EA900,ram+0x1ea900,4));
        assert(!memcmp(&D_801EA904,ram+0x1ea904,4));
        for(unsigned n=0;n<2;++n) {
            memcpy(expected[n].unk0,ram+0x100000+n*0x10000,0xb80);
            memcpy(expected[n].unk4F80,ram+0x104f80+n*0x10000,0xb4);
        }
        assert(!memcmp(cards,expected,sizeof cards));++cases;
    }
    printf("PASS %u retail/native directory fixtures; provider boundary only\n",cases);
}
