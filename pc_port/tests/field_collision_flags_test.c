#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
uint32_t D_800AFB20[16];
static uint32_t flags[256];
static unsigned char ram[0x200000];
extern uint32_t func_80092424(int32_t,int32_t);
extern void func_800924D4(int32_t,int32_t,int32_t);
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v) {
    (void)p;a&=0x1fffffff; if(a>sizeof(ram)-w)return -1;
    *v=0;for(unsigned i=0;i<w;i++)*v|=(uint32_t)ram[a+i]<<(i*8);return 0;
}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v) {
    (void)p;a&=0x1fffffff;if(a>sizeof(ram)-w)return -1;
    for(unsigned i=0;i<w;i++)ram[a+i]=(unsigned char)(v>>(i*8));return 0;
}
static uint32_t run(uint32_t entry,unsigned index,unsigned component,unsigned value) {
    PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;PcPortMipsCpu c;
    PcPortMipsCpuInit(&c,&bus);c.gpr[4]=index;c.gpr[5]=component;c.gpr[6]=value;
    c.gpr[31]=0x80010000;
    assert(PcPortMipsRun(&c,entry,0x80010000,1000)==0);return c.gpr[2];
}
int main(void) {
    FILE *f=fopen("disc/field.bin","rb");assert(f);
    size_t size=fread(ram+0x6faf0,1,0x90000,f);assert(size&&feof(f));fclose(f);
    assert((uintptr_t)flags<=UINT32_MAX);
    D_800AFB20[0]=(uint32_t)(uintptr_t)flags;D_800AFB20[1]=0x0076bcf0;
    wr(NULL,0x800afb20,4,0x80180000);wr(NULL,0x800afb24,4,0x80190000);
    const unsigned values[]={0,1,127,216,255};
    for(unsigned index=0;index<256;index++)for(unsigned c=0;c<4;c++)
    for(unsigned v=0;v<sizeof(values)/sizeof(values[0]);v++) {
        for(unsigned j=0;j<256;j++)flags[j]=0xa5b6c7d8u^j;
        memcpy(ram+0x180000,flags,sizeof(flags));
        assert(func_80092424(index,c)==run(0x80092424,index,c,0));
        func_800924D4(index,c,values[v]);run(0x800924d4,index,c,values[v]);
        assert(!memcmp(ram+0x180000,flags,sizeof(flags)));
        assert(func_80092424(index,c)==values[v]);
        assert(D_800AFB20[1]==0x0076bcf0);
    }
    puts("Collision flag native/retail differential: PASS (5120 cases)");
}
