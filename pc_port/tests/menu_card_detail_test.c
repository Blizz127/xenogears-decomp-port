#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
static SystemMenu menus[2];
static MenuUnk2 cards[2];
static u8 ram[0x200000],decoy[0x2000],*panels,*entry;
static unsigned screen,replace,context,step,gstep,owner,gowner;
typedef struct { unsigned kind,slot,owner,enabled; } Call;
static Call native[18],guest[18];
static const uint32_t targets[]={0x801e61b0,0x801e6ae8,0x801e6b70,
    0x801e6cfc,0x801e6f5c,0x801e71b4,0x801e68ac,0x801e733c};
#include "pointer.inc"
static int change_owner(unsigned count) {
    return replace==1 || (replace==2 && count==1) || (replace==3 && count%3==0);
}
static void call(unsigned kind,unsigned slot) {
    assert(step<18);
    native[step]=(Call){kind,slot,owner,kind==1?panels[owner*0x3000+0x1310+slot*0x87c]:0};
    ++step;if(change_owner(step))owner^=1;
    g_Menu=&menus[owner];g_Menu->renderContext=context+step*37;
}
/* Return a deliberately unrelated buffer: retail never consumes v0 here.
 * The unspecified prototype also lets the pre-repair source reproduce. */
static void *func_801E61B0() { call(0,0);return decoy; }
static void detail(unsigned kind,u8 slot,u8 *p) { assert(p==entry);call(kind,slot); }
static void func_801E6AE8(u8 s,u8 *p) { detail(1,s,p); }
static void func_801E6B70(u8 s,u8 *p) { detail(2,s,p); }
static void func_801E6CFC(u8 s,u8 *p) { detail(3,s,p); }
static void func_801E6F5C(u8 s,u8 *p) { detail(4,s,p); }
static void func_801E71B4(u8 s,void *p,u8 n) { assert(n==screen);detail(5,s,p); }
static void func_801E68AC(u8 *p) { assert(p==entry);call(6,0); }
static void func_801E733C() { call(7,0); }
#include "detail.inc"
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
    (void)o;unsigned kind;for(kind=0;kind<8 && t!=targets[kind];++kind){}
    if(kind==8)return 0;
    unsigned slot=0;
    if(kind>=1 && kind<=5) {
        slot=c->gpr[4];assert(slot<3 && c->gpr[5]==0x80100c94+screen*512);
        if(kind==5)assert(c->gpr[6]==screen);
    }
    if(kind==6)assert(c->gpr[4]==0x80100c94+screen*512);
    assert(gstep<18);
    guest[gstep]=(Call){kind,slot,gowner,kind==1?ram[0x140000+gowner*0x3000+0x1310+slot*0x87c]:0};
    ++gstep;if(change_owner(gstep))gowner^=1;
    put32(0x625a0,0x80080000+gowner*0x1000);
    put32(0x80308+gowner*0x1000,context+gstep*37);
    c->gpr[2]=0x80160000;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    panels=mmap(NULL,0x6000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x6000<=UINT32_MAX);
    assert((uintptr_t)cards>UINT32_MAX);unsigned cases=0;
    for(screen=0;screen<32;++screen)for(unsigned mask=0;mask<8;++mask)
    for(replace=0;replace<4;++replace)for(context=0;context<256;context+=17) {
        memset(menus,0,sizeof menus);memset(cards,0xa5,sizeof cards);
        memset(panels,0xa5,0x6000);memset(ram+0x140000,0xa5,0x6000);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk32C=&cards[n];menus[n].renderContext=context;
            uint32_t address=(uintptr_t)(panels+n*0x3000);memcpy(menus[n].unk34C,&address,4);
            put32(0x8032c+n*0x1000,0x80100000+n*0x10000);
            put32(0x8034c+n*0x1000,0x80140000+n*0x3000);
            put32(0x80308+n*0x1000,context);
        }
        entry=cards[0].unkB94+0x100+screen*512;
        for(unsigned i=0;i<3;++i)entry[0x1c+i]=(mask&(1u<<i))?i:0xff;
        memcpy(ram+0x100b94,cards[0].unkB94,sizeof cards[0].unkB94);
        step=gstep=owner=gowner=0;g_Menu=&menus[0];put32(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=screen;c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801e76ec,0xfffffffc,2000)==PC_PORT_MIPS_HALTED);
        func_801E76EC(screen);
        assert(step==gstep && owner==gowner && !memcmp(native,guest,step*sizeof(Call)));
        assert(!memcmp(panels,ram+0x140000,0x6000));++cases;
    }
    assert(munmap(panels,0x6000)==0);
    printf("PASS %u retail/native card detail caller fixtures\n",cases);
}
