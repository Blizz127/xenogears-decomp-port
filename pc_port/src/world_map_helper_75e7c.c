/* Retail world-map transition selector [0x80075E7C,0x80076098) and its
 * single-use terrain-cell selector leaf [0x80094028,0x80094060). */
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_75e7c.h"
#include "world_map_helper_93f18.h"
#include "world_map_terrain_cell.h"

#define WM_75E7C_AREA_REMAP       0x8009A3A0u
#define WM_75E7C_THRESHOLDS       0x8009B57Au
#define WM_75E7C_RECORD_BASES     0x8009D73Cu

/* Main-executable BSS, not world-overlay guest storage.  Native field/battle
 * consumers read these symbols directly after the world session exits. */
extern u8 D_800658DC[];
extern u8 D_80059508;

static u8 wm75e_lbu(u32 address)
{
    return *(const u8 *)PSX_ADDR(address);
}

static u16 wm75e_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm75e_lh(u32 address) __attribute__((unused));
static s16 wm75e_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm75e_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm75e_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

s32 wm_80094028(u32 vec_addr)
{
    s32 x = wm75e_bits_to_s32(wm75e_lw(vec_addr));
    s32 z = wm75e_bits_to_s32(wm75e_lw(vec_addr + 8u));
    u32 cell = wm_80093660(x, z);
#if defined(W34N18_MUTANT_CELL_BYTE_OFFSET)
    u32 packed = (u32)wm75e_lbu(cell + 2u);
#else
    u32 packed = (u32)wm75e_lbu(cell + 3u);
#endif
#if defined(W34N18_MUTANT_CELL_SHIFT)
    return (s32)((packed >> 1u) & 0x0Fu);
#else
    return (s32)((packed >> 2u) & 0x0Fu);
#endif
}

s32 wm_80075E7C(u32 vec_addr, s32 threshold)
{
    u8 weights[16];
    s32 selector = (s32)(s16)wm_80094028(vec_addr);
    s32 area = (s32)(s16)wm_80093F18(vec_addr);
    u32 bucket = 0u;
    u32 record_base;
    u32 weight_addr;
    s32 weight_sum = 0;
    s32 remainder;
    u32 selected = 0u;
    u32 i;

#if defined(W34N18_MUTANT_SKIP_AREA_REMAP)
    (void)area;
#else
    if (area == 4) {
        u32 offset = (u32)selector << 1u;
        selector = (s32)wm75e_lh(WM_75E7C_AREA_REMAP + offset);
    }
#endif

#if defined(W34N18_MUTANT_WRONG_BUCKET)
    while (threshold > (s32)wm75e_lhu(WM_75E7C_THRESHOLDS + bucket * 2u))
        bucket++;
#else
    while (threshold >= (s32)wm75e_lhu(WM_75E7C_THRESHOLDS + bucket * 2u))
        bucket++;
#endif

    record_base = wm75e_lw(WM_75E7C_RECORD_BASES +
                           ((u32)selector << 2u));
    weight_addr = record_base + 0x200u + bucket * 0x10u;
    for (i = 0u; i < 16u; i++) {
        weights[i] = wm75e_lbu(weight_addr + i);
        weight_sum += (s32)weights[i];
    }

    if (weight_sum <= 0) {
#if defined(W34N18_MUTANT_ZERO_SUM_WRITES)
        D_80059508 = 0u;
#endif
        return 0;
    }

    remainder = rand() % weight_sum;
#if defined(W34N18_MUTANT_UNWEIGHTED_PICK)
    selected = (u32)remainder & 0x0Fu;
#else
    while (selected < 16u) {
        s32 weight = (s32)weights[selected];
        if (weight != 0 && remainder < weight)
            break;
        remainder -= weight;
        selected++;
    }
#endif

#if defined(W34N18_MUTANT_COPY_RECORD_AREA)
    memcpy(D_800658DC, PSX_ADDR(record_base + 0x200u), 0x200u);
#elif defined(W34N18_MUTANT_SHORT_COPY)
    memcpy(D_800658DC, PSX_ADDR(record_base), 0x1F0u);
#else
    memcpy(D_800658DC, PSX_ADDR(record_base), 0x200u);
#endif
    D_80059508 = (u8)selected;
    return 1;
}
