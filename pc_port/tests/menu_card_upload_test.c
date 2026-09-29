#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u16 D_801EA590[6],D_801EA5DC[6];
static SystemMenu menu;
static MenuUnk2 card;
static u8 ram[0x200000],entry[64],allocation[0x400],raw[20],decoded[24];
static RECT upload;
static unsigned step,gstep,count;
static void func_80033B34(u16 *p,u8 *out,s32 n) {
    assert(step++==0 && n==(s32)count);
    assert(!memcmp(p,raw,n<10?2*n+2:20));memcpy(out,decoded,24);
}
void *HeapAlloc(u_int size,u_int flags) { assert(step++==1 && size==0x3f6 && flags==0);return allocation; }
static void clear_buffer(void *p,size_t n) { assert(step++==2 && p==allocation && n==0x3f6);memset(p,0,n); }
static s32 SystemRenderStringEntry(void *s,void *p,s32 width,s32 flag) {
    assert(step++==3 && p==allocation && width==0x24 && flag==0 && !memcmp(s,decoded,24));
    for(unsigned i=0;i<0x3f6;++i)assert(allocation[i]==0);
    memset(p,0x61,0x3f6);return 0;
}
int LoadImage(RECT *r,u_long *p) { assert(step++==4 && (void*)p==allocation && !memcmp(r,&upload,sizeof upload));return 0; }
int DrawSync(int mode) { assert(step++==5 && mode==0);return 0; }
u_int HeapFree(void *p) { assert(step++==6 && p==allocation);return 0; }
#define bzero clear_buffer
#include "upload.inc"
#undef bzero
static u8 *ptr(uint32_t a,unsigned w) { a&=0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a; }
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;u8 *p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;u8 *p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(8*i);return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;
    if(t==0x80033b34) {
        assert(gstep++==0 && c->gpr[6]<=10);count=c->gpr[6];
        memcpy(raw,ptr(c->gpr[4],20),count<10?count*2+2:20);memcpy(ptr(c->gpr[5],24),decoded,24);
    } else if(t==0x80031bdc) {
        assert(gstep++==1 && c->gpr[4]==0x3f6 && c->gpr[5]==0);c->gpr[2]=0x80160000;return 1;
    } else if(t==0x8003f8e8) {
        assert(gstep++==2 && c->gpr[4]==0x80160000 && c->gpr[5]==0x3f6);memset(ptr(c->gpr[4],0x3f6),0,0x3f6);
    } else if(t==0x80034eac) {
        assert(gstep++==3 && c->gpr[5]==0x80160000 && c->gpr[6]==0x24 && c->gpr[7]==0);
        assert(!memcmp(ptr(c->gpr[4],24),decoded,24));
        for(unsigned i=0;i<0x3f6;++i)assert(ram[0x160000+i]==0);
        memset(ram+0x160000,0x61,0x3f6);
    } else if(t==0x80044894) {
        assert(gstep++==4 && c->gpr[5]==0x80160000);memcpy(&upload,ptr(c->gpr[4],8),8);
    } else if(t==0x800445d0) { assert(gstep++==5 && c->gpr[4]==0);
    } else if(t==0x800320e8) { assert(gstep++==6 && c->gpr[4]==0x80160000);
    } else return 0;
    c->gpr[2]=0;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    assert((uintptr_t)&card>UINT32_MAX && (uintptr_t)allocation>UINT32_MAX);
    memset(&menu,0,sizeof menu);menu.unk32C=&card;g_Menu=&menu;unsigned cases=0;
    for(unsigned screen=0;screen<32;++screen)for(unsigned slot=0;slot<3;++slot)
    for(unsigned character=0;character<11;++character)for(unsigned length=0;length<=10;++length)
    for(unsigned pattern=0;pattern<2;++pattern) {
        memset(&card,0xa5,sizeof card);memset(entry,0xa5,sizeof entry);entry[0x1c+slot]=character;
        u8 *name=card.unkB94+0x100+screen*512+0x24+character*20;
        for(unsigned i=0;i<10;++i) { name[i*2]=pattern?0:i+1;name[i*2+1]=pattern?i+1:0; }
        if(length<10)name[length*2]=name[length*2+1]=0;
        memcpy(ram+0x100b94,card.unkB94,sizeof card.unkB94);memcpy(ram+0x150000,entry,sizeof entry);
        for(unsigned i=0;i<6;++i) { D_801EA590[i]=0xff00+screen*7+i*13;D_801EA5DC[i]=0x8000+screen*11+i*17; }
        memcpy(ram+0x1ea590,D_801EA590,12);memcpy(ram+0x1ea5dc,D_801EA5DC,12);
        for(unsigned i=0;i<24;++i)decoded[i]=i^screen;
        memset(allocation,0xa5,sizeof allocation);memset(ram+0x160000,0xa5,sizeof allocation);
        uint32_t p=0x80080000;memcpy(ram+0x625a0,&p,4);p=0x80100000;memcpy(ram+0x8032c,&p,4);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=slot;c.gpr[5]=0x80150000;c.gpr[6]=screen;
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;step=gstep=0;
        assert(PcPortMipsRun(&c,0x801e71b4,0xfffffffc,3000)==PC_PORT_MIPS_HALTED);
        func_801E71B4(slot,entry,screen);assert(step==7 && gstep==7);
        assert(!memcmp(allocation,ram+0x160000,sizeof allocation));++cases;
    }
    printf("PASS %u retail/native card name upload fixtures\n",cases);
}
