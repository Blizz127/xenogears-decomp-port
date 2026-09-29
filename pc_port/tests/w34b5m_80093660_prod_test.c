/*
 * W34B5-M — Production-linked test for world terrain cell lookup 0x80093660.
 *
 * Links the ACTUAL production helper (pc_port/src/world_map_terrain_cell.c).
 * Uses an independent oracle that computes expected guest pointers from a
 * high-level spec formulation WITHOUT calling the production helper.
 *
 * The oracle uses a mathematically independent formulation:
 *   - Wide (int64_t) intermediates
 *   - Biased division expressed as standalone (v+7)>>3 formula
 *   - 0x800 wrap via normalized modulo, not AND
 *   - s16 sign-extension via mask+subtract, not cast chain
 *   - Different control structure / expression order from production
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5m_80093660_prod_test.c \
 *     pc_port/src/world_map_terrain_cell.c \
 *     -o pc_port/build_native/w34b5m_80093660_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "psx_memory.h"
#include "world_map_terrain_cell.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

void PsxMemory_Init(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
}

/* =====================================================================
 * INDEPENDENT SPEC ORACLE
 *
 * Formulates the retail 0x80093660 computation as a high-level
 * mathematical specification.  Does NOT reproduce production's
 * bitwise decomposition, branch sequence, or cast chain.
 *
 * Key independence properties:
 *   A. Uses int64_t intermediates throughout
 *   B. Biased division is a standalone formula, not a copy of production's
 *      if/shift sequence
 *   C. Local coordinate extraction uses normalized modulo 2048, not AND 0x7FF
 *   D. Quadrant/cell decomposition uses comparison + subtraction, not the
 *      same >= 1024 / -= 1024 / >>= 4 / biased >>= 3 sequence
 *   E. s16 sign-extension uses mask + conditional subtract, not the
 *      (s32)(s16)(u16)(u32) cast chain
 *   F. 32-bit wrapping is explicit modulo 2^32 at each boundary
 *   G. No shared arithmetic helpers with production
 * ===================================================================== */

#define TERRAIN_STRIDE_ADDR_SPEC  0x8009D160u
#define TERRAIN_TABLE_ADDR_SPEC   0x8009C184u

/* Tile-coordinate division by 8 — truncation toward zero.
 *
 * The retail code implements:
 *     if (v < 0) v += 7;
 *     v >>= 3;
 *
 * This is the standard MIPS pattern for signed integer division by a
 * power-of-two with truncation toward zero (not floor).
 *
 * Proof:
 *   For v >= 0: no bias applied, v >>= 3 = floor(v/8) = truncation toward zero.
 *   For v < 0: bias +7 then sra-3.
 *     v = -8k (exact multiple): (−8k+7)>>3 = floor(−k + 7/8) = −k = v/8.
 *     v = −8k+r, r∈[−7,−1]: (−8k+r+7)>>3 = floor(−k + (r+7)/8) = −k
 *       since (r+7)∈[0,6] and (r+7)/8∈[0,0.75]. So result = −k = ceil(v/8)
 *       = truncation-toward-zero of v/8.
 *
 * Verified for: v=-17→-2, v=-9→-1, v=-8→-1, v=-1→0, v=0→0, v=1→0, v=7→0,
 *               v=8→1, v=15→1, v=16→2, v=127→15, v=128→16.
 *
 * Formulation uses C integer division (truncation toward zero per C99 §6.5.5),
 * which matches the retail result for every signed 32-bit input.
 * This is a DIFFERENT formulation from production's if/shift sequence. */
static inline int32_t spec_tile_div8(int64_t v)
{
    return (int32_t)(v / 8);
}

/* Sign-extend a 32-bit value to signed 16 bits.
 * Independent formulation: mask to 16 bits, then subtract 0x10000 if
 * bit 15 is set.  Does NOT use production's (s32)(s16)(u16)(u32) chain.
 *
 * Proof: for v16 = v & 0xFFFF:
 *   if v16 < 0x8000: result = v16 (positive, correct)
 *   if v16 >= 0x8000: result = v16 - 0x10000 (maps [0x8000,0xFFFF] to [-32768,-1])
 * This is equivalent to the MIPS sll-by-16 + sra-by-16 sequence. */
static inline int32_t spec_sign_extend_16(int64_t v)
{
    int32_t v16 = (int32_t)(v & 0xFFFF);
    if (v16 >= 0x8000)
        v16 -= 0x10000;
    return v16;
}

/* Independent oracle: high-level spec formulation of wm_80093660.
 *
 * Architecture:
 *   1. Wide arithmetic throughout (int64_t)
 *   2. Biased division via standalone formula
 *   3. Local coordinate via normalized modulo (not AND)
 *   4. Quadrant via comparison + subtraction (not same branch sequence)
 *   5. Cell index via integer division (not shifted biased division)
 *   6. s16 via mask+subtract (not cast chain)
 *   7. 32-bit wrapping via explicit modulo at register boundaries */
static u32 oracle_80093660(s32 x, s32 z)
{
    /* ---- Step 1: Raw tile coordinates ----
     * Arithmetic right shift by 20 extracts the tile-scale coordinate. */
    int64_t coarse_z_raw = (int64_t)(z >> 20);
    int64_t coarse_x_raw = (int64_t)(x >> 20);

    /* ---- Step 2: Truncation-toward-zero tile-coordinate division by 8 ----
     * Applies the retail addiu+sra sequence as a single C division.
     * The product (coarse_z * stride) uses the already-divided coarse_z.
     * coarse_x is also divided before the addition. */
    int64_t coarse_z_biased = spec_tile_div8(coarse_z_raw);
    int64_t coarse_x_biased = spec_tile_div8(coarse_x_raw);

    /* ---- Step 3: Tile stride index ----
     * tile_idx = coarse_z * stride + coarse_x (32-bit wrapping).
     * Uses wide signed multiplication, then wrap to u32. */
    int64_t stride = (int64_t)*(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC);
    int64_t tile_idx_64 = coarse_z_biased * stride + coarse_x_biased;
    u32 tile_idx = (u32)tile_idx_64;

    /* ---- Step 4: 16-bit sign extension ----
     * Independent formulation: mask to 16 bits, conditional subtract. */
    int32_t tile_idx_s16 = spec_sign_extend_16((int64_t)tile_idx);

    /* ---- Step 5: Local coordinates ----
     * Extract bits [22:12] as a non-negative value in [0, 2047].
     * Uses normalized modulo instead of AND 0x7FF. */
    int64_t local_x = ((int64_t)(x >> 12) % 2048 + 2048) % 2048;
    int64_t local_z = ((int64_t)(z >> 12) % 2048 + 2048) % 2048;

    /* ---- Step 6: Quadrant ----
     * Independent decomposition: comparison gives 0 or 1, subtraction
     * adjusts the local coordinate.  Different structure from production's
     * if (>= 1024) { quadrant = N; local -= 1024; } sequence. */
    int64_t qx = (local_x >= 1024) ? 1 : 0;
    int64_t qz = (local_z >= 1024) ? 1 : 0;
    int64_t quadrant = qx + 2 * qz;
    int64_t adj_x = local_x - qx * 1024;
    int64_t adj_z = local_z - qz * 1024;

    /* ---- Step 7: Cell index within quadrant ----
     * Each cell is 128 local-units wide (9 cells per axis per quadrant).
     * Uses integer division on guaranteed-non-negative values.
     * Independent from production's >> 4 / biased >> 3 sequence. */
    int64_t cell_x = adj_x / 128;
    int64_t cell_z = adj_z / 128;
    int64_t cell_idx = cell_z * 9 + cell_x;

    /* ---- Step 8: Table lookup and final address ----
     * Load quadrant base from terrain table at signed-16-bit index.
     * Compute final guest pointer.  32-bit wrapping via u32 arithmetic. */
    u32 table_offset = (u32)((u32)tile_idx_s16 << 2);
    u32 quadrant_base = *(u32 *)PSX_ADDR(TERRAIN_TABLE_ADDR_SPEC + table_offset);

    return quadrant_base + (u32)(cell_idx * 4) + (u32)(quadrant * 0x144);
}

/* ---- Fixture setup ---- */

/* Terrain patch: 4 quadrants × 9×9 cells × 4 bytes = 0x510 bytes.
 * Place at a high guest address to test guest-pointer behavior. */
#define PATCH_GUEST_ADDR  0x801C0000u
#define PATCH_SIZE        0x510  /* 4 * 0x144 */

static void setup_fixture(s32 stride, u32 patch_guest_addr, s32 table_index)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = stride;

    /* Place patch guest address in table at given index. */
    u32 entry_addr = TERRAIN_TABLE_ADDR_SPEC + (u32)(table_index * 4);
    *(u32 *)PSX_ADDR(entry_addr) = patch_guest_addr;

    /* Fill patch with known pattern. */
    u8 *patch = (u8 *)PSX_ADDR(patch_guest_addr);
    for (int i = 0; i < PATCH_SIZE; i++)
        patch[i] = (u8)(i & 0xFF);
}

static int verify_no_writes(u32 patch_guest_addr)
{
    u8 *patch = (u8 *)PSX_ADDR(patch_guest_addr);
    for (int i = 0; i < PATCH_SIZE; i++) {
        if (patch[i] != (u8)(i & 0xFF))
            return 0;
    }
    if (*(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) == 0)
        return 0;
    return 1;
}

/* ---- Test infrastructure ---- */

/* Safe left-shift for signed values: cast to unsigned before shift to avoid UB.
 * Equivalent to the two's-complement bit pattern result. */
#define SAFE_SHIFT_LEFT_20(v) ((s32)((u32)(v) << 20))

static int s_pass, s_fail;

#define CHECK(desc, expected, actual) do { \
    if ((expected) == (actual)) { \
        s_pass++; \
    } else { \
        s_fail++; \
        fprintf(stderr, "FAIL: %s\n  expected: 0x%08X\n  actual:   0x%08X\n", \
                desc, (u32)(expected), (u32)(actual)); \
    } \
} while(0)

int main(void)
{
    PsxMemory_Init();

    fprintf(stderr, "=== W34B5-MV: wm_80093660 production-linked test (independent oracle) ===\n\n");

    /*
     * Coordinate guide:
     *   local = (coord >> 12) mod 2048   (always in [0,2047])
     *   cell  = local / 128              (after quadrant subtraction)
     *   Each cell is 128 local-units wide.
     *   Quadrant boundary at local = 1024.
     *   To set local=L, use coord = L * 4096.
     */

    /* ---- GOLDEN VECTORS (hard-coded, independently derived) ---- */

    /* Golden 1: x=-1, z=-1  (Sol's required negative boundary)
     *
     * Derivation (high-level spec, NOT from production):
     *   coarse_z_raw = -1 >> 20 = -1
     *   coarse_x_raw = -1 >> 20 = -1
     *   coarse_z = biased_div8(-1) = (-1+7)>>3 = 6>>3 = 0
     *   coarse_x = biased_div8(-1) = 0
     *   tile_idx = 0*32 + 0 = 0
     *   tile_idx_s16 = 0
     *   local_x = ((-1>>12) % 2048 + 2048) % 2048 = ((-1)%2048+2048)%2048
     *           = (-1+2048)%2048 = 2047
     *   local_z = 2047
     *   qx = 1 (2047 >= 1024), qz = 1 → quadrant = 3
     *   adj_x = 2047-1024 = 1023, adj_z = 1023
     *   cell_x = 1023/128 = 7, cell_z = 1023/128 = 7
     *   cell_idx = 7*9+7 = 70
     *   quadrant_base = table[0] = PATCH_GUEST_ADDR
     *   final = PATCH + 70*4 + 3*0x144 = PATCH + 0x118 + 0x3CC = PATCH + 0x4E4 */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -1, z = -1;
        u32 expected = PATCH_GUEST_ADDR + 0x4E4;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=-1 z=-1 (quad=3, cell 7,7)", expected, actual);
        CHECK("Golden: x=-1 z=-1 oracle", expected, oracle_80093660(x, z));
        CHECK("Golden: x=-1 z=-1 no-write", 1, verify_no_writes(PATCH_GUEST_ADDR));
    }

    /* Golden 2: x=-1, z=0
     *   coarse_x = biased_div8(-1) = 0, coarse_z = 0
     *   tile_idx = 0, local_x = 2047, local_z = 0
     *   quadrant = 1, cell_x = 7, cell_z = 0
     *   final = PATCH + 7*4 + 1*0x144 = PATCH + 0x1C + 0x144 = PATCH + 0x160 */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -1, z = 0;
        u32 expected = PATCH_GUEST_ADDR + 0x160;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=-1 z=0 (quad=1, cell_x=7)", expected, actual);
        CHECK("Golden: x=-1 z=0 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 3: x=0, z=-1
     *   coarse_x = 0, coarse_z = biased_div8(-1) = 0
     *   tile_idx = 0, local_x = 0, local_z = 2047
     *   quadrant = 2, cell_x = 0, cell_z = 7
     *   final = PATCH + 63*4 + 2*0x144 = PATCH + 0xFC + 0x288 = PATCH + 0x384 */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = -1;
        u32 expected = PATCH_GUEST_ADDR + 0x384;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=0 z=-1 (quad=2, cell_z=7)", expected, actual);
        CHECK("Golden: x=0 z=-1 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 4: x=-8, z=-8
     *   Same as x=-1,z=-1: coarse values are 0, local values are 2047.
     *   -8 >> 20 = -1 (arithmetic shift), biased_div8(-1) = 0.
     *   (-8>>12) = -1, mod 2048 = 2047.
     *   Same result as Golden 1. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -8, z = -8;
        u32 expected = PATCH_GUEST_ADDR + 0x4E4;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=-8 z=-8 (same as -1,-1)", expected, actual);
        CHECK("Golden: x=-8 z=-8 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 5: x=-9, z=-9
     *   coarse_x_raw = -9>>20 = -1, biased_div8(-1) = 0.
     *   Same as above. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -9, z = -9;
        u32 expected = PATCH_GUEST_ADDR + 0x4E4;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=-9 z=-9 (same as -1,-1)", expected, actual);
        CHECK("Golden: x=-9 z=-9 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 6: x=0, z=0 (origin) */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = 0;
        u32 expected = PATCH_GUEST_ADDR;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=0 z=0 (origin)", expected, actual);
        CHECK("Golden: x=0 z=0 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 7: x=1, z=1 (smallest positive) */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 1, z = 1;
        u32 expected = PATCH_GUEST_ADDR;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: x=1 z=1 (smallest)", expected, actual);
        CHECK("Golden: x=1 z=1 oracle", expected, oracle_80093660(x, z));
    }

    /* Golden 8: tile_idx_s16 = -32768 (UB trigger value)
     * stride=32768, coarse_z=1, coarse_x=0 → tile_idx=32768=0x8000.
     * s16(0x8000) = -32768.
     * Table at 0x8009C184 + (u32)(-32768 << 2) = 0x8009C184 + 0xFFFE0000
     * = 0x8007C184 (wraps at 32 bits). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32768;
        *(u32 *)PSX_ADDR(0x8007C184u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = 8 << 20;
        u32 expected = PATCH_GUEST_ADDR;
        u32 actual = wm_80093660(x, z);
        CHECK("Golden: tile_idx_s16=-32768 (UB trigger)", expected, actual);
        CHECK("Golden: tile_idx_s16=-32768 oracle", expected, oracle_80093660(x, z));
    }

    /* ---- EXISTING CASES (kept for regression coverage) ---- */

    /* Case 1: Interior cell (2,2) in quadrant 0.
     * x = 257*4096 = 0x101000 → local_x=257, cell_x=2
     * z = 258*4096 = 0x102000 → local_z=258, cell_z=2
     * cell_idx = 2*9+2 = 20 */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 257 * 4096, z = 258 * 4096;
        u32 expected = PATCH_GUEST_ADDR + 20 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 1: interior cell (2,2) quad 0", expected, actual);
        CHECK("Case 1: oracle match", expected, oracle_80093660(x, z));
        CHECK("Case 1: no-write proof", 1, verify_no_writes(PATCH_GUEST_ADDR));
    }

    /* Case 2: Adjacent X cell → cell (3,2). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 384 * 4096, z = 258 * 4096;
        u32 expected = PATCH_GUEST_ADDR + (2 * 9 + 3) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 2: adjacent X cell (3,2) quad 0", expected, actual);
        CHECK("Case 2: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 3: Adjacent Z cell → cell (2,3). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 257 * 4096, z = 384 * 4096;
        u32 expected = PATCH_GUEST_ADDR + (3 * 9 + 2) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 3: adjacent Z cell (2,3) quad 0", expected, actual);
        CHECK("Case 3: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 4: Exact X cell boundary (127→128). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 127 * 4096, z = 0;
        u32 actual4a = wm_80093660(x, z);
        CHECK("Case 4a: local_x=127 → cell_x=0", PATCH_GUEST_ADDR + 0, actual4a);

        x = 128 * 4096;
        u32 actual4b = wm_80093660(x, z);
        CHECK("Case 4b: local_x=128 → cell_x=1", PATCH_GUEST_ADDR + 1 * 4, actual4b);
    }

    /* Case 5: Exact Z cell boundary. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = 127 * 4096;
        u32 actual5a = wm_80093660(x, z);
        CHECK("Case 5a: local_z=127 → cell_z=0", PATCH_GUEST_ADDR + 0, actual5a);

        z = 128 * 4096;
        u32 actual5b = wm_80093660(x, z);
        CHECK("Case 5b: local_z=128 → cell_z=1", PATCH_GUEST_ADDR + 9 * 4, actual5b);
    }

    /* Case 6: Quadrant X transition (1023→1024). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 1023 * 4096, z = 0;
        u32 actual6a = wm_80093660(x, z);
        CHECK("Case 6a: local_x=1023 quad=0 cell_x=7",
              PATCH_GUEST_ADDR + 7 * 4, actual6a);

        x = 1024 * 4096;
        u32 actual6b = wm_80093660(x, z);
        CHECK("Case 6b: local_x=1024 quad=1 cell_x=0",
              PATCH_GUEST_ADDR + 0x144 + 0, actual6b);
    }

    /* Case 7: Quadrant Z transition (1023→1024). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = 1023 * 4096;
        u32 actual7a = wm_80093660(x, z);
        CHECK("Case 7a: local_z=1023 quad=0 cell_z=7",
              PATCH_GUEST_ADDR + 7 * 9 * 4, actual7a);

        z = 1024 * 4096;
        u32 actual7b = wm_80093660(x, z);
        CHECK("Case 7b: local_z=1024 quad=2 cell_z=0",
              PATCH_GUEST_ADDR + 2 * 0x144, actual7b);
    }

    /* Case 8: Negative X (-4096). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -4096, z = 0;
        u32 expected = PATCH_GUEST_ADDR + 0x144 + 7 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 8: negative X (quad=1, cell_x=7)", expected, actual);
        CHECK("Case 8: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 9: Negative Z (-4096). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = -4096;
        u32 expected = PATCH_GUEST_ADDR + 2 * 0x144 + 7 * 9 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 9: negative Z (quad=2, cell_z=7)", expected, actual);
        CHECK("Case 9: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 10: Both negative. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = -4096, z = -4096;
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (7 * 9 + 7) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 10: both negative (quad=3)", expected, actual);
        CHECK("Case 10: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 11: Signed16 tile index (bit 15 set). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32768;
        *(u32 *)PSX_ADDR(0x8007C184u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 cx = 0, cz = 8 << 20;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(cx, cz);
        CHECK("Case 11: signed16 tile index (bit15 set)", expected, actual);
        CHECK("Case 11: oracle match", expected, oracle_80093660(cx, cz));
    }

    /* Case 12: Zero terrain table entry. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        s32 x = 257 * 4096, z = 258 * 4096;
        u32 expected = 0 + 20 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 12: zero table entry → offset only", expected, actual);
    }

    /* Case 13: Smallest coordinate (x=1, z=1). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 1, z = 1;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 13: smallest coordinate (1,1)", expected, actual);
    }

    /* Case 14: Max tile-0 coordinate. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 2047 * 4096, z = 2047 * 4096;
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (7 * 9 + 7) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 14: max tile-0 (quad=3, cell 7,7)", expected, actual);
        CHECK("Case 14: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 15: Non-unit stride. stride=100. */
    {
        setup_fixture(100, PATCH_GUEST_ADDR, 6);
        s32 x = 50 << 20, z = 0;
        u32 expected = PATCH_GUEST_ADDR + 4 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 15: stride=100, tile 6, cell_x=4", expected, actual);
        CHECK("Case 15: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 16: Negative coarse_z → 0 after +7>>3. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = -(1 << 20);
        u32 expected = PATCH_GUEST_ADDR + 2 * 0x144 + (6 * 9 + 0) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 16: negative coarse_z (quad=2, cell_z=6)", expected, actual);
        CHECK("Case 16: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 17: Quadrant 3 (both upper). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 1500 * 4096, z = 1500 * 4096;
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (3 * 9 + 3) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 17: quadrant 3 (both upper)", expected, actual);
        CHECK("Case 17: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 18: Negative coarse_x → tile_idx=-1. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009C180u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = -(9 << 20), z = 0;
        u32 expected = PATCH_GUEST_ADDR + 0x144 + 6 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 18: negative coarse_x=-1 (quad=1, cell_x=6)", expected, actual);
        CHECK("Case 18: oracle match", expected, oracle_80093660(x, z));
    }

    /* ---- EXPANDED NEGATIVE / BOUNDARY COVERAGE ---- */

    /* Case 19: INT32_MIN coordinate. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x80094184u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = (s32)0x80000000u;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 19: INT32_MIN z", expected, actual);
        CHECK("Case 19: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 20: INT32_MAX coordinate. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x800A4104u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = 0x7FFFFFFF;
        u32 expected = PATCH_GUEST_ADDR + 2 * 0x144 + 7 * 9 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 20: INT32_MAX z", expected, actual);
        CHECK("Case 20: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 21: tile_idx_s16 = -32768 (exact UB trigger value). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32768;
        *(u32 *)PSX_ADDR(0x8007C184u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = 8 << 20;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 21: tile_idx_s16=-32768 (UB trigger)", expected, actual);
        CHECK("Case 21: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 22: tile_idx_s16 = 32767 (max positive s16). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32767;
        *(u32 *)PSX_ADDR(0x800BC180u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = 8 << 20;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 22: tile_idx_s16=32767 (max positive)", expected, actual);
        CHECK("Case 22: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 24: Hard-coded bit-pattern assertion for s16 UB fix. */
    {
        s32 tidx = -32768;
        u32 shifted = (u32)((u32)tidx << 2);
        CHECK("Case 24a: (u32)-32768 << 2 = 0xFFFE0000", 0xFFFE0000u, shifted);

        u32 table_addr = 0x8009C184u + shifted;
        CHECK("Case 24b: table addr wraps to 0x8007C184", 0x8007C184u, table_addr);
    }

    /* Case 25: Hard-coded bit-pattern for tile_idx_s16 = -1. */
    {
        s32 tidx = -1;
        u32 shifted = (u32)((u32)tidx << 2);
        CHECK("Case 25a: (u32)-1 << 2 = 0xFFFFFFFC", 0xFFFFFFFCu, shifted);

        u32 table_addr = 0x8009C184u + shifted;
        CHECK("Case 25b: table addr wraps to 0x8009C180", 0x8009C180u, table_addr);
    }

    /* Case 26: Hard-coded bit-pattern for tile_idx_s16 = 0. */
    {
        s32 tidx = 0;
        u32 shifted = (u32)((u32)tidx << 2);
        CHECK("Case 26: (u32)0 << 2 = 0", 0u, shifted);
    }

    /* Case 27: Hard-coded bit-pattern for tile_idx_s16 = 1. */
    {
        s32 tidx = 1;
        u32 shifted = (u32)((u32)tidx << 2);
        CHECK("Case 27: (u32)1 << 2 = 4", 4u, shifted);
    }

    /* Case 28: Negative coarse_z = -1. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009C104u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0, z = -(8 << 20);
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 28: coarse_z=-1 (quad=0, cell_z=0)", expected, actual);
        CHECK("Case 28: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 29: Negative both axes, different magnitudes. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009BFFCu) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = -(17 << 20), z = -(25 << 20);
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (6 * 9 + 6) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 29: both negative, diff magnitudes (quad=3)", expected, actual);
        CHECK("Case 29: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 30: Origin (0,0). */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = 0;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 30: origin (0,0)", expected, actual);
        CHECK("Case 30: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 31: Large positive coordinates. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009C4B4u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 100 << 20, z = 50 << 20;
        u32 expected = PATCH_GUEST_ADDR + 1 * 0x144 + 36 * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 31: large positive (tile 204, quad=1)", expected, actual);
        CHECK("Case 31: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 32: Mixed signs (negative x, positive z). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009C204u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = -(5 << 20), z = 10 << 20;
        u32 expected = PATCH_GUEST_ADDR + (4 * 9 + 6) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 32: negative x, positive z (mixed)", expected, actual);
        CHECK("Case 32: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 33: tile_idx=0 explicit. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z = 0;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 33: tile_idx=0 explicit", expected, actual);
        CHECK("Case 33: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 34: Coarse boundary at z=8M. */
    {
        setup_fixture(32, PATCH_GUEST_ADDR, 0);
        s32 x = 0, z_below = (8 << 20) - 1;
        u32 actual_below = wm_80093660(x, z_below);
        CHECK("Case 34a: z=8M-1 → coarse_z=0, quad=2, cell_z=7",
              PATCH_GUEST_ADDR + 2 * 0x144 + 63 * 4, actual_below);

        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x8009C204u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 z_at = 8 << 20;
        u32 actual_at = wm_80093660(x, z_at);
        CHECK("Case 34b: z=8M → coarse_z=1", PATCH_GUEST_ADDR + 0, actual_at);
        CHECK("Case 34b: oracle match", actual_at, oracle_80093660(x, z_at));
    }

    /* Case 35: Cell boundary sweep — cell boundaries in X within quad 0. */
    {
        for (int k = 0; k < 8; k++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 lx = k * 128;
            s32 x = lx * 4096, z = 0;
            u32 expected_addr = PATCH_GUEST_ADDR + k * 4;
            u32 actual = wm_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc),
                     "Case 35: cell boundary X local=%d → cell=%d quad=0", lx, k);
            CHECK(desc, expected_addr, actual);
        }
    }

    /* Case 36: All 4 quadrant corners. */
    {
        struct { s32 lx, lz; int q; } quads[] = {
            { 0, 0, 0 }, { 1024, 0, 1 }, { 0, 1024, 2 }, { 1024, 1024, 3 }
        };
        for (int qi = 0; qi < 4; qi++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = quads[qi].lx * 4096, z = quads[qi].lz * 4096;
            u32 expected = PATCH_GUEST_ADDR + quads[qi].q * 0x144;
            u32 actual = wm_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc),
                     "Case 36: quad corner (%d,%d) → quad=%d",
                     quads[qi].lx, quads[qi].lz, quads[qi].q);
            CHECK(desc, expected, actual);
            {
                char desc2[80];
                snprintf(desc2, sizeof(desc2),
                         "Case 36: quad corner (%d,%d) oracle",
                         quads[qi].lx, quads[qi].lz);
                CHECK(desc2, expected, oracle_80093660(x, z));
            }
        }
    }

    /* ---- BOUNDARY SWEEP ---- */
    /* Sweep around meaningful transitions for both X and Z. */

    /* Case 37: X sweep around -2049..-2047 */
    {
        for (int dx = -2049; dx <= -2047; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 37: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 38: X sweep around -1025..-1023 */
    {
        for (int dx = -1025; dx <= -1023; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 38: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 39: X sweep around -129..-127 */
    {
        for (int dx = -129; dx <= -127; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 39: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 40: X sweep around -9..-7 */
    {
        for (int dx = -9; dx <= -7; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 40: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 41: X sweep around -2..2 */
    {
        for (int dx = -2; dx <= 2; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 41: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 42: X sweep around 7..9 */
    {
        for (int dx = 7; dx <= 9; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 42: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 43: X sweep around 127..129 */
    {
        for (int dx = 127; dx <= 129; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 43: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 44: X sweep around 1023..1025 */
    {
        for (int dx = 1023; dx <= 1025; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 44: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 45: X sweep around 2047..2049 */
    {
        for (int dx = 2047; dx <= 2049; dx++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = SAFE_SHIFT_LEFT_20(dx), z = 0;
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 45: X sweep x=%d<<20", dx);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 46: Z sweep around -2049..-2047 */
    {
        for (int dz = -2049; dz <= -2047; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 46: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 47: Z sweep around -1025..-1023 */
    {
        for (int dz = -1025; dz <= -1023; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 47: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 48: Z sweep around -129..-127 */
    {
        for (int dz = -129; dz <= -127; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 48: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 49: Z sweep around -9..-7 */
    {
        for (int dz = -9; dz <= -7; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 49: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 50: Z sweep around -2..2 */
    {
        for (int dz = -2; dz <= 2; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 50: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 51: Z sweep around 7..9 */
    {
        for (int dz = 7; dz <= 9; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 51: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 52: Z sweep around 127..129 */
    {
        for (int dz = 127; dz <= 129; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 52: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 53: Z sweep around 1023..1025 */
    {
        for (int dz = 1023; dz <= 1025; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 53: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 54: Z sweep around 2047..2049 */
    {
        for (int dz = 2047; dz <= 2049; dz++) {
            setup_fixture(32, PATCH_GUEST_ADDR, 0);
            s32 x = 0, z = SAFE_SHIFT_LEFT_20(dz);
            u32 prod = wm_80093660(x, z);
            u32 spec = oracle_80093660(x, z);
            char desc[80];
            snprintf(desc, sizeof(desc), "Case 54: Z sweep z=%d<<20", dz);
            CHECK(desc, prod, spec);
        }
    }

    /* Case 55: Combined XZ sweep around quadrant transitions.
     * X in {1023,1024,1025}, Z in {1023,1024,1025}. */
    {
        int xvals[] = { 1023, 1024, 1025 };
        int zvals[] = { 1023, 1024, 1025 };
        for (int xi = 0; xi < 3; xi++) {
            for (int zi = 0; zi < 3; zi++) {
                setup_fixture(32, PATCH_GUEST_ADDR, 0);
                s32 x = xvals[xi] * 4096, z = zvals[zi] * 4096;
                u32 prod = wm_80093660(x, z);
                u32 spec = oracle_80093660(x, z);
                char desc[80];
                snprintf(desc, sizeof(desc),
                         "Case 55: XZ sweep (%d,%d)", xvals[xi], zvals[zi]);
                CHECK(desc, prod, spec);
            }
        }
    }

    /* Case 56: Combined negative XZ sweep.
     * X in {-9,-8,-7,-1,0,1}, Z in {-9,-8,-7,-1,0,1}. */
    {
        int vals[] = { -9, -8, -7, -1, 0, 1 };
        for (int xi = 0; xi < 6; xi++) {
            for (int zi = 0; zi < 6; zi++) {
                setup_fixture(32, PATCH_GUEST_ADDR, 0);
                s32 x = SAFE_SHIFT_LEFT_20(vals[xi]), z = SAFE_SHIFT_LEFT_20(vals[zi]);
                u32 prod = wm_80093660(x, z);
                u32 spec = oracle_80093660(x, z);
                char desc[80];
                snprintf(desc, sizeof(desc),
                         "Case 56: neg XZ (%d,%d)", vals[xi], vals[zi]);
                CHECK(desc, prod, spec);
            }
        }
    }

    /* ---- EXTREME VALUES ---- */

    /* Case 57: INT32_MIN for both x and z.
     * tile_idx = coarse_z(-2048)*32 + coarse_x(-2048) = -256*32 + (-256) = -8448.
     * (s16)-8448 = -8448.
     * Table at 0x8009C184 + (u32)(-8448<<2) = 0x8009C184 + 0xFFFF7C00
     * = 0x80093D84 (wraps at 32 bits).
     * local_x = local_z = 0. quadrant=0, cell=(0,0). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x80093D84u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = (s32)0x80000000u, z = (s32)0x80000000u;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 57: INT32_MIN both", expected, actual);
        CHECK("Case 57: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 58: INT32_MAX for both x and z.
     * tile_idx = 255*32 + 255 = 8160+255 = 8415.
     * (s16)8415 = 8415.
     * Table at 0x8009C184 + 8415*4 = 0x8009C184 + 0x837C = 0x800A4500.
     * local_x = local_z = 2047. quadrant=3, cell=(7,7). */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x800A4500u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0x7FFFFFFF, z = 0x7FFFFFFF;
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (7 * 9 + 7) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 58: INT32_MAX both", expected, actual);
        CHECK("Case 58: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 59: INT32_MIN+1 for both.
     * coarse_z_raw = (0x80000001)>>20 = -2048 (still -2048 for nearby values).
     * Actually: 0x80000001>>20 = 0xFFFFF800 = -2048. Same as INT32_MIN.
     * local_z = (0x80000001>>12)&... = (0xFFFFF800001>>12) mod 2048.
     * 0x80000001>>12 = 0xFFFFF800001... wait, that's 64-bit.
     * In 32-bit: 0x80000001 >> 12 = 0xFFFF80000... no, 0x80000001 is 32-bit.
     * 0x80000001 >> 12 = 0xFFFFF800001 >> 12... let me just compute:
     * 0x80000001 = -2147483647.
     * -2147483647 >> 12 = -524288 (arithmetic shift).
     * -524288 mod 2048 = (-524288 % 2048 + 2048) % 2048.
     * -524288 / 2048 = -256 exactly. -524288 % 2048 = 0.
     * So local_z = 0.
     * Same as INT32_MIN case. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x80093D84u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = (s32)0x80000001u, z = (s32)0x80000001u;
        u32 expected = PATCH_GUEST_ADDR + 0;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 59: INT32_MIN+1 both", expected, actual);
        CHECK("Case 59: oracle match", expected, oracle_80093660(x, z));
    }

    /* Case 60: INT32_MAX-1 for both.
     * 0x7FFFFFFE >> 20 = 2047. Same coarse as INT32_MAX.
     * (0x7FFFFFFE>>12) mod 2048 = (0x7FFFFFFE>>12 = 0x7FFFF) mod 2048.
     * 0x7FFFF = 524287. 524287 mod 2048 = 524287 - 255*2048 = 524287-522240 = 2047.
     * Same as INT32_MAX. */
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(s32 *)PSX_ADDR(TERRAIN_STRIDE_ADDR_SPEC) = 32;
        *(u32 *)PSX_ADDR(0x800A4500u) = PATCH_GUEST_ADDR;
        u8 *patch = (u8 *)PSX_ADDR(PATCH_GUEST_ADDR);
        for (int i = 0; i < PATCH_SIZE; i++) patch[i] = (u8)(i & 0xFF);

        s32 x = 0x7FFFFFFE, z = 0x7FFFFFFE;
        u32 expected = PATCH_GUEST_ADDR + 3 * 0x144 + (7 * 9 + 7) * 4;
        u32 actual = wm_80093660(x, z);
        CHECK("Case 60: INT32_MAX-1 both", expected, actual);
        CHECK("Case 60: oracle match", expected, oracle_80093660(x, z));
    }

    /* ---- Results ---- */
    fprintf(stderr, "\n=== Results: %d PASS / %d TOTAL ===\n", s_pass, s_pass + s_fail);
    if (s_fail > 0)
        fprintf(stderr, "*** %d FAILURE(S) ***\n", s_fail);

    return s_fail > 0 ? 1 : 0;
}
