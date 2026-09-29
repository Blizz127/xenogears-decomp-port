#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
SystemMenu *g_Menu;
static SystemMenu menu,expected;
static u8 ram[0x200000];
#include "time.inc"
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
static void check(uint32_t value) {
    memset(&menu,0xa5,sizeof menu);memcpy(&expected,&menu,sizeof menu);g_Menu=&menu;
    memset(ram+0x802ec,0xa5,28);uint32_t address=0x80080000;memcpy(ram+0x625a0,&address,4);
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};PcPortMipsCpu c;
    PcPortMipsCpuInit(&c,&bus);c.gpr[4]=value;c.gpr[5]=0xdeadbeef;
    c.gpr[29]=0x801b0000;c.gpr[31]=0xfffffffc;
    assert(PcPortMipsRun(&c,0x801c7f34,0xfffffffc,1000)==PC_PORT_MIPS_HALTED);
    memcpy(expected.unk2EC,ram+0x802ec,28);
#ifdef LEGACY_TIME
    u8 sink[6];func_801C7F34((s32)value,sink);
#else
    func_801C7F34(value);
#endif
    assert(!memcmp(&menu,&expected,sizeof menu));
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    assert((uintptr_t)&menu>UINT32_MAX);unsigned cases=0;
    for(uint32_t value=0;value<262144;++value) { check(value);++cases; }
    uint32_t random=0x31b5d43b;
    for(unsigned i=0;i<262144;++i) { random=random*1664525u+1013904223u;check(random);++cases; }
    const uint32_t divisors[]={21600000,2160000,216000,36000,3600,600,60};
    for(unsigned i=0;i<7;++i)for(unsigned n=0;n<256 && n<=UINT32_MAX/divisors[i];++n) {
        uint32_t low=n*divisors[i],high=(UINT32_MAX/divisors[i]-n)*divisors[i];
        for(int d=-1;d<=1;++d) { check(low+d);check(high+d);cases+=2; }
    }
    printf("PASS %u retail/native menu time field fixtures\n",cases);
}
