/*
 * Focused production-linked oracle for retail world helpers
 *   0x800952B0 (reference-vector sign projector, leaf)
 *   0x80095324 (wall-tangent projector via OuterProduct12/VectorNormal)
 *
 * Expected values are hand-derived from the retail disassembly.  The
 * reference dot product is recomputed with 64-bit arithmetic truncated
 * to the low word (independent formulation of mult/mflo + addu).
 * The two libgte residents are provided by THIS file as recording,
 * test-controlled implementations (same pattern as the accepted
 * w34b21_c1b_85760 certificate); the production link uses PsyCross.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_952b0.h"
#include "world_map_helper_95324.h"

#define MOV_VEC   0x800A2000u
#define OUT_VEC   0x800A2100u
#define REF_VEC   0x800A2200u
#define NORM_VEC  0x800A2300u
#define CANARY    0xC5C5C5C5u

#define SC_UNIT   0x1F800000u
#define SC_DOWN   0x1F800010u
#define SC_RAW    0x1F800020u

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} TVec;

static int s_failures;

static void check_u32(const char *name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

static void check_s32(const char *name, s32 got, s32 expected)
{
    if (got != expected) {
        fprintf(stderr,
                "ASSERTION %s: got=%d (0x%08x) expected=%d (0x%08x)\n",
                name, got, (u32)got, expected, (u32)expected);
        s_failures++;
    }
}

static void check_ptr(const char *name, const void *got, const void *expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=%p expected=%p\n", name, got,
                expected);
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

/* Independent reference: low word of the s64 dot, 32-bit wrap sum. */
static u32 ref_dot_lo(s32 rx, s32 mx, s32 rz, s32 mz)
{
    u32 a = (u32)(((s64)rx * (s64)mx) & 0xFFFFFFFFll);
    u32 b = (u32)(((s64)rz * (s64)mz) & 0xFFFFFFFFll);

    return a + b;
}

/* ------------------------------------------------------------------ */
/* Store/load traces                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    u32 address;
    u32 value;
} MemEvent;

static MemEvent s_b0_stores[16];
static u32 s_b0_store_count;

void wm_952b0_test_load(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

void wm_952b0_test_store(u32 address, u32 value)
{
    if (s_b0_store_count < 16u) {
        s_b0_stores[s_b0_store_count].address = address;
        s_b0_stores[s_b0_store_count].value = value;
    }
    s_b0_store_count++;
}

static MemEvent s_24_stores[16];
static u32 s_24_store_count;

void wm_95324_test_load(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

void wm_95324_test_store(u32 address, u32 value)
{
    if (s_24_store_count < 16u) {
        s_24_stores[s_24_store_count].address = address;
        s_24_stores[s_24_store_count].value = value;
    }
    s_24_store_count++;
}

/* ------------------------------------------------------------------ */
/* Test-controlled libgte residents (recording)                        */
/* ------------------------------------------------------------------ */

static u32 s_gte_seq;
static u32 s_op12_seq;
static const void *s_op12_v0;
static const void *s_op12_v1;
static const void *s_op12_v2;
static TVec s_op12_v1_snapshot;
static u32 s_vn_seq;
static const void *s_vn_v0;
static const void *s_vn_v1;
static TVec s_vn_v0_snapshot;
static TVec s_forced_raw;   /* what OuterProduct12 writes to v2 */
static TVec s_forced_unit;  /* what VectorNormal writes to v1 */

void OuterProduct12(TVec *v0, TVec *v1, TVec *v2)
{
    s_op12_seq = ++s_gte_seq;
    s_op12_v0 = v0;
    s_op12_v1 = v1;
    s_op12_v2 = v2;
    s_op12_v1_snapshot = *v1;
    *v2 = s_forced_raw;
}

long VectorNormal(TVec *v0, TVec *v1)
{
    s_vn_seq = ++s_gte_seq;
    s_vn_v0 = v0;
    s_vn_v1 = v1;
    s_vn_v0_snapshot = *v0;
    *v1 = s_forced_unit;
    return 1;
}

static void reset_traces(void)
{
    s_b0_store_count = 0;
    s_24_store_count = 0;
    s_gte_seq = 0;
    s_op12_seq = 0;
    s_vn_seq = 0;
}

static void seed_out_canary(void)
{
    mem_write_u32(OUT_VEC + 0u, CANARY);
    mem_write_u32(OUT_VEC + 4u, CANARY);
    mem_write_u32(OUT_VEC + 8u, CANARY);
}

/* ------------------------------------------------------------------ */
/* Section A: 952B0 focused                                            */
/* ------------------------------------------------------------------ */

static void run_952b0(s32 mx, s32 mz, s32 rx, s32 rz,
                      s32 want_x, s32 want_z, const char *tag)
{
    char name[96];

    reset_traces();
    mem_write_s32(MOV_VEC + 0u, mx);
    mem_write_s32(MOV_VEC + 8u, mz);
    mem_write_s32(REF_VEC + 0u, rx);
    mem_write_s32(REF_VEC + 8u, rz);
    seed_out_canary();

    wm_800952B0(MOV_VEC, OUT_VEC, REF_VEC);

    snprintf(name, sizeof(name), "A.%s.x", tag);
    check_s32(name, mem_read_s32(OUT_VEC + 0u), want_x);
    snprintf(name, sizeof(name), "A.%s.y", tag);
    check_u32(name, mem_read_u32(OUT_VEC + 4u), 0u);
    snprintf(name, sizeof(name), "A.%s.z", tag);
    check_s32(name, mem_read_s32(OUT_VEC + 8u), want_z);
    snprintf(name, sizeof(name), "A.%s.storecount", tag);
    check_u32(name, s_b0_store_count, 3u);
    printf("A %s mov=(%d,%d) ref=(%d,%d) -> (%d,0,%d)\n", tag, mx, mz,
           rx, rz, want_x, want_z);
}

static void section_a(void)
{
    u32 dot;

    /* dot > 0: (4096,0)·(4096,0) = 16777216. */
    run_952b0(4096, 0, 4096, 0, 4096, 0, "pos");
    /* dot < 0: (-1,0)·(4096,0) = -4096. */
    run_952b0(-1, 0, 4096, 7, -4096, -7, "neg");
    /* dot == 0 exactly: orthogonal. */
    run_952b0(0, 55, 4096, 0, 0, 0, "zero");
    /* dot == 0 via low-word wrap: 65536 * 65536 -> low word 0 (the s64
     * dot is +2^32; retail takes the zero path). */
    dot = ref_dot_lo(65536, 0, 0, 0);
    if (dot != 0u) {
        fprintf(stderr, "ASSERTION A.wrap.ref: dot=0x%08x\n", dot);
        s_failures++;
    }
    run_952b0(65536, 0, 65536, 12345, 0, 0, "wrap_zero");
    /* Wrap changes the sign: 0x00010000 * 0x00018000 = 0x180000000 ->
     * low word 0x80000000 (negative) although the s64 dot is positive. */
    dot = ref_dot_lo(0x18000, 0, 0, 0) ; /* documentation only */
    run_952b0(0x18000, 0, 0x10000, 3, -0x10000, -3, "wrap_neg");
    /* negu wrap at INT_MIN: ref.x = 0x80000000 -> -ref.x = 0x80000000.
     * dot = lo32(0x80000000 * 1) = 0x80000000 < 0. */
    run_952b0(1, 0, (s32)0x80000000, 5, (s32)0x80000000, -5, "int_min");

    /* Store-order audits. */
    reset_traces();
    mem_write_s32(MOV_VEC + 0u, -1);
    mem_write_s32(MOV_VEC + 8u, 0);
    mem_write_s32(REF_VEC + 0u, 4096);
    mem_write_s32(REF_VEC + 8u, 7);
    seed_out_canary();
    wm_800952B0(MOV_VEC, OUT_VEC, REF_VEC);
    check_u32("A.order.neg.0", s_b0_stores[0].address, OUT_VEC + 0u);
    check_u32("A.order.neg.1", s_b0_stores[1].address, OUT_VEC + 8u);
    check_u32("A.order.neg.2", s_b0_stores[2].address, OUT_VEC + 4u);

    reset_traces();
    mem_write_s32(MOV_VEC + 0u, 0);
    mem_write_s32(MOV_VEC + 8u, 0);
    seed_out_canary();
    wm_800952B0(MOV_VEC, OUT_VEC, REF_VEC);
    check_u32("A.order.zero.0", s_b0_stores[0].address, OUT_VEC + 8u);
    check_u32("A.order.zero.1", s_b0_stores[1].address, OUT_VEC + 0u);
    check_u32("A.order.zero.2", s_b0_stores[2].address, OUT_VEC + 4u);
    printf("A store orders ok\n");
}

/* ------------------------------------------------------------------ */
/* Section B: 95324 focused                                            */
/* ------------------------------------------------------------------ */

static void run_95324(s32 mx, s32 mz, s32 ux, s32 uy, s32 uz,
                      s32 want_x, s32 want_z, const char *tag)
{
    char name[96];

    reset_traces();
    s_forced_unit.vx = ux;
    s_forced_unit.vy = uy;
    s_forced_unit.vz = uz;
    s_forced_unit.pad = 0;
    /* Distinct from unit (u32 wrap keeps INT_MIN cases defined): proves
     * the dot uses VectorNormal's output, not OuterProduct12's raw
     * result. */
    s_forced_raw.vx = (s32)((u32)ux * 3u);
    s_forced_raw.vy = (s32)((u32)uy * 3u);
    s_forced_raw.vz = (s32)((u32)uz * 3u);
    s_forced_raw.pad = 0;
    mem_write_s32(MOV_VEC + 0u, mx);
    mem_write_s32(MOV_VEC + 8u, mz);
    mem_write_s32(NORM_VEC + 0u, 111);
    mem_write_s32(NORM_VEC + 4u, 222);
    mem_write_s32(NORM_VEC + 8u, 333);
    /* Canary the down-vector slots so a skipped store is visible. */
    mem_write_u32(SC_DOWN + 0u, CANARY);
    mem_write_u32(SC_DOWN + 4u, CANARY);
    mem_write_u32(SC_DOWN + 8u, CANARY);
    seed_out_canary();

    wm_80095324(NORM_VEC, MOV_VEC, OUT_VEC);

    snprintf(name, sizeof(name), "B.%s.x", tag);
    check_s32(name, mem_read_s32(OUT_VEC + 0u), want_x);
    snprintf(name, sizeof(name), "B.%s.y", tag);
    check_u32(name, mem_read_u32(OUT_VEC + 4u), 0u);
    snprintf(name, sizeof(name), "B.%s.z", tag);
    check_s32(name, mem_read_s32(OUT_VEC + 8u), want_z);

    /* GTE contract: order, argument identity, down-vector contents. */
    snprintf(name, sizeof(name), "B.%s.op12_first", tag);
    check_u32(name, s_op12_seq, 1u);
    snprintf(name, sizeof(name), "B.%s.vn_second", tag);
    check_u32(name, s_vn_seq, 2u);
    snprintf(name, sizeof(name), "B.%s.op12_v0", tag);
    check_ptr(name, s_op12_v0, PSX_ADDR(NORM_VEC));
    snprintf(name, sizeof(name), "B.%s.op12_v1", tag);
    check_ptr(name, s_op12_v1, PSX_ADDR(SC_DOWN));
    snprintf(name, sizeof(name), "B.%s.op12_v2", tag);
    check_ptr(name, s_op12_v2, PSX_ADDR(SC_RAW));
    snprintf(name, sizeof(name), "B.%s.vn_v0", tag);
    check_ptr(name, s_vn_v0, PSX_ADDR(SC_RAW));
    snprintf(name, sizeof(name), "B.%s.vn_v1", tag);
    check_ptr(name, s_vn_v1, PSX_ADDR(SC_UNIT));
    snprintf(name, sizeof(name), "B.%s.down_x", tag);
    check_s32(name, s_op12_v1_snapshot.vx, 0);
    snprintf(name, sizeof(name), "B.%s.down_y", tag);
    check_s32(name, s_op12_v1_snapshot.vy, -0x1000);
    snprintf(name, sizeof(name), "B.%s.down_z", tag);
    check_s32(name, s_op12_v1_snapshot.vz, 0);
    snprintf(name, sizeof(name), "B.%s.vn_in", tag);
    check_s32(name, s_vn_v0_snapshot.vx, (s32)((u32)ux * 3u));
    printf("B %s mov=(%d,%d) unit=(%d,%d,%d) -> (%d,0,%d)\n", tag,
           mx, mz, ux, uy, uz, want_x, want_z);
}

static void section_b(void)
{
    /* dot > 0. */
    run_95324(4096, 0, 100, 999, 7, 100, 7, "pos");
    /* dot < 0. */
    run_95324(-4096, 0, 100, 999, 7, -100, -7, "neg");
    /* dot == 0 exactly (movement orthogonal to tangent). */
    run_95324(0, 500, 100, 999, 0, 0, 0, "zero");
    /* Low-word wrap to zero: unit.x=65536, mov.x=65536. */
    run_95324(65536, 0, 65536, 1, 0, 0, 0, "wrap_zero");
    /* negu wrap at INT_MIN. */
    run_95324(1, 0, (s32)0x80000000, 0, 9, (s32)0x80000000, -9, "int_min");
    /* Distinct y in the unit tangent proves +8 (not +4) feeds the dot:
     * with mov=(0, 1), dot = unit.z = 7 > 0. */
    run_95324(0, 1, 55, -3, 7, 55, 7, "z_from_p8");
}

/* ------------------------------------------------------------------ */

int main(void)
{
    PsxMemory_Init();

    section_a();
    section_b();

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I1 0x800952B0 + 0x80095324 focused oracle PASS\n");
    return 0;
}
