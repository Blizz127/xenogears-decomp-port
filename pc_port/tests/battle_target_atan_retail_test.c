/* Caller-domain quotient boundaries, not unrestricted ratan2 acceptance. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int PcPortDirectionNativeAtan(int, int);
extern short ratan_tbl[1025];
static uint8_t ram[0x200000], seen[1025];
static unsigned cases, y_less, y_ge;
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram));
    if (at>=0x57030 && at<0x57832) {
        assert(width==2 && !(at&1));
        seen[(at-0x57030)/2]=1;
    }
    if (address==0x8004B3E8) y_less++;
    if (address==0x8004B474) y_ge++;
    *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque; (void)address; (void)width; (void)value;
    assert(!"retail atan must not write memory"); return -1;
}
static void check(int y, int x) {
    assert(y>=-65535 && y<=65535 && x>=-65535 && x<=65535);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16; r<=23; r++) cpu.gpr[r]=0xAABB0000+r;
    cpu.gpr[4]=(uint32_t)y; cpu.gpr[5]=(uint32_t)x;
    cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    assert(PcPortMipsRun(&cpu,0x8004B32C,0xFFFFFFFC,1000)==0);
    int native_result=PcPortDirectionNativeAtan(y,x);
    assert(native_result==(int32_t)cpu.gpr[2]);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
    for (unsigned r=16; r<=23; r++) assert(cpu.gpr[r]==0xAABB0000+r);
    cases++;
}
int main(void) {
    FILE *f=fopen("disc/SLUS_006.64","rb"); assert(f);
    assert(!fseek(f,0x800,SEEK_SET));
    assert(fread(ram+0x10000,1,0x49800,f)==0x49800); fclose(f);
    short original_table[1025];
    memcpy(original_table,ratan_tbl,sizeof(original_table));
    for (unsigned i=0; i<1025; i++) assert(ratan_tbl[i]==(int16_t)get(0x57030+i*2,2));
#ifdef XBT_ATAN_CORRUPT_LAST
    ratan_tbl[1024]^=1; /* Isolated native-data negative control, never oracle data. */
#endif
    check(0,0);
    const unsigned denominators[]={1,2,3,7,31,255,1023,1024,1025,32767,32768,65534,65535};
    for (unsigned di=0; di<sizeof(denominators)/sizeof(denominators[0]); di++) {
        unsigned d=denominators[di];
        for (unsigned q=0; q<=1024; q++) for (int side=-1; side<=1; side++) {
            /* First numerator with floor(n*1024/d)>=q, plus both neighbors. */
            int n=(int)((q*d+1023)/1024)+side;
            if (n<0 || n>(int)d) continue;
            for (unsigned swap=0; swap<2; swap++) for (unsigned signs=0; signs<4; signs++) {
                int y=swap ? (int)d : n, x=swap ? n : (int)d;
                check(signs&1 ? -y : y,signs&2 ? -x : x);
            }
        }
    }
    assert(cases==303121);
    for (unsigned i=0; i<1025; i++) assert(seen[i]);
    assert(y_less && y_ge && memcmp(original_table,ratan_tbl,sizeof(original_table))==0);
    printf("TARGET ATAN real-native/retail PASS %u executions, all1025 table entries read\n",cases);
}
