/* Retail differential for the SPU-RAM block allocators of
 * src/slus_006.64/system/sound.c (XENO_PC_PORT arms):
 *   SoundSpuMemoryAllocateBlock  func_800394B8  SoundSpuMemoryAllocateBlockAtAddress
 * The production sound.c TU is linked unmodified.  Retail = pinned
 * disc/SLUS_006.64 on PcPortMipsRun (test oracle only); the helper
 * SoundSpuMemoryGetFreeBlock runs as production C natively and as retail code
 * on the oracle side, so the whole chain is compared.  The twelve-entry
 * descriptor array g_SoundSpuMemoryBlocks (0xC0 bytes) is compared after
 * every call along with the return value.
 * usage: main_exe_sound_mem_retail_test [fn-name] */
#include "slus_oracle.h"

extern u32 SoundSpuMemoryAllocateBlock(s32, s32);
extern void* func_800394B8(s32);
extern u32 SoundSpuMemoryAllocateBlockAtAddress(s32, s32, s32);

u8 g_SoundSpuMemoryBlocks[0xC0] __attribute__((aligned(16)));

static const Glob g_blocks[] = { { "g_SoundSpuMemoryBlocks", g_SoundSpuMemoryBlocks, 0x8006F9FCu, 0xC0, 0 } };

/* link-only stubs (never reached) */
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
uint8_t g_PsxScratchpad[4096];

static void put_block(unsigned i, unsigned flags, unsigned unk1, int next, s32 addr, s32 size)
{
    u8 *b = g_SoundSpuMemoryBlocks + i * 16;
    b[0] = (u8)flags; b[1] = (u8)unk1; put16(b + 2, (uint32_t)next);
    put32(b + 4, (uint32_t)addr); put32(b + 8, (uint32_t)size); put32(b + 12, rnd() & 1 ? 0 : rnd());
}
/* A coherent descriptor list: block 0 reserved at 0, an acyclic next-chain of
 * n in-use blocks (ascending addresses with random gaps, occasionally
 * out of order), remaining slots free (flags 0) or stale garbage. */
static void build_blocks(uint32_t id)
{
    unsigned n = rn(12);               /* extra in-use blocks 0..11 */
    unsigned idx[12], i, j;
    s32 addr = 0x1010 + (s32)(rn(4) ? 0 : rn(0x4000));
    for (i = 0; i < 12; i++) idx[i] = i;
    for (i = 11; i > 1; i--) { j = 1 + rn(i); unsigned t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
    for (i = 0; i < 12; i++) put_block(i, 0, rn(3) ? 0 : rn(8), (int)rn(12), (s32)rnd(), (s32)rnd());
    put_block(0, 0x81, 5, 0, 0, 0x1010);
    unsigned prev = 0;
    for (i = 0; i < n && i < 11; i++) {
        unsigned b = idx[i + 1];
        s32 gap = rn(3) ? (s32)rn(0x3000) : (s32)rn(0x20000);
        s32 size = 0x10 + (s32)rn(0x4000);
        s32 a = addr + gap;
        if ((id % 11) == 5) a = (s32)rn(0x80000);      /* unsorted chain */
        if (a + size > 0x80800) a = 0x80000 - size;
        put_block(b, 0x80, rn(4) ? 0 : rn(6), 0, a, size);
        put16(g_SoundSpuMemoryBlocks + prev * 16 + 2, b);
        prev = b; addr = a + size;
    }
    /* the head: block 0's size/address follow retail (reserved at 0) */
    if (rn(8) == 0) put32(g_SoundSpuMemoryBlocks + 8, 0x1010 + rn(0x2000));
}
static void setup_alloc(uint32_t id)
{
    build_blocks(id);
    s32 size;
    switch (rn(5)) {
    case 0: size = 0x100000 + (s32)rn(0x10000); break;               /* never fits */
    case 1: size = (s32)(rn(8) * 0x400); break;
    case 2: size = 0x2000 + (s32)rn(0x20000); break;
    default: size = 0x10 + (s32)rn(0x4000); break;
    }
    A[0] = (u32)size; A[1] = rnd();
}
static uint32_t nat_alloc(void) { return SoundSpuMemoryAllocateBlock((s32)A[0], (s32)A[1]); }
static uint32_t nat_best(void) { return (uint32_t)(uintptr_t)func_800394B8((s32)A[0]); }
static void setup_at(uint32_t id)
{
    build_blocks(id);
    s32 size = rn(3) ? 0x10 + (s32)rn(0x3000) : (s32)rn(0x20000);
    s32 addr;
    switch (rn(4)) {
    case 0: addr = (s32)rn(0x1010); break;                  /* inside the reserved head */
    case 1: addr = 0x80000 - (s32)rn(0x4000); break;       /* near the end */
    default: addr = 0x1010 + (s32)rn(0x20000); break;
    }
    /* sometimes aim exactly at an existing block boundary */
    if (rn(4) == 0) { unsigned b = rn(12); addr = (s32)((u32)*(s32 *)(g_SoundSpuMemoryBlocks + b * 16 + 4) + (u32)*(s32 *)(g_SoundSpuMemoryBlocks + b * 16 + 8)); }
    A[0] = (u32)size; A[1] = (u32)addr; A[2] = rnd();
}
static uint32_t nat_at(void) { return SoundSpuMemoryAllocateBlockAtAddress((s32)A[0], (s32)A[1], (s32)A[2]); }

static const Fn fns[] = {
    { "SoundSpuMemoryAllocateBlock", 0x800393B8u, 1, setup_alloc, nat_alloc },
    { "func_800394B8", 0x800394B8u, 1, setup_alloc, nat_best },
    { "SoundSpuMemoryAllocateBlockAtAddress", 0x800395B8u, 1, setup_at, nat_at },
};

int main(int argc, char **argv)
{
    const char *only = argc > 1 ? argv[1] : NULL;
    unsigned cases = getenv("XENO_CASES") ? (unsigned)atoi(getenv("XENO_CASES")) : 4000;
    int bad = 0, ran = 0;
    cur_what = "MAIN EXE SOUND MEM";
    setvbuf(stdout, NULL, _IOLBF, 0);
    map_low();
    load_slus(cur_what);
    globs = g_blocks; nglobs = 1;
    for (unsigned k = 0; k < sizeof(fns) / sizeof(fns[0]); k++) {
        const Fn *f = &fns[k];
        unsigned fails = 0;
        if (only && strcmp(only, f->name)) continue;
        prints_left = 1;
        for (unsigned id = 0; id < cases; id++) fails += (unsigned)run_case(f, id + 1, 0);
        printf("MAIN EXE SOUND MEM %s %s cases=%u diverged=%u\n", f->name, fails ? "DIVERGES" : "PASS", cases, fails);
        ran++; bad += fails ? 1 : 0;
    }
    if (!ran) { fprintf(stderr, "no such function\n"); return 2; }
    printf("MAIN EXE SOUND MEM %s (%d function(s) diverge)\n", bad ? "FAIL" : "ALL PASS", bad);
    return bad ? 1 : 0;
}
