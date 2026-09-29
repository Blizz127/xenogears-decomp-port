#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u32 D_801E9FE0[9];
static SystemMenu menus[2];
static u8 ram[0x200000],entry[64],resources[2][16],*panels;
static unsigned owner,gowner,step,gstep,swap,mutate;
static uint32_t args[13][7];
#include "pointer.inc"
#include "time.inc"
static void alter(u8 *p,unsigned count) {
    if(!mutate)return;
    p[0x23]+=17;
    u32 value;memcpy(&value,p,4);value+=0x10001+count;memcpy(p,&value,4);
}
static s32 func_8002675C(u8 *resource,s32 glyph,void *dst,s32 context,s32 x,s32 y,s32 scale) {
    assert(step<gstep && resource==resources[owner]);
    uint32_t actual[]={glyph,(uintptr_t)((u8*)dst-panels),context,x,y,scale,owner};
    assert(!memcmp(actual,args[step],sizeof actual));
    ++step;alter(entry,step);if(swap)owner^=1;g_Menu=&menus[owner];return step*31;
}
#include "summary.inc"
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
    (void)o;if(t!=0x8002675c)return 0;
    assert(gstep<13 && c->gpr[4]==0x80160000+gowner*16);
    args[gstep][0]=c->gpr[5];args[gstep][1]=c->gpr[6]-0x80100000;args[gstep][2]=c->gpr[7];
    for(unsigned i=0;i<3;++i)memcpy(&args[gstep][3+i],ptr(c->gpr[29]+16+i*4,4),4);
    args[gstep][6]=gowner;++gstep;alter(ram+0x150000,gstep);if(swap)gowner^=1;
    put32(0x625a0,0x80080000+gowner*0x1000);c->gpr[2]=gstep*31;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(D_801E9FE0,ram+0x1e9fe0,sizeof D_801E9FE0);
    panels=mmap(NULL,0x6000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x6000<=UINT32_MAX);
    assert((uintptr_t)entry>UINT32_MAX && (uintptr_t)resources>UINT32_MAX);
    const u32 times[]={0,59,60,599,600,3599,3600,215999,216000,2160000,21600000,UINT32_MAX,0x80000000};
    unsigned cases=0;
    for(unsigned value=0;value<256;++value)for(unsigned t=0;t<sizeof(times)/sizeof(times[0]);++t)
    for(swap=0;swap<2;++swap)for(mutate=0;mutate<2;++mutate) {
        memset(menus,0,sizeof menus);memset(entry,0xa5,sizeof entry);
        memset(panels,0xa5,0x6000);memset(ram+0x100000,0xa5,0x6000);
        memcpy(entry,&times[t],4);entry[0x23]=value;memcpy(ram+0x150000,entry,sizeof entry);
        for(unsigned n=0;n<2;++n) {
            menus[n].unk2DC=resources[n];menus[n].renderContext=value*257+n;
            memset(menus[n].unk2EC,0xa5,sizeof menus[n].unk2EC);
            uint32_t address=(uintptr_t)(panels+n*0x3000);memcpy(menus[n].unk34C,&address,4);
            put32(0x802dc+n*0x1000,0x80160000+n*16);
            put32(0x8034c+n*0x1000,0x80100000+n*0x3000);
            put32(0x80308+n*0x1000,menus[n].renderContext);
            memcpy(ram+0x802ec+n*0x1000,menus[n].unk2EC,28);
        }
        owner=gowner=step=gstep=0;g_Menu=&menus[0];put32(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=0x80150000;
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&c,0x801e68ac,0xfffffffc,4000)==PC_PORT_MIPS_HALTED);
        func_801E68AC(entry);assert(step==13 && gstep==13 && owner==gowner);
        assert(!memcmp(panels,ram+0x100000,0x6000));assert(!memcmp(entry,ram+0x150000,sizeof entry));
        for(unsigned n=0;n<2;++n)assert(!memcmp(menus[n].unk2EC,ram+0x802ec+n*0x1000,28));
        ++cases;
    }
    assert(munmap(panels,0x6000)==0);
    printf("PASS %u retail/native integrated card summary fixtures\n",cases);
}
