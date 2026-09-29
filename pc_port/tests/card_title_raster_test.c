/* Retail instruction oracle; font lookup, allocator and GPU calls intercepted. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
extern void bzero(void *, size_t);
SystemMenu *g_Menu;
u8 D_801EA8C0;
static u8 ram[0x200000], pixels[288], output[4128], glyph[32];
#if defined(REAL_KROM) || defined(NO_BIOS_KROM)
#include "krom_rom.h"
#endif
#ifdef REAL_KROM
static u8 bios[0x80000];
#endif
#if defined(REAL_KROM) || defined(NO_BIOS_KROM)
u16 D_801EA5D0[128];
#include "convert.inc"
#endif
static unsigned allocs, frees, calls, gcalls, uploads, syncs, reject, wide;
static unsigned offsets[32];
static u8 *input;
static u8 *ptr(u32 a,unsigned n) {a&=0x1fffffff;assert((uint64_t)a+n<=sizeof ram);return ram+a;}
static int read_bus(void *o,u32 a,unsigned n,u32 *v) {
    (void)o;const u8 *p;
#ifdef REAL_KROM
    u32 physical=a&0x1fffffff;
    if(physical>=0x1fc00000 && (uint64_t)physical+n<=0x1fc80000)p=bios+physical-0x1fc00000;
    else
#endif
    p=ptr(a,n);
    *v=0;for(unsigned i=0;i<n;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int write_bus(void *o,u32 a,unsigned n,u32 v) {
    (void)o;u8*p=ptr(a,n);for(unsigned i=0;i<n;++i)p[i]=v>>(8*i);return 0;
}
static unsigned is_wide(unsigned i) {return wide==1 || (wide==2 && (i&1));}
static unsigned rejected(unsigned i) {return reject==1 || (reject==2 && (i%3==1));}
static void *HeapAlloc(unsigned size,unsigned flags) {
    assert(flags==1 && allocs<2 && size==(allocs?4096:256));
    return allocs++?output+16:pixels+16;
}
static void HeapFree(void *p) {assert(frees<2 && p==(frees?output+16:pixels+16));++frees;}
#if !defined(REAL_KROM) && !defined(NO_BIOS_KROM)
static void *func_801E65E4(u8 *p) {
    assert(calls<gcalls && (unsigned)(p-input)==offsets[calls]);
    D_801EA8C0=is_wide(calls);
    return rejected(calls++)?(void*)(intptr_t)-1:glyph;
}
#endif
int LoadImage(RECT *r,u_long *p) {
    assert(uploads++==0 && r->x==320 && r->y==224 && r->w==64 && r->h==32);
    assert((void*)p==output+16 && memcmp(p,ram+0x120010,4096)==0);return 0;
}
int DrawSync(int mode) {assert(mode==0 && uploads==1 && syncs++==0);return 0;}
#include "compress.inc"
#include "raster.inc"
static unsigned ga,gf,gu,gs,guest_input;
static int bridge(void *o,PcPortMipsCpu *c,u32 target) {
    (void)o;u32 a=c->gpr[4],b=c->gpr[5];
    switch(target) {
    case 0x80031bdc:
        assert(ga<2 && a==(ga?4096:256) && b==1);
        c->gpr[2]=ga++?0x80120010:0x80110010;return 1;
    case 0x8003f8e8: assert(a==0x80120010 && b==4096);memset(ptr(a,b),0,b);return 1;
    case 0x801e65e4:
        assert(gcalls<32);offsets[gcalls]=a-guest_input;
#ifdef REAL_KROM
        ++gcalls;return 0; /* Execute the retail converter, not a test double. */
    case 0x800405c4: {
        PcPortMipsBus bus={.read=read_bus,.write=write_bus};PcPortMipsCpu bios_cpu;
        PcPortMipsCpuInit(&bios_cpu,&bus);bios_cpu.gpr[4]=a;
        bios_cpu.gpr[29]=0x801a0000;bios_cpu.gpr[31]=0xfffffffc;
        /* Fixtures use selector-independent codes, proven by the full census. */
        assert(PcPortMipsRun(&bios_cpu,0x65e0,0xfffffffc,10000)==PC_PORT_MIPS_HALTED);
        c->gpr[2]=bios_cpu.gpr[2];return 1;
    }
#elif defined(NO_BIOS_KROM)
        {
            const u8 *font = func_801E65E4(ptr(a, 2));
            ++gcalls;
            ram[0x1ea8c0] = D_801EA8C0;
            if (font == (const u8 *)(intptr_t)-1) c->gpr[2] = UINT32_MAX;
            else {memcpy(ram + 0x140000, font, 32);c->gpr[2] = 0x80140000;}
            return 1;
        }
#else
        ram[0x1ea8c0]=is_wide(gcalls);
        c->gpr[2]=rejected(gcalls++)?UINT32_MAX:0x80140000;return 1;
#endif
    case 0x80044894:
        assert(gu++==0 && b==0x80120010);
        {const u8 rect[]={0x40,1,0xe0,0,0x40,0,0x20,0};assert(memcmp(ptr(a,8),rect,8)==0);}return 1;
    case 0x800445d0: assert(a==0 && gu==1 && gs++==0);return 1;
    case 0x800320e8: assert(gf<2 && a==(gf?0x80120010:0x80110010));++gf;return 1;
    default:return 0;
    }
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
#ifdef REAL_KROM
    f=fopen("disc/scph5500.bin","rb");assert(f);
    assert(fread(bios,1,sizeof bios,f)==sizeof bios);fclose(f);
    memcpy(ram,bios+0xfb00,0x10000);
    memcpy(D_801EA5D0,ram+0x1ea5d0,sizeof D_801EA5D0);
#elif defined(NO_BIOS_KROM)
    D_801EA5D0['A'] = 0x8141; D_801EA5D0['B'] = 0x8142; D_801EA5D0['C'] = 0x8143;
    memcpy(ram+0x1ea5d0,D_801EA5D0,sizeof D_801EA5D0);
#endif
    SystemMenu menu={0};MenuUnk2 card;memset(&card,0,sizeof card);g_Menu=&menu;menu.unk32C=&card;
    u32 v=0x80080000;memcpy(ram+0x625a0,&v,4);v=0x80090000;memcpy(ram+0x8032c,&v,4);
    const unsigned lengths[]={0,1,15,16,17,31,32,33};unsigned cases=0;
    for(unsigned screen=0;screen<32;++screen)for(unsigned l=0;l<8;++l)
    for(wide=0;wide<3;++wide)for(reject=0;reject<3;++reject)for(unsigned pattern=0;pattern<3;++pattern) {
#if defined(REAL_KROM) || defined(NO_BIOS_KROM)
        if(reject)continue; /* Rejection schedules are covered by the mock mode. */
#endif
        input=card.unkB94+4+screen*512;guest_input=0x80090b98+screen*512;
        memset(input,0x53,66);unsigned end=0;
        for(unsigned i=0;i<lengths[l];++i) {
#if defined(REAL_KROM) || defined(NO_BIOS_KROM)
            const u16 codes[]={0x8140,0x889f,0x9872};
            if(is_wide(i)){input[end]=codes[pattern]>>8;input[end+1]=codes[pattern];}
            else input[end]='A'+pattern;
#endif
            end+=1+is_wide(i);
        }
        input[end]=0;memcpy(ptr(guest_input,66),input,66);
        for(unsigned i=0;i<32;++i)glyph[i]=pattern==0?0:pattern==1?255:(u8)(i*71+l*13);
        memcpy(ram+0x140000,glyph,32);
        memset(pixels,0xa5,sizeof pixels);memset(output,0xa5,sizeof output);
        memcpy(ram+0x110000,pixels,sizeof pixels);memcpy(ram+0x120000,output,sizeof output);
        allocs=frees=calls=gcalls=uploads=syncs=ga=gf=gu=gs=0;
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=screen;cpu.gpr[29]=0x801b0000;cpu.gpr[31]=0xfffffffc;
        assert(PcPortMipsRun(&cpu,0x801e6668,0xfffffffc,1000000)==PC_PORT_MIPS_HALTED);
        func_801E6668(screen);
        assert(ga==2 && gf==2 && gu==1 && gs==1 && allocs==2 && frees==2 && uploads==1 && syncs==1);
        assert(gcalls==(lengths[l]<32?lengths[l]:32));
#if !defined(REAL_KROM) && !defined(NO_BIOS_KROM)
        assert(calls==gcalls);
#endif
        assert(memcmp(pixels,ram+0x110000,sizeof pixels)==0);
        assert(memcmp(output,ram+0x120000,sizeof output)==0);++cases;
    }
#ifdef REAL_KROM
    printf("PASS %u BIOS-backed retail/native title raster fixtures; allocator/GPU intercepted\n",cases);
#elif defined(NO_BIOS_KROM)
    printf("PASS %u no-BIOS Jiskan-backed retail/native title raster fixtures; allocator/GPU intercepted\n",cases);
#else
    printf("PASS %u retail/native title raster fixtures; provider/GPU intercepted\n",cases);
#endif
}
