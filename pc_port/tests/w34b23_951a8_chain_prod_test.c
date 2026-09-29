/*
 * Focused production-linked oracle for the retail world movement chain
 *   0x80093FE4 (attribute nibble wrapper)
 *   0x80094088 (slide-vector selector)
 *   0x800951A8 (movement resolver)
 *
 * Expected values are hand-derived from the retail disassembly and the
 * retail direction table extracted from disc/world_map.bin (verified
 * against the loaded image in section A).  The test's reference dot
 * product uses 64-bit arithmetic truncated to the low word -- an
 * independent formulation of the retail mult/mflo semantics.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93e8c.h"
#include "world_map_helper_93fe4.h"
#include "world_map_helper_94088.h"
#include "world_map_helper_951a8.h"

#define OVERLAY_BASE   0x8006FAF0u
#define OVERLAY_SPAN   180416u
#define TABLE_ADDR     0x8009B264u
#define SRC_VEC        0x800A2000u
#define DST_VEC        0x800A2100u
#define BASE_VEC       0x800A2200u
#define CANARY         0xC5C5C5C5u

static int s_failures;

/* ------------------------------------------------------------------ */
/* Assertion helpers                                                   */
/* ------------------------------------------------------------------ */

static void check_s32(const char* name, s32 got, s32 expected)
{
    if (got != expected) {
        fprintf(stderr,
                "ASSERTION %s: got=%d (0x%08x) expected=%d (0x%08x)\n",
                name, got, (u32)got, expected, (u32)expected);
        s_failures++;
    }
}

static void check_u32(const char* name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

/* ------------------------------------------------------------------ */
/* Guest memory helpers                                                */
/* ------------------------------------------------------------------ */

static u32 mem_read_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void mem_write_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static void mem_write_s32(u32 addr, s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    mem_write_u32(addr, b);
}

static s32 mem_read_s32(u32 addr)
{
    u32 b = mem_read_u32(addr);
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

/* ------------------------------------------------------------------ */
/* Test seams                                                          */
/* ------------------------------------------------------------------ */

/* 93FE4 -> 93E8C: forced value or forward to the real helper. */
static int s_force_93e8c;
static s32 s_forced_93e8c_value;
static u32 s_93e8c_calls;
static u32 s_93e8c_last_arg;

s32 wm_93fe4_test_93e8c(u32 vec_addr)
{
    s_93e8c_calls++;
    s_93e8c_last_arg = vec_addr;
    if (s_force_93e8c)
        return s_forced_93e8c_value;
    return wm_80093E8C(vec_addr);
}

/* 94088 -> 93FE4: forced nibble or forward to the real helper. */
static int s_force_93fe4;
static s32 s_forced_93fe4_value;
static u32 s_93fe4_calls;
static u32 s_93fe4_last_arg;

s32 wm_94088_test_93fe4(u32 vec_addr)
{
    s_93fe4_calls++;
    s_93fe4_last_arg = vec_addr;
    if (s_force_93fe4)
        return s_forced_93fe4_value;
    return wm_80093FE4(vec_addr);
}

/* 94088 memory trace. */
typedef struct MemEvent {
    u32 address;
    u32 value;
} MemEvent;

static MemEvent s_88_loads[16];
static u32 s_88_load_count;
static MemEvent s_88_stores[16];
static u32 s_88_store_count;

void wm_94088_test_load(u32 address, u32 value)
{
    if (s_88_load_count < 16u) {
        s_88_loads[s_88_load_count].address = address;
        s_88_loads[s_88_load_count].value = value;
    }
    s_88_load_count++;
}

void wm_94088_test_store(u32 address, u32 value)
{
    if (s_88_store_count < 16u) {
        s_88_stores[s_88_store_count].address = address;
        s_88_stores[s_88_store_count].value = value;
    }
    s_88_store_count++;
}

/* 951A8 -> 94A5C seam (always forced; 94A5C is separately certified). */
static s32 s_forced_94a5c_value;
static u32 s_94a5c_calls;
static u32 s_94a5c_base;
static u32 s_94a5c_dir;
static s32 s_94a5c_scale;
static s32 s_94a5c_mode;

s32 wm_951a8_test_94a5c(u32 base_vec, u32 dir_vec, s32 scale, s32 mode)
{
    s_94a5c_calls++;
    s_94a5c_base = base_vec;
    s_94a5c_dir = dir_vec;
    s_94a5c_scale = scale;
    s_94a5c_mode = mode;
    return s_forced_94a5c_value;
}

/* 951A8 -> 94088 seam: forced result or forward to the real helper. */
static int s_force_88;
static s32 s_forced_88_value;
static u32 s_88_seam_calls;
static u32 s_88_seam_attr;
static u32 s_88_seam_src;
static u32 s_88_seam_dst;

s32 wm_951a8_test_94088(u32 attr_vec, u32 src_vec, u32 dst_vec)
{
    s_88_seam_calls++;
    s_88_seam_attr = attr_vec;
    s_88_seam_src = src_vec;
    s_88_seam_dst = dst_vec;
    if (s_force_88)
        return s_forced_88_value;
    return wm_80094088(attr_vec, src_vec, dst_vec);
}

/* 951A8 memory trace. */
static MemEvent s_a8_stores[16];
static u32 s_a8_store_count;

void wm_951a8_test_load(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

void wm_951a8_test_store(u32 address, u32 value)
{
    if (s_a8_store_count < 16u) {
        s_a8_stores[s_a8_store_count].address = address;
        s_a8_stores[s_a8_store_count].value = value;
    }
    s_a8_store_count++;
}

static void reset_traces(void)
{
    s_force_93e8c = 0;
    s_force_93fe4 = 0;
    s_force_88 = 0;
    s_93e8c_calls = 0;
    s_93fe4_calls = 0;
    s_94a5c_calls = 0;
    s_88_seam_calls = 0;
    s_88_load_count = 0;
    s_88_store_count = 0;
    s_a8_store_count = 0;
}

/* ------------------------------------------------------------------ */
/* Retail fixture                                                      */
/* ------------------------------------------------------------------ */

static void load_overlay_image(void)
{
    FILE* f = fopen("disc/world_map.bin", "rb");
    static u8 buf[OVERLAY_SPAN];

    if (f == NULL) {
        fprintf(stderr, "ASSERTION fixture: disc/world_map.bin missing\n");
        exit(1);
    }
    if (fread(buf, 1, OVERLAY_SPAN, f) != OVERLAY_SPAN) {
        fprintf(stderr, "ASSERTION fixture: short read\n");
        exit(1);
    }
    fclose(f);
    memcpy(PSX_ADDR(OVERLAY_BASE), buf, OVERLAY_SPAN);
}

/* Retail direction table, hand-transcribed from the world_map.bin dump
 * (16 entries, 16-byte stride, X at +0 / Z at +8).  Section A verifies
 * the loaded image agrees. */
static const s32 k_tab_x[16] = {
    -1832, 1832, 1832, -1832, -3664, 3664, 3664, -3664,
    -2896, 2896, 0, 4096, -2896, 2896, 2896, -2896,
};
static const s32 k_tab_z[16] = {
    3664, 3664, 3664, 3664, 1832, 1832, 1832, 1832,
    2896, 2896, 4096, 0, 2896, 2896, 2896, 2896,
};

/* Independent low-word dot product: 64-bit products truncated to the
 * low 32 bits, summed with 32-bit wraparound. */
static u32 ref_dot_lo(s32 x, s32 nx, s32 z, s32 nz)
{
    u32 px = (u32)(((s64)x * (s64)nx) & 0xFFFFFFFFll);
    u32 pz = (u32)(((s64)z * (s64)nz) & 0xFFFFFFFFll);

    return px + pz;
}

/* ------------------------------------------------------------------ */
/* Section A: retail table verification                                */
/* ------------------------------------------------------------------ */

static void section_a(void)
{
    int i;
    char name[64];

    for (i = 0; i < 16; i++) {
        snprintf(name, sizeof(name), "A.tab[%d].x", i);
        check_s32(name, mem_read_s32(TABLE_ADDR + (u32)i * 16u), k_tab_x[i]);
        snprintf(name, sizeof(name), "A.tab[%d].y", i);
        check_s32(name, mem_read_s32(TABLE_ADDR + (u32)i * 16u + 4u), 0);
        snprintf(name, sizeof(name), "A.tab[%d].z", i);
        check_s32(name, mem_read_s32(TABLE_ADDR + (u32)i * 16u + 8u), k_tab_z[i]);
    }
    printf("A retail table verified (16 entries)\n");
}

/* ------------------------------------------------------------------ */
/* Section B: 93FE4 focused                                            */
/* ------------------------------------------------------------------ */

static void section_b(void)
{
    static const s32 forced[] = { 0, 1, 7, 8, 15, 16, 0x123, -1, 0x7FF0, -32768 };
    static const s32 expect[] = { 0, 1, 7, 8, 15, 0, 3, 15, 0, 0 };
    u32 i;

    for (i = 0; i < sizeof(forced) / sizeof(forced[0]); i++) {
        char name[64];

        reset_traces();
        s_force_93e8c = 1;
        s_forced_93e8c_value = forced[i];
        snprintf(name, sizeof(name), "B.mask[%u]", i);
        check_s32(name, wm_80093FE4(0x800A3000u), expect[i]);
        check_u32("B.callcount", s_93e8c_calls, 1u);
        check_u32("B.arg", s_93e8c_last_arg, 0x800A3000u);
        printf("B forced=%d -> %d\n", forced[i], expect[i]);
    }
}

/* ------------------------------------------------------------------ */
/* Section C: 94088 focused                                            */
/* ------------------------------------------------------------------ */

static void run_94088(s32 nibble, s32 sx, s32 sz,
                      s32 want_ret, s32 want_dx, s32 want_dz,
                      const char* tag)
{
    char name[96];
    s32 got;

    reset_traces();
    s_force_93fe4 = 1;
    s_forced_93fe4_value = nibble;
    mem_write_s32(SRC_VEC + 0u, sx);
    mem_write_u32(SRC_VEC + 4u, 0u);
    mem_write_s32(SRC_VEC + 8u, sz);
    mem_write_u32(DST_VEC + 0u, CANARY);
    mem_write_u32(DST_VEC + 4u, CANARY);
    mem_write_u32(DST_VEC + 8u, CANARY);

    got = wm_80094088(0x800A3000u, SRC_VEC, DST_VEC);

    snprintf(name, sizeof(name), "C.%s.ret", tag);
    check_s32(name, got, want_ret);
    snprintf(name, sizeof(name), "C.%s.dx", tag);
    check_s32(name, mem_read_s32(DST_VEC + 0u), want_dx);
    snprintf(name, sizeof(name), "C.%s.dz", tag);
    check_s32(name, mem_read_s32(DST_VEC + 8u), want_dz);
    snprintf(name, sizeof(name), "C.%s.dy_untouched", tag);
    check_u32(name, mem_read_u32(DST_VEC + 4u), CANARY);
    snprintf(name, sizeof(name), "C.%s.fe4_arg", tag);
    check_u32(name, s_93fe4_last_arg, 0x800A3000u);

    /* Load-address audit: first table read must hit
     * 0x8009B264 + 16*nibble (retail cell). */
    if (s_88_load_count >= 2u) {
        snprintf(name, sizeof(name), "C.%s.tabx_addr", tag);
        check_u32(name, s_88_loads[1].address,
                  TABLE_ADDR + (u32)nibble * 16u);
    } else {
        fprintf(stderr, "ASSERTION C.%s.loads: count=%u\n", tag,
                s_88_load_count);
        s_failures++;
    }
    printf("C %s n=%d src=(%d,%d) -> ret=%d dst=(%d,%d)\n", tag, nibble,
           sx, sz, want_ret, want_dx, want_dz);
}

static void section_c(void)
{
    int n;

    /* Directed cases (hand-derived expectations). */
    run_94088(0, 4096, 0, 1, 1832, -3664, "neg_dot_n0");
    run_94088(11, 4096, 0, 1, 4096, 0, "pos_dot_n11");
    run_94088(11, -4096, 0, 1, -4096, 0, "neg_dot_n11");
    run_94088(10, 12345, 0, 0, 12345, 0, "zero_dot_n10");
    run_94088(11, 0, 555, 0, 0, 555, "zero_dot_n11");
    /* Low-word wraparound: 0x40000000 * 0x1000 has a zero low word, so
     * retail takes the dot==0 copy path despite the huge algebraic dot. */
    run_94088(11, 0x40000000, 0, 0, 0x40000000, 0, "wrap_zero_n11");
    run_94088(0, 1832, 3664, 1, -1832, 3664, "pos_dot_n0");

    /* Full nibble sweep with src=(1,1); expectations from the reference
     * dot recomputed independently against the hand-transcribed table. */
    for (n = 0; n < 16; n++) {
        u32 dot = ref_dot_lo(1, k_tab_x[n], 1, k_tab_z[n]);
        s32 sdot;
        char tag[32];

        memcpy(&sdot, &dot, sizeof(sdot));
        snprintf(tag, sizeof(tag), "sweep_n%d", n);
        if (sdot == 0)
            run_94088((s32)n, 1, 1, 0, 1, 1, tag);
        else if (sdot < 0)
            run_94088((s32)n, 1, 1, 1, -k_tab_x[n], -k_tab_z[n], tag);
        else
            run_94088((s32)n, 1, 1, 1, k_tab_x[n], k_tab_z[n], tag);
    }
}

/* ------------------------------------------------------------------ */
/* Section D: 951A8 focused                                            */
/* ------------------------------------------------------------------ */

static void seed_out_canary(void)
{
    mem_write_u32(DST_VEC + 0u, CANARY);
    mem_write_u32(DST_VEC + 4u, CANARY);
    mem_write_u32(DST_VEC + 8u, CANARY);
}

static void section_d(void)
{
    s32 got;

    /* v == 0: blocked -> return 1, out untouched, no 94088 call. */
    reset_traces();
    s_forced_94a5c_value = 0;
    seed_out_canary();
    got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 0x1000, 5);
    check_s32("D.v0.ret", got, 1);
    check_u32("D.v0.calls88", s_88_seam_calls, 0u);
    check_u32("D.v0.out0", mem_read_u32(DST_VEC + 0u), CANARY);
    check_u32("D.v0.out4", mem_read_u32(DST_VEC + 4u), CANARY);
    check_u32("D.v0.out8", mem_read_u32(DST_VEC + 8u), CANARY);
    check_u32("D.v0.a5c_base", s_94a5c_base, BASE_VEC);
    check_u32("D.v0.a5c_dir", s_94a5c_dir, SRC_VEC);
    check_s32("D.v0.a5c_scale", s_94a5c_scale, 0x1000);
    check_s32("D.v0.a5c_mode", s_94a5c_mode, 5);
    printf("D v=0 ok\n");

    /* Mode sign extension: lh semantics. */
    reset_traces();
    s_forced_94a5c_value = 0;
    (void)wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 7, 0x00018000);
    check_s32("D.mode.signext", s_94a5c_mode, -32768);
    reset_traces();
    s_forced_94a5c_value = 0;
    (void)wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 7, 0x00012345);
    check_s32("D.mode.lowhalf", s_94a5c_mode, 0x2345);
    printf("D mode lh semantics ok\n");

    /* v == 1, slide selector returns 1: out left to the selector. */
    reset_traces();
    s_forced_94a5c_value = 1;
    s_force_88 = 1;
    s_forced_88_value = 1;
    seed_out_canary();
    got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 0x800, 1);
    check_s32("D.v1r1.ret", got, 0);
    check_u32("D.v1r1.calls88", s_88_seam_calls, 1u);
    check_u32("D.v1r1.attr", s_88_seam_attr, 0x1F800000u);
    check_u32("D.v1r1.src", s_88_seam_src, SRC_VEC);
    check_u32("D.v1r1.dst", s_88_seam_dst, DST_VEC);
    check_u32("D.v1r1.out0", mem_read_u32(DST_VEC + 0u), CANARY);
    check_u32("D.v1r1.out4", mem_read_u32(DST_VEC + 4u), CANARY);
    check_u32("D.v1r1.out8", mem_read_u32(DST_VEC + 8u), CANARY);
    check_u32("D.v1r1.stores", s_a8_store_count, 0u);
    printf("D v=1 r=1 ok\n");

    /* v == 1, slide selector returns 0: out zeroed, order +8 +4 +0. */
    reset_traces();
    s_forced_94a5c_value = 1;
    s_force_88 = 1;
    s_forced_88_value = 0;
    seed_out_canary();
    got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 0x800, 1);
    check_s32("D.v1r0.ret", got, 0);
    check_u32("D.v1r0.out0", mem_read_u32(DST_VEC + 0u), 0u);
    check_u32("D.v1r0.out4", mem_read_u32(DST_VEC + 4u), 0u);
    check_u32("D.v1r0.out8", mem_read_u32(DST_VEC + 8u), 0u);
    check_u32("D.v1r0.storecount", s_a8_store_count, 3u);
    check_u32("D.v1r0.store0", s_a8_stores[0].address, DST_VEC + 8u);
    check_u32("D.v1r0.store1", s_a8_stores[1].address, DST_VEC + 4u);
    check_u32("D.v1r0.store2", s_a8_stores[2].address, DST_VEC + 0u);
    printf("D v=1 r=0 ok\n");

    /* v == 2: X block; sign(dir.X) selects the sign, bgez includes 0. */
    {
        static const s32 xs[] = { 0, 1, -1, 0x7FFFFFFF, (s32)0x80000000 };
        static const s32 want[] = { 0x1000, 0x1000, -0x1000, 0x1000, -0x1000 };
        u32 i;

        for (i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
            char name[64];

            reset_traces();
            s_forced_94a5c_value = 2;
            mem_write_s32(SRC_VEC + 0u, xs[i]);
            mem_write_s32(SRC_VEC + 8u, 999);
            seed_out_canary();
            got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 1, 1);
            snprintf(name, sizeof(name), "D.v2[%u].ret", i);
            check_s32(name, got, 0);
            snprintf(name, sizeof(name), "D.v2[%u].out0", i);
            check_s32(name, mem_read_s32(DST_VEC + 0u), want[i]);
            snprintf(name, sizeof(name), "D.v2[%u].out4", i);
            check_u32(name, mem_read_u32(DST_VEC + 4u), 0u);
            snprintf(name, sizeof(name), "D.v2[%u].out8", i);
            check_u32(name, mem_read_u32(DST_VEC + 8u), 0u);
            snprintf(name, sizeof(name), "D.v2[%u].order", i);
            check_u32(name, s_a8_store_count, 3u);
            check_u32("D.v2.o0", s_a8_stores[0].address, DST_VEC + 0u);
            check_u32("D.v2.o1", s_a8_stores[1].address, DST_VEC + 8u);
            check_u32("D.v2.o2", s_a8_stores[2].address, DST_VEC + 4u);
        }
        printf("D v=2 ok\n");
    }

    /* v == 3: Z block; reads dir.Z, store order +4 +0 +8. */
    {
        static const s32 zs[] = { 0, 7, -7, (s32)0x80000000 };
        static const s32 want[] = { 0x1000, 0x1000, -0x1000, -0x1000 };
        u32 i;

        for (i = 0; i < sizeof(zs) / sizeof(zs[0]); i++) {
            char name[64];

            reset_traces();
            s_forced_94a5c_value = 3;
            mem_write_s32(SRC_VEC + 0u, -5);
            mem_write_s32(SRC_VEC + 8u, zs[i]);
            seed_out_canary();
            got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 1, 1);
            snprintf(name, sizeof(name), "D.v3[%u].ret", i);
            check_s32(name, got, 0);
            snprintf(name, sizeof(name), "D.v3[%u].out8", i);
            check_s32(name, mem_read_s32(DST_VEC + 8u), want[i]);
            snprintf(name, sizeof(name), "D.v3[%u].out0", i);
            check_u32(name, mem_read_u32(DST_VEC + 0u), 0u);
            snprintf(name, sizeof(name), "D.v3[%u].out4", i);
            check_u32(name, mem_read_u32(DST_VEC + 4u), 0u);
            snprintf(name, sizeof(name), "D.v3[%u].order", i);
            check_u32(name, s_a8_store_count, 3u);
            check_u32("D.v3.o0", s_a8_stores[0].address, DST_VEC + 4u);
            check_u32("D.v3.o1", s_a8_stores[1].address, DST_VEC + 0u);
            check_u32("D.v3.o2", s_a8_stores[2].address, DST_VEC + 8u);
        }
        printf("D v=3 ok\n");
    }
}

/* ------------------------------------------------------------------ */
/* Section E: chain integration (real 93FE4 + 94088, retail table)     */
/* ------------------------------------------------------------------ */

static void section_e(void)
{
    s32 got;

    /* v == 1 routed through the REAL wm_80094088 -> wm_80093FE4 chain,
     * with only 93E8C forced (its own decode is separately certified).
     * Forced nibble 11 (tab = (4096, 0)); dir = (-4096, 0) gives a
     * negative dot, so out must receive (-4096, ?, 0). */
    reset_traces();
    s_forced_94a5c_value = 1;
    s_force_88 = 0;          /* forward to real 94088 */
    s_force_93fe4 = 0;       /* forward to real 93FE4 */
    s_force_93e8c = 1;
    s_forced_93e8c_value = 0x7B; /* & 0xF = 11 */
    mem_write_s32(SRC_VEC + 0u, -4096);
    mem_write_u32(SRC_VEC + 4u, 0u);
    mem_write_s32(SRC_VEC + 8u, 0);
    seed_out_canary();
    got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 0x1000, 2);
    check_s32("E.ret", got, 0);
    check_s32("E.out0", mem_read_s32(DST_VEC + 0u), -4096);
    check_u32("E.out4", mem_read_u32(DST_VEC + 4u), CANARY);
    check_s32("E.out8", mem_read_s32(DST_VEC + 8u), 0);
    check_u32("E.e8c_arg", s_93e8c_last_arg, 0x1F800000u);
    check_u32("E.e8c_calls", s_93e8c_calls, 1u);

    /* Same chain, dot == 0: nibble 10 (tab (0,4096)), dir (77,0):
     * the selector copies, then 951A8 zeroes everything. */
    reset_traces();
    s_forced_94a5c_value = 1;
    s_force_93e8c = 1;
    s_forced_93e8c_value = 10;
    mem_write_s32(SRC_VEC + 0u, 77);
    mem_write_s32(SRC_VEC + 8u, 0);
    seed_out_canary();
    got = wm_800951A8(BASE_VEC, SRC_VEC, DST_VEC, 0x1000, 2);
    check_s32("E.zero.ret", got, 0);
    check_u32("E.zero.out0", mem_read_u32(DST_VEC + 0u), 0u);
    check_u32("E.zero.out4", mem_read_u32(DST_VEC + 4u), 0u);
    check_u32("E.zero.out8", mem_read_u32(DST_VEC + 8u), 0u);
    printf("E chain integration ok\n");
}

/* ------------------------------------------------------------------ */

int main(void)
{
    PsxMemory_Init();
    load_overlay_image();

    section_a();
    section_b();
    section_c();
    section_d();
    section_e();

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B23 0x800951A8 chain focused oracle PASS\n");
    return 0;
}
