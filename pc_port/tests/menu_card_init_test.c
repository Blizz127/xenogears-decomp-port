#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u32 D_801EA494[9],D_801E9F98[9],D_801E9FBC[9];
u16 D_801EA590[6],D_801EA5DC[6],g_SystemPalette1;
static SystemMenu menus[2];
static u8 ram[0x200000],resources[2][16],*panels;
static unsigned owner,gowner,step,gstep,changes;
static s32 result;
static uint32_t trace[36][9];
#include "pointer.inc"
static void advance(unsigned kind) {
    ++step;if(changes==1 || (changes==2 && kind==0) || (changes==3 && kind==3))owner^=1;
    g_Menu=&menus[owner];g_Menu->renderContext^=1;
}
static void record(unsigned kind,void *p,s32 a,s32 b,s32 c,s32 d,s32 e,s32 f) {
    assert(step<gstep);
    uint32_t v[]={kind,(uintptr_t)((u8*)p-panels),a,b,c,d,e,f,owner};
    assert(!memcmp(v,trace[step],sizeof v));advance(kind);
}
static s32 func_8002675C(u8 *res,s32 glyph,void *p,s32 ctx,s32 x,s32 y,s32 scale) {
    assert(res==resources[owner]);record(0,p,glyph,ctx,x,y,scale,0);return result;
}
static void func_801E927C(POLY_FT4 *p) { memset(p,0x31,8);record(1,p,0,0,0,0,0,0); }
u_short GetTPage(int tp,int abr,int x,int y) {
    assert(tp==0 && abr==0 && x==384 && y==0);record(2,panels,0,0,384,0,0,0);return 0xa500|step;
}
static void func_801E920C(POLY_FT4 *p,s32 x,s32 y,s32 u,s32 v,s32 w,s32 h) {
    ((u8*)p)[4]=0x72;record(3,p,x,y,u,v,w,h);
}
#include "init.inc"
static u8 *ptr(uint32_t a,unsigned w) { a&=0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a; }
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;u8 *p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;u8 *p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static void put32(unsigned a,uint32_t v) { memcpy(ram+a,&v,4); }
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;unsigned kind;uint32_t v[9]={0};
    if(t==0x8002675c) {
        kind=0;assert(c->gpr[4]==0x80160000+gowner*16);
        v[1]=c->gpr[6]-0x80100000;v[2]=c->gpr[5];v[3]=c->gpr[7];
        for(unsigned i=0;i<3;++i)memcpy(&v[4+i],ptr(c->gpr[29]+16+i*4,4),4);
    } else if(t==0x801e927c) {
        kind=1;v[1]=c->gpr[4]-0x80100000;memset(ptr(c->gpr[4],8),0x31,8);
    } else if(t==0x80043a1c) {
        kind=2;assert(c->gpr[4]==0 && c->gpr[5]==0 && c->gpr[6]==384 && c->gpr[7]==0);v[4]=384;
    } else if(t==0x801e920c) {
        kind=3;v[1]=c->gpr[4]-0x80100000;ptr(c->gpr[4],5)[4]=0x72;
        for(unsigned i=0;i<3;++i)v[2+i]=c->gpr[5+i];
        for(unsigned i=0;i<3;++i)memcpy(&v[5+i],ptr(c->gpr[29]+16+i*4,4),4);
    } else return 0;
    v[0]=kind;v[8]=gowner;assert(gstep<36);memcpy(trace[gstep],v,sizeof v);++gstep;
    if(changes==1 || (changes==2 && kind==0) || (changes==3 && kind==3))gowner^=1;
    put32(0x625a0,0x80080000+gowner*0x1000);ram[0x80308+gowner*0x1000]^=1;
    c->gpr[2]=kind==0?(uint32_t)result:(0xa500|gstep);return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(D_801E9F98,ram+0x1e9f98,sizeof D_801E9F98);memcpy(D_801E9FBC,ram+0x1e9fbc,sizeof D_801E9FBC);
    panels=mmap(NULL,0x20000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x20000<=UINT32_MAX);
    assert((uintptr_t)resources>UINT32_MAX && sizeof(POLY_FT4)==40);
    const s32 returns[]={0,3,255};unsigned cases=0;
    for(unsigned mask=0;mask<512;++mask)for(unsigned r=0;r<3;++r)
    for(changes=0;changes<4;++changes)for(unsigned ctx=0;ctx<2;++ctx) {
        result=returns[r];memset(menus,0,sizeof menus);memset(panels,0xa5,0x20000);memset(ram+0x100000,0xa5,0x20000);
        for(unsigned i=0;i<9;++i)D_801EA494[i]=(mask&(1u<<i))?(i?i:UINT32_MAX):0xffff;
        memcpy(ram+0x1ea494,D_801EA494,sizeof D_801EA494);
        for(unsigned i=0;i<6;++i) { D_801EA590[i]=mask*37+i;D_801EA5DC[i]=mask*19+i*31; }
        memcpy(ram+0x1ea590,D_801EA590,sizeof D_801EA590);memcpy(ram+0x1ea5dc,D_801EA5DC,sizeof D_801EA5DC);
        g_SystemPalette1=mask*73;memcpy(ram+0x595d4,&g_SystemPalette1,2);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk2DC=resources[n];menus[n].renderContext=ctx^n;uint32_t p=(uintptr_t)(panels+n*0x10000);
            memcpy(menus[n].unk34C,&p,4);put32(0x8034c+n*0x1000,0x80100000+n*0x10000);
            put32(0x802dc+n*0x1000,0x80160000+n*16);put32(0x80308+n*0x1000,ctx^n);
        }
        owner=gowner=step=gstep=0;g_Menu=&menus[0];put32(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801e61b0,0xfffffffc,10000)==PC_PORT_MIPS_HALTED);
        func_801E61B0();assert(step==gstep && owner==gowner);assert(!memcmp(panels,ram+0x100000,0x20000));++cases;
    }
    assert(munmap(panels,0x20000)==0);printf("PASS %u retail/native card initializer fixtures\n",cases);
}
