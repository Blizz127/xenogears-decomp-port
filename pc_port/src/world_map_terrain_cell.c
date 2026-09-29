/*
 * W34B5-M: World-map terrain grid-cell pointer lookup 0x80093660.
 *
 * Exact transcription of retail 0x80093660–0x8009373C.
 * Leaf function: no calls, no stores, no allocation.
 *
 * Given signed fixed-point X/Z world coordinates, computes the
 * 32-bit guest address of the terrain cell sample containing that point
 * within the quadrant-organized terrain grid.
 *
 * Reads globals:
 *   0x8009D160  (s32)   terrain row stride
 *   0x8009C184  (u32[]) terrain pointer table (indexed by signed16-bit tile index)
 *
 * Both the production game build and the production-linked test link
 * this same object.  Do NOT duplicate this function elsewhere.
 */
#include "psx_memory.h"
#include "world_map_terrain_cell.h"

/* Retail global addresses (signed-immediate derivation from lui 0x800A). */
#define WM_TERRAIN_STRIDE_ABS   0x8009D160u  /* s32: lui 0x800A + addiu -0x2EA0 */
#define WM_TERRAIN_TABLE_ABS    0x8009C184u  /* u32[]: lui 0x800A + addiu -0x3E7C */

u32 wm_80093660(s32 x, s32 z)
{
    s32 coarse_z, coarse_x;
    s32 stride;
    s32 tile_idx;
    s32 local_x, local_z;
    s32 quadrant;
    s32 cell_x, cell_z, cell_idx;
    s32 tile_idx_s16;
    u32 quadrant_base;

    /* ---- Coarse tile coordinates ----
     * MIPS branch-delay ordering:
     *   80093660: sra v0, a1, 20          → v0 = z >> 20
     *   80093664: bgez v0, 0x80093670     → branch if v0 >= 0
     *   80093668: [delay] sra a2, a0, 20  → a2 = x >> 20 (always)
     *   8009366C: addiu v0, v0, 7         → v0 += 7 (only if v0 < 0)
     *   80093670: ...
     *   80093678: sra v0, v0, 3           → v0 >>= 3 (coarse_z FINAL)
     *   8009367C: bgez a2, 0x80093688     → branch if a2 >= 0
     *   80093680: [delay] mult v0, v1     → (always, result used later)
     *   80093684: addiu a2, a2, 7         → a2 += 7 (only if a2 < 0)
     *   80093688: ...
     *   80093698: sra v0, a2, 3           → v0 = a2 >> 3 (coarse_x FINAL)
     */

    /* z coarse tile */
    coarse_z = z >> 20;
    if (coarse_z < 0)
        coarse_z += 7;
    coarse_z >>= 3;

    /* x coarse tile (delay slot of z bgez; raw value before adjustment) */
    coarse_x = x >> 20;

    /* ---- Tile index = coarse_z * stride + coarse_x ---- */

    /* 80093674: lw v1, -0x2EA0(v1)  →  load stride from 0x8009D160 */
    stride = *(s32*)PSX_ADDR(WM_TERRAIN_STRIDE_ABS);

    /* 80093680: mult v0, v1  — in delay slot of bgez a2.
     * At this point, v0 = coarse_z (already ÷8'd) but a2 = coarse_x
     * has NOT yet been adjusted (+7) or ÷8'd.  The mult uses raw a2.
     * Result is consumed at 8009369C (mflo) and 800936A0 (addu a2,t0,v0),
     * which come AFTER coarse_x is adjusted and ÷8'd at 80093684/80093698.
     * So: product = coarse_z * stride (correct), and the addu adds the
     * ADJUSTED coarse_x (after +7 and >>3). */
    {
        s64 prod = (s64)coarse_z * (s64)stride;
        s32 product_lo = (s32)(u32)prod;

        /* Now adjust and ÷8 coarse_x (80093684 + 80093698) */
        if (coarse_x < 0)
            coarse_x += 7;
        coarse_x >>= 3;

        tile_idx = (s32)((u32)product_lo + (u32)coarse_x);
    }

    /* ---- Local coordinates within tile (12-bit sub-tile position) ---- */

    /* 80093688: sra v0, a0, 12; 8009368C: andi a0, v0, 0x7FF */
    local_x = (x >> 12) & 0x7FF;

    /* 80093690: sra v0, a1, 12; 80093694: andi a1, v0, 0x7FF */
    local_z = (z >> 12) & 0x7FF;

    /* ---- Quadrant selection (each quadrant covers 1024 units) ---- */

    quadrant = 0;
    if (local_x >= 1024) {
        quadrant = 1;
        local_x -= 1024;
    }
    if (local_z >= 1024) {
        quadrant |= 2;
        local_z -= 1024;
    }

    /* ---- Cell index within quadrant (9×9 grid, 16-unit cells) ---- */

    /* z-component: cell_z = (local_z >> 4) adjusted-toward-zero ÷ 8 */
    cell_z = local_z >> 4;
    if (cell_z < 0)
        cell_z += 7;
    cell_z >>= 3;

    /* x-component: cell_x = (local_x >> 4) adjusted-toward-zero ÷ 8 */
    cell_x = local_x >> 4;
    if (cell_x < 0)
        cell_x += 7;
    cell_x >>= 3;

    /* 800936E4-800936F8: cell_idx = cell_z * 9 + cell_x */
    cell_idx = cell_z * 9 + cell_x;

    /* ---- Table lookup: tile index → quadrant base pointer ---- */

    /* 800936FC-80093708: sign-extend tile_idx to 16 bits.
     * sll a1, a2, 16; sra a1, a1, 16  →  sign_extend16(tile_idx) */
    tile_idx_s16 = (s32)(s16)(u16)(u32)tile_idx;

    /* 8009370C-80093714: load base from table.
     * sll v0, a1, 2  →  tile_idx_s16 * 4 (byte offset).
     * MIPS sll operates on register bit pattern; cast to u32 before
     * shifting to avoid C UB when tile_idx_s16 is negative. */
    quadrant_base = *(u32*)PSX_ADDR(WM_TERRAIN_TABLE_ABS +
                                    (u32)((u32)tile_idx_s16 << 2));

    /* ---- Compute final cell guest address ---- */

    /* 8009370C-80093718: quadrant_offset = quadrant * 0x144
     * 0x144 = 9 * 9 * 4 = 324 bytes per quadrant */
    /* 80093728: cell_offset = cell_idx * 4 */

    /* 80093734: addu a0, a0, v1  →  quadrant_base + cell_offset
     * 8009373C: addu v0, a0, v0  →  + quadrant_offset */
    return quadrant_base + (u32)(cell_idx * 4) + (u32)(quadrant * 0x144);
}
