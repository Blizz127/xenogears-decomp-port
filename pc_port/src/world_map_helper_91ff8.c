/*
 * World-map helper 0x80091FF8 (camera view setup with terrain sampling).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_91ff8.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_96f18.h"
#include "world_map_terrain_cell.h"

#define SCRATCH      0x1F800000u
#define HEAD_MIRROR  0x8009BD3Au
#define DIR_AREA     0x8009BD3Cu
#define D_8009D560   0x8009D560u
#define D_8009BD40   0x8009BD40u
#define D_8009BD44   0x8009BD44u
#define D_8009BE28   0x8009BE28u
#define D_8009BE30   0x8009BE30u
#define D_8009B214   0x8009B214u
#define D_8009B234   0x8009B234u
#define D_8009BCDC   0x8009BCDCu

static s16 f8_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 f8_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 f8_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void f8_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void f8_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static s8 f8_lb(u32 a) { return *(s8*)PSX_ADDR(a); }

s32 wm_80091FF8(s32 max_candidates, u32 rot_table, u32 height_table)
{
    s32 candidate = 0;
    s32 height_offset;
    s32 minimum_height = 0;
    height_offset = f8_lw(D_8009D560) >> 12;
    f8_sh(SCRATCH + 0xAAu, f8_lhu(HEAD_MIRROR));
    f8_sh(SCRATCH + 0xACu, f8_lhu(DIR_AREA));

    do {
        s32 scale = f8_lw(D_8009BCDC);
        s32 half_scale = scale / 2;
        s32 view_height = (s32)((u32)f8_lw(D_8009B214 +
                                                     (u32)candidate * 4u) -
                                ((u32)half_scale << 12));
        s32 grid_x_offset;
        s32 grid_z_offset;
        u32 grid_x_start;
        u32 grid_z;
        u32 row;

        f8_sh(SCRATCH + 0xA8u,
              f8_lhu(rot_table + (u32)candidate * 2u));
        wm_80096F18(D_8009BD40, D_8009BE28, view_height,
                    SCRATCH + 0xA8u);

        grid_x_offset = ((s32)f8_lh(D_8009BD40) - 0x180) * 0x1000;
        grid_z_offset = ((s32)f8_lh(D_8009BD44) + 0x180) * 0x1000;
#if defined(WM_91FF8_MUTANT_M1)
        grid_x_start = (u32)grid_x_offset & UINT32_C(0xFFF80000);
#else
        grid_x_start = ((u32)f8_lw(D_8009BE28) + (u32)grid_x_offset) &
                       UINT32_C(0xFFF80000);
#endif
#if defined(WM_91FF8_MUTANT_M2)
        grid_z = ((u32)f8_lw(D_8009BE30) + (u32)grid_z_offset) &
                 UINT32_C(0xFFF80000);
#else
        grid_z = ((u32)f8_lw(D_8009BE30) - (u32)grid_z_offset) &
                 UINT32_C(0xFFF80000);
#endif

#if defined(WM_91FF8_MUTANT_M3)
        minimum_height = INT32_MAX;
#else
        minimum_height = 0;
#endif
        f8_sw(SCRATCH + 8u, (s32)grid_z);
        for (row = 0u; row < 6u; row++) {
            u32 column;

            f8_sw(SCRATCH, (s32)grid_x_start);
            for (column = 0u; column < 6u; column++) {
                u32 sample;
                s32 terrain_height;

                wm_80093354(SCRATCH);
                sample = wm_80093660(f8_lw(SCRATCH),
                                     f8_lw(SCRATCH + 8u));
                terrain_height = (s32)f8_lb(sample);
                f8_sw(SCRATCH + 4u, terrain_height);
                if (terrain_height < minimum_height)
                    minimum_height = terrain_height;
                f8_sw(SCRATCH,
                      (s32)((u32)f8_lw(SCRATCH) + UINT32_C(0x80000)));
            }
            f8_sw(SCRATCH + 8u,
                  (s32)((u32)f8_lw(SCRATCH + 8u) + UINT32_C(0x80000)));
        }

        {
            s32 threshold = (s32)((u32)(s32)f8_lh(
                                      height_table + (u32)candidate * 2u) +
                                  (u32)height_offset + UINT32_C(0x50));
            s32 minimum_scaled = minimum_height * 8;

            if (threshold < minimum_scaled)
                break;
        }
        candidate++;
#if defined(WM_91FF8_MUTANT_M4)
    } while (candidate < 1);
#else
    } while (candidate < 3);
#endif

    if (candidate < max_candidates) {
        s32 table_value = f8_lh(D_8009B234 + (u32)max_candidates * 2u);
        s32 minimum_minus_80 = minimum_height * 8 - 0x50;
        s32 difference = (s32)((u32)table_value + (u32)height_offset -
                               (u32)minimum_minus_80);

        if (difference < 0)
            difference = (s32)(0u - (u32)difference);
#if !defined(WM_91FF8_MUTANT_M5)
        if (difference < 0x41)
            candidate = max_candidates;
#endif
    }
    return candidate;
}
