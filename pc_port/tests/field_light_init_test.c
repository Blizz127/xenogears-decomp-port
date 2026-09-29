/* Compare the actual FieldLoad call sequence with original retail instructions.
 * No lighting callbacks, GTE model, or invented matrix values are involved. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char ram[0x200000];
static unsigned char actual[96];
static int memory(uint32_t address, unsigned width, uint32_t *v, int write) {
    if (address < 0x80000000 || (uint64_t)address + width > 0x80200000)
        return -1;
    unsigned char *p = ram + (address & 0x1fffff);
    if (write) {
        for (unsigned i=0; i<width; i++) p[i] = *v >> (8*i);
    } else {
        *v=0;
        for (unsigned i=0; i<width; i++) *v |= (uint32_t)p[i] << (8*i);
    }
    return 0;
}
static int rd(void *u,uint32_t a,unsigned w,uint32_t *v) {
    (void)u; return memory(a,w,v,0);
}
static int wr(void *u,uint32_t a,unsigned w,uint32_t v) {
    (void)u; return memory(a,w,&v,1);
}
/* Generated from the two production FieldLoad statements, verbatim. */
#include "light_init_calls.inc"
int main(void) {
    FILE *f=fopen("disc/field.bin","rb");
    if (!f || fread(ram+0x6faf0,1,260862,f)!=260862) return 2;
    fclose(f);
    for (unsigned seed=0;seed<256;seed++) {
        for (unsigned i=0;i<sizeof actual;i++)
            actual[i]=ram[0xb220c+i]=(unsigned char)(seed+i*37);
        memset(ram+0x1ff000,seed,0x100);
        PcPortMipsBus bus={.read=rd,.write=wr};
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
        for(unsigned i=1;i<32;i++) cpu.gpr[i]=0xa5a50000+seed*257+i;
        cpu.gpr[29]=0x801ff000;
        int result=PcPortMipsRun(&cpu,0x80071618,0x8007167c,200);
        if(result) {fprintf(stderr,"FAIL retail: %s\n",cpu.error);return 1;}
        native_init(actual+0x30);
        if(memcmp(actual,ram+0xb220c,sizeof actual)) {
            for(unsigned i=0;i<sizeof actual;i++) if(actual[i]!=ram[0xb220c+i]) {
                fprintf(stderr,"FAIL matrix seed=%u offset=%u native=%02x retail=%02x\n",
                        seed,i,actual[i],ram[0xb220c+i]);break;
            }
            return 1;
        }
    }
    puts("FIELD LIGHT INITIALIZATION PASS cases=256");
    for(unsigned seed=0;seed<65536;seed++) {
        int32_t args[9];
        for(unsigned i=0;i<9;i++)
            args[i]=(int32_t)((0x87650000u+i*0x11110000u) | ((seed+i*7919)&65535));
        memset(actual,0xa5,sizeof actual);
        memset(ram+0xb220c,0xa5,sizeof actual);
        PcPortMipsBus bus={.read=rd,.write=wr};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=0x800b223c;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffc;
        for(unsigned i=0;i<3;i++) cpu.gpr[5+i]=(uint32_t)args[i];
        for(unsigned i=3;i<9;i++) {
            uint32_t v=(uint32_t)args[i];
            if(memory(0x801ff010+(i-3)*4,4,&v,1)) return 2;
        }
        if(PcPortMipsRun(&cpu,0x80077844,0xfffffffc,40)) return 2;
        func_80077844((short*)(actual+0x30),args[0],args[1],args[2],args[3],
                     args[4],args[5],args[6],args[7],args[8]);
        if(memcmp(actual,ram+0xb220c,sizeof actual)) {
            fprintf(stderr,"FAIL matrix writer seed=%u\n",seed);return 1;
        }
    }
    puts("FIELD MATRIX WRITER PASS cases=65536");
    return 0;
}
