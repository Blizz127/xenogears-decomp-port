#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
u32 D_801EA01C,D_801EA020;
#ifdef WORD_DIGITS
u32 D_801EA02C,D_801EA030,D_801EA034,D_801EA038;
#define MAX_CALLS 6
#elif defined(BYTE_PAIR)
u32 D_801EA03C,D_801EA040,D_801EA044,D_801EA048;
#define MAX_CALLS 4
#else
#define MAX_CALLS 3
#endif
static SystemMenu menus[2];
static u8 ram[0x200000],entry[64],resources[2][16],*panels;
static unsigned owner,gowner,step,gstep,swap;
static s32 result;
static uint32_t args[MAX_CALLS][7];
#include "pointer.inc"
#include "decimal.inc"
static s32 func_8002675C(u8 *resource,s32 glyph,void *dst,s32 context,s32 x,s32 y,s32 scale) {
    assert(step<gstep && resource==resources[owner]);
    uint32_t actual[]={glyph,(uintptr_t)((u8*)dst-panels),context,x,y,scale,owner};
    assert(!memcmp(actual,args[step],sizeof actual));
    ++step;if(swap)owner^=1;g_Menu=&menus[owner];return result;
}
#include "digits.inc"
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
    assert(gstep<MAX_CALLS && c->gpr[4]==0x80160000+gowner*16);
    args[gstep][0]=c->gpr[5];args[gstep][1]=c->gpr[6]-0x80100000;args[gstep][2]=c->gpr[7];
    for(unsigned i=0;i<3;++i)memcpy(&args[gstep][3+i],ptr(c->gpr[29]+16+i*4,4),4);
    args[gstep][6]=gowner;++gstep;if(swap)gowner^=1;
    put32(0x625a0,0x80080000+gowner*0x1000);c->gpr[2]=result;return 1;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(&D_801EA01C,ram+0x1ea01c,4);memcpy(&D_801EA020,ram+0x1ea020,4);
#ifdef WORD_DIGITS
    memcpy(&D_801EA02C,ram+0x1ea02c,4);memcpy(&D_801EA030,ram+0x1ea030,4);
    memcpy(&D_801EA034,ram+0x1ea034,4);memcpy(&D_801EA038,ram+0x1ea038,4);
#elif defined(BYTE_PAIR)
    memcpy(&D_801EA03C,ram+0x1ea03c,4);memcpy(&D_801EA040,ram+0x1ea040,4);
    memcpy(&D_801EA044,ram+0x1ea044,4);memcpy(&D_801EA048,ram+0x1ea048,4);
#endif
    panels=mmap(NULL,0x20000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panels!=MAP_FAILED && (uintptr_t)panels+0x20000<=UINT32_MAX);
    assert((uintptr_t)entry>UINT32_MAX && (uintptr_t)resources>UINT32_MAX);
    const s32 results[]={0,1,3,255};const u8 presets[]={0xff,0,3,9};unsigned cases=0;
    for(unsigned slot=0;slot<3;++slot)
#ifdef WORD_DIGITS
    for(unsigned value=0;value<65536;++value) {
    /* Exhaust every halfword at each slot; expand callback combinations
     * at 256 uniformly spaced values, including both endpoints. */
    unsigned expanded=value%257==0;
#else
    for(unsigned value=0;value<256;++value) {
    unsigned expanded=1;
#endif
    for(unsigned r=0;r<(expanded?4:1);++r)for(swap=0;swap<(expanded?2:1);++swap)
    for(unsigned preset=0;preset<(expanded?4:1);++preset) {
        result=results[r];memset(menus,0,sizeof menus);memset(entry,0xa5,sizeof entry);
        memset(panels,0xa5,0x20000);memset(ram+0x100000,0xa5,0x20000);
        entry[0x16+slot]=value;entry[0x19+slot]=255-value;memcpy(ram+0x150000,entry,sizeof entry);
#ifdef WORD_DIGITS
        u16 a=value,b=65535-value;memcpy(entry+4+slot*2,&a,2);memcpy(entry+10+slot*2,&b,2);
        memcpy(ram+0x150000,entry,sizeof entry);
#elif defined(BYTE_PAIR)
        entry[0x10+slot]=value;entry[0x13+slot]=255-value;
        memcpy(ram+0x150000,entry,sizeof entry);
#endif
        for(unsigned n=0;n<2;++n) {
            menus[n].unk2DC=resources[n];menus[n].renderContext=value*257+n;
            memset(menus[n].digits,presets[preset],9);
            uint32_t address=(uintptr_t)(panels+n*0x10000);memcpy(menus[n].unk34C,&address,4);
            put32(0x802dc+n*0x1000,0x80160000+n*16);
            put32(0x8034c+n*0x1000,0x80100000+n*0x10000);
            put32(0x80308+n*0x1000,menus[n].renderContext);
            memcpy(ram+0x8031c+n*0x1000,menus[n].digits,9);
        }
        owner=gowner=step=gstep=0;g_Menu=&menus[0];put32(0x625a0,0x80080000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=slot;c.gpr[5]=0x80150000;
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
#ifdef WORD_DIGITS
        assert(PcPortMipsRun(&c,0x801e6cfc,0xfffffffc,4000)==PC_PORT_MIPS_HALTED);
        func_801E6CFC(slot,entry);
#elif defined(BYTE_PAIR)
        assert(PcPortMipsRun(&c,0x801e6f5c,0xfffffffc,4000)==PC_PORT_MIPS_HALTED);
        func_801E6F5C(slot,entry);
#else
        assert(PcPortMipsRun(&c,0x801e6b70,0xfffffffc,4000)==PC_PORT_MIPS_HALTED);
        func_801E6B70(slot,entry);
#endif
        assert(step==gstep && owner==gowner);
        assert(!memcmp(panels,ram+0x100000,0x20000));
        for(unsigned n=0;n<2;++n)assert(!memcmp(menus[n].digits,ram+0x8031c+n*0x1000,9));
        ++cases;
    }
    }
    assert(munmap(panels,0x20000)==0);
#ifdef WORD_DIGITS
    printf("PASS %u retail/native integrated card halfword fixtures\n",cases);
#elif defined(BYTE_PAIR)
    printf("PASS %u retail/native integrated card byte-pair fixtures\n",cases);
#else
    printf("PASS %u retail/native integrated card digit fixtures\n",cases);
#endif
}
