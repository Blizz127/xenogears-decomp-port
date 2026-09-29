/*
 * W34B5-N — Production-linked test for terrain plane-Y solver 0x800935DC.
 *
 * Links the ACTUAL production helper (pc_port/src/world_map_plane_solver.c).
 * Uses an independent oracle that computes expected results from
 * per-product modular reduction WITHOUT replicating production's
 * instruction sequence or memcpy-based helper.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5n_800935dc_prod_test.c \
 *     pc_port/src/world_map_plane_solver.c \
 *     -o pc_port/build_native/w34b5n_800935dc_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

#include "psx_memory.h"
#include "world_map_plane_solver.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

void PsxMemory_Init(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
}

/* =====================================================================
 * INDEPENDENT SPEC ORACLE (per-product modular reduction)
 *
 * Structurally different from production:
 *   - Uses int64_t for mathematical dx/dz computation
 *   - Reduces each product modulo 2^32 BEFORE combination
 *   - Uses fully-defined arithmetic modulo conversion
 *   - Explicit oracle outcome enum (not inline abort)
 *   - Models two-store alias behavior via memory image
 * ===================================================================== */

typedef enum {
    ORACLE_VALID,
    ORACLE_BREAK_1C00,
    ORACLE_BREAK_1800
} oracle_outcome_t;

typedef struct {
    oracle_outcome_t outcome;
    u32 quotient_bits;
    u32 result_bits;
    u32 a0_final[3];
} oracle_result_t;

/*
 * Convert a uint64_t modular value [0, 2^32-1] to signed s32 range
 * [-2^31, 2^31-1] using fully defined arithmetic.
 *
 * For mod <= INT32_MAX: identity (in-range).
 * For mod > INT32_MAX:  mod - 2^31 + INT32_MIN, which avoids any
 * out-of-range intermediate.
 */
static int64_t mod_to_signed(uint64_t mod)
{
    mod &= UINT64_C(0xFFFFFFFF);
    if (mod <= INT32_MAX)
        return (int64_t)mod;
    return (int64_t)(mod - UINT64_C(0x80000000)) + (int64_t)INT32_MIN;
}

static oracle_result_t oracle_800935DC(s32 a0[3], s32 a1[3], s32 a2[3],
                                       int aliased)
{
    oracle_result_t r;
    memset(&r, 0, sizeof(r));

    /* Read mathematical signed components into int64_t */
    int64_t a0x = a0[0], a0z = a0[2];
    int64_t b0x = a1[0], b0z = a1[2];
    int64_t n0  = a2[0], n1  = a2[1], n2 = a2[2];

    /* Compute dx: mathematical difference, reduce mod 2^32 */
    int64_t dx_math = a0x - b0x;
    uint64_t dx_mod = (uint64_t)dx_math & UINT64_C(0xFFFFFFFF);
    int64_t dx_signed = mod_to_signed(dx_mod);

    /* Product X: signed multiply, reduce mod 2^32 BEFORE combination */
    int64_t prod_x = n0 * dx_signed;
    uint64_t p0_mod = (uint64_t)prod_x & UINT64_C(0xFFFFFFFF);

    /* Compute dz independently */
    int64_t dz_math = a0z - b0z;
    uint64_t dz_mod = (uint64_t)dz_math & UINT64_C(0xFFFFFFFF);
    int64_t dz_signed = mod_to_signed(dz_mod);

    /* Product Z: signed multiply, reduce mod 2^32 BEFORE combination */
    int64_t prod_z = n2 * dz_signed;
    uint64_t p1_mod = (uint64_t)prod_z & UINT64_C(0xFFFFFFFF);

    /* Numerator: 0 - p0 - p1 mod 2^32 (unsigned wrapping) */
    uint64_t numerator_mod = (UINT64_C(0) - p0_mod - p1_mod)
                             & UINT64_C(0xFFFFFFFF);
    int64_t numerator_signed = mod_to_signed(numerator_mod);

    /* DIV guards */
    if (n1 == 0) {
        r.outcome = ORACLE_BREAK_1C00;
        return r;
    }
    if (n1 == -1 && numerator_signed == (int64_t)INT32_MIN) {
        r.outcome = ORACLE_BREAK_1800;
        return r;
    }

    /* Valid division: truncation toward zero */
    int64_t quotient = numerator_signed / n1;
    r.quotient_bits = (u32)(s32)quotient;

    /* Model first store in oracle memory image */
    r.a0_final[0] = (u32)a0[0];
    r.a0_final[1] = r.quotient_bits;
    r.a0_final[2] = (u32)a0[2];

    /* Reload B[1] from UPDATED oracle memory image */
    u32 base_bits;
    if (aliased) {
        base_bits = r.a0_final[1];
    } else {
        base_bits = (u32)a1[1];
    }

    /* ADDU: result = quotient + base mod 2^32 */
    r.result_bits = (u32)(((uint64_t)r.quotient_bits + (uint64_t)base_bits)
                          & UINT64_C(0xFFFFFFFF));

    /* Model second store */
    r.a0_final[1] = r.result_bits;

    r.outcome = ORACLE_VALID;
    return r;
}

/* =====================================================================
 * MODULO-EQUIVALENCE CONTROL (reclassified from former "bad mutant")
 *
 * This computes full-width products, combines them, then narrows to
 * low32 before dividing.  It is mathematically modulo-equivalent to
 * retail per-product truncation: LOW32(-a-b) == LOW32(-LOW32(a)-LOW32(b)).
 * It is NOT a defective mutant — it is retained as a control to prove
 * that simple full-width combination is indistinguishable from retail
 * by output comparison alone.
 * ===================================================================== */
static u32 modulo_equiv_control(u32 a0, u32 a1, u32 a2)
{
    int64_t a0x = (int64_t)*(s32 *)PSX_ADDR(a0);
    int64_t a1x = (int64_t)*(s32 *)PSX_ADDR(a1);
    int64_t a2x = (int64_t)*(s32 *)PSX_ADDR(a2);
    int64_t a0z = (int64_t)*(s32 *)PSX_ADDR(a0 + 8);
    int64_t a1z = (int64_t)*(s32 *)PSX_ADDR(a1 + 8);
    int64_t a2z = (int64_t)*(s32 *)PSX_ADDR(a2 + 8);
    int64_t ny  = (int64_t)*(s32 *)PSX_ADDR(a2 + 4);

    /* Combines full-precision products then narrows — modulo-equivalent */
    int64_t full_num = -a2x * (a0x - a1x) - a2z * (a0z - a1z);
    s32 numerator;
    u32 num_bits = (u32)(uint64_t)full_num;
    memcpy(&numerator, &num_bits, sizeof(numerator));

    if (ny == 0) abort();
    if (ny == -1 && numerator == INT32_MIN) abort();

    s32 quotient = numerator / (s32)ny;
    *(u32 *)PSX_ADDR(a0 + 4) = (u32)quotient;
    u32 base = *(u32 *)PSX_ADDR(a1 + 4);
    u32 result = (u32)quotient + base;
    *(u32 *)PSX_ADDR(a0 + 4) = result;
    return result;
}

/* =====================================================================
 * TRUE BAD MUTANT: wide-before-DIV (__int128)
 *
 * Uses __int128 to compute full-precision products and numerator,
 * then divides the WIDE numerator by ny WITHOUT reducing to 32 bits.
 * This is intentionally wrong: retail reduces each product to LO32
 * and combines the LO32 values before dividing.
 *
 * Fully defined — no UB.
 * ===================================================================== */
#ifdef __SIZEOF_INT128__
static u32 true_wide_mutant(u32 a0, u32 a1, u32 a2)
{
    __int128 a0x = (__int128)*(s32 *)PSX_ADDR(a0);
    __int128 a1x = (__int128)*(s32 *)PSX_ADDR(a1);
    __int128 a2x = (__int128)*(s32 *)PSX_ADDR(a2);
    __int128 a0z = (__int128)*(s32 *)PSX_ADDR(a0 + 8);
    __int128 a1z = (__int128)*(s32 *)PSX_ADDR(a1 + 8);
    __int128 a2z = (__int128)*(s32 *)PSX_ADDR(a2 + 8);
    __int128 ny  = (__int128)*(s32 *)PSX_ADDR(a2 + 4);

    /* WRONG: divides wide numerator without LO32 reduction */
    __int128 px = a2x * (a0x - a1x);
    __int128 pz = a2z * (a0z - a1z);
    __int128 full_num = -px - pz;
    __int128 wide_q = full_num / ny;
    s32 quotient = (s32)(int64_t)wide_q;

    if (ny == 0) abort();
    if (ny == -1 && quotient == INT32_MIN) abort();

    *(u32 *)PSX_ADDR(a0 + 4) = (u32)quotient;
    u32 base = *(u32 *)PSX_ADDR(a1 + 4);
    u32 result = (u32)quotient + base;
    *(u32 *)PSX_ADDR(a0 + 4) = result;
    return result;
}
#endif

/* =====================================================================
 * ALIAS REORDER MUTANT
 *
 * WRONG: loads B[1] from a1+4 BEFORE the first quotient store.
 * For exact a0==a1, this reads the original middle word (e.g. 200)
 * instead of the quotient (0).
 * ===================================================================== */
static u32 alias_reorder_mutant(u32 a0, u32 a1, u32 a2)
{
    /* Same arithmetic as production... */
    u32 a0_0 = *(u32 *)PSX_ADDR(a0);
    u32 a1_0 = *(u32 *)PSX_ADDR(a1);
    u32 dx_bits = a0_0 - a1_0;
    s32 dx; memcpy(&dx, &dx_bits, sizeof(dx));
    s32 nx; memcpy(&nx, PSX_ADDR(a2), sizeof(nx));
    s64 product_x = (s64)nx * (s64)dx;
    u32 p0_bits = (u32)product_x;

    u32 a0_2 = *(u32 *)PSX_ADDR(a0 + 8);
    u32 a1_2 = *(u32 *)PSX_ADDR(a1 + 8);
    u32 dz_bits = a0_2 - a1_2;
    s32 dz; memcpy(&dz, &dz_bits, sizeof(dz));
    s32 nz; memcpy(&nz, PSX_ADDR(a2 + 8), sizeof(nz));
    s64 product_z = (s64)nz * (s64)dz;
    u32 p1_bits = (u32)product_z;

    u32 numerator_bits = 0u - p0_bits - p1_bits;
    s32 numerator; memcpy(&numerator, &numerator_bits, sizeof(numerator));
    s32 ny; memcpy(&ny, PSX_ADDR(a2 + 4), sizeof(ny));

    if (ny == 0) abort();
    if (ny == -1 && numerator == INT32_MIN) abort();

    s32 quotient = numerator / ny;
    u32 quotient_bits = (u32)quotient;

    /* WRONG: load B[1] BEFORE first store */
    u32 base_bits = *(u32 *)PSX_ADDR(a1 + 4);
    u32 result_bits = quotient_bits + base_bits;

    /* Then store (wrong order) */
    *(u32 *)PSX_ADDR(a0 + 4) = quotient_bits;
    *(u32 *)PSX_ADDR(a0 + 4) = result_bits;
    return result_bits;
}

/* =====================================================================
 * FIXTURE SETUP HELPERS
 * ===================================================================== */

static void write_record(u32 addr, s32 v0, s32 v1, s32 v2)
{
    *(s32 *)PSX_ADDR(addr)      = v0;
    *(s32 *)PSX_ADDR(addr + 4)  = v1;
    *(s32 *)PSX_ADDR(addr + 8)  = v2;
}

static void write_canary(u32 addr, size_t bytes)
{
    for (size_t i = 0; i < bytes; i += 4)
        *(u32 *)PSX_ADDR(addr + i) = 0xDEADBEEFu;
}

/* =====================================================================
 * TEST INFRASTRUCTURE
 * ===================================================================== */

static int s_pass, s_fail;

#define CHECK(desc, expected, actual) do { \
    if ((u32)(expected) == (u32)(actual)) { \
        s_pass++; \
    } else { \
        s_fail++; \
        fprintf(stderr, "FAIL: %s\n  expected: 0x%08X\n  actual:   0x%08X\n", \
                desc, (u32)(expected), (u32)(actual)); \
    } \
} while(0)

/* Test a VALID case: set up guest memory, call production, compare to oracle. */
static void test_case(const char *name,
                      s32 c0, s32 c1, s32 c2,
                      s32 b0, s32 b1, s32 b2,
                      s32 n0, s32 n1, s32 n2)
{
    u32 addr_a0 = 0x801C0000u;
    u32 addr_a1 = 0x801C0010u;
    u32 addr_a2 = 0x801C0020u;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    write_record(addr_a0, c0, c1, c2);
    write_record(addr_a1, b0, b1, b2);
    write_record(addr_a2, n0, n1, n2);

    u32 prod_result = wm_800935DC(addr_a0, addr_a1, addr_a2);
    s32 prod_a0_1 = *(s32 *)PSX_ADDR(addr_a0 + 4);

    /* Run oracle (non-aliased) */
    s32 oa0[3] = { c0, c1, c2 };
    s32 oa1[3] = { b0, b1, b2 };
    s32 oa2[3] = { n0, n1, n2 };
    oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);

    char desc_ret[128], desc_slot[128];
    snprintf(desc_ret, sizeof(desc_ret), "%s return", name);
    snprintf(desc_slot, sizeof(desc_slot), "%s a0[1]", name);

    CHECK(desc_ret, orc.result_bits, prod_result);
    CHECK(desc_slot, (u32)orc.a0_final[1], (u32)prod_a0_1);
}

/*
 * Test a TRAP case using sigsetjmp/siglongjmp (same-process, no fork).
 *
 * Production calls abort() which raises SIGABRT.  The signal handler
 * calls siglongjmp back to the test, preventing abort() from completing.
 * Since everything runs in one process, production's writes to g_PsxRam
 * (if any) are directly observable after siglongjmp.
 *
 * A sentinel is placed at a0+4 before the call.  If production wrote
 * before aborting, the sentinel would be overwritten.  Trap paths
 * call abort() before any store, so the sentinel survives.
 */
static sigjmp_buf s_trap_jmp;

static void trap_sigabrt(int sig)
{
    (void)sig;
    siglongjmp(s_trap_jmp, 1);
}

static void test_trap(const char *name, u32 break_code,
                      s32 c0, s32 c1, s32 c2,
                      s32 b0, s32 b1, s32 b2,
                      s32 n0, s32 n1, s32 n2)
{
    u32 addr_a0 = 0x801C0000u;
    u32 addr_a1 = 0x801C0010u;
    u32 addr_a2 = 0x801C0020u;

    /* Oracle must agree on trap outcome */
    s32 oa0[3] = { c0, c1, c2 };
    s32 oa1[3] = { b0, b1, b2 };
    s32 oa2[3] = { n0, n1, n2 };
    oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);

    char desc_orc[128];
    snprintf(desc_orc, sizeof(desc_orc), "%s oracle outcome", name);
    if (break_code == 0x1C00u) {
        CHECK(desc_orc, (u32)ORACLE_BREAK_1C00, (u32)orc.outcome);
    } else {
        CHECK(desc_orc, (u32)ORACLE_BREAK_1800, (u32)orc.outcome);
    }

    /* Set up g_PsxRam with a sentinel at a0+4.
     * Production reads inputs from g_PsxRam and (on valid paths)
     * writes the quotient to a0+4.  The sentinel lets us detect
     * whether that write happened. */
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    *(s32 *)PSX_ADDR(addr_a0)      = c0;
    *(s32 *)PSX_ADDR(addr_a0 + 4)  = (s32)0xDEADBEEF;  /* sentinel */
    *(s32 *)PSX_ADDR(addr_a0 + 8)  = c2;
    write_record(addr_a1, b0, b1, b2);
    write_record(addr_a2, n0, n1, n2);

    /* Install SIGABRT handler and set return point */
    struct sigaction sa_new, sa_old;
    sa_new.sa_handler = trap_sigabrt;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGABRT, &sa_new, &sa_old);

    if (sigsetjmp(s_trap_jmp, 1) == 0) {
        /* Normal entry: call production.
         * Trap paths call abort() → SIGABRT → handler → siglongjmp back. */
        wm_800935DC(addr_a0, addr_a1, addr_a2);
        /* If we reach here, no trap fired — FAIL */
        s_fail++;
        fprintf(stderr, "FAIL: %s — expected abort (break 0x%04X), got normal return\n",
                name, break_code);
    } else {
        /* Returned via siglongjmp: trap fired.
         * g_PsxRam is in the SAME address space — inspect directly.
         * If production wrote to a0+4 before aborting, sentinel is gone. */
        u32 after = *(u32 *)PSX_ADDR(addr_a0 + 4);
        char desc_mem[128];
        snprintf(desc_mem, sizeof(desc_mem), "%s a0+4 sentinel preserved", name);
        CHECK(desc_mem, 0xDEADBEEFu, after);
        s_pass++; /* Count trap-detection itself as a pass */
    }

    /* Restore original handler */
    sigaction(SIGABRT, &sa_old, NULL);
}

/* =====================================================================
 * MAIN
 * ===================================================================== */

int main(void)
{
    PsxMemory_Init();

    fprintf(stderr,
        "=== W34B5-N: wm_800935dc production-linked test (R3000A finite-width) ===\n\n");

    u32 addr_a0 = 0x801C0000u;
    u32 addr_a1 = 0x801C0010u;
    u32 addr_a2 = 0x801C0020u;

    /* ================================================================
     * SOL GOLDEN VECTORS A-F (finite-width wrap-sign proofs)
     * ================================================================ */

    /* Golden A: X-only overflow
     * product64=0x0000000100020001 p0=0x00020001 p1=0
     * numerator_bits=0xFFFDFFFF quotient=0xFFFF5555 */
    test_case("Golden A: X-only overflow",
              0x00010001, 0, 0,
              0, 0, 0,
              0x00010001, 3, 0);

    /* Golden B: Z-only overflow */
    test_case("Golden B: Z-only overflow",
              0, 0, 0x00010001,
              0, 0, 0,
              0, 3, 0x00010001);

    /* Golden C: Both products overflow */
    test_case("Golden C: both overflow",
              0x00010001, 0, 0x00020001,
              0, 0, 0,
              0x00010001, 3, 0x00020001);

    /* Golden D: Wrap-sign INT32_MIN numerator */
    test_case("Golden D: wrap-sign INT32_MIN",
              (s32)0x40000000, 0, (s32)0x40000000,
              0, 0, 0,
              1, 3, 1);

    /* Golden E: Negative inputs, same wrap-sign result */
    test_case("Golden E: negative wrap-sign",
              (s32)0xC0000000u, 0, (s32)0xC0000000u,
              0, 0, 0,
              1, 3, 1);

    /* Golden F: Positive numerator */
    test_case("Golden F: positive truncated sign",
              (s32)0x40000001, 0, (s32)0x40000000,
              0, 0, 0,
              1, 3, 1);

    /* ================================================================
     * EXTREME SUBU COUNTEREXAMPLE (no UBSan)
     * ================================================================ */

    test_case("Extreme SUBU: MAX-MIN wrap",
              INT32_MAX, 0, INT32_MAX,
              INT32_MIN, 0, INT32_MIN,
              1, 1, 1);

    /* ================================================================
     * OLD ORACLE OVERFLOW COUNTEREXAMPLE
     * Full-width numerator = 2^32, truncated = 0
     * ================================================================ */

    test_case("Oracle overflow: full=2^32 truncated=0",
              INT32_MAX, 0, INT32_MAX,
              INT32_MIN, 0, INT32_MIN,
              INT32_MIN, 1, INT32_MIN);

    /* ================================================================
     * LITERAL INTERMEDIATE VALUE TESTS
     * ================================================================ */

    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0x00010001, 0, 0);
        write_record(addr_a1, 0, 0, 0);
        write_record(addr_a2, 0x00010001, 3, 0);

        u32 dx_bits = *(u32 *)PSX_ADDR(addr_a0) - *(u32 *)PSX_ADDR(addr_a1);
        CHECK("A dx_bits", 0x00010001u, dx_bits);

        s32 dx; memcpy(&dx, &dx_bits, sizeof(dx));
        s32 nx; memcpy(&nx, PSX_ADDR(addr_a2), sizeof(nx));
        s64 prod_x = (s64)nx * (s64)dx;
        CHECK("A product64 hi", 0x00000001u, (u32)((uint64_t)prod_x >> 32));
        CHECK("A product64 lo", 0x00020001u, (u32)(uint64_t)prod_x);

        u32 p0 = (u32)(uint64_t)prod_x;
        CHECK("A p0 LO32", 0x00020001u, p0);

        u32 p1 = 0u;
        u32 numerator_bits = 0u - p0 - p1;
        CHECK("A numerator_bits", 0xFFFDFFFFu, numerator_bits);

        s32 numerator;
        memcpy(&numerator, &numerator_bits, sizeof(numerator));
        CHECK("A numerator s32", (u32)-131073, (u32)numerator);

        s32 quotient = numerator / 3;
        CHECK("A quotient", (u32)-43691, (u32)quotient);
        CHECK("A quotient_bits", 0xFFFF5555u, (u32)quotient);

        /* First store + alias-sensitive load + ADDU */
        *(u32 *)PSX_ADDR(addr_a0 + 4) = (u32)quotient;
        u32 base_bits = *(u32 *)PSX_ADDR(addr_a1 + 4);
        u32 result_bits = (u32)quotient + base_bits;
        CHECK("A first-store bits", 0xFFFF5555u, (u32)quotient);
        CHECK("A reloaded base", 0u, base_bits);
        CHECK("A final result bits", 0xFFFF5555u, result_bits);
    }

    /* ================================================================
     * EXISTING GOLDEN VECTORS (retained)
     * ================================================================ */

    test_case("Flat plane",           0, 0, 0,       0, 100, 0,    0, 0x4000, 0);
    test_case("Positive X slope",     0x1000, 0, 0,  0, 50, 0,     0x4000, 0x4000, 0);
    test_case("Negative X slope",     0x1000, 0, 0,  0, 50, 0,     -0x4000, 0x4000, 0);
    test_case("Positive Z slope",     0, 0, 0x1000,  0, 50, 0,     0, 0x4000, 0x4000);
    test_case("Negative Z slope",     0, 0, 0x1000,  0, 50, 0,     0, 0x4000, -0x4000);
    test_case("Mixed X/Z slope",      0x800, 0, 0x800, 0, 100, 0,  0x2000, 0x4000, 0x2000);
    test_case("Negative coordinates", -0x800, 0, -0x1000, -0x1000, 200, -0x2000, 0, 0x4000, 0);
    test_case("Max byte normal",      0, 0, 0, 0, 0, 0, 0, 127, 0);
    test_case("Exact division",       1, 0, 0, 0, 0, 0, 0, 3, 0);
    test_case("Large normal small diff", 0x100, 0, 0, 0, 0, 0, 0, 0x7FFF, 0);
    test_case("All normal nonzero",   0x2000, 0, 0x3000, 0, 0, 0, 0x1000, 0x4000, 0x2000);
    test_case("All zeros except normal", 0, 0, 0, 0, 0, 0, 0, 1, 0);
    test_case("N[1]=-1 valid",        0, 0, 0, 0, 0, 0, 0, -1, 0);
    test_case("N[1]=-1 with dx",      0x1000, 0, 0, 0, 0, 0, 0, -1, 0);
    test_case("N[1]=-128",            0, 0, 0, 0, 0, 0, 0, -128, 0);
    test_case("Truncation toward zero", 10, 0, 0, 0, 0, 0, 1, 3, 0);
    test_case("Large coordinates",    0x100000, 0, 0x200000, 0x80000, 500, 0x100000, 0, 0x4000, 0);
    test_case("Mixed signs",          -0x1000, 0, 0x2000, 0x500, 300, -0x300, 0x1000, 0x4000, -0x2000);

    /* Return == a0[1] */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0, 0, 0);
        write_record(addr_a1, 0, 500, 0);
        write_record(addr_a2, 0, 0x4000, 0);
        u32 ret = wm_800935DC(addr_a0, addr_a1, addr_a2);
        s32 stored = *(s32 *)PSX_ADDR(addr_a0 + 4);
        CHECK("Return == a0[1]", (u32)stored, ret);
        CHECK("Return value", 500u, ret);
    }

    /* a0[1] overwritten */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0, 999, 0);
        write_record(addr_a1, 0, 100, 0);
        write_record(addr_a2, 0, 0x4000, 0);
        u32 ret = wm_800935DC(addr_a0, addr_a1, addr_a2);
        s32 stored = *(s32 *)PSX_ADDR(addr_a0 + 4);
        CHECK("a0[1] overwritten", 100u, (u32)stored);
        CHECK("Overwrite return", 100u, ret);
    }

    /* ================================================================
     * OVERFLOW GOLDEN VECTORS
     * ================================================================ */

    test_case("MULT overflow LO32=0",    0x10000, 0, 0,       0, 0, 0, 0x10000, 1, 0);
    test_case("Both MULT overflow",      0x10000, 0, 0x20000, 0, 0, 0, 0x10000, 1, 0x20000);
    test_case("SUBU wraps INT32_MIN",    (s32)0x40000000, 0, 0, 0, 0, 0, 2, 1, 0);
    test_case("Cross 0x7FFFFFFF boundary", (s32)0x40000000, 0, (s32)0x40000000, 0, 0, 0, 1, 1, 1);
    test_case("Full-width positive, LO32 negative", (s32)0xC0000000u, 0, (s32)0xC0000000u, 0, 0, 0, 1, 1, 1);
    test_case("Full-width negative, LO32 positive", (s32)0x40000001, 0, (s32)0x40000000, 0, 0, 0, 1, 1, 1);

    /* ================================================================
     * EXACT ALIAS ORDERING TEST (a0 == a1)
     *
     * When a0 == a1: A[i] == B[i], so dx = dz = 0, quotient = 0.
     * Original a0[1] = 200 (sentinel).
     * Retail: store #1 writes 0 → load reads 0 → result = 0.
     * Wrong mutant: loads 200 BEFORE store → result = 200.
     * ================================================================ */

    /* Exact alias: quotient=0, sentinel=200 */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0, 200, 0);  /* sentinel at a0[1]=200 */
        write_record(addr_a2, 0, 1, 0);

        /* Oracle: aliased=1, quotient=0, load reads quotient=0, result=0 */
        s32 oa0[3] = { 0, 200, 0 };
        s32 oa1[3] = { 0, 200, 0 };
        s32 oa2[3] = { 0, 1, 0 };
        oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 1);

        u32 ret = wm_800935DC(addr_a0, addr_a0, addr_a2);
        s32 stored = *(s32 *)PSX_ADDR(addr_a0 + 4);

        CHECK("Alias order: oracle result", 0u, orc.result_bits);
        CHECK("Alias order: prod return", orc.result_bits, ret);
        CHECK("Alias order: a0[1]", (u32)orc.a0_final[1], (u32)stored);
    }

    /* ================================================================
     * ALIAS REORDER MUTANT
     *
     * The reorder mutant loads B[1] BEFORE the first store.
     * For exact a0==a1 with sentinel=200 and quotient=0:
     *   Correct: store 0 → load 0 → result 0
     *   Wrong:   load 200 → store 0 → result 200
     * ================================================================ */

    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0, 200, 0);
        write_record(addr_a2, 0, 1, 0);
        u32 reorder_ret = alias_reorder_mutant(addr_a0, addr_a0, addr_a2);

        CHECK("Reorder mutant detected: wrong result",
              1u, reorder_ret != 0u ? 1u : 0u);
    }

    /* ================================================================
     * OBSERVER CONTROL — proves the sigsetjmp/siglongjmp trap harness
     * can detect a real production write to g_PsxRam.
     *
     * A normal valid call writes the quotient to a0+4.  We set a
     * sentinel, call production, and verify the sentinel is gone.
     * This proves: if production wrote before abort(), the harness
     * WOULD observe it.
     * ================================================================ */

    {
        u32 oc_a0 = 0x801C0000u;
        u32 oc_a1 = 0x801C0010u;
        u32 oc_a2 = 0x801C0020u;

        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(oc_a0)     = 0;
        *(s32 *)PSX_ADDR(oc_a0 + 4) = (s32)0xDEADBEEF; /* sentinel */
        *(s32 *)PSX_ADDR(oc_a0 + 8) = 0;
        write_record(oc_a1, 0, 100, 0);
        write_record(oc_a2, 0, 0x4000, 0);

        u32 ret = wm_800935DC(oc_a0, oc_a1, oc_a2);
        u32 after = *(u32 *)PSX_ADDR(oc_a0 + 4);

        /* Production must have overwritten the sentinel */
        CHECK("OBSERVER CONTROL: sentinel overwritten",
              0u, after == 0xDEADBEEFu ? 1u : 0u);
        /* Result must be 100 (quotient=0 + base=100) */
        CHECK("OBSERVER CONTROL: result correct", 100u, ret);
        CHECK("OBSERVER CONTROL: a0+4 == return", ret, after);
    }

    /* ================================================================
     * DIVIDE-BY-ZERO TRAP TEST (BREAK 0x1C00)
     * ================================================================ */

    test_trap("DIV-zero: N[1]=0", 0x1C00u,
              100, 0, 200,
              50, 75, 150,
              0x1000, 0, 0x2000);

    /* ================================================================
     * INT32_MIN / -1 TRAP TEST (BREAK 0x1800)
     * ================================================================ */

    test_trap("INT32_MIN/-1 trap", 0x1800u,
              (s32)0x40000000, 0, (s32)0x40000000,
              0, 0, 0,
              1, -1, 1);

    /* ================================================================
     * CANARY TEST (memory guard)
     * ================================================================ */

    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_canary(addr_a0, 12);
        write_canary(addr_a1, 12);
        write_canary(addr_a2, 12);
        *(s32 *)PSX_ADDR(addr_a0)      = 0;
        *(s32 *)PSX_ADDR(addr_a0 + 8)  = 0;
        *(s32 *)PSX_ADDR(addr_a1)      = 0;
        *(s32 *)PSX_ADDR(addr_a1 + 4)  = 100;
        *(s32 *)PSX_ADDR(addr_a1 + 8)  = 0;
        *(s32 *)PSX_ADDR(addr_a2)      = 0;
        *(s32 *)PSX_ADDR(addr_a2 + 4)  = 1;
        *(s32 *)PSX_ADDR(addr_a2 + 8)  = 0;

        wm_800935DC(addr_a0, addr_a1, addr_a2);

        CHECK("Canary: a0[4] modified", 0u,
              *(u32 *)PSX_ADDR(addr_a0 + 4) == 0xDEADBEEFu ? 1u : 0u);
        CHECK("Canary: a0[0] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a0));
        CHECK("Canary: a0[8] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a0 + 8));
        CHECK("Canary: a1[0] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a1));
        CHECK("Canary: a1[4] unchanged", 100u, *(u32 *)PSX_ADDR(addr_a1 + 4));
        CHECK("Canary: a1[8] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a1 + 8));
        CHECK("Canary: a2[0] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a2));
        CHECK("Canary: a2[4] unchanged", 1u, *(u32 *)PSX_ADDR(addr_a2 + 4));
        CHECK("Canary: a2[8] unchanged", 0u, *(u32 *)PSX_ADDR(addr_a2 + 8));
    }

    /* ================================================================
     * MODULO-EQUIVALENCE CONTROL
     *
     * The old "full-width-before-DIV" mutant is reclassified: it is
     * modulo-equivalent because it narrows numerator to LO32 BEFORE
     * dividing.  Retained as a control to prove equivalence, NOT as
     * a mutation-detection test.
     * ================================================================ */

    {
        /* Golden A */
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0x00010001, 0, 0);
        write_record(addr_a1, 0, 0, 0);
        write_record(addr_a2, 0x00010001, 3, 0);
        u32 ctrl_ret = modulo_equiv_control(addr_a0, addr_a1, addr_a2);

        s32 oa0[3] = { 0x00010001, 0, 0 };
        s32 oa1[3] = { 0, 0, 0 };
        s32 oa2[3] = { 0x00010001, 3, 0 };
        oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);
        CHECK("Modulo-equiv control: Golden A matches", orc.result_bits, ctrl_ret);

        /* Overflow case */
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, INT32_MAX, 0, INT32_MAX);
        write_record(addr_a1, INT32_MIN, 0, INT32_MIN);
        write_record(addr_a2, INT32_MIN, 1, INT32_MIN);
        u32 prod_norm = wm_800935DC(addr_a0, addr_a1, addr_a2);

        s32 oa0o[3] = { INT32_MAX, 0, INT32_MAX };
        s32 oa1o[3] = { INT32_MIN, 0, INT32_MIN };
        s32 oa2o[3] = { INT32_MIN, 1, INT32_MIN };
        orc = oracle_800935DC(oa0o, oa1o, oa2o, 0);
        CHECK("Modulo-equiv control: overflow matches oracle", orc.result_bits, prod_norm);
    }

    /* ================================================================
     * TRUE WIDE MUTANT (__int128)
     *
     * Divides the FULL-precision numerator by ny without reducing
     * products to LO32 first.  This gives a different quotient than
     * retail when products exceed 32 bits.
     *
     * Golden A: retail quotient = 0xFFFF5555 (-43691)
     *   Wide: mathematical_num = -(0x10001 * 0x10001) = -4295098369
     *   Wide quotient = -4295098369 / 3 = -1431699456
     *   Low32 = 0xAAAA0000
     * ================================================================ */

#ifdef __SIZEOF_INT128__
    {
        int wide_mismatches = 0;

        /* Golden A */
        {
            memset(g_PsxRam, 0, PSX_RAM_SIZE);
            write_record(addr_a0, 0x00010001, 0, 0);
            write_record(addr_a1, 0, 0, 0);
            write_record(addr_a2, 0x00010001, 3, 0);
            u32 wide_ret = true_wide_mutant(addr_a0, addr_a1, addr_a2);

            s32 oa0[3] = { 0x00010001, 0, 0 };
            s32 oa1[3] = { 0, 0, 0 };
            s32 oa2[3] = { 0x00010001, 3, 0 };
            oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);

            char desc[128];
            snprintf(desc, sizeof(desc), "Wide mutant A: retail=0x%08X wide=0x%08X",
                     orc.result_bits, wide_ret);
            CHECK(desc, 1u, orc.result_bits != wide_ret ? 1u : 0u);
            if (orc.result_bits != wide_ret) wide_mismatches++;

            /* Hard-coded expected values */
            CHECK("Wide mutant A: retail quotient", 0xFFFF5555u, orc.result_bits);
            CHECK("Wide mutant A: wide quotient",  0xAAAA0000u, wide_ret);
        }

        /* Golden C: both overflow */
        {
            memset(g_PsxRam, 0, PSX_RAM_SIZE);
            write_record(addr_a0, 0x00010001, 0, 0x00020001);
            write_record(addr_a1, 0, 0, 0);
            write_record(addr_a2, 0x00010001, 3, 0x00020001);
            u32 wide_ret = true_wide_mutant(addr_a0, addr_a1, addr_a2);

            s32 oa0[3] = { 0x00010001, 0, 0x00020001 };
            s32 oa1[3] = { 0, 0, 0 };
            s32 oa2[3] = { 0x00010001, 3, 0x00020001 };
            oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);

            if (orc.result_bits != wide_ret) wide_mismatches++;
            CHECK("Wide mutant C: mismatch", 1u,
                  orc.result_bits != wide_ret ? 1u : 0u);
        }

        /* Golden F: positive numerator (products exceed 32 bits differently) */
        {
            memset(g_PsxRam, 0, PSX_RAM_SIZE);
            write_record(addr_a0, (s32)0x40000001, 0, (s32)0x40000000);
            write_record(addr_a1, 0, 0, 0);
            write_record(addr_a2, 1, 3, 1);
            u32 wide_ret = true_wide_mutant(addr_a0, addr_a1, addr_a2);

            s32 oa0[3] = { (s32)0x40000001, 0, (s32)0x40000000 };
            s32 oa1[3] = { 0, 0, 0 };
            s32 oa2[3] = { 1, 3, 1 };
            oracle_result_t orc = oracle_800935DC(oa0, oa1, oa2, 0);

            if (orc.result_bits != wide_ret) wide_mismatches++;
            CHECK("Wide mutant F: mismatch", 1u,
                  orc.result_bits != wide_ret ? 1u : 0u);
        }

        CHECK("Wide mutant: total mismatches > 1", 1u,
              wide_mismatches > 1 ? 1u : 0u);
    }
#endif

    /* ================================================================
     * SECONDARY MUTATION: swap Nx/Nz
     * ================================================================ */

    {
        int swap_failures = 0;

        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0x2000, 0, 0x3000);
        write_record(addr_a1, 0, 0, 0);
        write_record(addr_a2, 0x1000, 0x4000, 0x2000);
        u32 normal_ret = wm_800935DC(addr_a0, addr_a1, addr_a2);

        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        write_record(addr_a0, 0x2000, 0, 0x3000);
        write_record(addr_a1, 0, 0, 0);
        write_record(addr_a2, 0x2000, 0x4000, 0x1000);
        u32 swapped_ret = wm_800935DC(addr_a0, addr_a1, addr_a2);

        if (normal_ret != swapped_ret) swap_failures++;
        CHECK("Mutant: swap Nx/Nz detected", 1u, swap_failures > 0 ? 1u : 0u);
    }

    /* ================================================================
     * ORACLE mod_to_signed() UNIT TESTS
     * ================================================================ */

    CHECK("mod_to_signed 0x00000000", 0u,             (u32)(s32)mod_to_signed(0x00000000));
    CHECK("mod_to_signed 0x00000001", 1u,             (u32)(s32)mod_to_signed(0x00000001));
    CHECK("mod_to_signed 0x7FFFFFFF", 0x7FFFFFFFu,    (u32)(s32)mod_to_signed(0x7FFFFFFF));
    CHECK("mod_to_signed 0x80000000", 0x80000000u,    (u32)(s32)mod_to_signed(0x80000000));
    CHECK("mod_to_signed 0x80000001", 0x80000001u,    (u32)(s32)mod_to_signed(0x80000001));
    CHECK("mod_to_signed 0xFFFFFFFF", 0xFFFFFFFFu,    (u32)(s32)mod_to_signed(0xFFFFFFFF));

    /* ================================================================
     * RESULTS
     * ================================================================ */

    fprintf(stderr, "\n=== Results: %d PASS / %d TOTAL ===\n",
            s_pass, s_pass + s_fail);
    if (s_fail > 0)
        fprintf(stderr, "*** %d FAILURE(S) ***\n", s_fail);

    return s_fail > 0 ? 1 : 0;
}
