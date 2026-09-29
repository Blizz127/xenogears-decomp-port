/*
 * W34B4C PRODUCTION-LINKED test for terrain/position initializer
 * 0x80097BC0–0x80097CB4.
 *
 * This test links against the ACTUAL production object compiled from
 * pc_port/src/world_map_terrain_init.c.  It does NOT contain a copied
 * initializer implementation.  The one authoritative definition of
 * wm_80097BC0 comes from the production module.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b4c_prod_test.c \
 *     pc_port/src/world_map_terrain_init.c \
 *     -o pc_port/build_native/w34b4c_prod_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_terrain_init.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* ---- Stub tracking for called helpers ---- */
static int s_stub_800981C8_calls;
static int s_stub_80097DC0_calls;
static u32  s_stub_800981C8_arg;

void wm_800981C8(u32 pos_ptr) {
    s_stub_800981C8_calls++;
    s_stub_800981C8_arg = pos_ptr;
}

void wm_80097DC0(void) {
    s_stub_80097DC0_calls++;
}

static void reset_stubs(void) {
    s_stub_800981C8_calls = 0;
    s_stub_80097DC0_calls = 0;
    s_stub_800981C8_arg = 0;
}

/* ---- Retail layout constants ---- */

/* Identity-like source matrix at 0x8009A180 (32 bytes). */
#define WM_IDENTITY_MATRIX_SRC  0x8009A180u
#define WM_MATRIX_SIZE          32

/* Terrain matrix destination (D534). */
#define WM_D534_ABS             0x8009D534u

/* Retail clear is 256 descending words C580,C57C,...,C184. Its byte extent
 * is the half-open interval [C184,C584). */
#define WM_C580_ABS             0x8009C580u
#define WM_CLEAR_LOW_ABS        0x8009C184u
#define WM_CLEAR_BYTE_COUNT     1024

/* Terrain period (C618). */
#define WM_C618_ABS             0x8009C618u

/* Terrain active flag (BBB8). */
#define WM_BBB8_ABS             0x8009BBB8u

/* Terrain sub-flag (C5BC). */
#define WM_C5BC_ABS             0x8009C5BCu

/* Masked position destinations (outside the retail clear extent). */
#define WM_BBB4_ABS             0x8009BBB4u
#define WM_BBBC_ABS             0x8009BBBCu

/* Terrain cell index halfwords. */
#define WM_C838_ABS             0x8009C838u
#define WM_C83A_ABS             0x8009C83Au
#define WM_C83C_ABS             0x8009C83Cu

/* Retail input position pointer (caller passes 0x8009C5AC). C5AC is above
 * the highest cleared word C580 and is preserved by the retail loop. */
#define WM_C5AC_ABS             0x8009C5ACu

/* Test input address — outside the retail clear extent for non-zero tests. */
#define WM_TEST_INPUT           0x8009C000u

/* Position mask. */
#define WM_POS_MASK             0x007FFFFFu

/* Neighboring globals — must NOT be touched.
 * These are all outside the retail clear extent. */
#define WM_BCDC_ABS             0x8009BCDCu
#define WM_D7CC_ABS             0x8009D7CCu
#define WM_OT_PTR0              0x8009BC38u
#define WM_OT_PTR1              0x8009BCB0u
#define WM_BE3C                 0x8009BE3Cu
#define WM_POOL_BE24            0x8009BE24u
#define WM_CONV_TABLE_A_BASE    0x80099E8Cu
#define WM_CONV_TABLE_B_BASE    0x8009A034u

/* Framebuffer environment records — must NOT be touched. */
#define WM_ENVREC_BASE          0x8009BBC8u
#define WM_ENVREC_STRIDE        0x78u

#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))
#define WM_U16(a) (*(uint16_t*)PSX_ADDR(a))
#define WM_U8(a)  (*(uint8_t*)PSX_ADDR(a))

/* Test harness. */
static int total = 0, pass = 0, fail = 0;
static void check(const char* name, int cond) {
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else { fail++; printf("  FAIL: %s\n", name); }
}

static void reset_state(void) {
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    wm_tpi_reset();
    reset_stubs();
}

/* Write the retail identity-like matrix to 0x8009A180 in PSX memory. */
static void write_identity_matrix(void) {
    static const u16 identity[16] = {
        0x1000, 0, 0, 0,
        0x1000, 0, 0, 0,
        0x1000, 0, 0, 0,
        0, 0, 0, 0
    };
    memcpy(PSX_ADDR(WM_IDENTITY_MATRIX_SRC), identity, WM_MATRIX_SIZE);
}

/* Write a test position vector at a guest address outside [C184,C584). */
static void write_test_position(u32 addr, u32 x, u32 y, u32 z) {
    WM_U32(addr + 0) = x;
    WM_U32(addr + 4) = y;
    WM_U32(addr + 8) = z;
}

/* Compute FNV-1a hash of a region for before/after comparison. */
static uint32_t hash_region(uint32_t addr, size_t len) {
    uint32_t h = 0x811c9dc5;
    uint8_t* p = (uint8_t*)PSX_ADDR(addr);
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x01000193;
    }
    return h;
}

int main(void)
{
    uint32_t hash_before, hash_after;

    printf("=== W34B4C PRODUCTION-LINKED Test ===\n");
    printf("    (links against production wm_80097BC0)\n\n");

    /* ================================================================
     * 1. Zeroed state — clean initialization (retail C5AC input = 0)
     * ================================================================ */
    printf("--- Clean zeroed state ---\n");
    reset_state();
    write_identity_matrix();
    wm_80097BC0(WM_C5AC_ABS);

    check("C618 == 1024", WM_U32(WM_C618_ABS) == 1024);
    check("BBB8 == 0", WM_U32(WM_BBB8_ABS) == 0);
    check("C5BC == 0", WM_U32(WM_C5BC_ABS) == 0);
    check("C838 == 2 (u16)", WM_U16(WM_C838_ABS) == 2);
    check("C83A == 0 (u16)", WM_U16(WM_C83A_ABS) == 0);
    check("C83C == 2 (u16)", WM_U16(WM_C83C_ABS) == 2);
    check("BBB4 == 0 (masked X from zero input)",
          WM_U32(WM_BBB4_ABS) == 0);
    check("BBBC == 0 (masked Z from zero input)",
          WM_U32(WM_BBBC_ABS) == 0);

    /* D534 matrix: must contain copied identity values. */
    check("D534[0] == 0x1000 (identity matrix)",
          WM_U16(WM_D534_ABS) == 0x1000);
    check("D534[1] == 0",
          WM_U16(WM_D534_ABS + 2) == 0);
    check("D534[4] == 0x1000",
          WM_U16(WM_D534_ABS + 8) == 0x1000);
    check("D534[8] == 0x1000",
          WM_U16(WM_D534_ABS + 16) == 0x1000);
    check("D534[12] == 0",
          WM_U16(WM_D534_ABS + 24) == 0);

    /* Representative words from the retail descending clear extent. */
    check("clear C580 == 0", WM_U32(0x8009C580u) == 0);
    check("clear C500 == 0", WM_U32(0x8009C500u) == 0);
    check("clear C400 == 0", WM_U32(0x8009C400u) == 0);
    check("clear C280 == 0", WM_U32(0x8009C280u) == 0);
    check("clear C184 == 0", WM_U32(0x8009C184u) == 0);

    /* Helpers must have been called. */
    check("wm_800981C8 called once", s_stub_800981C8_calls == 1);
    check("wm_80097DC0 called once", s_stub_80097DC0_calls == 1);
    check("wm_800981C8 received pos_ptr",
          s_stub_800981C8_arg == WM_C5AC_ABS);

    /* ================================================================
     * 2. Dirty/sentinel pre-state — verify overwrite
     * ================================================================ */
    printf("--- Dirty pre-state ---\n");
    reset_state();
    write_identity_matrix();

    memset(PSX_ADDR(WM_D534_ABS), 0xCC, WM_MATRIX_SIZE);
    memset(PSX_ADDR(WM_CLEAR_LOW_ABS), 0xDD, WM_CLEAR_BYTE_COUNT);
    WM_U32(WM_C618_ABS) = 0xDEADBEEF;
    WM_U32(WM_BBB8_ABS) = 0x11111111;
    WM_U32(WM_C5BC_ABS) = 0x22222222;
    WM_U32(WM_BBB4_ABS) = 0x33333333;
    WM_U32(WM_BBBC_ABS) = 0x44444444;
    WM_U16(WM_C838_ABS) = 99;
    WM_U16(WM_C83A_ABS) = 88;
    WM_U16(WM_C83C_ABS) = 77;

    wm_80097BC0(WM_C5AC_ABS);

    check("dirty C618 overwritten to 1024", WM_U32(WM_C618_ABS) == 1024);
    check("dirty BBB8 overwritten to 0", WM_U32(WM_BBB8_ABS) == 0);
    check("dirty C5BC overwritten to 0", WM_U32(WM_C5BC_ABS) == 0);
    check("dirty BBB4 overwritten (masked X=0)", WM_U32(WM_BBB4_ABS) == 0);
    check("dirty BBBC overwritten (masked Z=0)", WM_U32(WM_BBBC_ABS) == 0);
    check("dirty C838 overwritten to 2", WM_U16(WM_C838_ABS) == 2);
    check("dirty C83A overwritten to 0", WM_U16(WM_C83A_ABS) == 0);
    check("dirty C83C overwritten to 2", WM_U16(WM_C83C_ABS) == 2);
    check("dirty D534[0] overwritten to 0x1000",
          WM_U16(WM_D534_ABS) == 0x1000);
    check("dirty retail clear extent overwritten to zero",
          WM_U32(WM_C580_ABS) == 0 && WM_U32(WM_CLEAR_LOW_ABS) == 0 &&
          WM_U32(0x8009C400u) == 0);

    /* ================================================================
     * 3. Natural Lahan-like inputs (positive position)
     *    Uses WM_TEST_INPUT outside the retail clear extent.
     * ================================================================ */
    printf("--- Natural Lahan-like inputs ---\n");
    reset_state();
    write_identity_matrix();

    /* X=0x00100000, Y=0, Z=0x00200000. */
    write_test_position(WM_TEST_INPUT, 0x00100000, 0, 0x00200000);

    wm_80097BC0(WM_TEST_INPUT);

    check("Lahan X masked: BBB4 == 0x00100000",
          WM_U32(WM_BBB4_ABS) == (0x00100000 & WM_POS_MASK));
    check("Lahan Z masked: BBBC == 0x00200000",
          WM_U32(WM_BBBC_ABS) == (0x00200000 & WM_POS_MASK));
    check("Lahan C618 == 1024", WM_U32(WM_C618_ABS) == 1024);

    /* ================================================================
     * 4. Alternate positive position
     * ================================================================ */
    printf("--- Alternate positive position ---\n");
    reset_state();
    write_identity_matrix();

    /* X=0x003FFFFF, Z=0x005ABCDE.
     * After mask: X=0x003FFFFF, Z=0x001ABCDE. */
    write_test_position(WM_TEST_INPUT, 0x003FFFFF, 0, 0x005ABCDE);

    wm_80097BC0(WM_TEST_INPUT);

    check("alt X masked: BBB4 == 0x003FFFFF",
          WM_U32(WM_BBB4_ABS) == (0x003FFFFF & WM_POS_MASK));
    check("alt Z masked: BBBC == 0x001ABCDE",
          WM_U32(WM_BBBC_ABS) == (0x005ABCDE & WM_POS_MASK));

    /* ================================================================
     * 5. Negative/signed position — mask preserves lower 23 bits
     * ================================================================ */
    printf("--- Negative/signed position ---\n");
    reset_state();
    write_identity_matrix();

    /* X=0xFF000000, Z=0x80000001.
     * After mask: X=0, Z=1. */
    write_test_position(WM_TEST_INPUT, 0xFF000000, 0, 0x80000001);

    wm_80097BC0(WM_TEST_INPUT);

    check("negative X masked: BBB4 == 0",
          WM_U32(WM_BBB4_ABS) == (0xFF000000 & WM_POS_MASK));
    check("negative Z masked: BBBC == 1",
          WM_U32(WM_BBBC_ABS) == (0x80000001 & WM_POS_MASK));

    /* ================================================================
     * 6. Exact C618 output verification
     * ================================================================ */
    printf("--- C618 exact output ---\n");
    reset_state();
    write_identity_matrix();
    WM_U32(WM_C618_ABS) = 0xFFFFFFFF;
    wm_80097BC0(WM_C5AC_ABS);
    check("C618 exactly 1024 after dirty", WM_U32(WM_C618_ABS) == 1024);

    /* ================================================================
     * 7. Exact D534 output verification (full 32-byte copy)
     * ================================================================ */
    printf("--- D534 exact output ---\n");
    reset_state();
    write_identity_matrix();
    wm_80097BC0(WM_C5AC_ABS);

    check("D534 word 0 == 0x00001000", WM_U32(WM_D534_ABS + 0) == 0x00001000);
    check("D534 word 1 == 0x00000000", WM_U32(WM_D534_ABS + 4) == 0x00000000);
    check("D534 word 2 == 0x00001000", WM_U32(WM_D534_ABS + 8) == 0x00001000);
    check("D534 word 3 == 0x00000000", WM_U32(WM_D534_ABS + 12) == 0x00000000);
    check("D534 word 4 == 0x00001000", WM_U32(WM_D534_ABS + 16) == 0x00001000);
    check("D534 word 5 == 0x00000000", WM_U32(WM_D534_ABS + 20) == 0x00000000);
    check("D534 word 6 == 0x00000000", WM_U32(WM_D534_ABS + 24) == 0x00000000);
    check("D534 word 7 == 0x00000000", WM_U32(WM_D534_ABS + 28) == 0x00000000);

    /* ================================================================
     * 8. BE28/BE2C/BE30 behavior — NOT modified by this function
     * ================================================================ */
    printf("--- BE28/BE2C/BE30 unchanged ---\n");
    reset_state();
    write_identity_matrix();

    WM_U32(0x8009BE28u) = 0x12345678;
    WM_U32(0x8009BE2Cu) = 0x9ABCDEF0;
    WM_U32(0x8009BE30u) = 0x11223344;

    wm_80097BC0(WM_C5AC_ABS);

    check("BE28 unchanged", WM_U32(0x8009BE28u) == 0x12345678);
    check("BE2C unchanged", WM_U32(0x8009BE2Cu) == 0x9ABCDEF0);
    check("BE30 unchanged", WM_U32(0x8009BE30u) == 0x11223344);

    /* ================================================================
     * 9. Every other retail write verified
     * ================================================================ */
    printf("--- All retail writes ---\n");
    reset_state();
    write_identity_matrix();
    write_test_position(WM_TEST_INPUT, 0x00ABCDEF, 0, 0x00123456);
    wm_80097BC0(WM_TEST_INPUT);

    check("BBB8 == 0 (terrain active flag)", WM_U32(WM_BBB8_ABS) == 0);
    check("C5BC == 0 (terrain sub-flag)", WM_U32(WM_C5BC_ABS) == 0);
    check("C618 == 1024 (terrain period)", WM_U32(WM_C618_ABS) == 1024);
    check("BBB4 == masked X",
          WM_U32(WM_BBB4_ABS) == (0x00ABCDEF & WM_POS_MASK));
    check("BBBC == masked Z",
          WM_U32(WM_BBBC_ABS) == (0x00123456 & WM_POS_MASK));
    check("C838 == 2 (cell index X)", WM_U16(WM_C838_ABS) == 2);
    check("C83A == 0 (cell index zero)", WM_U16(WM_C83A_ABS) == 0);
    check("C83C == 2 (cell index Z)", WM_U16(WM_C83C_ABS) == 2);

    /* ================================================================
     * 10. Neighboring memory guards (all outside retail clear extent)
     * ================================================================ */
    printf("--- Memory guards ---\n");
    reset_state();
    write_identity_matrix();

    WM_U32(WM_BCDC_ABS) = 0xAAAAAAAA;
    WM_U32(WM_D7CC_ABS) = 0xBBBBBBBB;
    WM_U32(WM_OT_PTR0) = 0x005A2048;
    WM_U32(WM_OT_PTR1) = 0x005A3050;
    WM_U32(WM_BE3C) = 0x12345678;
    WM_U32(WM_POOL_BE24) = 0xAABBCCDD;
    WM_U32(WM_CONV_TABLE_A_BASE) = 666;
    WM_U32(WM_CONV_TABLE_B_BASE) = 555;

    wm_80097BC0(WM_C5AC_ABS);

    check("BCDC unchanged", WM_U32(WM_BCDC_ABS) == 0xAAAAAAAA);
    check("D7CC unchanged", WM_U32(WM_D7CC_ABS) == 0xBBBBBBBB);
    check("OT ptr0 unchanged", WM_U32(WM_OT_PTR0) == 0x005A2048);
    check("OT ptr1 unchanged", WM_U32(WM_OT_PTR1) == 0x005A3050);
    check("BE3C unchanged", WM_U32(WM_BE3C) == 0x12345678);
    check("pool pointer unchanged", WM_U32(WM_POOL_BE24) == 0xAABBCCDD);
    check("Table A unchanged", WM_U32(WM_CONV_TABLE_A_BASE) == 666);
    check("Table B unchanged", WM_U32(WM_CONV_TABLE_B_BASE) == 555);

    /* ================================================================
     * 11. Framebuffer environment unchanged
     * ================================================================ */
    printf("--- Framebuffer environment unchanged ---\n");
    reset_state();
    write_identity_matrix();

    memset(PSX_ADDR(WM_ENVREC_BASE), 0xCC, WM_ENVREC_STRIDE * 2);

    hash_before = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);
    wm_80097BC0(WM_C5AC_ABS);
    hash_after = hash_region(WM_ENVREC_BASE, WM_ENVREC_STRIDE * 2);

    check("framebuffer hash unchanged", hash_before == hash_after);

    /* ================================================================
     * 12. BCDC unchanged
     * ================================================================ */
    printf("--- BCDC unchanged ---\n");
    reset_state();
    write_identity_matrix();
    WM_U32(WM_BCDC_ABS) = 256;
    wm_80097BC0(WM_C5AC_ABS);
    check("BCDC still 256", WM_U32(WM_BCDC_ABS) == 256);

    /* ================================================================
     * 13. OT roots unchanged
     * ================================================================ */
    printf("--- OT roots unchanged ---\n");
    reset_state();
    write_identity_matrix();
    WM_U32(WM_OT_PTR0) = 0x005A2048;
    WM_U32(WM_OT_PTR1) = 0x005A3050;
    wm_80097BC0(WM_C5AC_ABS);
    check("OT root 0 preserved", WM_U32(WM_OT_PTR0) == 0x005A2048);
    check("OT root 1 preserved", WM_U32(WM_OT_PTR1) == 0x005A3050);

    /* ================================================================
     * 14. Callback pool unchanged
     * ================================================================ */
    printf("--- Callback pool unchanged ---\n");
    reset_state();
    write_identity_matrix();
    WM_U32(WM_POOL_BE24) = 0xAABBCCDD;
    wm_80097BC0(WM_C5AC_ABS);
    check("callback pool preserved", WM_U32(WM_POOL_BE24) == 0xAABBCCDD);

    /* ================================================================
     * 15. Table A/B unchanged
     * ================================================================ */
    printf("--- Table A/B unchanged ---\n");
    reset_state();
    write_identity_matrix();
    WM_U32(WM_CONV_TABLE_A_BASE) = 0x1111;
    WM_U32(WM_CONV_TABLE_B_BASE) = 0x2222;
    wm_80097BC0(WM_C5AC_ABS);
    check("Table A preserved", WM_U32(WM_CONV_TABLE_A_BASE) == 0x1111);
    check("Table B preserved", WM_U32(WM_CONV_TABLE_B_BASE) == 0x2222);

    /* ================================================================
     * 16. Clear-boundary and store-width guards
     * ================================================================ */
    printf("--- Store-width guards ---\n");
    reset_state();
    write_identity_matrix();

    WM_U32(0x8009C180u) = 0xA1B2C3D4u;
    WM_U32(0x8009C584u) = 0x55667788u;

    wm_80097BC0(WM_C5AC_ABS);

    check("C180 below clear unchanged", WM_U32(0x8009C180u) == 0xA1B2C3D4u);
    check("C584 above clear unchanged", WM_U32(0x8009C584u) == 0x55667788u);
    check("C838 is u16==2", WM_U16(WM_C838_ABS) == 2);
    check("C83A is u16==0", WM_U16(WM_C83A_ABS) == 0);
    check("C83C is u16==2", WM_U16(WM_C83C_ABS) == 2);

    /* ================================================================
     * 17. Instrumentation counter
     * ================================================================ */
    printf("--- Instrumentation ---\n");
    reset_state();
    write_identity_matrix();
    check("initial call count == 0", wm_tpi_get_calls() == 0);
    wm_80097BC0(WM_C5AC_ABS);
    check("call count == 1 after one call", wm_tpi_get_calls() == 1);
    wm_tpi_reset();
    check("call count == 0 after reset", wm_tpi_get_calls() == 0);

    /* ================================================================
     * 18. Helper call tracking
     * ================================================================ */
    printf("--- Helper call tracking ---\n");
    reset_state();
    write_identity_matrix();
    wm_80097BC0(WM_C5AC_ABS);

    check("wm_800981C8 called exactly once",
          s_stub_800981C8_calls == 1);
    check("wm_80097DC0 called exactly once",
          s_stub_80097DC0_calls == 1);
    check("wm_800981C8 received correct arg",
          s_stub_800981C8_arg == WM_C5AC_ABS);

    /* ================================================================
     * 19. Idempotency
     * ================================================================ */
    printf("--- Idempotency ---\n");
    reset_state();
    write_identity_matrix();
    wm_80097BC0(WM_C5AC_ABS);
    {
        uint32_t c618_first = WM_U32(WM_C618_ABS);
        uint32_t d534_hash_first = hash_region(WM_D534_ABS, WM_MATRIX_SIZE);

        /* Second call — should produce same structural output. */
        wm_80097BC0(WM_C5AC_ABS);

        check("idempotent C618", WM_U32(WM_C618_ABS) == c618_first);
        check("idempotent D534 hash",
              hash_region(WM_D534_ABS, WM_MATRIX_SIZE) == d534_hash_first);
        check("idempotent call count == 2", wm_tpi_get_calls() == 2);
    }

    /* ================================================================
     * 20. Mask edge cases
     * ================================================================ */
    printf("--- Mask edge cases ---\n");
    reset_state();
    write_identity_matrix();

    /* Input exactly at mask boundary: 0x007FFFFF. */
    write_test_position(WM_TEST_INPUT, 0x007FFFFF, 0, 0x007FFFFF);
    wm_80097BC0(WM_TEST_INPUT);
    check("mask boundary X: BBB4 == 0x007FFFFF",
          WM_U32(WM_BBB4_ABS) == 0x007FFFFF);
    check("mask boundary Z: BBBC == 0x007FFFFF",
          WM_U32(WM_BBBC_ABS) == 0x007FFFFF);

    /* Input just above mask: 0x00800000 → masked to 0. */
    reset_state();
    write_identity_matrix();
    write_test_position(WM_TEST_INPUT, 0x00800000, 0, 0x00800000);
    wm_80097BC0(WM_TEST_INPUT);
    check("above mask X: BBB4 == 0",
          WM_U32(WM_BBB4_ABS) == 0);
    check("above mask Z: BBBC == 0",
          WM_U32(WM_BBBC_ABS) == 0);

    /* ================================================================
     * Summary
     * ================================================================ */
    printf("\n=== Results: %d/%d passed", pass, total);
    if (fail > 0)
        printf(", %d FAILED", fail);
    printf(" ===\n");

    return fail > 0 ? 1 : 0;
}
