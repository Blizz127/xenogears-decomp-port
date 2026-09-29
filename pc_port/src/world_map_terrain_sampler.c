/*
 * World-map terrain height sampler 0x80093978.
 *
 * Retail boundary: [0x80093978, 0x80093A5C), 0xE4 bytes / 57 instructions.
 * Leaf orchestrator: calls wm_80093660, wm_80093740, wm_800935DC.
 *
 * Given signed world X/Z coordinates, samples the terrain height by:
 *   1. Looking up the terrain cell (wm_80093660)
 *   2. Computing the surface normal (wm_80093740)
 *   3. Solving the plane equation (wm_800935DC)
 *   4. Scaling the result left by 3
 *
 * Returns signed terrain height in 15-bit fixed-point format.
 */
#include "world_map_terrain_sampler.h"
#include "world_map_terrain_cell.h"
#include "world_map_terrain_normal.h"
#include "world_map_plane_solver.h"
#include "psx_memory.h"

#include <string.h>

/*
 * Retail stack-record addresses.  In retail these are sp-relative; in the
 * PC port we use fixed guest-memory addresses since the world-map context
 * is single-threaded.
 */
#define WM93978_QUERY_ADDR   0x801C0200u   /* sp+16: query {local_x, result, -local_z} */
#define WM93978_BASE_ADDR    0x801C0210u   /* sp+32: base  {base_x, base_y, base_z}     */
#define WM93978_NORMAL_ADDR  0x801C0220u   /* sp+48: normal (12 bytes, from wm_80093740) */

static s32 wm_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void wm_store_s32(u32 addr, s32 value)
{
    memcpy(PSX_ADDR(addr), &value, sizeof(value));
}

static s32 wm_load_s8(u32 addr)
{
    int8_t value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

/* Retail LB followed by SLL 12, expressed entirely as bit operations. */
static s32 wm_height_sll12(s32 height)
{
    u32 height_bits;
    memcpy(&height_bits, &height, sizeof(height_bits));
    return wm_bits_to_s32(height_bits << 12);
}

static u8 wm_load_u8(u32 addr)
{
    u8 value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

s32 wm_80093978(s32 x, s32 z)
{
    /* Cell lookup: wm_80093660(x, z) */
    u32 cell_addr = wm_80093660(x, z);

    /* ---- Local coordinate computation ---- */
    /* Retail: biased floor-divide-by-8, masked to 16 bits.
     * For negative values: add 7 then arithmetic-right-shift by 3.
     * Equivalent to (u32)(x/8) & 0xFFFF after 16-bit masking. */
    u32 local_x = (u32)(x / 8) & UINT32_C(0xFFFF);
    u32 local_z = (u32)(z / 8) & UINT32_C(0xFFFF);
    u32 neg_local_z = 0u - local_z;   /* SUBU: 0 - local_z */

    /* Write query record: {local_x, ?, -local_z} */
    wm_store_s32(WM93978_QUERY_ADDR + 0, wm_bits_to_s32(local_x));
    /* sp+20 (result slot) will be written by wm_800935DC */
    wm_store_s32(WM93978_QUERY_ADDR + 8, wm_bits_to_s32(neg_local_z));

    /* ---- Flag-based base-point construction ---- */
    u8 flag = wm_load_u8(cell_addr + 1u);

    if ((flag & 0x80u) != 0u) {
        /* flag1: base at h00 corner */
        wm_store_s32(WM93978_BASE_ADDR + 0, 0);           /* base_x = 0 */
        wm_store_s32(WM93978_BASE_ADDR + 4,
                     wm_height_sll12(wm_load_s8(cell_addr))); /* base_y = h00 << 12 */
        wm_store_s32(WM93978_BASE_ADDR + 8, 0);           /* base_z = 0 */
    } else {
        /* flag0: base at h10 corner */
        wm_store_s32(WM93978_BASE_ADDR + 0,
                     (s32)UINT32_C(0x10000));               /* base_x = 0x10000 */
        wm_store_s32(WM93978_BASE_ADDR + 4,
                     wm_height_sll12(wm_load_s8(cell_addr + 4u))); /* base_y = h10 << 12 */
        wm_store_s32(WM93978_BASE_ADDR + 8, 0);           /* base_z = 0 */
    }

    /* ---- Surface normal via wm_80093740 ---- */
    wm_80093740(WM93978_NORMAL_ADDR, x, z);

    /* ---- Plane-height solve via wm_800935DC ---- */
    wm_800935DC(WM93978_QUERY_ADDR, WM93978_BASE_ADDR, WM93978_NORMAL_ADDR);

    /* ---- Final scaling: load solved Y, shift left by 3 ---- */
    /* Retail loads from sp+20 (the store slot), not from v0 return.
     * wm_800935DC writes and returns the same value, so both are equivalent. */
    u32 result_bits;
    memcpy(&result_bits, PSX_ADDR(WM93978_QUERY_ADDR + 4u), sizeof(result_bits));

    /* SLL v0,v0,3 — unsigned shift to avoid signed left-shift UB */
    u32 scaled_bits = result_bits << 3;

    return wm_bits_to_s32(scaled_bits);
}

/* Entry for the matched C in src/world_map/main.c (0x80093978 is not
 * matched yet); both arguments are scalars. */
s32 func_80093978(s32 x, s32 z)
{
    return wm_80093978(x, z);
}
