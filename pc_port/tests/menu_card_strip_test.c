#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u16 D_801EA04C,D_801EA050;
static SystemMenu menus[2];
static u8 ram[0x200000],*panels;
static unsigned owner,gowner,step,gstep,mask,flip;
static uint32_t initArgs[16];
#include "pointer.inc"
static void advance(unsigned kind) {
    ++step;if(mask&(1u<<kind))owner^=1;g_Menu=&menus[owner];
    if(flip)g_Menu->renderContext^=1;
}
static void func_801E927C(POLY_FT4 *p) {
    assert(step%3==0 && step<48 && (u8*)p==panels+initArgs[step/3]);
    memset(p,0x31,8);advance(0);
}
u_short GetTPage(int tp,int abr,int x,int y) {
    assert(step%3==1 && tp==0 && abr==0 && x==320 && y==128);
    advance(1);return 0xa500|step;
}
u_short GetClut(int x,int y) {
    assert(step%3==2 && x==0 && y==448);advance(2);return 0xb600|step;
}
#include "strip.inc"
static u8 *ptr(uint32_t a,unsigned w) {
    a &= 0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a;
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;u8 *p=ptr(a,w);*v=0;
    for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;u8 *p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static void put32(unsigned a,uint32_t v) { memcpy(ram+a,&v,4); }
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;unsigned kind;
    if(t==0x801e927c) {
        kind=0;assert(gstep%3==0 && gstep<48);
        initArgs[gstep/3]=c->gpr[4]-0x80100000;memset(ptr(c->gpr[4],8),0x31,8);
    } else if(t==0x80043a1c) {
        kind=1;assert(gstep%3==1 && c->gpr[4]==0 && c->gpr[5]==0 && c->gpr[6]==320 && c->gpr[7]==128);
    } else if(t==0x80043a58) {
        kind=2;assert(gstep%3==2 && c->gpr[4]==0 && c->gpr[5]==448);
    } else return 0;
    ++gstep;if(mask&(1u<<kind))gowner^=1;
    put32(0x625a0,0x80080000+gowner*0x1000);
    if(flip)ram[0x80308+gowner*0x1000]^=1;
    c->gpr[2]=(kind==1?0xa500:0xb600)|gstep;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    assert(sizeof(POLY_FT4)==40);
    panels=mmap(NULL,0x6000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x6000<=UINT32_MAX);
    assert((uintptr_t)menus>UINT32_MAX);
    const u16 edges[]={0,1,32760,32767,32768,65520,65534,65535};unsigned cases=0;
    for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y)for(unsigned ctx=0;ctx<2;++ctx)
    for(mask=0;mask<8;++mask)for(flip=0;flip<2;++flip) {
        D_801EA04C=edges[x];D_801EA050=edges[y];
        memcpy(ram+0x1ea04c,&D_801EA04C,2);memcpy(ram+0x1ea050,&D_801EA050,2);
        memset(menus,0,sizeof menus);memset(panels,0xa5,0x6000);memset(ram+0x100000,0xa5,0x6000);
        for(unsigned n=0;n<2;++n) {
            menus[n].renderContext=ctx^n;uint32_t p=(uintptr_t)(panels+n*0x3000);
            memcpy(menus[n].unk34C,&p,4);put32(0x8034c+n*0x1000,0x80100000+n*0x3000);
            put32(0x80308+n*0x1000,menus[n].renderContext);
        }
        owner=gowner=step=gstep=0;g_Menu=&menus[0];put32(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801e733c,0xfffffffc,10000)==PC_PORT_MIPS_HALTED);
        func_801E733C();assert(step==48 && gstep==48 && owner==gowner);
        assert(!memcmp(panels,ram+0x100000,0x6000));
        for(unsigned n=0;n<2;++n)assert(menus[n].renderContext==ram[0x80308+n*0x1000]);
        ++cases;
    }
    assert(munmap(panels,0x6000)==0);printf("PASS %u retail/native card strip fixtures\n",cases);
}
