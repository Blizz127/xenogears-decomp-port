#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
extern void func_8001FBE4(void*,uint32_t,void*);
/* Link guards only: these data handlers must not call a dependency.
 * These are not replacement implementations and never enter the port. */
#define GUARD(name) void name(void){fputs("SPRITE DATA FAIL unexpected callee " #name "\n",stderr);abort();}
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE DATA FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE DATA FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE DATA FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE DATA FAIL unexpected angle helper\n",stderr); abort(); }
uint32_t D_80018644,D_80059198,D_800592E4;
uint8_t D_800591AD,D_800591B0,D_800591B3;
int16_t D_800592E8,D_800592EA;
uint32_t D_8004FBB8[8],D_8006F99C[4],D_8006F9AC[4],D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static int scale_override = -65536;
static struct {uint32_t sprite[64],base[16],aux[8];uint8_t ops[4];} fixture,initial,expected;
static uint8_t* address(uint32_t a,unsigned w)
{
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return (uint8_t*)(uintptr_t)a;
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    return NULL;
}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v)
{
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v)
{
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;
}
static void run(unsigned opcode,unsigned value,unsigned seed,unsigned test)
{
    for(unsigned i=0;i<sizeof(fixture);++i)((uint8_t*)&fixture)[i]=(uint8_t)(i*37+seed*73+value);
    uint8_t*s=(uint8_t*)fixture.sprite;
    fixture.sprite[0x20/4]=seed&1?0:(uint32_t)(uintptr_t)fixture.base;
    fixture.sprite[0x7c/4]=(uint32_t)(uintptr_t)fixture.aux;
    fixture.sprite[0xa8/4]=seed&2?0xffffffffu:0;
    fixture.sprite[0xac/4]=seed&2?4:0;
    if(scale_override!=-65536){uint16_t scale=(uint16_t)scale_override;memcpy(s+0x2c,&scale,2);}
    uint8_t*ops=seed&4?s+0x8c:fixture.ops;
    ops[0]=(uint8_t)value;ops[1]=(uint8_t)(value>>8);
    initial=fixture;
    PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(uint32_t)(uintptr_t)s;cpu.gpr[5]=opcode|((seed&4)?0xffffff00u:0);cpu.gpr[6]=(uint32_t)(uintptr_t)ops;cpu.gpr[29]=0x801fff00;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x8001fbe4,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE DATA FAIL retail opcode=%02x %s\n",opcode,cpu.error);abort();}
    expected=fixture;fixture=initial;
    func_8001FBE4(s,opcode|((seed&4)?0xffffff00u:0),ops);
    if(memcmp(&fixture,&expected,sizeof(fixture))){for(unsigned d=0;d<sizeof(fixture);++d)if(((uint8_t*)&fixture)[d]!=((uint8_t*)&expected)[d]){fprintf(stderr,"SPRITE DATA FAIL case=%u opcode=%02x value=%04x seed=%u diff=%u native=%02x retail=%02x\n",test,opcode,value,seed,d,((uint8_t*)&fixture)[d],((uint8_t*)&expected)[d]);break;}abort();}
}

/* B5's additional census is separate from the existing broad-handler path.
 * Transform destinations are aligned and entirely mapped. Layouts +34/+38
 * overwrite flags before the final dirty-bit OR, making order observable. */
static int run_b5_alias(unsigned value, unsigned mode, unsigned layout,
                       unsigned alias, unsigned variant, unsigned test,
                       unsigned *coupled_cases)
{
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    uint8_t *bases[] = {(uint8_t *)fixture.base, (uint8_t *)fixture.base,
                        sprite + 0x24, sprite + 0x34, sprite + 0x38};
    uint8_t *base = bases[layout];
    uint8_t *locations[] = {fixture.ops, sprite + 0x2c, sprite + 0x2d,
        sprite + 0x3c, sprite + 0x3d, base + 6, base + 7, base + 8,
        base + 9, base + 0xa, base + 0xb};
    uint8_t *ops = locations[alias];
    int coupled = ops == sprite + 0x3c;
    /* A byte physically sharing the mode bits cannot vary independently.
     * Enumerate every realizable byte/mode pair without rewriting either. */
    if (coupled && (value & 3u) != mode) return 0;
    if (coupled) ++*coupled_cases;
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + value);
    uint32_t packed_base = layout == 1 ? 0 : (uint32_t)(uintptr_t)base;
    uint32_t flags = ((variant & 1u) ? 0xfffffffcu : 0) | mode;
    memcpy(sprite + 0x20, &packed_base, sizeof(packed_base));
    memcpy(sprite + 0x3c, &flags, sizeof(flags));
    ops[0] = (uint8_t)value;
    assert((sprite[0x3c] & 3u) == mode && ops[0] == value);
    assert(address((uint32_t)(uintptr_t)ops, 1) == ops);
    assert(layout == 1 || address(packed_base + 6, 6) == base + 6);
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    uint32_t opcode = (variant & 2u) ? 0xdeadbeb5u : 0xb5u;
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00;
    cpu.gpr[31] = 0xfffffffcu;
    if (PcPortMipsRun(&cpu, 0x8001fbe4, 0xfffffffcu, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE DATA FAIL B5 alias oracle case=%u layout=%u "
                "alias=%u: %s\n", test, layout, alias, cpu.error);
        abort();
    }
    expected = fixture;
    fixture = initial;
    func_8001FBE4(sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        for (unsigned i = 0; i < sizeof(fixture); ++i) {
            if (((uint8_t *)&fixture)[i] == ((uint8_t *)&expected)[i]) continue;
            fprintf(stderr, "SPRITE DATA FAIL B5 alias case=%u byte=%02x "
                    "mode=%u layout=%u alias=%u variant=%u diff=%u "
                    "native=%02x retail=%02x\n", test, value, mode, layout,
                    alias, variant, i, ((uint8_t *)&fixture)[i],
                    ((uint8_t *)&expected)[i]);
            break;
        }
        abort();
    }
    return 1;
}
static unsigned run_b5_alias_census(void)
{
    unsigned count = 0, coupled = 0;
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    for (unsigned layout = 0; layout < 5; ++layout)
        for (unsigned alias = 0; alias < 11; ++alias)
            for (unsigned value = 0; value < 256; ++value)
                for (unsigned mode = 0; mode < 4; ++mode)
                    for (unsigned variant = 0; variant < 4; ++variant)
                        count += (unsigned)run_b5_alias(value, mode, layout,
                                                       alias, variant, count, &coupled);
    assert(count == 206848 && coupled == 6144);
    printf("SPRITE DATA PASS %u B5 alias cases: %u independent byte/mode "
           "cases and %u physically coupled flag-byte cases; 5 layouts, "
           "11 byte locations, 2 flag patterns, 2 opcode prefixes\n",
           count, count - coupled, coupled);
    puts("SPRITE DATA B5 scope: pinned retail dispatcher plus real scale "
         "helper; full fixture memory. Selected aligned overlaps only; "
         "no arbitrary-pointer, register-state or visible-runtime claim.");
    return count;
}
int main(int argc,char**argv)
{
    if(argc==2&&!strcmp(argv[1],"unhandled")){
        func_8001FBE4(NULL,0x12340090,NULL);
        fputs("SPRITE DATA FAIL unhandled instruction returned\n",stderr);
        return 1;
    }
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x800,SEEK_SET));
    size_t n=fread(ram+0x10000,1,sizeof(ram)-0x10000,f);assert(n>0x12000);assert(!fclose(f));
    if(argc==2&&!strcmp(argv[1],"b5")){
        unsigned count=0;
        for(unsigned value=0;value<256;++value)
            for(unsigned seed=0;seed<8;++seed)run(0xb5,value,seed,count++);
        printf("SPRITE DATA PASS %u B5 cases; real scale helper and full memory\n",count);
        count += run_b5_alias_census();
        printf("SPRITE DATA PASS %u total focused B5 cases\n", count);
        return 0;
    }
    const unsigned opcodes[]={0x8a,0xad,0xae,0xaf,0xb5,0xb6,0xb7,0xb8,0xc9,0xcc,0xed,0xee,0xef};
    unsigned count=0;
    for(unsigned k=0;k<sizeof(opcodes)/sizeof(*opcodes);++k)
        for(unsigned value=0;value<65536;++value)
            for(unsigned seed=0;seed<8;++seed)run(opcodes[k],value,seed,count++);
    const unsigned values[]={0,1,0xfff,0x1000,0x7fff,0x8000,0x8001,0xffff};
    for(scale_override=0;scale_override<65536;++scale_override)
        for(unsigned k=0;k<8;++k)run(0xee,values[k],k,count++);
    printf("SPRITE DATA PASS %u native/retail cases across %zu handlers\n",count,sizeof(opcodes)/sizeof(*opcodes));
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
