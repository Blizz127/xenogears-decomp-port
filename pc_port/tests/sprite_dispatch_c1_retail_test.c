#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"

extern void func_8001FBE4(void *, uint32_t, void *);
/* Link guards only. C1 uses its real math helpers and RNG; other paths cannot silently
 * turn this test into a comparison against replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE C1 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
GUARD(func_8001D4E8)
int32_t g_WorkListCurTimer; uint8_t D_8006BE10[32];
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(__wrap_ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(__wrap_MulMatrix0)
GUARD(__wrap_ReadGeomOffset)  GUARD(__wrap_ScaleMatrix) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(func_80023290)
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE C1 FAIL unexpected angle helper\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
extern uint32_t g_RandomSeed;
extern int __real_rand(void);
static unsigned native_rand_calls;
int __wrap_rand(void) { ++native_rand_calls; return __real_rand(); }
extern unsigned int MFC2(int), CFC2(int);
extern void MTC2(unsigned int, int), CTC2(unsigned int, int);
extern int doCOP2(int);
int g_cfg_pgxpTextureCorrection;
static uint8_t ram[0x200000];
static struct {uint32_t before[8], sprite[64], between[8], ops[2], after[8];} fixture, initial, expected;
static unsigned cases, rand_entries, handler_entries, slow_entries;
static PcPortMipsCpu *active_cpu;
static int16_t retail_matrix[9], native_matrix[9], retail_angles[3];
static void read_rotation_controls(int16_t matrix[9])
{
    for(unsigned i=0;i<9;++i)matrix[i]=(int16_t)(CFC2((int)(i/2))>>((i&1)*16));
}
static uint32_t word(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void put16(uint8_t *p, uint16_t v) { memcpy(p,&v,2); }
static void put32(uint8_t *p, uint32_t v) { memcpy(p,&v,4); }
static uint8_t *address(uint32_t a, unsigned w)
{
    uintptr_t seed=(uintptr_t)&g_RandomSeed, first=(uintptr_t)&fixture;
    if(a>=seed && (uint64_t)a+w<=seed+4) return (uint8_t*)(uintptr_t)a;
    if(a>=0x8005a1fcu && (uint64_t)a+w<=0x8005a200u) return (uint8_t*)&g_RandomSeed+(a-0x8005a1fcu);
    if(a>=first && (uint64_t)a+w<=first+sizeof(fixture)) return (uint8_t*)(uintptr_t)a;
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u) return ram+(a&0x1fffffu);
    return NULL;
}
static int rd(void *u,uint32_t a,unsigned w,uint32_t *v)
{
    (void)u; uint8_t*p=address(a,w); if(!p)return -1;
    handler_entries+=a==0x800201f0u; rand_entries+=a==0x8003fa38u; slow_entries+=a==0x80022cacu;
    if(active_cpu && a==0x8003f738u)memcpy(retail_angles,address(active_cpu->gpr[4],6),6);
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(i*8);return 0;
}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v)
{
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(i*8));return 0;
}
static uint32_t c2read(void*u,int control,unsigned r){(void)u;return control?CFC2(r):MFC2(r);}
static void c2write(void*u,int control,unsigned r,uint32_t v){(void)u;if(control)CTC2(v,r);else MTC2(v,r);}
static int c2command(void*u,uint32_t v){(void)u;doCOP2((int)v);return 0;}

/* Opcode operands can share either output/scale bytes or the live RNG seed.
 * Coupled bytes form one physical input image; no expected C formula is used.
 * Retail stack padding varies and is excluded from equality checks. */
static void compare(unsigned byte,int16_t scale,uint16_t timer,uint32_t seed,
                    unsigned position,unsigned alias,unsigned variant)
{
    for(unsigned i=0;i<sizeof(fixture);++i)((uint8_t*)&fixture)[i]=(uint8_t)(i*37u+byte+variant*73u);
    uint8_t*p=(uint8_t*)fixture.sprite;
    const int16_t positions[][3]={{0,0,0},{1,-1,1},{-32768,32767,-32768},
                                  {32767,-32768,32767},{1234,-5678,9012}};
    for(unsigned i=0;i<3;++i)put32(p+i*4,((uint32_t)(uint16_t)positions[position][i]<<16)|(0x1357u+i));
    put16(p+0x2c,(uint16_t)scale);put16(p+0x3a,timer);g_RandomSeed=seed;
    uint8_t*ops[]={ (uint8_t*)fixture.ops,p,p+1,p+2,p+3,p+4,p+5,p+6,p+7,p+8,p+9,p+10,p+11,
                    p+0x2c,p+0x2d,p+0x3a,p+0x3b,(uint8_t*)&g_RandomSeed,
                    (uint8_t*)&g_RandomSeed+1,(uint8_t*)&g_RandomSeed+2,(uint8_t*)&g_RandomSeed+3};
    uint8_t*op=ops[alias];if(alias<17)*op=(uint8_t)byte;
    initial=fixture;uint32_t initial_seed=g_RandomSeed;
    memset(ram+0x1ffe00,(int)(variant*73u+0xa5u),0x200);
    memset(&gteRegs,0,sizeof(gteRegs));
    PcPortMipsBus bus={.read=rd,.write=wr,.cop2_read=c2read,.cop2_write=c2write,.cop2_command=c2command};
    PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    const uint32_t opcodes[]={0xc1u,0x100c1u,0xffffffc1u,0xdeadbec1u};uint32_t opcode=opcodes[variant&3];
    cpu.gpr[4]=(uint32_t)(uintptr_t)p;cpu.gpr[5]=opcode;cpu.gpr[6]=(uint32_t)(uintptr_t)op;
    cpu.gpr[29]=0x801fff00u;cpu.gpr[31]=0xfffffffcu;
    handler_entries=rand_entries=slow_entries=0;
    active_cpu=&cpu;
    int result=PcPortMipsRun(&cpu,0x8001fbe4u,0xfffffffcu,10000);
    active_cpu=NULL;
    if(result!=PC_PORT_MIPS_HALTED || handler_entries!=1 || rand_entries!=3 || slow_entries!=1){
        fprintf(stderr,"SPRITE C1 FAIL oracle case=%u pc=%08x entries=%u/%u/%u: %s\n",cases,cpu.pc,handler_entries,rand_entries,slow_entries,cpu.error);exit(2);
    }
    expected=fixture;uint32_t expected_seed=g_RandomSeed;
    read_rotation_controls(retail_matrix);
    fixture=initial;g_RandomSeed=initial_seed;memset(&gteRegs,0,sizeof(gteRegs));native_rand_calls=0;
    if(cases==0){printf("SPRITE C1 first retail case: operand=%02x scale=%d timer=%04x seed=%08x expected_xyz=%08x,%08x,%08x next_seed=%08x\n",byte,scale,timer,seed,expected.sprite[0],expected.sprite[1],expected.sprite[2],expected_seed);fflush(stdout);}
    func_8001FBE4(p,opcode,op);
    read_rotation_controls(native_matrix);
    /* These nine halfwords become live GTE rotation controls. Padding and
     * unrelated register contents are deliberately outside this comparison. */
    if(memcmp(&fixture,&expected,sizeof(fixture)) || g_RandomSeed!=expected_seed || native_rand_calls!=3 ||
       memcmp(native_matrix,retail_matrix,sizeof(native_matrix))){
        unsigned diff=0;while(diff<sizeof(fixture)&&((uint8_t*)&fixture)[diff]==((uint8_t*)&expected)[diff])++diff;
        fprintf(stderr,"SPRITE C1 FAIL case=%u byte=%02x scale=%d timer=%04x seed=%08x position=%u alias=%u variant=%u diff=%u xyz=%08x,%08x,%08x/%08x,%08x,%08x next_seed=%08x/%08x rand_calls=%u\n",cases,byte,scale,timer,seed,position,alias,variant,diff,word(p),word(p+4),word(p+8),expected.sprite[0],expected.sprite[1],expected.sprite[2],g_RandomSeed,expected_seed,native_rand_calls);
        fprintf(stderr,"SPRITE C1 matrices angles=%d,%d,%d",retail_angles[0],retail_angles[1],retail_angles[2]);
        for(unsigned i=0;i<9;++i)fprintf(stderr," m%u=%d/%d",i,native_matrix[i],retail_matrix[i]);
        fputc('\n',stderr);exit(1);
    }
    ++cases;
}
/* Derive seeds from actual retail RNG instructions. Cover every first random
 * byte and each low-12-bit X/Y angle separately; do not substitute an LCG. */
static void select_seeds(uint32_t seeds[3][4096])
{
    uint8_t seen[3][4096]={{0}};unsigned found[3]={0};
    PcPortMipsBus bus={.read=rd,.write=wr};
    for(uint32_t candidate=0;candidate<1048576u &&
        (found[0]<256 || found[1]<4096 || found[2]<4096);++candidate){
        g_RandomSeed=candidate;
        for(unsigned step=0;step<3;++step){
            PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[31]=0xfffffffcu;
            assert(PcPortMipsRun(&cpu,0x8003fa38u,0xfffffffcu,100)==PC_PORT_MIPS_HALTED);
            unsigned value=cpu.gpr[2]&(step?4095u:255u);
            if(!seen[step][value]){seen[step][value]=1;seeds[step][value]=candidate;++found[step];}
        }
    }
    assert(found[0]==256 && found[1]==4096 && found[2]==4096);
}
int main(void)
{
    assert((uintptr_t)&fixture+sizeof(fixture)<=UINT32_MAX && (uintptr_t)&g_RandomSeed+4<=UINT32_MAX);
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));
    assert(fread(ram+0x10000,1,sizeof(ram)-0x10000,f)>0x48000);assert(!fclose(f));
    compare(0x40,8192,0,1,4,0,0);
    const int16_t scales[]={0,1,-1,4095,-4095,4096,8192,32767,-32768};
    const uint16_t timers[]={0,1,1023,1024,1025,32767,32768,65535};
    const uint32_t edges[]={0,1,0xffffffffu,0x7fffffffu,0x80000000u,0x12345678u,0x80000001u,0xff0000ffu};
    for(unsigned byte=0;byte<256;++byte)
        for(unsigned scale=0;scale<9;++scale)
            for(unsigned timer=0;timer<8;++timer)
                for(unsigned seed=0;seed<8;++seed)
                    for(unsigned position=0;position<5;++position)
                        compare(byte,scales[scale],timers[timer],edges[seed],position,0,
                                (byte^scale^timer^seed^position)&3);
    static uint32_t seeds[3][4096];select_seeds(seeds);
    for(unsigned random=0;random<256;++random)
        for(unsigned byte=0;byte<256;++byte)
            for(unsigned sign=0;sign<2;++sign)
                compare(byte,sign?-4096:4096,0,seeds[0][random],(random+byte)%5,0,(random^byte^sign)&3);
    for(unsigned axis=1;axis<3;++axis)
        for(unsigned angle=0;angle<4096;++angle)
            for(unsigned sign=0;sign<2;++sign)
                compare(255,sign?-4096:4096,1024,seeds[axis][angle],angle%5,0,(angle^sign)&3);
    unsigned alias_start=cases;
    const int16_t alias_scales[]={1,-1,8192,-32768};
    const uint16_t alias_timers[]={0,1,1024,65535};
    for(unsigned alias=1;alias<21;++alias)
        for(unsigned byte=0;byte<256;++byte)
            for(unsigned seed=0;seed<8;++seed)
                for(unsigned variant=0;variant<4;++variant)
                    compare(byte,alias_scales[variant],alias_timers[variant],edges[seed],
                            (byte+seed+variant)%5,alias,variant);
    assert(cases==1048577 && cases-alias_start==163840);
    printf("SPRITE C1 PASS %u cases: all operand bytes, signed scale/position and timer boundaries, "
           "all random-byte/operand pairs and each 4096-angle axis cycle, %u output/scale/timer/seed "
           "aliases, whole fixture and RNG state with exactly 3 calls, nine rotation halfwords\n",cases,cases-alias_start);
    puts("SPRITE C1 scope: actual retail/native dispatcher and RNG/math helpers; shared PsyCross GTE "
         "backend, paired angles from real RNG sequences, varied retail stack padding; "
         "no hardware-GTE, exhaustive 32-bit seeds, padding/full register-state or rendering claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
