/* Shared harness for the sprite-draw retail-differential tests (M1(b)).
 *
 * Retail side: the pinned retail SLUS instructions run on the port's MIPS
 * interpreter (PcPortMipsRun). Guest RAM is g_PsxRam (SLUS loaded at
 * 0x80010000 from disc/SLUS_006.64 at runtime only). Fixtures live in a
 * fixed low mmap region so a guest address IS the host pointer; the native
 * port body reads the same bytes through its (uintptr_t)u32 casts.
 * Both sides use the PsyCross GTE backend (cop2 + libgte); retail libgte
 * (RotMatrix/SetRotMatrix/RotTransPers4/GetTPage...) is INTERPRETED, native
 * calls the PsyCross C versions: any difference between them shows up. Only
 * Heap/LoadImage-class services are bridged to one shared recording stub.
 * This is not a PS1 hardware oracle. */
#ifndef SPRITE_DRAW_RETAIL_HARNESS_H
#define SPRITE_DRAW_RETAIL_HARNESS_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"

extern unsigned int MFC2(int), CFC2(int);
extern void MTC2(unsigned int, int), CTC2(unsigned int, int);
extern int doCOP2(int);

#define LOW_BASE 0x00E00000u
#define LOW_SIZE 0x10000u
#define GP_VALUE 0x80059170u /* runtime $gp: asm gp_rel(D_80059194)=0x24 */

static uint8_t *low;           /* == (uint8_t*)LOW_BASE */
static uint8_t low_init[LOW_SIZE], low_retail[LOW_SIZE];

/* ---- deterministic rng ---- */
static uint32_t rng_state = 1;
static uint32_t rnd(void) { rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5; return rng_state; }
static uint32_t rn(uint32_t n) { return rnd() % n; }
static int32_t rrange(int32_t lo, int32_t hi) { return lo + (int32_t)rn((uint32_t)(hi - lo + 1)); }
static void seed_case(uint32_t id) { rng_state = id * 2654435761u + 0x9e3779b9u; if (!rng_state) rng_state = 1; for (int i = 0; i < 4; i++) rnd(); }

/* ---- memory ---- */
static uint8_t *gaddr(uint32_t a, unsigned w)
{
    if (a >= LOW_BASE && (uint64_t)a + w <= (uint64_t)LOW_BASE + LOW_SIZE) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return g_PsxRam + (a & 0x1fffffu);
    return NULL;
}
static int h_rd(void *u, uint32_t a, unsigned w, uint32_t *v)
{
    (void)u; uint8_t *p = gaddr(a, w); if (!p) return -1;
    *v = 0; for (unsigned i = 0; i < w; i++) *v |= (uint32_t)p[i] << (8 * i);
    return 0;
}
static int h_wr(void *u, uint32_t a, unsigned w, uint32_t v)
{
    (void)u; uint8_t *p = gaddr(a, w); if (!p) return -1;
    for (unsigned i = 0; i < w; i++) p[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
static uint32_t g32(uint32_t a) { uint32_t v; if (h_rd(0, a, 4, &v)) abort(); return v; }
static void p32(uint32_t a, uint32_t v) { if (h_wr(0, a, 4, v)) abort(); }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static uint32_t lo_addr(uint32_t off) { return LOW_BASE + off; }

/* ---- recorded boundary services (shared by retail bridge and native stubs) ---- */
static uint32_t heap_next;
#define HEAP_BASE_OFF 0xF000u
typedef struct { uint32_t id, a[5]; } CallRec;
static CallRec calllog[256]; static unsigned ncalls;
static void note_call(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
{
    if (ncalls < 256) { calllog[ncalls] = (CallRec){id, {a0, a1, a2, a3, a4}}; }
    ncalls++;
}
static uint32_t svc_heap_alloc(uint32_t size, uint32_t arg)
{
    uint32_t r = heap_next; heap_next += (size + 7u) & ~7u; note_call(1, size, arg, 0, 0, 0); return r;
}
static void svc_note_800234AC(uint32_t sprite) { note_call(2, sprite, 0, 0, 0, 0); }
static void svc_note_800251C8(uint32_t addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h) { note_call(3, addr, x, y, w, h); }

/* Isolation switch (XENO_SDR_HOST_ROTMATRIX=1): service retail RotMatrix
 * (8003F738) with the same host PsyCross RotMatrix the native body calls, to
 * separate a body bug from the known host-vs-retail RotMatrix rounding gap. */
static int host_rotmatrix;

static int h_bridge(void *u, PcPortMipsCpu *c, uint32_t t)
{
    (void)u;
    switch (t) {
    case 0x8003f738u:
        if (!host_rotmatrix) return 0;
        RotMatrix((SVECTOR*)gaddr(c->gpr[4], 8), (MATRIX*)gaddr(c->gpr[5], 32)); c->gpr[2] = c->gpr[5]; return 1;
    case 0x80031bdcu: c->gpr[2] = svc_heap_alloc(c->gpr[4], c->gpr[5]); return 1;
    case 0x800234acu: svc_note_800234AC(c->gpr[4]); return 1;
    case 0x800251c8u: svc_note_800251C8(c->gpr[4], c->gpr[5], c->gpr[6], c->gpr[7], g32(c->gpr[29] + 16)); return 1;
    default: return 0;
    }
}
static uint32_t h_c2r(void *u, int ctl, unsigned r) { (void)u; return ctl ? CFC2((int)r) : MFC2((int)r); }
static void h_c2w(void *u, int ctl, unsigned r, uint32_t v) { (void)u; if (ctl) CTC2(v, (int)r); else MTC2(v, (int)r); }
static int h_c2c(void *u, uint32_t v) { (void)u; doCOP2((int)v); return 0; }

static uint32_t retail_v0;
static uint32_t retail_stack_args[8]; static unsigned retail_nstack; /* args 5.. at sp+16 */
static const char *(*ctx_fn)(void);
static int retail_call(uint32_t entry, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, const char *what)
{
    PcPortMipsBus bus = {.read = h_rd, .write = h_wr, .bridge = h_bridge, .cop2_read = h_c2r, .cop2_write = h_c2w, .cop2_command = h_c2c};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0; cpu.gpr[5] = a1; cpu.gpr[6] = a2; cpu.gpr[7] = a3;
    cpu.gpr[28] = GP_VALUE; cpu.gpr[29] = 0x801ff000u; cpu.gpr[31] = 0xfffffffcu;
    memset(g_PsxRam + 0x1ffe00, 0xd3, 0x200);
    for (unsigned i = 0; i < retail_nstack; i++) p32(0x801ff010u + 4 * i, retail_stack_args[i]);
    if (PcPortMipsRun(&cpu, entry, 0xfffffffcu, 200000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "%s FAIL retail oracle did not halt (%s) pc=%08x: %s\n", what, "run", cpu.pc, cpu.error);
        if (ctx_fn) fprintf(stderr, "  context: %s\n", ctx_fn());
        exit(2);
    }
    retail_v0 = cpu.gpr[2];
    return 0;
}

static void load_slus(const char *what)
{
    const char *path = getenv("XENO_SLUS"); if (!path) path = "disc/SLUS_006.64";
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s FAIL cannot open %s (set XENO_SLUS)\n", what, path); exit(2); }
    if (fseek(f, 0x800, SEEK_SET)) exit(2);
    size_t n = fread(g_PsxRam + 0x10000, 1, 0x48800, f); fclose(f);
    if (n < 0x47000) { fprintf(stderr, "%s FAIL short SLUS read %zu\n", what, n); exit(2); }
}
static void map_low(void)
{
    void *p = mmap((void *)(uintptr_t)LOW_BASE, LOW_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void *)(uintptr_t)LOW_BASE) { perror("mmap low fixture"); exit(2); }
    low = p;
}

/* ---- GTE helper ---- */
typedef struct { int16_t rot[9]; int32_t tr[3]; int32_t ofx, ofy; uint32_t h, dqa, dqb, zsf3, zsf4; } GteSetup;
static void gte_setup(const GteSetup *g)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    for (int i = 0; i < 5; i++) CTC2((uint16_t)g->rot[2 * i] | (i < 4 ? (uint32_t)(uint16_t)g->rot[2 * i + 1] << 16 : 0u), i);
    CTC2((uint32_t)g->tr[0], 5); CTC2((uint32_t)g->tr[1], 6); CTC2((uint32_t)g->tr[2], 7);
    CTC2((uint32_t)g->ofx, 24); CTC2((uint32_t)g->ofy, 25); CTC2(g->h, 26); CTC2(g->dqa, 27); CTC2(g->dqb, 28);
    CTC2(g->zsf3, 29); CTC2(g->zsf4, 30);
}

/* ---- diff reporting ---- */
static unsigned gte_only_diffs;
static unsigned prints_left = 2;
static int take_print(void) { if (!prints_left) return 0; prints_left--; return 1; }
static int diff_low(const char *what, const char *fn, unsigned id, const char *casedesc,
                    const uint8_t *native, const uint8_t *retail, const char *(*label)(uint32_t off, char *buf))
{
    for (uint32_t i = 0; i < LOW_SIZE; i++) if (native[i] != retail[i]) {
        char buf[96]; if (!take_print()) return 1;
        fprintf(stderr, "%s FAIL %s case=%u [%s] first diff low+0x%x (%s) native=%02x retail=%02x\n",
                what, fn, id, casedesc, i, label ? label(i, buf) : "?", native[i], retail[i]);
        return 1;
    }
    return 0;
}
#endif
