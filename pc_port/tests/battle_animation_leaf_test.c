/* Native decomp leaves versus the actual battle overlay bytes. No bridges,
 * replacement opcodes, or simulated call results participate in this test. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

extern uint32_t func_800B168C(uint32_t, uint32_t) __attribute__((weak));
extern uint32_t func_800B16A4(const uint8_t*) __attribute__((weak));
static uint8_t overlay[0x60000];
static size_t overlay_size;
static uint8_t* data;
#define BASE 0x100000u
#define SIZE 0x10000u
static int read_bus(void* unused,uint32_t a,unsigned w,uint32_t* v) {
    const uint8_t* p;
    (void)unused;
    if (a>=0x8006faf0u && (uint64_t)a+w<=0x8006faf0u+overlay_size)
        p=overlay+(a-0x8006faf0u);
    else if(a>=BASE && (uint64_t)a+w<=BASE+SIZE) p=data+(a-BASE);
    else return -1;
    *v=0;
    for(unsigned i=0;i<w;++i) *v|=(uint32_t)p[i]<<(8*i);
    return 0;
}
static int write_bus(void* u,uint32_t a,unsigned w,uint32_t v) {
    (void)u;(void)a;(void)w;(void)v; return -1; /* leaves must not write */
}
static uint32_t retail(uint32_t entry,uint32_t a0,uint32_t a1) {
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=a0;cpu.gpr[5]=a1;cpu.gpr[31]=0xfffffffcu;
    int rc=PcPortMipsRun(&cpu,entry,0xfffffffcu,100000);
    if(rc!=PC_PORT_MIPS_HALTED) {fprintf(stderr,"LEAF retail failed: %s\n",cpu.error);exit(1);}
    return cpu.gpr[2];
}
static void word(unsigned offset,uint32_t v) {
    for(unsigned i=0;i<4;++i) data[offset+i]=(uint8_t)(v>>(8*i));
}
int main(void) {
    if(!func_800B168C || !func_800B16A4) {fputs("BATTLE ANIM LEAF FAIL missing owner\n",stderr);return 1;}
    FILE* f=fopen("disc/battle.bin","rb");assert(f);
    overlay_size=fread(overlay,1,sizeof(overlay),f);assert(feof(f));assert(fclose(f)==0);
    data=mmap((void*)(uintptr_t)BASE,SIZE,PROT_READ|PROT_WRITE,
              MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    assert(data==(void*)(uintptr_t)BASE);
    const uint32_t bases[]={0,BASE,0x80000000u,0xfffffff0u};
    const uint32_t indices[]={0,1,2,255,65535,0x80000000u,0xffffffffu};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<7;++j) {
        uint32_t expected=retail(0x800b168c,bases[i],indices[j]);
        if(func_800B168C(bases[i],indices[j])!=expected) {
            fputs("BATTLE ANIM LEAF FAIL index stride/wrap\n",stderr);return 1;
        }
    }
    for(unsigned count=0;count<=64;++count) {
        memset(data,0xa5,SIZE);
        unsigned offset=0x100;
        word(0x10,offset);word(0x14,count);
        for(unsigned i=0;i<count;++i) {
            data[offset]=(uint8_t)(i*37+count);
            data[offset+1]=(uint8_t)(i*19+3);
            offset+=((unsigned)data[offset+1]+1)*4;
        }
        uint8_t before[SIZE];memcpy(before,data,SIZE);
        uint32_t expected=retail(0x800b16a4,BASE,0);
        uint32_t actual=func_800B16A4(data);
        if(actual!=expected || memcmp(before,data,SIZE)) {
            fprintf(stderr,"BATTLE ANIM LEAF FAIL count=%u native=%u retail=%u\n",count,actual,expected);return 1;
        }
    }
    puts("BATTLE ANIM LEAF PASS 28 index/wrap and 65 variable-record cases");
    return 0;
}
