/*
 * World-map helper 0x80099708 (terrain vertex-grid producer).
 * Retail boundary: [0x80099708, 0x8009980C).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_99708.h"
#include "world_map_helper_9980c.h"

#define WM_99708_SCRATCH       0x1F800000u
#define WM_99708_ANGLE_X       0x8009C618u
#define WM_99708_ANGLE_Z       0x8009C5BCu
#define WM_99708_SINE_TABLE    0x800523F0u

static s16 wm_99708_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm_99708_lw(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_99708_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_99708_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_99708_sign8(u32 value)
{
    u32 low = value & 0xFFu;
    return low < 0x80u ? (s32)low : (s32)(low | 0xFFFFFF00u);
}


/* Base height term: retail 0x80099790-94 (`sll v1,v1,0x18; sra v1,v1,0x15`)
 * and 0x800997AC-B0 (`sll v0,v1,0x18; sra v0,v0,5`) sign-extend the LOW byte
 * of the packed word and multiply by 8. The high byte is colour (see
 * 0x80099858-70 in wm_8009980C). */
static s32 wm_99708_base_height(u32 packed)
{
#if defined(WM_99708_MUTANT_HIGH_BYTE)      /* pre-W34C6 shape */
    return wm_99708_sign8(packed >> 24) * 8;
#else
    return wm_99708_sign8(packed) * 8;
#endif
}

void wm_80099708(u32 tile_data, u32 ot_base, u32 packet_base, u32 origin)
{
    u32 scratch = WM_99708_SCRATCH;
    u32 source = tile_data;
    s32 origin_x = wm_99708_lh(origin);
    s32 z = wm_99708_lh(origin + 4u);
    s32 angle_x = wm_99708_lw(WM_99708_ANGLE_X);
    s32 angle_z_base = wm_99708_lw(WM_99708_ANGLE_Z);
    u32 row;

    for (row = 0u; row < 9u; row++) {
        s32 x = origin_x;
        u32 sine_x_index = (u32)angle_x & 0xFFFu;
        s32 sine_x_twice =
            (s32)wm_99708_lh(WM_99708_SINE_TABLE + sine_x_index * 4u) * 2;
        s32 angle_z = angle_z_base;
        u32 column;

        for (column = 0u; column < 9u; column++) {
            u32 packed = (u32)wm_99708_lw(source);
            u32 vertex_y;

            if ((packed & 0x1000u) != 0u) {
                u32 sine_z_index = (u32)angle_z & 0xFFFu;
                s32 sine_z = wm_99708_lh(
                    WM_99708_SINE_TABLE + sine_z_index * 4u);
                int64_t product = (int64_t)sine_z * (int64_t)sine_x_twice;
                s32 height = (s32)(product >> 20) +
                             wm_99708_base_height(packed);
                vertex_y = (u32)height << 16;
            } else {
                vertex_y = (u32)wm_99708_base_height(packed) << 16;
            }

            wm_99708_sw(scratch, vertex_y | ((u32)x & 0xFFFFu));
            wm_99708_sh(scratch + 4u, (u16)z);
            scratch += 8u;
            source += 4u;
            x += 0x80;
            angle_z += 0x200;
        }

        z -= 0x80;
        angle_x += 0x200;
    }

    wm_8009980C(tile_data, ot_base, packet_base);
}
