/*
 * World-map helper 0x800981C8 (tile index window builder).
 * Retail boundary: [0x800981C8, 0x800983A0), 118 instructions. Leaf.
 *
 * W34C5 re-derivation from disc/world_map.bin (retail PCs cited inline).
 * Writes the 9x9 window of global tile numbers (tz*width + tx) into
 * D_8009D570, wrapping both axes against the world width/height, after
 * saving the previous window (162 bytes) to D_8009D318.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_981c8.h"

#define D_8009D160  0x8009D160u  /* world width in tiles  (0x800981D4 lw) */
#define D_8009D2B4  0x8009D2B4u  /* world height in tiles (0x800981DC lw) */
#define D_8009C838  0x8009C838u  /* map_x (s16)           (0x800981F8 lh) */
#define D_8009C83C  0x8009C83Cu  /* map_z (s16)           (0x8009821C lh) */
#define D_8009D318  0x8009D318u  /* previous-window copy  (0x80098280) */
#define D_8009D570  0x8009D570u  /* tile index window     (0x80098288) */

static s16 t8_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 t8_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void t8_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

static s32 t8_sra(s32 value, unsigned shift)
{
    /* MIPS sra: arithmetic shift of the two's-complement value. */
    return value < 0 ? -(((-value) - 1) >> shift) - 1 : value >> shift;
}

void wm_800981C8(u32 pos_vec)
{
    s32 width = t8_lw(D_8009D160);            /* t2 */
    s32 height = t8_lw(D_8009D2B4);           /* t4 */
    s32 x_tile, z_tile, tx0, tz, tx;
    s32 row, col;

    /* 0x800981E0-0x800981F0: x_tile = (x sra 12), +7 if negative, sra 3. */
    x_tile = t8_sra(t8_lw(pos_vec), 12);
    if (x_tile < 0)
        x_tile += 7;
    x_tile = t8_sra(x_tile, 3);
    /* 0x80098208-0x80098222: same for z (pos+8). */
    z_tile = t8_sra(t8_lw(pos_vec + 8u), 12);
    if (z_tile < 0)
        z_tile += 7;
    z_tile = t8_sra(z_tile, 3);

    /* 0x80098210 / 0x80098230: relative to (map + 2) << 8. */
    tx0 = x_tile - (((s32)t8_lh(D_8009C838) + 2) << 8);
    tz = z_tile - (((s32)t8_lh(D_8009C83C) + 2) << 8);

    /* 0x8009822C-0x80098250: wrap x; negative adds, strictly-greater subtracts. */
    if (tx0 < 0)
        tx0 += width << 8;
    else if ((width << 8) < tx0)
        tx0 -= width << 8;
    /* 0x80098254-0x80098274: wrap z the same way. */
    if (tz < 0)
        tz += height << 8;
    else if ((height << 8) < tz)
        tz -= height << 8;
#if defined(WM_981C8_MUTANT_M7)             /* non-strict wrap compare */
    if (tx0 == (width << 8))
        tx0 = 0;
#endif

    /* 0x80098278 / 0x80098298: sra 8. */
    tx0 = t8_sra(tx0, 8);
    tz = t8_sra(tz, 8);

    /* 0x8009829C-0x8009832C: save the previous window, 0xA0 bytes plus one
     * trailing halfword (162 bytes). */
    memcpy(PSX_ADDR(D_8009D318), PSX_ADDR(D_8009D570), 0xA0u);
    t8_sh(D_8009D318 + 0xA0u, (u16)t8_lh(D_8009D570 + 0xA0u));

    /* 0x80098330-0x8009839C: 9 rows x 9 columns (counters 8 down to -1). */
    {
        u32 out = D_8009D570;
#if defined(WM_981C8_MUTANT_M1)             /* old shape: 8x8 window */
        const s32 rows = 8, cols = 8;
#else
        const s32 rows = 9, cols = 9;
#endif
        for (row = 0; row < rows; row++) {
            s32 row_base;
#if !defined(WM_981C8_MUTANT_M2)            /* 0x80098344-0x80098350 */
            if (!(tz < height))
                tz = 0;
#endif
            row_base = tz * width;                /* 0x8009835C mult */
            tx = tx0;                             /* 0x8009834C move a3,t3 */
            for (col = 0; col < cols; col++) {
                if (!(tx < width))                /* 0x80098364-0x80098370 */
                    tx = 0;
                t8_sh(out, (u16)(row_base + tx)); /* 0x80098378 */
                out += 2u;
                tx++;                             /* 0x80098388 */
            }
            tz++;                                 /* 0x80098394 */
        }
    }
}
