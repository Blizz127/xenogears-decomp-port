#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "world_map_convergence.h"
static unsigned char guest[0x200000];
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v) {
    (void)p;a&=0x1fffffffu;if(a>sizeof(guest)-w)return -1;
    *v=0;for(unsigned i=0;i<w;i++)*v|=(uint32_t)guest[a+i]<<(8*i);return 0;
}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v) {
    (void)p;a&=0x1fffffffu;if(a>sizeof(guest)-w)return -1;
    for(unsigned i=0;i<w;i++)guest[a+i]=(unsigned char)(v>>(8*i));return 0;
}
int main(void) {
    FILE *f=fopen("disc/world_map.bin","rb");assert(f);
    size_t n=fread(guest+0x6faf0,1,0x80000,f);assert(n&&feof(f));fclose(f);
    PsxMemory_Init();
    for(unsigned c=0;c<16;c++) {
        uint32_t selector=c==15?UINT32_MAX:c;
        for(unsigned i=0;i<0x2000;i++)guest[0x100000+i]=(unsigned char)(i*37+5);
        memcpy(PSX_ADDR(0x80100000),guest+0x100000,0x2000);
        wr(NULL,0x8009be24,4,0x80100000);wr(NULL,0x8009c610,4,selector);
        *(uint32_t*)PSX_ADDR(0x8009be24)=0x80100000;
        *(uint32_t*)PSX_ADDR(0x8009c610)=selector;
        wm_80072784_convergence_resume();
        PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        assert(PcPortMipsRun(&cpu,0x80072784,0x8007290c,2000)==0);
        assert(!memcmp(PSX_ADDR(0x80100000),guest+0x100000,0x2000));
    }
    puts("World resume callbacks: retail differential PASS (all selector arms, complete pool)");
}
