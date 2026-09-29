#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern void f2_leaf_impl(void *);
static unsigned native_leaf_entries;
static uint8_t native_leaf_entry[0xc0];
void func_8001F6B0(void *p) { ++native_leaf_entries; memcpy(native_leaf_entry, p, sizeof(native_leaf_entry)); f2_leaf_impl(p); }

#define GUARD(name) void name(void) { fputs("SPRITE F2 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL) GUARD(func_8001CE74)
GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68) GUARD(func_80022D44) GUARD(func_80023B84) GUARD(func_80022CAC)
GUARD(func_80023124) GUARD(func_80023290) GUARD(func_8001FB30) GUARD(func_800245D8)
GUARD(func_8002C3E8) GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10)
uint32_t D_80018644, D_800592E4, D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int32_t D_80059198; uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA; int g_cfg_pgxpTextureCorrection;
extern uint32_t g_RandomSeed; extern int __real_rand(void);
int __wrap_rand(void) { return __real_rand(); }

static uint8_t ram[0x200000];
static struct { uint8_t pre[0x200], sprite[0xc0], ops[4], base[0x80], header[0x40], table_a[0x40], table_b[0x40], prim[0x18 * 64]; } fixture, initial, expected;
static unsigned cases, handler_entries, clamp_entries, leaf_entries, b2_entries;
static uint32_t clamp_args[3][2], b2_args[6];
static uint8_t leaf_entry[0xc0];
static unsigned native_b2_entries;
static uint32_t native_b2_args[6];
void func_800B2AEC(void *model, void *buffer0, void *buffer1, int32_t red, int32_t green, int32_t blue)
{
    ++native_b2_entries;
    native_b2_args[0] = (uint32_t)(uintptr_t)model;
    native_b2_args[1] = (uint32_t)(uintptr_t)buffer0;
    native_b2_args[2] = (uint32_t)(uintptr_t)buffer1;
    native_b2_args[3] = (uint32_t)red; native_b2_args[4] = (uint32_t)green; native_b2_args[5] = (uint32_t)blue;
}

static uint8_t *address(uint32_t a, unsigned w) {
    uintptr_t first=(uintptr_t)&fixture;
    if(a>=first && (uint64_t)a+w<=first+sizeof(fixture)) return (uint8_t *)(uintptr_t)a;
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u) return ram+(a&0x1fffffu);
    return NULL;
}
static int rd(void *u,uint32_t a,unsigned w,uint32_t *v) {
    (void)u; uint8_t*p=address(a,w); if(!p)return -1; handler_entries+=a==0x80020fc8u;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(i*8);return 0;
}
static int wr(void *u,uint32_t a,unsigned w,uint32_t v) {
    (void)u;uint8_t*p=address(a,w);if(!p)return -1;for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(i*8));return 0;
}
static int bridge(void *u,PcPortMipsCpu*c,uint32_t target) {
    (void)u;
    if(target==0x80021ad8u){
        if(clamp_entries<3){clamp_args[clamp_entries][0]=c->gpr[4];clamp_args[clamp_entries][1]=c->gpr[5];}
        ++clamp_entries;return 0;
    }
    if(target==0x8001f6b0u){uint8_t*p=address(c->gpr[4],sizeof(leaf_entry));if(!p)return -1;++leaf_entries;memcpy(leaf_entry,p,sizeof(leaf_entry));return 0;}
    if(target==0x800b2aecu){
        uint8_t*p=address(c->gpr[29]+0x10,8);if(!p)return -1;
        ++b2_entries;b2_args[0]=c->gpr[4];b2_args[1]=c->gpr[5];b2_args[2]=c->gpr[6];b2_args[3]=c->gpr[7];
        b2_args[4]=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
        p=address(c->gpr[29]+0x14,4);b2_args[5]=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return 1;
    }
    return 0;
}
static void put32(uint8_t*p,uint32_t v){memcpy(p,&v,4);} static void put16(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static uint32_t get32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}

static void compare(unsigned vector, unsigned mode, unsigned type, unsigned bit1,
                    unsigned header, unsigned base_alias, unsigned source_alias) {
    static const uint8_t deltas[][3]={{0,0,0},{1,1,1},{0x7f,0x80,0xff},{0x80,0x81,0x01},{2,0xfe,3},{0xff,4,0xfc},{0x10,0xf0,0x7f},{0x55,0xaa,0x33},{1,0,0},{4,0,0}};
    static const uint32_t colors[]={0,0x000000ffu,0x0000ff00u,0x00ff0000u,0x00808080u,0x00ffffffu,0x007f0180u,0x00fe027fu};
    for(unsigned i=0;i<sizeof(fixture);++i)((uint8_t*)&fixture)[i]=(uint8_t)(i*37u+vector*19u+0x53u);
    uint8_t*p=fixture.sprite;
    unsigned edge=vector<10;
    unsigned scalar=vector-10;
    uint8_t d0=edge?deltas[vector][0]:(uint8_t)scalar;
    uint8_t d1=edge?deltas[vector][1]:(uint8_t)(scalar*37u);
    uint8_t d2=edge?deltas[vector][2]:(uint8_t)(scalar*91u);
    put32(p+0x28,colors[vector&7]);put32(p+0x3c,(mode&3u)|0xa0u);put32(p+0x40,((type&15u)<<13)|(bit1?2u:0u));
    uint8_t *base=base_alias==0?fixture.base:(base_alias==1?p:(base_alias==2?p+4:p-0x18));
    put32(p+0x20,(uint32_t)(uintptr_t)base);put32(p+0x24,(uint32_t)(uintptr_t)fixture.base);
    put32(fixture.base+0x2c,(uint32_t)(uintptr_t)fixture.table_a);put32(fixture.base+0x30,(uint32_t)(uintptr_t)fixture.prim);put32(fixture.base+0x34,header?(uint32_t)(uintptr_t)fixture.header:0);
    put32(p+0x2c,(uint32_t)(uintptr_t)fixture.table_a);put32(p+0x30,(uint32_t)(uintptr_t)fixture.prim);put32(p+0x34,(uint32_t)(uintptr_t)fixture.header);put32(p+0x38,header?(uint32_t)(uintptr_t)fixture.header:0);
    put16(fixture.base+0x38,0x1234);put16(fixture.base+0x3a,0x8001);put16(fixture.base+0x3c,0xfffe);
    if(base_alias==1){put32(p+0x34,header?(uint32_t)(uintptr_t)fixture.header:0);}
    if(base_alias==2){put32(p+0x34,(uint32_t)(uintptr_t)fixture.prim);}
    if(base_alias==3){put32(p+0x14,(uint32_t)(uintptr_t)fixture.table_a);put32(p+0x18,(uint32_t)(uintptr_t)fixture.prim);put32(p+0x1c,header?(uint32_t)(uintptr_t)fixture.header:0);}
    fixture.ops[0]=d0;fixture.ops[1]=d1;fixture.ops[2]=d2;
    uint8_t *source=source_alias==0?fixture.ops:(source_alias<=3?p+0x27+source_alias:(source_alias<=9?base+0x38+(source_alias-4):p+0x40));
    initial=fixture;
    handler_entries=clamp_entries=leaf_entries=b2_entries=native_b2_entries=0;memset(clamp_args,0,sizeof(clamp_args));memset(b2_args,0,sizeof(b2_args));memset(native_b2_args,0,sizeof(native_b2_args));
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
    c.gpr[4]=(uint32_t)(uintptr_t)p;c.gpr[5]=0xf2;c.gpr[6]=(uint32_t)(uintptr_t)source;c.gpr[29]=0x801fff00;c.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&c,0x8001fbe4u,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE F2 FAIL retail pc=%08x vector=%u mode=%u type=%u base=%u source=%u: %s\n",c.pc,vector,mode,type,base_alias,source_alias,c.error);exit(2);}
    uint8_t *current_base=(uint8_t *)(uintptr_t)get32(p+0x20);
    unsigned expect_leaf=(get32(p+0x3c)&3u)==1u;
    unsigned expect_b2=(((get32(p+0x40)>>13)&15u)==15u)&&get32(current_base+0x34)!=0&&!(get32(p+0x40)&2u);
    if(handler_entries!=1||clamp_entries!=3||leaf_entries!=expect_leaf||b2_entries!=expect_b2){fprintf(stderr,"SPRITE F2 FAIL oracle vector/mode/type/base/source=%u/%u/%u/%u/%u entries=%u/%u/%u/%u expected leaf/b2=%u/%u\n",vector,mode,type,base_alias,source_alias,handler_entries,clamp_entries,leaf_entries,b2_entries,expect_leaf,expect_b2);exit(2);}
    expected=fixture;fixture=initial;native_leaf_entries=0;native_b2_entries=0;memset(native_leaf_entry,0,sizeof(native_leaf_entry));func_8001FBE4(p,0xf2u,source);
    if(memcmp(&fixture,&expected,sizeof(fixture))||native_leaf_entries!=expect_leaf||native_b2_entries!=expect_b2||memcmp(native_b2_args,b2_args,sizeof(b2_args))||(expect_leaf&&memcmp(native_leaf_entry,leaf_entry,sizeof(leaf_entry)))){int diff=memcmp(&fixture,&expected,sizeof(fixture));fprintf(stderr,"SPRITE F2 FAIL case=%u vector=%u mode=%u type=%u bit1=%u header=%u base=%u source=%u diff=%d leaf=%u/%u b2=%u/%u\n",cases,vector,mode,type,bit1,header,base_alias,source_alias,diff,native_leaf_entries,expect_leaf,native_b2_entries,expect_b2);exit(1);}++cases;
}
int main(void){
    assert((uintptr_t)&fixture+sizeof(fixture)<=UINT32_MAX);FILE*f=fopen("disc/SLUS_006.64","rb");assert(f&&!fseek(f,0x800,SEEK_SET));assert(fread(ram+0x10000,1,sizeof(ram)-0x10000,f)>0x48000);assert(!fclose(f));
    if(getenv("SPRITE_F2_QUICK")){unsigned vector=getenv("SPRITE_F2_VECTOR")?strtoul(getenv("SPRITE_F2_VECTOR"),0,0):2;unsigned mode=getenv("SPRITE_F2_MODE")?strtoul(getenv("SPRITE_F2_MODE"),0,0):1;unsigned type=getenv("SPRITE_F2_TYPE")?strtoul(getenv("SPRITE_F2_TYPE"),0,0):15;unsigned bit1=getenv("SPRITE_F2_BIT1")?strtoul(getenv("SPRITE_F2_BIT1"),0,0):0;unsigned header=getenv("SPRITE_F2_HEADER")?strtoul(getenv("SPRITE_F2_HEADER"),0,0):1;unsigned alias=getenv("SPRITE_F2_ALIAS")?strtoul(getenv("SPRITE_F2_ALIAS"),0,0):0;unsigned source_alias=getenv("SPRITE_F2_SOURCE_ALIAS")?strtoul(getenv("SPRITE_F2_SOURCE_ALIAS"),0,0):0;compare(vector,mode,type,bit1,header,alias,source_alias);printf("SPRITE F2 QUICK PASS %u cases\n",cases);return 0;}
    for(unsigned vector=0;vector<266;++vector)for(unsigned mode=0;mode<4;++mode)for(unsigned type_i=0;type_i<2;++type_i)for(unsigned bit1=0;bit1<2;++bit1)for(unsigned header=0;header<2;++header)for(unsigned alias=0;alias<4;++alias)for(unsigned source_alias=0;source_alias<11;++source_alias){if(alias==3&&(source_alias!=0||(mode==2&&vector!=9)))continue;compare(vector,mode,type_i?15:0,bit1,header,alias,source_alias);}
    printf("SPRITE F2 PASS %u cases: 256-byte RGB census, signed deltas, source/base aliases, clamp/mode2 gates, and six-argument B2 observation\n",cases);
    puts("SPRITE F2 scope: actual retail/native dispatcher and clamp/leaf; B2AEC is a boundary spy and remains leaf-unverified, with no rendered-output claim.");return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
