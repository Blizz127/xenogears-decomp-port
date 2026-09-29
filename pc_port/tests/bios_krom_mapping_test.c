/* Execute the supplied BIOS KROM mapping. This is provider authority
 * evidence, not a native provider implementation or glyph rendering test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
static uint8_t bios[0x80000],ram[0x200000];
#ifdef TEST_NATIVE_KROM
#include "krom_mapping.h"
static unsigned long native_comparisons;
static unsigned long span_comparisons;
static void check_span(uint32_t code, const uint8_t *selector,
                       uint32_t address, size_t length) {
    const uint8_t *span=bios; /* Stale output must be cleared on failure. */
    PcPortKromResult expected;
    if(address==UINT32_MAX)expected=PC_PORT_KROM_REJECTED;
    else if(address<0xbfc00000 || address>0xbfc80000 || length>0xbfc80000-address)
        expected=PC_PORT_KROM_OUTSIDE_ROM;
    else expected=PC_PORT_KROM_OK;
    assert(PcPortKromResolveRomSpan(bios,code,selector,length,&span)==expected);
    if(expected==PC_PORT_KROM_OK) {
        assert(span==bios+(address-0xbfc00000));
        assert(memcmp(span,bios+(address-0xbfc00000),length)==0);
    } else assert(span==NULL);
    ++span_comparisons;
}
#endif
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;a&=0x1fffffff;const uint8_t*p;
    if((uint64_t)a+w<=sizeof ram)p=ram+a;
    else if(a>=0x1fc00000 && (uint64_t)a+w<=0x1fc80000)p=bios+a-0x1fc00000;
    else return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o;a&=0x1fffffff;if((uint64_t)a+w>sizeof ram)return -1;
    for(unsigned i=0;i<w;++i)ram[a+i]=v>>(8*i);return 0;
}
static uint32_t run_with_scratch(uint32_t code,uint8_t scratch) {
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=code;
    cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffc;
    /* KROM's outer frame is 32 bytes, mapping helper frame is 40 bytes.
     * Gap branches load helper sp+28 without initializing it. */
    ram[0x1ff000-32-40+28]=scratch;
    int result=PcPortMipsRun(&cpu,0x65e0,0xfffffffc,10000);
    if(result)fprintf(stderr,"KROM input=%08x %s\n",code,cpu.error);
    assert(result==PC_PORT_MIPS_HALTED);
#ifdef TEST_NATIVE_KROM
    uint32_t native=PcPortKromMapAddress(bios,code,scratch);
    if(native!=cpu.gpr[2])fprintf(stderr,"DIFF code=%08x scratch=%u native=%08x bios=%08x\n",code,scratch,native,cpu.gpr[2]);
    assert(native==cpu.gpr[2]);++native_comparisons;
    check_span(code,&scratch,cpu.gpr[2],32);
#endif
    return cpu.gpr[2];
}
static uint32_t run(uint32_t code) { return run_with_scratch(code,0); }
int main(void) {
    FILE*f=fopen("disc/scph5500.bin","rb");assert(f);
    assert(fread(bios,1,sizeof bios,f)==sizeof bios);fclose(f);
    uint32_t vector;memcpy(&vector,bios+0x10374+0x51*4,4);assert(vector==0x65e0);
    /* BIOS relocated low-RAM code and its tables; font reads remain ROM. */
    memcpy(ram,bios+0xfb00,0x10000);
    assert(run(0x8140)==0xbfc66000);
    assert(run(0)==UINT32_MAX && run(0xffff)==UINT32_MAX);
    uint32_t accepted=0,rejected=0,min=UINT32_MAX,max=0,hash=2166136261u;
    for(uint32_t code=0;code<0x10000;++code) {
        uint32_t address=run(code);
        assert(run(code|0x12340000)==address);
        assert(run(code|0xffff0000)==address);
        for(unsigned b=0;b<4;++b)hash=(hash^((address>>(8*b))&255))*16777619u;
        if(address==UINT32_MAX) {++rejected;continue;}
        ++accepted;if(address<min)min=address;if(address>max)max=address;
        assert(address>=0xbfc00000 && (uint64_t)address+32<=0xbfc80000);
        uint32_t word;assert(!read_bus(NULL,address+28,4,&word));
    }
    assert(accepted==4947 && rejected==60589);
    assert(min==0xbfc66000 && max==0xbfc7f8c0 && hash==0xd124e60a);
    unsigned dependent=0;
    for(uint32_t code=0;code<65536;++code) {
        uint32_t zero=run_with_scratch(code,0);
        uint32_t one=run_with_scratch(code,1);
        if(zero!=one)++dependent;
    }
    uint32_t gap_zero=run_with_scratch(0x817f,0),gap_one=run_with_scratch(0x817f,1);
    assert(dependent==371 && gap_zero==0xbfc66762 && gap_one==0xbfc66744);
    for(unsigned scratch=0;scratch<256;++scratch)
        assert(run_with_scratch(0x8140,scratch)==0xbfc66000);
    printf("STACK_DEPENDENCE %u codes differ for scratch selectors 0/1; 817F=%08x/%08x\n",dependent,gap_zero,gap_one);
    uint16_t title_table[128];
    f=fopen("disc/menu.bin","rb");assert(f);
    assert(fseek(f,0x801ea5d0-0x801c5000,SEEK_SET)==0);
    assert(fread(title_table,1,sizeof title_table,f)==sizeof title_table);fclose(f);
    unsigned title_dependent=0;
    for(unsigned lead=0;lead<128;++lead) {
        uint32_t code=lead<32?0x8140:title_table[lead];
        if(run_with_scratch(code,0)!=run_with_scratch(code,1))++title_dependent;
    }
    printf("TITLE_SINGLE_BYTE %u of 128 lead-byte mappings depend on scratch 0/1\n",title_dependent);
    printf("BIOS KROM mapping: accepted=%u rejected=%u min=%08x max=%08x fnv=%08x\n",accepted,rejected,min,max,hash);
    puts("PASS zero-scratch census only: 65536 inputs plus upper-word aliases; 32-byte spans inside ROM");
    puts("NOT_UNIVERSAL_MAPPING: some byte gaps consume uninitialized BIOS stack state");
    puts("NOT_NATIVE_ACCEPTANCE: no provider replacement, title raster or GPU executed");
#ifdef TEST_NATIVE_KROM
    /* All code/selector pairs, not only the original zero/one witnesses. */
    unsigned unknown=0;
    for(uint32_t code=0;code<65536;++code) {
        uint32_t first=run_with_scratch(code,0);int same=1;
        for(unsigned scratch=1;scratch<256;++scratch)
            if(run_with_scratch(code,scratch)!=first)same=0;
        if(same)check_span(code,NULL,first,32);
        else {
            const uint8_t *span=bios;
            assert(PcPortKromResolveRomSpan(bios,code,NULL,32,&span)==PC_PORT_KROM_NEEDS_SELECTOR);
            assert(span==NULL);++unknown;
        }
    }
    uint8_t selector=0;
    uint32_t address=run_with_scratch(0x8140,selector);
    const size_t lengths[]={0,1,30,31,32,0x1a000,0x1a001,SIZE_MAX};
    for(unsigned i=0;i<sizeof lengths/sizeof *lengths;++i)
        check_span(0x8140,&selector,address,lengths[i]);
    printf("PASS %lu native/BIOS mapping comparisons\n",native_comparisons);
    printf("PASS %lu ROM span comparisons; %u unknown-selector codes remain explicit\n",span_comparisons,unknown);
#endif
}
