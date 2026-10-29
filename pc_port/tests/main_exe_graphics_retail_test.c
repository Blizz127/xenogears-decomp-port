/* Retail differential for the hand-written graphics.c arms:
 *   GfxLoadImageAccelerated (0x80022A0C)  GfxLoadClutsAccelerated (0x80022A70)
 * The production graphics.c TU is linked unmodified.  Retail = pinned
 * disc/SLUS_006.64 on PcPortMipsRun (test oracle only).  The boundary is
 * LoadImage: a recording spy on both sides compares the RECT contents
 * (x,y,w,h read at call time) and the data pointer.  HeapAlloc(0x2000,1) and
 * HeapFree(block) are recording spies on both sides and part of the compared
 * callee trace (order + args), and are checked to be balanced.  Port-only
 * (DIVERGES): retail switches $sp into the scratch block for LoadImage and
 * stores the caller's guest $sp word at block+0x1F00; the native LoadImage
 * runs on the host stack.  The spy block sits at LOW_BASE+0xE000, outside the
 * LOW_CMP window, so that one guest-stack word is not compared (it is the only
 * retail write there: LoadImage itself is bridged).  For the cluts loop
 * D_800592F0 holds a stack address (guest stack vs host stack) and is excluded
 * from the global compare; D_800592F4 (data pointer) is compared.
 * usage: main_exe_graphics_retail_test [fn-name] */
#include "slus_oracle.h"

extern void GfxLoadImageAccelerated(void);
extern void GfxLoadClutsAccelerated(void*, s32, s32);
extern void* D_800592F0;
extern void* D_800592F4;

static unsigned heap_allocs, heap_frees;
int LoadImage(void *rect, void *data)
{
    const s16 *r = rect;
    return (int)spy(1, (uint32_t)(uint16_t)r[0] | ((uint32_t)(uint16_t)r[1] << 16),
                    (uint32_t)(uint16_t)r[2] | ((uint32_t)(uint16_t)r[3] << 16), (uint32_t)(uintptr_t)data, 0, 0, 0);
}
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
uint8_t g_PsxScratchpad[4096];
void *HeapAlloc(unsigned s, unsigned f) { heap_allocs++; spy(2, s, f, 0, 0, 0, 0); return (void *)(uintptr_t)(LOW_BASE + 0xE000u); }
unsigned HeapFree(void *p) { heap_frees++; spy(3, (uint32_t)(uintptr_t)p, 0, 0, 0, 0, 0); return 0; }

static int bridge(PcPortMipsCpu *c, uint32_t t)
{
    if (t == 0x80031BDCu) { heap_allocs++; spy(2, c->gpr[4], c->gpr[5], 0, 0, 0, 0); c->gpr[2] = LOW_BASE + 0xE000u; return 1; } /* HeapAlloc */
    if (t == 0x800320E8u) { heap_frees++; spy(3, c->gpr[4], 0, 0, 0, 0, 0); c->gpr[2] = 0; return 1; }                          /* HeapFree */
    if (t == 0x80044894u) { /* LoadImage */
        const s16 *r = hp(c->gpr[4]);
        c->gpr[2] = (uint32_t)spy(1, (uint32_t)(uint16_t)r[0] | ((uint32_t)(uint16_t)r[1] << 16),
                                  (uint32_t)(uint16_t)r[2] | ((uint32_t)(uint16_t)r[3] << 16), c->gpr[5], 0, 0, 0);
        return 1;
    }
    return 0;
}

static const Glob g_gfx[] = { { "D_800592F4", &D_800592F4, 0x800592F4u, 8, 1 } };

static void setup_image(uint32_t id)
{
    (void)id;
    for (unsigned i = 0; i < 0x400; i++) low[i] = (u8)rnd();
    static s16 rect[4];
    for (int i = 0; i < 4; i++) rect[i] = (s16)rnd();
    memcpy(low + 0x100, rect, 8);
    D_800592F0 = lo(0x100);
    D_800592F4 = lo(0x200);
    feed[0] = (int32_t)rnd();
}
static const Glob g_gfx2[] = { { "D_800592F4", &D_800592F4, 0x800592F4u, 8, 1 }, { "D_800592F0", &D_800592F0, 0x800592F0u, 8, 1 } };
static uint32_t nat_image(void) { GfxLoadImageAccelerated(); return 0; }

static void setup_cluts(uint32_t id)
{
    unsigned n = rn(5);
    (void)id;
    for (unsigned i = 0; i < 0x1000; i++) low[i] = (u8)rnd();
    put32(lo(0x1000 - 0x1000), n);
    for (unsigned i = 0; i < n; i++) {
        uint32_t off = 0x100 + 0x40 * i;
        put32(lo(4 + 4 * i), off);
        put16(lo(off), rn(256)); put16(lo(off + 2), rn(4));
    }
    A[0] = lo_addr(0); A[1] = (uint32_t)rnd_val(); A[2] = (uint32_t)rnd_val();
    D_800592F0 = NULL; D_800592F4 = NULL;
    for (int i = 0; i < 8; i++) feed[i] = (int32_t)rnd();
}
static uint32_t nat_cluts(void) { GfxLoadClutsAccelerated(lo(0), (s32)A[1], (s32)A[2]); return 0; }

static const Fn fns[] = {
    { "GfxLoadImageAccelerated", 0x80022A0Cu, 0, setup_image, nat_image },
    { "GfxLoadClutsAccelerated", 0x80022A70u, 0, setup_cluts, nat_cluts },
};

int main(int argc, char **argv)
{
    const char *only = argc > 1 ? argv[1] : NULL;
    unsigned cases = getenv("XENO_CASES") ? (unsigned)atoi(getenv("XENO_CASES")) : 3000;
    int bad = 0, ran = 0;
    cur_what = "MAIN EXE GRAPHICS";
    setvbuf(stdout, NULL, _IOLBF, 0);
    map_low();
    load_slus(cur_what);
    user_bridge = bridge;
    for (unsigned k = 0; k < sizeof(fns) / sizeof(fns[0]); k++) {
        const Fn *f = &fns[k];
        unsigned fails = 0;
        if (only && strcmp(only, f->name)) continue;
        /* image: both pointers are low-region fixtures and compared; cluts: D_800592F0 is a stack address
         * (guest vs host stack), so only the data pointer D_800592F4 is compared */
        if (!strcmp(f->name, "GfxLoadImageAccelerated")) { globs = g_gfx2; nglobs = 2; } else { globs = g_gfx; nglobs = 1; }
        prints_left = 1;
        heap_allocs = heap_frees = 0;
        for (unsigned id = 0; id < cases; id++) fails += (unsigned)run_case(f, id + 1, 0);
        if (heap_allocs != heap_frees || heap_allocs == 0) { printf("MAIN EXE GRAPHICS %s heap pair unbalanced allocs=%u frees=%u\n", f->name, heap_allocs, heap_frees); fails++; }
        printf("MAIN EXE GRAPHICS %s %s cases=%u diverged=%u (heap pairs both sides=%u)\n", f->name, fails ? "DIVERGES" : "PASS", cases, fails, heap_allocs);
        ran++; bad += fails ? 1 : 0;
    }
    if (!ran) { fprintf(stderr, "no such function\n"); return 2; }
    printf("MAIN EXE GRAPHICS %s (%d function(s) diverge)\n", bad ? "FAIL" : "ALL PASS", bad);
    return bad ? 1 : 0;
}
