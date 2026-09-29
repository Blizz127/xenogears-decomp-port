/* Boundary witness, NOT classifier acceptance: execute retail 9270 with
 * an empty-slot 0xFF and observe the first read outside its allocation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
static uint8_t card[0x5034], code[0x3b000];
static uint32_t rejected;
static int read_bus(void *opaque,uint32_t a,unsigned w,uint32_t *v) {
    (void)opaque;
    if(a==0x800625a0 && w==4) { *v=0x80080000;return 0; }
    if(a==0x8008032c && w==4) { *v=0x80100000;return 0; }
    uint8_t *p=NULL;
    if(a>=0x80100000 && (uint64_t)a+w<=0x80100000+sizeof card) p=card+a-0x80100000;
    else if(a>=0x801c5000 && (uint64_t)a+w<=0x801c5000+sizeof code) p=code+a-0x801c5000;
    if(!p) { rejected=a;return -1; }
    *v=0; for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(i*8);return 0;
}
static int write_bus(void *opaque,uint32_t a,unsigned w,uint32_t v) {
    (void)opaque;
    if(a<0x80100000 || (uint64_t)a+w>0x80100000+sizeof card) return -1;
    for(unsigned i=0;i<w;++i)card[a-0x80100000+i]=v>>(i*8);return 0;
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    size_t n=fread(code,1,sizeof code,f);fclose(f);assert(n>0x43a8);
    for(unsigned port=0;port<2;++port) {
        memset(card,0,sizeof card);card[0x4fae + port*16]=0xff;rejected=0;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=port;cpu.gpr[31]=0xfffffffc;
        int result=PcPortMipsRun(&cpu,0x801c9270,0xfffffffc,2000);
        assert(result==PC_PORT_MIPS_FAULT && rejected==0x80105bbc);
    }
    puts("PASS boundary witness: retail empty-slot scan reads card+0x5BBC outside 0x5034 bytes (both ports); classifier remains unresolved");
}
