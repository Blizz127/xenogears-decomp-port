/*
 * W34B5-O — production-linked test for world terrain normal helper 0x80093740.
 *
 * The real wm_80093740 is linked with the accepted wm_80093660 and the real
 * PC-port OuterProduct0/VectorNormal compatibility implementations.  The
 * oracle below is a high-level triangle model; it does not call any of them.
 */
#include <limits.h>
#include "../src/gte_normalize_table.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "psx_memory.h"
#include "world_map_terrain_normal.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
_Alignas(16) uint8_t g_PsxScratchpad[4096];

void PsxMemory_Init(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
}

#define WM_TERRAIN_STRIDE_ABS 0x8009D160u
#define WM_TERRAIN_TABLE_ABS  0x8009C184u
#define WM_COEFF_0_ABS         0x8009B244u
#define WM_COEFF_1_ABS         0x8009B24Cu
#define WM_COEFF_2_ABS         0x8009B254u
#define WM_COEFF_3_ABS         0x8009B25Cu
#define TEST_GRID_BASE         0x80120000u
#define TEST_OUTPUT_REGION     0x801C0100u
#define TEST_OUTPUT_ADDR       (TEST_OUTPUT_REGION + 8u)

typedef enum {
    TRI_FLAG1_NEG,
    TRI_FLAG1_NONNEG,
    TRI_FLAG0_NEG,
    TRI_FLAG0_NONNEG
} triangle_t;

typedef struct {
    const char *name;
    s32 x;
    s32 z;
    u8 flag;
    int8_t h00;
    int8_t h10;
    int8_t h01;
    int8_t h11;
    s32 coeff[4];
    u32 sum_bits;
    triangle_t triangle;
    s32 edge0[3];
    s32 edge1[3];
    s32 cross[3];
    s32 normal[3];
    u32 return_bits;
} golden_t;

typedef struct {
    u32 sum_bits;
    triangle_t triangle;
    s32 edge0[3];
    s32 edge1[3];
    s32 cross[3];
    s32 normal[3];
    u32 return_bits;
} oracle_t;

static int s_pass;
static int s_fail;

static void check(const char *name, int condition)
{
    if (condition) {
        s_pass++;
    } else {
        s_fail++;
        fprintf(stderr, "FAIL: %s\n", name);
    }
}

static s32 bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 s32_to_bits(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 load_s32(const void *src)
{
    s32 value;
    memcpy(&value, src, sizeof(value));
    return value;
}

static u32 load_u32(const void *src)
{
    u32 value;
    memcpy(&value, src, sizeof(value));
    return value;
}

static void store_s32(void *dst, s32 value)
{
    memcpy(dst, &value, sizeof(value));
}

static void store_u32(void *dst, u32 value)
{
    memcpy(dst, &value, sizeof(value));
}

static u32 mul_low32(u32 lhs_bits, s32 rhs)
{
    int64_t product = (int64_t)bits_to_s32(lhs_bits) * (int64_t)rhs;
    return (u32)(uint64_t)product;
}

static s32 cross_component(int64_t value)
{
    return bits_to_s32((u32)(uint64_t)value);
}

/* =====================================================================
 * INDEPENDENT NORMALIZATION ORACLE
 *
 * Reproduces VectorNormal's observable semantics without calling it.
 * Uses the same inverse-sqrt lookup table (a public API constant) but
 * implements the algorithm independently with different code structure.
 *
 * Contract (matching retail PsyQ VectorNormalWork):
 *   1. Truncate inputs to s16
 *   2. squared = (uint32_t)(sx*sx + sy*sy + sz*sz)
 *   3. If squared == 0: output (0,0,0), return 0
 *   4. CLZ on squared → lzcEven → shift → table index
 *   5. scale = table[index]
 *   6. out_i = clamp_s32((scale * si) >> shift)
 *   7. Return squared
 * ===================================================================== */

static s32 oracle_clamp_s32(int64_t value)
{
    if (value > INT64_C(0x7FFFFFFF))
        return INT32_MAX;
    if (value < INT64_C(-0x80000000))
        return INT32_MIN;
    return (s32)value;
}

/* Public API constant: PsyQ GTE inverse-sqrt table.
 * Indexed by normalized squared magnitude; each entry ≈ 4096/√(index+64). */
static int16_t s_oracle_InvSqrtTable[XENO_GTE_NORM_TABLE_LEN];
__attribute__((constructor)) static void s_oracle_InvSqrtTable_Init(void)
{
    XenoGteNormTableFill(s_oracle_InvSqrtTable);
}

static void oracle_normalize(s32 cx, s32 cy, s32 cz,
                             s32 *nx, s32 *ny, s32 *nz,
                             u32 *ret_bits)
{
    /* Step 1: truncate to s16 (matching VectorNormalWork) */
    s32 sx = (int16_t)cx;
    s32 sy = (int16_t)cy;
    s32 sz = (int16_t)cz;

    /* Step 2: squared magnitude */
    u32 squared = (u32)(sx * sx + sy * sy + sz * sz);

    /* Step 3: zero-vector early out */
    if (squared == 0u) {
        *nx = 0;  *ny = 0;  *nz = 0;
        *ret_bits = 0u;
        return;
    }

    /* Step 4: leading zeros → even CLZ → shift */
    int lzc = __builtin_clz(squared);
    int lzc_even = lzc & ~1;
    int shift = (31 - lzc_even) >> 1;

    /* Step 5: normalize squared into table index range [64, 128) */
    int index;
    if (lzc_even >= 24)
        index = (int)(squared << (lzc_even - 24));
    else
        index = (int)(squared >> (24 - lzc_even));
    index -= 0x40;
    if (index < 0) index = 0;
    if (index >= (int)(sizeof(s_oracle_InvSqrtTable) /
                       sizeof(s_oracle_InvSqrtTable[0])))
        index = (int)(sizeof(s_oracle_InvSqrtTable) /
                      sizeof(s_oracle_InvSqrtTable[0])) - 1;

    /* Step 6: table lookup for inverse-sqrt scale */
    s32 scale = s_oracle_InvSqrtTable[index];

    /* Step 7: normalize each component (arithmetic right shift = floor div) */
    *nx = oracle_clamp_s32(((int64_t)scale * sx) >> shift);
    *ny = oracle_clamp_s32(((int64_t)scale * sy) >> shift);
    *nz = oracle_clamp_s32(((int64_t)scale * sz) >> shift);

    /* Step 8: return squared magnitude */
    *ret_bits = squared;
}

/* High-level triangle oracle: vertices first, then edge1 x edge0. */
static oracle_t oracle_geometry(const golden_t *g)
{
    oracle_t r;
    s32 u_div8 = g->x / 8;
    s32 v_div8 = g->z / 8;
    u32 u = s32_to_bits(u_div8) & 0xFFFFu;
    u32 v = s32_to_bits(v_div8) & 0xFFFFu;
    u32 p0;
    u32 p1;
    s32 base[3];
    s32 p_a[3];
    s32 p_b[3];

    memset(&r, 0, sizeof(r));

    if ((g->flag & 0x80u) != 0u) {
        p0 = mul_low32(u, g->coeff[2]);
        p1 = mul_low32(0u - v, g->coeff[3]);
        r.sum_bits = p0 + p1;

        base[0] = 0;  base[1] = g->h00; base[2] = 0;
        if (bits_to_s32(r.sum_bits) < 0) {
            r.triangle = TRI_FLAG1_NEG;
            p_a[0] = 16; p_a[1] = g->h11; p_a[2] = -16;
            p_b[0] = 0;  p_b[1] = g->h01; p_b[2] = -16;
        } else {
            r.triangle = TRI_FLAG1_NONNEG;
            p_a[0] = 16; p_a[1] = g->h10; p_a[2] = 0;
            p_b[0] = 16; p_b[1] = g->h11; p_b[2] = -16;
        }
    } else {
        u32 u_minus_cell = u + UINT32_C(0xFFFF0000);
        p0 = mul_low32(u_minus_cell, g->coeff[0]);
        p1 = mul_low32(0u - v, g->coeff[1]);
        r.sum_bits = p0 + p1;

        base[0] = 16; base[1] = g->h10; base[2] = 0;
        if (bits_to_s32(r.sum_bits) < 0) {
            r.triangle = TRI_FLAG0_NEG;
            p_a[0] = 0;  p_a[1] = g->h01; p_a[2] = -16;
            p_b[0] = 0;  p_b[1] = g->h00; p_b[2] = 0;
        } else {
            r.triangle = TRI_FLAG0_NONNEG;
            p_a[0] = 16; p_a[1] = g->h11; p_a[2] = -16;
            p_b[0] = 0;  p_b[1] = g->h01; p_b[2] = -16;
        }
    }

    for (int i = 0; i < 3; i++) {
        r.edge0[i] = p_a[i] - base[i];
        r.edge1[i] = p_b[i] - base[i];
    }

    r.cross[0] = cross_component((int64_t)r.edge1[1] * r.edge0[2] -
                                 (int64_t)r.edge1[2] * r.edge0[1]);
    r.cross[1] = cross_component((int64_t)r.edge1[2] * r.edge0[0] -
                                 (int64_t)r.edge1[0] * r.edge0[2]);
    r.cross[2] = cross_component((int64_t)r.edge1[0] * r.edge0[1] -
                                 (int64_t)r.edge1[1] * r.edge0[0]);

    /* Independent normalization: does NOT call VectorNormal. */
    oracle_normalize(r.cross[0], r.cross[1], r.cross[2],
                     &r.normal[0], &r.normal[1], &r.normal[2],
                     &r.return_bits);
    return r;
}

/* Mathematical floor division used only by the independent cell locator. */
static s32 floor_div_pow2(s32 value, s32 divisor)
{
    s32 q = value / divisor;
    s32 rem = value % divisor;
    if (rem != 0 && value < 0)
        q--;
    return q;
}

/* Independent high-level location for the accepted cell helper's result. */
static u32 fixture_cell_address(s32 x, s32 z)
{
    s32 tile_x = floor_div_pow2(x, INT32_C(1) << 20) / 8;
    s32 tile_z = floor_div_pow2(z, INT32_C(1) << 20) / 8;
    s32 tile_index = tile_z + tile_x; /* fixture stride is one */
    s32 local_x = floor_div_pow2(x, 4096) & 0x7FF;
    s32 local_z = floor_div_pow2(z, 4096) & 0x7FF;
    s32 quadrant = 0;

    if (local_x >= 1024) {
        local_x -= 1024;
        quadrant |= 1;
    }
    if (local_z >= 1024) {
        local_z -= 1024;
        quadrant |= 2;
    }

    check("fixture tile index is zero", tile_index == 0);
    return TEST_GRID_BASE + (u32)(quadrant * 0x144) +
           (u32)(((local_z / 128) * 9 + (local_x / 128)) * 4);
}

static const golden_t s_goldens[] = {
    {
        "A flat axis", 800, 1600, 0x80, 0, 0, 0, 0,
        { 2896, -2896, 2896, 2896 }, 0xFFFB94C0u, TRI_FLAG1_NEG,
        { 16, 0, -16 }, { 0, 0, -16 }, { 0, -256, 0 },
        { 0, -4096, 0 }, 0x00010000u
    },
    {
        "B positive slope", 2400, 800, 0x80, 0, 4, 7, 12,
        { 2896, -2896, 2896, 2896 }, 0x0008D680u, TRI_FLAG1_NONNEG,
        { 16, 4, 0 }, { 16, 12, -16 }, { 64, -256, -128 },
        { 893, -3575, -1788 }, 0x00015000u
    },
    {
        "C negative slope", 800, 2400, 0x80, 10, 0, 2, -6,
        { 2896, -2896, 2896, 2896 }, 0xFFF72980u, TRI_FLAG1_NEG,
        { 16, -16, -16 }, { 0, -8, -16 }, { -128, -256, 128 },
        { -1672, -3344, 1672 }, 0x00018000u
    },
    {
        "D mixed flag0 low", 8000, 16000, 0x00, -3, 5, 9, 20,
        { 2896, -2896, 2896, 2896 }, 0xF5349180u, TRI_FLAG0_NEG,
        { -16, 4, -16 }, { -16, -8, 0 }, { 128, -256, -192 },
        { 1521, -3042, -2282 }, 0x0001D000u
    },
    {
        "E mixed flag0 high", 320000, 240000, 0x00, -3, 5, 9, 20,
        { 2896, -2896, 2896, 2896 }, 0x00C54300u, TRI_FLAG0_NONNEG,
        { 0, 15, -16 }, { -16, 4, -16 }, { 176, -256, -240 },
        { 1839, -2675, -2508 }, 0x00025A00u
    },
    {
        "F flag1 zero boundary", 262144, 262144, 0x80, 1, 6, -4, 9,
        { 2896, -2896, 2896, 2896 }, 0x00000000u, TRI_FLAG1_NONNEG,
        { 16, 5, 0 }, { 16, 8, -16 }, { 80, -256, -48 },
        { 1206, -3861, -724 }, 0x00012200u
    },
    {
        "G flag0 zero boundary", 262144, 262144, 0x00, 1, 6, -4, 9,
        { 2896, -2896, 2896, 2896 }, 0x00000000u, TRI_FLAG0_NONNEG,
        { 0, 3, -16 }, { -16, -10, -16 }, { 208, -256, -48 },
        { 2561, -3153, -592 }, 0x0001B200u
    },
    {
        "H signed height extrema", 8, 16, 0x80, -128, 127, 127, -128,
        { 2896, -2896, 2896, 2896 }, 0xFFFFF4B0u, TRI_FLAG1_NEG,
        { 16, 0, -16 }, { 0, 255, -16 }, { -4080, -256, -4080 },
        { -2896, -182, -2896 }, 0x01FD0200u
    },
    {
        "I opposite signed extrema", 8, 16, 0x00, 127, -128, -128, 127,
        { 2896, -2896, 2896, 2896 }, 0xF4B021F0u, TRI_FLAG0_NEG,
        { -16, 0, -16 }, { -16, 255, 0 }, { -4080, -256, 4080 },
        { -2896, -182, 2895 }, 0x01FD0200u
    },
    {
        "J negative divide rounding", -1, 7, 0x80, 2, 3, 4, 5,
        { 2896, -2896, 2896, 2896 }, 0x00000000u, TRI_FLAG1_NONNEG,
        { 16, 1, 0 }, { 16, 3, -16 }, { 16, -256, -32 },
        { 254, -4064, -508 }, 0x00010500u
    },
    {
        "K exact signed boundary", 8, 0, 0x80, 0, 1, 2, 3,
        { 2896, -2896, INT32_MIN, 1 }, 0x80000000u, TRI_FLAG1_NEG,
        { 16, 3, -16 }, { 0, 2, -16 }, { 16, -256, -32 },
        { 254, -4064, -508 }, 0x00010500u
    },
    {
        "L MULT high-word wrap", 524280, 8, 0x80, 0, 10, -10, 20,
        { 2896, -2896, INT32_MAX, INT32_MAX }, 0xFFFF0002u, TRI_FLAG1_NEG,
        { 16, 20, -16 }, { 0, -10, -16 }, { 480, -256, 160 },
        { 3478, -1855, 1159 }, 0x0004E800u
    },
    {
        "M two-product modulo wrap", 262144, 262144, 0x80, 0, 2, 4, 6,
        { 2896, -2896, INT32_MIN, INT32_MIN }, 0x00000000u,
        TRI_FLAG1_NONNEG,
        { 16, 2, 0 }, { 16, 6, -16 }, { 32, -256, -64 },
        { 493, -3944, -986 }, 0x00011400u
    },
    {
        "N signed height -255", 8, 16, 0x80, 127, -128, -128, 127,
        { 2896, -2896, 2896, 2896 }, 0xFFFFF4B0u, TRI_FLAG1_NEG,
        { 16, 0, -16 }, { 0, -255, -16 }, { 4080, -256, 4080 },
        { 2895, -182, 2895 }, 0x01FD0200u
    },
    {
        "O large slope", 2400, 800, 0x80, 0, 50, 30, 80,
        { 2896, -2896, 2896, 2896 }, 0x0008D680u, TRI_FLAG1_NONNEG,
        { 16, 50, 0 }, { 16, 80, -16 }, { 800, -256, -480 },
        { 3390, -1085, -2035 }, 0x000E4800u
    }
};

static void run_golden(const golden_t *g)
{
    oracle_t oracle = oracle_geometry(g);
    u32 cell_addr;
    uint8_t cell_before[44];
    uint8_t output_expected[32];
    uint8_t scratch_expected[256];
    uint8_t coeff_before[4][4];
    u32 ret_bits;
    char desc[160];

#define CASE_CHECK(label, condition) do { \
    snprintf(desc, sizeof(desc), "%s: %s", g->name, label); \
    check(desc, (condition)); \
} while (0)

    CASE_CHECK("oracle sum literal", oracle.sum_bits == g->sum_bits);
    CASE_CHECK("oracle triangle literal", oracle.triangle == g->triangle);
    CASE_CHECK("oracle edge0 literal", memcmp(oracle.edge0, g->edge0, 12) == 0);
    CASE_CHECK("oracle edge1 literal", memcmp(oracle.edge1, g->edge1, 12) == 0);
    CASE_CHECK("oracle cross literal", memcmp(oracle.cross, g->cross, 12) == 0);
    CASE_CHECK("oracle normal matches literal",
               memcmp(oracle.normal, g->normal, 12) == 0);
    CASE_CHECK("oracle return matches literal",
               oracle.return_bits == g->return_bits);

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0xA5, 4096);
    store_s32(PSX_ADDR(WM_TERRAIN_STRIDE_ABS), 1);
    store_u32(PSX_ADDR(WM_TERRAIN_TABLE_ABS), TEST_GRID_BASE);
    store_s32(PSX_ADDR(WM_COEFF_0_ABS), g->coeff[0]);
    store_s32(PSX_ADDR(WM_COEFF_1_ABS), g->coeff[1]);
    store_s32(PSX_ADDR(WM_COEFF_2_ABS), g->coeff[2]);
    store_s32(PSX_ADDR(WM_COEFF_3_ABS), g->coeff[3]);

    cell_addr = fixture_cell_address(g->x, g->z);
    memset(PSX_ADDR(cell_addr), 0x5A, 44);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0, &g->h00, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 1, &g->flag, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 4, &g->h10, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0x24, &g->h01, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0x28, &g->h11, 1);
    memcpy(cell_before, PSX_ADDR(cell_addr), sizeof(cell_before));
    for (int i = 0; i < 4; i++)
        memcpy(coeff_before[i], PSX_ADDR(WM_COEFF_0_ABS + (u32)i * 8u), 4);

    memset(PSX_ADDR(TEST_OUTPUT_REGION), 0xCD, sizeof(output_expected));
    memset(output_expected, 0xCD, sizeof(output_expected));
    for (int i = 0; i < 3; i++)
        store_s32(output_expected + 8 + i * 4, oracle.normal[i]);

    memset(scratch_expected, 0xA5, sizeof(scratch_expected));
    for (int i = 0; i < 3; i++) {
        store_s32(scratch_expected + i * 4, oracle.edge0[i]);
        store_s32(scratch_expected + 0x10 + i * 4, oracle.edge1[i]);
        store_s32(scratch_expected + 0x20 + i * 4, oracle.cross[i]);
    }

    ret_bits = s32_to_bits(wm_80093740(TEST_OUTPUT_ADDR, g->x, g->z));

    CASE_CHECK("return vs oracle", ret_bits == oracle.return_bits);
    CASE_CHECK("normal vs oracle",
               memcmp(PSX_ADDR(TEST_OUTPUT_ADDR), oracle.normal, 12) == 0);
    CASE_CHECK("output canaries exact",
               memcmp(PSX_ADDR(TEST_OUTPUT_REGION), output_expected,
                      sizeof(output_expected)) == 0);
    CASE_CHECK("scratchpad exact write set",
               memcmp(g_PsxScratchpad, scratch_expected,
                      sizeof(scratch_expected)) == 0);
    CASE_CHECK("cell input preserved",
               memcmp(PSX_ADDR(cell_addr), cell_before, sizeof(cell_before)) == 0);
    CASE_CHECK("coefficient 0 preserved",
               memcmp(PSX_ADDR(WM_COEFF_0_ABS), coeff_before[0], 4) == 0);
    CASE_CHECK("coefficient 1 preserved",
               memcmp(PSX_ADDR(WM_COEFF_1_ABS), coeff_before[1], 4) == 0);
    CASE_CHECK("coefficient 2 preserved",
               memcmp(PSX_ADDR(WM_COEFF_2_ABS), coeff_before[2], 4) == 0);
    CASE_CHECK("coefficient 3 preserved",
               memcmp(PSX_ADDR(WM_COEFF_3_ABS), coeff_before[3], 4) == 0);
    CASE_CHECK("stride preserved", load_s32(PSX_ADDR(WM_TERRAIN_STRIDE_ABS)) == 1);
    CASE_CHECK("table preserved", load_u32(PSX_ADDR(WM_TERRAIN_TABLE_ABS)) ==
                                  TEST_GRID_BASE);
#undef CASE_CHECK
}

/* Mutant normalization: uses scale-1 from table (wrong scale). */
static void mutant_normalize_scale(s32 cx, s32 cy, s32 cz,
                                   s32 *nx, s32 *ny, s32 *nz)
{
    s32 sx = (int16_t)cx;
    s32 sy = (int16_t)cy;
    s32 sz = (int16_t)cz;
    u32 squared = (u32)(sx * sx + sy * sy + sz * sz);
    if (squared == 0u) { *nx = 0; *ny = 0; *nz = 0; return; }

    int lzc = __builtin_clz(squared);
    int lzc_even = lzc & ~1;
    int shift = (31 - lzc_even) >> 1;
    int index;
    if (lzc_even >= 24) index = (int)(squared << (lzc_even - 24));
    else                index = (int)(squared >> (24 - lzc_even));
    index -= 0x40;
    if (index < 0) index = 0;
    if (index >= (int)(sizeof(s_oracle_InvSqrtTable) /
                       sizeof(s_oracle_InvSqrtTable[0])))
        index = (int)(sizeof(s_oracle_InvSqrtTable) /
                      sizeof(s_oracle_InvSqrtTable[0])) - 1;

    /* WRONG: subtract 1 from scale */
    s32 scale = s_oracle_InvSqrtTable[index] - 1;
    *nx = oracle_clamp_s32(((int64_t)scale * sx) >> shift);
    *ny = oracle_clamp_s32(((int64_t)scale * sy) >> shift);
    *nz = oracle_clamp_s32(((int64_t)scale * sz) >> shift);
}

/* Mutant normalization: uses squared magnitude directly (wrong divisor). */
static void mutant_normalize_squared(s32 cx, s32 cy, s32 cz,
                                     s32 *nx, s32 *ny, s32 *nz)
{
    s32 sx = (int16_t)cx;
    s32 sy = (int16_t)cy;
    s32 sz = (int16_t)cz;
    u32 squared = (u32)(sx * sx + sy * sy + sz * sz);
    if (squared == 0u) { *nx = 0; *ny = 0; *nz = 0; return; }

    /* WRONG: use squared as divisor (not sqrt) */
    int64_t sq = (int64_t)squared;
    *nx = (s32)(((int64_t)sx * 4096) / sq);
    *ny = (s32)(((int64_t)sy * 4096) / sq);
    *nz = (s32)(((int64_t)sz * 4096) / sq);
}

static void run_mutation_test(void)
{
    const golden_t *g = &s_goldens[0];
    oracle_t oracle = oracle_geometry(g);
    s32 swapped_cross[3];
    s32 swapped_normal[3] = { 0, 4096, 0 };

    swapped_cross[0] = cross_component((int64_t)oracle.edge0[1] * oracle.edge1[2] -
                                       (int64_t)oracle.edge0[2] * oracle.edge1[1]);
    swapped_cross[1] = cross_component((int64_t)oracle.edge0[2] * oracle.edge1[0] -
                                       (int64_t)oracle.edge0[0] * oracle.edge1[2]);
    swapped_cross[2] = cross_component((int64_t)oracle.edge0[0] * oracle.edge1[1] -
                                       (int64_t)oracle.edge0[1] * oracle.edge1[0]);

    check("mutation: swapped winding changes cross",
          memcmp(swapped_cross, g->cross, sizeof(swapped_cross)) != 0);
    check("mutation: swapped winding changes normalized output",
          memcmp(swapped_normal, g->normal, sizeof(swapped_normal)) != 0);
    check("mutation: swapped winding concrete Y", swapped_normal[1] == 4096 &&
          g->normal[1] == -4096);

    /* Normalization-scale mutation: use golden B (nontrivial slope) */
    {
        const golden_t *gb = &s_goldens[1]; /* B positive slope */
        oracle_t ob = oracle_geometry(gb);
        s32 mnx, mny, mnz;
        mutant_normalize_scale(ob.cross[0], ob.cross[1], ob.cross[2],
                               &mnx, &mny, &mnz);
        check("mutation: wrong normalization scale changes normal",
              mnx != ob.normal[0] || mny != ob.normal[1] || mnz != ob.normal[2]);
    }

    /* Normalization-rounding mutation: use golden E (nontrivial) */
    {
        const golden_t *ge = &s_goldens[4]; /* E mixed flag0 high */
        oracle_t oe = oracle_geometry(ge);
        s32 mnx, mny, mnz;
        mutant_normalize_squared(oe.cross[0], oe.cross[1], oe.cross[2],
                                 &mnx, &mny, &mnz);
        check("mutation: wrong magnitude normalization changes normal",
              mnx != oe.normal[0] || mny != oe.normal[1] || mnz != oe.normal[2]);
    }
}

/* The caller at 0x800749AC supplies physical scratchpad address 0x1F800030. */
static void run_scratch_output_test(void)
{
    const golden_t *g = &s_goldens[0];
    oracle_t oracle = oracle_geometry(g);
    uint8_t expected[0x60];
    u32 cell_addr;
    u32 ret_bits;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0xA5, 4096);
    store_s32(PSX_ADDR(WM_TERRAIN_STRIDE_ABS), 1);
    store_u32(PSX_ADDR(WM_TERRAIN_TABLE_ABS), TEST_GRID_BASE);
    store_s32(PSX_ADDR(WM_COEFF_0_ABS), g->coeff[0]);
    store_s32(PSX_ADDR(WM_COEFF_1_ABS), g->coeff[1]);
    store_s32(PSX_ADDR(WM_COEFF_2_ABS), g->coeff[2]);
    store_s32(PSX_ADDR(WM_COEFF_3_ABS), g->coeff[3]);

    cell_addr = fixture_cell_address(g->x, g->z);
    memset(PSX_ADDR(cell_addr), 0x5A, 44);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0, &g->h00, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 1, &g->flag, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 4, &g->h10, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0x24, &g->h01, 1);
    memcpy((uint8_t *)PSX_ADDR(cell_addr) + 0x28, &g->h11, 1);

    memset(expected, 0xA5, sizeof(expected));
    for (int i = 0; i < 3; i++) {
        store_s32(expected + i * 4, oracle.edge0[i]);
        store_s32(expected + 0x10 + i * 4, oracle.edge1[i]);
        store_s32(expected + 0x20 + i * 4, oracle.cross[i]);
        store_s32(expected + 0x30 + i * 4, oracle.normal[i]);
    }

    ret_bits = s32_to_bits(wm_80093740(0x1F800030u, g->x, g->z));
    check("scratch output: return vs oracle", ret_bits == oracle.return_bits);
    check("scratch output: exact vectors/normal/canaries",
          memcmp(g_PsxScratchpad, expected, sizeof(expected)) == 0);
    check("scratch output: normal vs oracle",
          memcmp(g_PsxScratchpad + 0x30, oracle.normal, 12) == 0);
}

int main(void)
{
    fprintf(stderr, "=== W34B5-O: wm_80093740 production-linked test ===\n");

    for (u32 i = 0; i < (u32)(sizeof(s_goldens) / sizeof(s_goldens[0])); i++)
        run_golden(&s_goldens[i]);
    run_mutation_test();
    run_scratch_output_test();

    fprintf(stderr, "=== Results: %d PASS / %d TOTAL ===\n",
            s_pass, s_pass + s_fail);
    return s_fail == 0 ? 0 : 1;
}
