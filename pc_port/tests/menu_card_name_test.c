#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
s32 D_801EA004[3],D_801EA010[3];
static SystemMenu menu;
static u8 ram[0x200000],entry[64],resources[16];
static u8 *panel;
static uint32_t args[6];
static unsigned calls;
#include "pointer.inc"
static s32 func_8002675C(u8 *resource,s32 glyph,void *dst,s32 context,s32 x,s32 y,s32 scale) {
    assert(calls++==0 && resource==resources);
    assert(glyph==(s32)args[0] && (u8*)dst==panel+args[1]);
    assert(context==(s32)args[2] && x==(s32)args[3] && y==(s32)args[4] && scale==(s32)args[5]);
    return 0x12345678;
}
#include "name.inc"
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
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;if(t!=0x8002675c)return 0;
    assert(calls++==0 && c->gpr[4]==0x80160000);
    args[0]=c->gpr[5];args[1]=c->gpr[6]-0x80100000;args[2]=c->gpr[7];
    for(unsigned i=0;i<3;++i)memcpy(&args[3+i],ptr(c->gpr[29]+16+4*i,4),4);
    c->gpr[2]=0x12345678;return 1;
}
static void put32(unsigned a,uint32_t v) { memcpy(ram+a,&v,4); }
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    memcpy(D_801EA004,ram+0x1ea004,sizeof D_801EA004);
    memcpy(D_801EA010,ram+0x1ea010,sizeof D_801EA010);
    panel=mmap(NULL,0x3000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panel!=MAP_FAILED && (uintptr_t)panel+0x3000<=UINT32_MAX);
    assert((uintptr_t)entry>UINT32_MAX && (uintptr_t)resources>UINT32_MAX);
    const s32 contexts[]={0,1,-1,INT32_MAX};unsigned cases=0;
    for(unsigned slot=0;slot<3;++slot)for(unsigned ch=0;ch<256;++ch)
    for(unsigned ctx=0;ctx<4;++ctx) {
        memset(&menu,0,sizeof menu);g_Menu=&menu;menu.unk2DC=resources;
        menu.renderContext=contexts[ctx];uint32_t address=(uintptr_t)panel;
        memcpy(menu.unk34C,&address,4);memset(entry,0xa5,sizeof entry);entry[0x1c+slot]=ch;
        memset(panel,0x5a,0x3000);memset(ram+0x100000,0x5a,0x3000);
        memcpy(ram+0x150000,entry,sizeof entry);
        put32(0x625a0,0x80080000);put32(0x802dc,0x80160000);
        put32(0x80308,contexts[ctx]);put32(0x8034c,0x80100000);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu c;
        PcPortMipsCpuInit(&c,&bus);c.gpr[4]=slot;c.gpr[5]=0x80150000;
        c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;calls=0;
        assert(PcPortMipsRun(&c,0x801e6ae8,0xfffffffc,1000)==PC_PORT_MIPS_HALTED && calls==1);
        calls=0;func_801E6AE8(slot,entry);assert(calls==1);
        assert(!memcmp(panel,ram+0x100000,0x3000));++cases;
    }
    assert(munmap(panel,0x3000)==0);
    printf("PASS %u retail/native card name fixtures\n",cases);
}
