/*
 * W34C5 — tile producers certificate: wm_800981C8 (index window),
 * wm_80097DC0 (initial slot population), wm_800980D4 (position wrap /
 * paging gate). Oracles are retail loop geometry, not the port's shape.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_981c8.h"
#include "world_map_helper_97dc0.h"
#include "world_map_helper_980d4.h"

#define D160 0x8009D160u
#define D2B4 0x8009D2B4u
#define C838 0x8009C838u
#define C83C 0x8009C83Cu
#define D318 0x8009D318u
#define D570 0x8009D570u
#define C184 0x8009C184u
#define BCD8 0x8009BCD8u
#define D558 0x8009D558u
#define POS  0x8009BBB4u

static int s_failures;
#define ASSERT_MSG(cond, name, ...)                                        \
    do { if (!(cond)) { s_failures++;                                     \
        fprintf(stderr, "ASSERTION %s FAILED: ", name);                   \
        fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static u32 lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static s16 lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

/* ---- stubs for the producers' externs ---- */
static u32 s_c3d8_values[2];
static int s_c3d8_calls;
u32 func_8002C3D8(void) { return s_c3d8_values[(s_c3d8_calls++) & 1]; }

static int s_sector = 0x4400;
int ArchiveDecodeSector(int entryIndex) { (void)entryIndex; return s_sector; }
static char s_path[] = "WORLD.BIN";
char* ArchiveGetFilePath(int entryIndex) { (void)entryIndex; return s_path; }

static u32 s_heap_next = 0x800A0000u;
static int s_heap_calls;
void* HeapAlloc(u32 size, u32 flags)
{
    void* p = PSX_ADDR(s_heap_next);
    (void)flags;
    s_heap_calls++;
    s_heap_next += (size + 0xFu) & ~0xFu;
    return p;
}

typedef struct { u32 w[4]; int kind; } Rec;   /* kind: 3 = 9623C, 4 = 962B0, 8 = 96328, 5 = 965A4 */
static Rec s_rec[300];
static int s_nrec;
s32 wm_8009623C(u32 a, u32 b, u32 c)
{ Rec r = {{a, b, c, 0}, 3}; if (s_nrec < 300) s_rec[s_nrec++] = r; return 0; }
s32 wm_800962B0(u32 a, u32 b, u32 c, u32 d)
{ Rec r = {{a, b, c, d}, 4}; if (s_nrec < 300) s_rec[s_nrec++] = r; return 0; }
s32 wm_80096328(void) { Rec r = {{0,0,0,0}, 8}; if (s_nrec < 300) s_rec[s_nrec++] = r; return 0; }
s32 wm_800965A4(void) { Rec r = {{0,0,0,0}, 5}; if (s_nrec < 300) s_rec[s_nrec++] = r; return 0; }

/* ---- 981C8 reference: retail loop geometry written independently ---- */
static s32 sra(s32 v, unsigned n) { return v < 0 ? -(((-v) - 1) >> n) - 1 : v >> n; }
static void ref_window(s32 x, s32 z, s32 w, s32 h, s32 mx, s32 mz, u16 out[81])
{
    s32 xt = sra(x, 12); if (xt < 0) xt += 7; xt = sra(xt, 3);
    s32 zt = sra(z, 12); if (zt < 0) zt += 7; zt = sra(zt, 3);
    s32 tx0 = xt - ((mx + 2) << 8), tz = zt - ((mz + 2) << 8);
    if (tx0 < 0) tx0 += w << 8; else if ((w << 8) < tx0) tx0 -= w << 8;
    if (tz < 0) tz += h << 8; else if ((h << 8) < tz) tz -= h << 8;
    tx0 = sra(tx0, 8); tz = sra(tz, 8);
    for (int r = 0; r < 9; r++) {
        if (tz >= h) tz = 0;
        s32 tx = tx0;
        for (int c = 0; c < 9; c++) {
            if (tx >= w) tx = 0;
            out[r * 9 + c] = (u16)(tz * w + tx);
            tx++;
        }
        tz++;
    }
}

static void seed_world(s32 w, s32 h, s32 mx, s32 mz, s32 x, s32 z)
{
    PsxMemory_Init();
    sw(D160, (u32)w); sw(D2B4, (u32)h);
    sh(C838, (u16)mx); sh(C83C, (u16)mz);
    sw(POS, (u32)x); sw(POS + 8u, (u32)z);
    for (u32 i = 0; i < 81u; i++) sh(D570 + i * 2u, (u16)(0x7000u + i));  /* previous window */
    memset(PSX_ADDR(D570 + 162u), 0xC3, 64);                                /* canary after window */
    memset(PSX_ADDR(D318 + 162u), 0xC3, 64);
}

static void test_981c8(const char* name, s32 w, s32 h, s32 mx, s32 mz, s32 x, s32 z)
{
    u16 expect[81];
    u16 prev[81];
    seed_world(w, h, mx, mz, x, z);
    for (u32 i = 0; i < 81u; i++) prev[i] = (u16)lh(D570 + i * 2u);
    ref_window(x, z, w, h, mx, mz, expect);
    wm_800981C8(POS);
    for (u32 i = 0; i < 81u; i++) {
        u16 got = (u16)lh(D570 + i * 2u);
        ASSERT_MSG(got == expect[i], "window_value", "%s: D570[%u]=%u expected %u", name, i, got, expect[i]);
        ASSERT_MSG(got < (u16)(w * h), "window_domain", "%s: D570[%u]=%u >= %d tiles", name, i, got, w * h);
        if (got != expect[i]) break;
    }
    ASSERT_MSG(memcmp(PSX_ADDR(D318), prev, 162) == 0, "prev_copy_162", "%s: previous window not copied (162 bytes)", name);
    const u8* c = (const u8*)PSX_ADDR(D570 + 162u);
    ASSERT_MSG(c[0] == 0xC3 && c[63] == 0xC3, "window_canary", "%s: wrote past 81 entries", name);
    const u8* d = (const u8*)PSX_ADDR(D318 + 162u);
    ASSERT_MSG(d[0] == 0xC3 && d[63] == 0xC3, "copy_canary", "%s: wrote past 162-byte copy", name);
}

static int in_window(u16 tile, const u16 win[81])
{ for (int i = 0; i < 81; i++) if (win[i] == tile) return 1; return 0; }

static void test_97dc0(int ready)
{
    u16 win[81];
    seed_world(16, 16, 2, 2, 0x0C000000, 0x0E000000);   /* tx0 = 12, tz = 14: both axes wrap */
    wm_800981C8(POS);
    for (u32 i = 0; i < 81u; i++) win[i] = (u16)lh(D570 + i * 2u);
    memset(PSX_ADDR(C184 + 256u * 4u), 0xC3, 64);         /* canary after the 256-entry table */
    sw(BCD8, 7u);
    s_c3d8_values[0] = ready ? 0u : 5u; s_c3d8_values[1] = ready ? 0u : 5u; s_c3d8_calls = 0;
    s_heap_calls = 0; s_nrec = 0; s_heap_next = 0x800A0000u;

    wm_80097DC0();

    ASSERT_MSG(s_heap_calls == 81, "alloc_count", "ready=%d: %d HeapAlloc calls, expected 81", ready, s_heap_calls);
    /* Every window tile has a guest-address slot that maps back to a heap buffer. */
    int nonnull = 0;
    for (u32 t = 0; t < 256u; t++) {
        u32 v = lw(C184 + t * 4u);
        if (v) nonnull++;
        if (in_window((u16)t, win)) {
            ASSERT_MSG(v != 0u, "slot_filled", "ready=%d: window tile %u left NULL", ready, t);
            ASSERT_MSG((v & 0xFF000000u) == 0x80000000u && v >= 0x800A0000u && v < s_heap_next,
                       "slot_guest_domain", "ready=%d: C184[%u]=0x%08x is not a guest heap address", ready, t, v);
        } else {
            ASSERT_MSG(v == 0u, "slot_outside_window", "ready=%d: tile %u outside window was written 0x%08x", ready, t, v);
        }
        if (s_failures > 8) break;
    }
    ASSERT_MSG(nonnull == 81, "slot_count", "ready=%d: %d slots non-null, expected 81", ready, nonnull);
    const u8* c = (const u8*)PSX_ADDR(C184 + 256u * 4u);
    ASSERT_MSG(c[0] == 0xC3 && c[63] == 0xC3, "table_canary", "ready=%d: wrote past C184[256)", ready);

    if (ready) {
        /* Records: 9 centre tiles (rows/cols 3..5, row-major), terminator, 96328,
         * 72 remaining window tiles row-major, terminator, 96328. */
        ASSERT_MSG(s_nrec == 9 + 2 + 72 + 2, "record_count", "%d records", s_nrec);
        static const int centre[9] = {30,31,32,39,40,41,48,49,50};
        for (int i = 0; i < 9 && s_nrec >= 9; i++) {
            u16 tile = win[centre[i]];
            ASSERT_MSG(s_rec[i].kind == 3 && s_rec[i].w[0] == (u32)s_sector + tile && s_rec[i].w[1] == 0x710u
                       && s_rec[i].w[2] == lw(C184 + tile * 4u),
                       "centre_order", "record %d = kind %d (0x%x,0x%x,0x%x) expected centre tile %u",
                       i, s_rec[i].kind, s_rec[i].w[0], s_rec[i].w[1], s_rec[i].w[2], tile);
        }
        ASSERT_MSG(s_nrec > 10 && s_rec[9].kind == 3 && s_rec[9].w[0] == 0 && s_rec[10].kind == 8,
                   "centre_flush", "no (0,0,0) + 96328 after the centre pass");
        ASSERT_MSG(s_nrec == 85 && s_rec[83].kind == 3 && s_rec[83].w[0] == 0 && s_rec[84].kind == 8,
                   "final_flush", "no (0,0,0) + 96328 after the full pass");
        /* Remaining 72 are the non-centre window tiles in row-major order. */
        int k = 11;
        for (int i = 0; i < 81 && k < s_nrec; i++) {
            int is_centre = (i / 9 >= 3 && i / 9 <= 5 && i % 9 >= 3 && i % 9 <= 5);
            if (is_centre) continue;
            ASSERT_MSG(s_rec[k].w[0] == (u32)s_sector + win[i], "full_order",
                       "record %d src 0x%x expected tile %u", k, s_rec[k].w[0], win[i]);
            k++;
        }
    } else {
        ASSERT_MSG(s_nrec == 81 + 2, "path_record_count", "%d records", s_nrec);
        for (int i = 0; i < 81 && i < s_nrec; i++)
            ASSERT_MSG(s_rec[i].kind == 4 && s_rec[i].w[1] == ((u32)win[i] << 11) && s_rec[i].w[2] == 0x710u,
                       "path_records", "record %d kind %d off 0x%x expected tile %u << 11", i, s_rec[i].kind, s_rec[i].w[1], win[i]);
        ASSERT_MSG(s_nrec == 83 && s_rec[81].kind == 4 && s_rec[81].w[0] == 0 && s_rec[82].kind == 5,
                   "path_flush", "no (0,0,0,0) + 965A4 after the path pass");
    }
}

static void test_980d4_case(const char* name, s32 x, s32 z, s32 ex, s32 ez, u16 flags, s16 mx, s16 mz)
{
    PsxMemory_Init();
    sw(POS, (u32)x); sw(POS + 8u, (u32)z);
    sh(D558, 0xFFFFu); sh(C838, 0x7777u); sh(C83C, 0x7777u);
    sw(0x8009D808u, 0x12345678u);
    wm_800980D4(POS);
    ASSERT_MSG((s32)lw(POS) == ex && (s32)lw(POS + 8u) == ez, "wrap_position",
               "%s: pos=(%d,%d) expected (%d,%d)", name, (s32)lw(POS), (s32)lw(POS + 8u), ex, ez);
    ASSERT_MSG((u16)lh(D558) == flags, "d558_bits", "%s: D558=0x%04x expected 0x%04x", name, (u16)lh(D558), flags);
    ASSERT_MSG(lh(C838) == mx && lh(C83C) == mz, "map_publish", "%s: map=(%d,%d) expected (%d,%d)",
               name, lh(C838), lh(C83C), mx, mz);
    ASSERT_MSG(lw(0x8009D808u) == 0x12345678u, "d808_untouched", "%s: D808 written", name);
}

int main(void)
{
    /* 981C8: asymmetric fixtures; observed-route fixture first. */
    test_981c8("route", 16, 16, 2, 2, 0x0C000000, 0x0E000000);
    test_981c8("neg_pos", 16, 14, 2, 2, -0x00123456, -0x02345678);
    test_981c8("wide", 24, 10, 1, 5, 0x1F000000, 0x03000000);
    test_981c8("high_wrap", 16, 16, 15, 15, 0x00800000, 0x00800000);

    test_97dc0(1);
    test_97dc0(0);

    test_980d4_case("inside", 0x00100000, -0x00200000, 0x00100000, -0x00200000, 0, 2, 1);
    test_980d4_case("x_high", 0x00900000, 0x00000000, 0x00100000, 0, 8, 2, 2);
    test_980d4_case("x_low", -0x00900000, 0x00000000, -0x00100000, 0, 4, 1, 2);
    test_980d4_case("z_high", 0x00000000, 0x00A00000, 0, 0x00200000, 2, 2, 2);
    test_980d4_case("z_low", 0x00000000, -0x00A00000, 0, -0x00200000, 1, 2, 1);
    test_980d4_case("both", 0x00900000, -0x00A00000, 0x00100000, -0x00200000, 8 | 1, 2, 1);
    test_980d4_case("boundary", 0x00800000, -0x00800000, 0x00800000, -0x00800000, 0, 3, 1);

    if (s_failures) { fprintf(stderr, "W34C5 tile producers certificate: %d failure(s)\n", s_failures); return 1; }
    printf("W34C5 tile producers certificate PASS\n");
    return 0;
}
