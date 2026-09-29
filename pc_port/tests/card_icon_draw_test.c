#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u32 D_801E981C[30];
static SystemMenu menus[2];
static MenuUnk2 cards[2];
static MenuManager managers[2];
static GfxEnvironment gfx[2];
static u8 ram[0x200000],*panels;
static unsigned owner,gowner,step,gstep,changes;
static u32 trace[31][4];
static int replace_at(unsigned boundary) {
#ifdef CARD_DRAW_DETAIL
    return changes==1 || (changes>=2 && boundary==changes-2);
#else
    return (changes&(1u<<boundary))!=0;
#endif
}
#include "pointer.inc"
static void advance(void) {
    if(replace_at(step))owner^=1;
    ++step;g_Menu=&menus[owner];
}
static void event(u32 kind,u32 a,u32 b,u32 c) {
    u32 got[]={kind,a,b,c};assert(step<gstep && !memcmp(got,trace[step],sizeof got));advance();
}
static void func_801CE2B4(s32 n,void *p,s32 ctx) { event(0,n,(u8*)p-panels,ctx); }
u_short GetClut(int x,int y) { event(1,x,y,0);return 0xbeef; }
void AddPrim(void *ot,void *p) {
    assert(ot==&gfx[owner].ot[4]);event(2,owner,(u8*)p-panels,0);
}
#include "draw.inc"
static u8 *ptr(u32 a,unsigned w) {a&=0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a;}
static int read_bus(void *o,u32 a,unsigned w,u32 *v) {
    (void)o;u8*p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;
}
static int write_bus(void *o,u32 a,unsigned w,u32 v) {
    (void)o;u8*p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(i*8);return 0;
}
static void put(u32 a,u32 v) {memcpy(ptr(a,4),&v,4);}
static int bridge(void *o,PcPortMipsCpu *c,u32 target) {
    (void)o;u32 v[4]={0};
    if(target==0x801ce2b4) {v[0]=0;v[1]=c->gpr[4];v[2]=c->gpr[5]-0x80120000;v[3]=c->gpr[6];}
    else if(target==0x80043a58) {v[0]=1;v[1]=c->gpr[4];v[2]=c->gpr[5];}
    else if(target==0x80043b48) {
        assert(c->gpr[4]==0x80130080+gowner*0x1000);
        v[0]=2;v[1]=gowner;v[2]=c->gpr[5]-0x80120000;
    } else return 0;
    assert(gstep<31);memcpy(trace[gstep],v,sizeof v);
    if(replace_at(gstep))gowner^=1;
    ++gstep;put(0x625a0,0x80080000+gowner*0x1000);
    c->gpr[2]=0xbeef;return 1;
}
int main(void) {
    FILE*f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(D_801E981C,ram+0x1e981c,sizeof D_801E981C);
    panels=mmap(NULL,0x6000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x6000<=UINT32_MAX);
    assert((uintptr_t)cards>UINT32_MAX && sizeof(POLY_FT4)==40);
    unsigned cases=0;
#ifdef CARD_DRAW_DETAIL
    for(unsigned selected=0;selected<8;++selected)for(unsigned wanted=0;wanted<16;++wanted)
    for(unsigned ctx=0;ctx<2;++ctx)for(changes=0;changes<33;++changes)for(unsigned frame=0;frame<2;++frame) {
#else
    for(unsigned selected=0;selected<30;++selected)for(unsigned wanted=0;wanted<32;++wanted)
    for(unsigned ctx=0;ctx<2;++ctx)for(changes=0;changes<8;++changes)for(unsigned frame=0;frame<3;++frame) {
#endif
        memset(menus,0,sizeof menus);memset(cards,0,sizeof cards);memset(managers,0,sizeof managers);
        memset(panels,0xa5,0x6000);memset(ram+0x120000,0xa5,0x6000);
        for(unsigned n=0;n<2;++n) {
            unsigned choice=(selected+n*7)%30,id=(wanted+n*13)%32;
            unsigned animation=(frame+n)%3;
            memset(ram+0x100000+n*0x6000,0,0x5034);
            for(unsigned i=0;i<0xb80;++i)cards[n].unk0[i]=(u8)(i*19+wanted*7+n*31);
            memcpy(ram+0x100000+n*0x6000,cards[n].unk0,0xb80);
            cards[n].unk4F7C=choice;
            assert(D_801E981C[choice]+0x2e<sizeof cards[n].unk4F80);
            cards[n].unk4F80[0x2e + D_801E981C[choice]]=id;
            put(0x104f7c+n*0x6000,choice);
            ram[0x104fae + n*0x6000+D_801E981C[choice]]=id;
            menus[n].unk32C=&cards[n];menus[n].pManager=&managers[n];managers[n].unkB=1;
            menus[n].pGfxEnv=&gfx[n];menus[n].renderContext=ctx^n;
            memcpy(menus[n].unk4CC,&animation,4);
            u32 p=(uintptr_t)(panels+n*0x3000);memcpy(menus[n].unk34C,&p,4);
            panels[n*0x3000+0x2dbc]=ram[0x122dbc+n*0x3000]=0;
#ifdef CARD_DRAW_DETAIL
            panels[n*0x3000+0x2dbc]=1;
            for(unsigned slot=0;slot<3;++slot) {
                u8 *meta=panels+n*0x3000+slot*0x87c;
                for(unsigned k=0;k<6;++k)meta[0x1308+k]=(u8)(wanted*17+k*29+n*43+slot*53);
                meta[0x130e]=(frame+slot+n)&1;
                meta[0x130f]=((wanted>>slot)+n)&1;
                meta[0x1310]=((selected^(n*7))>>slot)&1;
                meta[0x1311]=(ctx+slot+n)&1;
                meta[0x1312]=(u8)(wanted*19+slot*31+n*23);
            }
            memcpy(ram+0x120000+n*0x3000,panels+n*0x3000,0x3000);
#endif
            put(0x8032c+n*0x1000,0x80100000+n*0x6000);
            put(0x8033c+n*0x1000,0x80140000+n*0x100);ram[0x14000b+n*0x100]=1;
            put(0x8034c+n*0x1000,0x80120000+n*0x3000);
            put(0x801d4+n*0x1000,0x80130000+n*0x1000);
            put(0x80308+n*0x1000,ctx^n);put(0x804cc+n*0x1000,animation);
        }
        owner=gowner=step=gstep=0;g_Menu=&menus[0];put(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801b0000;cpu.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&cpu,0x801d02d8,0xfffffffc,10000)==PC_PORT_MIPS_HALTED);
#ifndef CARD_DRAW_DETAIL
        assert(gstep==4);
#endif
        func_801D02D8();assert(step==gstep && owner==gowner);
        assert(!memcmp(panels,ram+0x120000,0x6000));++cases;
    }
    assert(munmap(panels,0x6000)==0);
#ifdef CARD_DRAW_DETAIL
    printf("PASS %u retail/native card detail draw fixtures\n",cases);
#else
    printf("PASS %u retail/native card icon draw fixtures\n",cases);
#endif
}
