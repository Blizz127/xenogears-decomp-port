/*
 * World-map helper 0x8009932C (terrain rendering context and dispatch).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_9932c.h"
#include "world_map_helper_99708.h"

#define WM_9932C_SCRATCH       0x1F800000u
#define WM_9932C_SHADE_SOURCE  0x8009CCB4u
#define WM_9932C_COLOR_SOURCE  0x8009CD54u
#define WM_9932C_MATRIX_SOURCE 0x8009D534u
#define WM_9932C_CAMERA        0x8009C808u
#define WM_9932C_MAP_X         0x8009C838u
#define WM_9932C_MAP_Z         0x8009C83Cu
#define WM_9932C_TILE_INDEX    0x8009D570u
#define WM_9932C_SKIP_TABLE    0x8009D618u
#define WM_9932C_QUAD_TABLE    0x8009D650u
#define WM_9932C_TILE_POINTERS 0x8009C184u
#define WM_9932C_PACKET_COUNT  0x8009D7DCu

static s16 wm_9932c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_9932c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_9932c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_9932c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_9932c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_9932c_dispatch(u32 tile_data, u32 ot_base, u32 packet_base,
                              u32 origin)
{
    u32 packet_count = wm_9932c_lw(WM_9932C_PACKET_COUNT);
    wm_80099708(tile_data, ot_base, packet_base + packet_count * 0x20u,
                origin);
}

static void wm_9932c_set_cell_origins(u16 x, u16 z)
{
    wm_9932c_sh(WM_9932C_SCRATCH + 0x330u, (u16)(x + 0x400u));
    wm_9932c_sh(WM_9932C_SCRATCH + 0x334u, z);
    wm_9932c_sh(WM_9932C_SCRATCH + 0x338u, x);
    wm_9932c_sh(WM_9932C_SCRATCH + 0x33Cu, (u16)(z - 0x400u));
    wm_9932c_sh(WM_9932C_SCRATCH + 0x340u, (u16)(x + 0x400u));
    wm_9932c_sh(WM_9932C_SCRATCH + 0x344u, (u16)(z - 0x400u));
}

void wm_8009932C(u32 ot_base, u32 packet_base, u32 position)
{
    u32 i;
    s32 start_x;
    s32 start_z;
    u32 cell = 0u;
    u32 grid_z;

    for (i = 0u; i < 64u; i++) {
        wm_9932c_sh(WM_9932C_SCRATCH + 0x288u + i * 2u,
                    wm_9932c_lhu(WM_9932C_SHADE_SOURCE + i * 2u));
    }
    for (i = 0u; i < 7u; i++) {
        wm_9932c_sh(WM_9932C_SCRATCH + 0x308u + i * 2u,
                    wm_9932c_lhu(WM_9932C_COLOR_SOURCE + i * 2u));
    }
    memcpy(PSX_ADDR(WM_9932C_SCRATCH + 0x350u),
           PSX_ADDR(WM_9932C_MATRIX_SOURCE), 32u);

    (void)CompMatrix((MATRIX *)PSX_ADDR(WM_9932C_CAMERA),
                     (MATRIX *)PSX_ADDR(WM_9932C_SCRATCH + 0x350u),
                     (MATRIX *)PSX_ADDR(WM_9932C_SCRATCH + 0x370u));
    SetRotMatrix((MATRIX *)PSX_ADDR(WM_9932C_SCRATCH + 0x370u));
    SetTransMatrix((MATRIX *)PSX_ADDR(WM_9932C_SCRATCH + 0x370u));

    start_x = -(s32)(((u32)(s32)wm_9932c_lw(position) >> 12) & 0x7FFu) -
              0x1000;
    start_z = -(s32)(((u32)(s32)wm_9932c_lw(position + 8u) >> 12) & 0x7FFu) -
              0x1000;
    wm_9932c_sw(WM_9932C_SCRATCH + 0x318u, (u32)start_x);
    wm_9932c_sw(WM_9932C_SCRATCH + 0x320u, (u32)start_z);
    wm_9932c_sw(WM_9932C_PACKET_COUNT, 0u);
    wm_9932c_sh(WM_9932C_SCRATCH + 0x32Cu, (u16)(0u - (u32)start_z));

    for (grid_z = 0u; grid_z < 5u; grid_z++) {
        u32 grid_x;

        wm_9932c_sh(WM_9932C_SCRATCH + 0x328u, (u16)start_x);
        for (grid_x = 0u; grid_x < 5u; grid_x++, cell++) {
            s16 skip = wm_9932c_lh(WM_9932C_SKIP_TABLE + cell * 2u);
            u16 x = wm_9932c_lhu(WM_9932C_SCRATCH + 0x328u);

            if (skip != -1) {
                u16 z = wm_9932c_lhu(WM_9932C_SCRATCH + 0x32Cu);
                s32 map_z = (s32)grid_z + wm_9932c_lh(WM_9932C_MAP_Z);
                s32 map_x = (s32)grid_x + wm_9932c_lh(WM_9932C_MAP_X);
                s32 tile_slot = map_z * 9 + map_x;
                s16 tile_index = wm_9932c_lh(
                    WM_9932C_TILE_INDEX + (u32)tile_slot * 2u);
                u32 tile_base = wm_9932c_lw(
                    WM_9932C_TILE_POINTERS + (u32)(s32)tile_index * 4u);
                u16 q0 = wm_9932c_lhu(WM_9932C_QUAD_TABLE + cell * 8u);
                u16 q1 = wm_9932c_lhu(WM_9932C_QUAD_TABLE + cell * 8u + 2u);
                u16 q2 = wm_9932c_lhu(WM_9932C_QUAD_TABLE + cell * 8u + 4u);
                u16 q3 = wm_9932c_lhu(WM_9932C_QUAD_TABLE + cell * 8u + 6u);
                s16 combined = (s16)(q0 | q1 | q2 | q3);

                wm_9932c_set_cell_origins(x, z);
                if (combined != -1 || (s16)q0 != combined) {
                    wm_9932c_dispatch(tile_base, ot_base, packet_base,
                                      WM_9932C_SCRATCH + 0x328u);
                }
                if (combined != -1 || (s16)q1 != combined) {
                    wm_9932c_dispatch(tile_base + 0x144u, ot_base, packet_base,
                                      WM_9932C_SCRATCH + 0x330u);
                }
                if (combined != -1 || (s16)q2 != combined) {
                    wm_9932c_dispatch(tile_base + 0x288u, ot_base, packet_base,
                                      WM_9932C_SCRATCH + 0x338u);
                }
                if (combined != -1 || (s16)q3 != combined) {
                    wm_9932c_dispatch(tile_base + 0x3CCu, ot_base, packet_base,
                                      WM_9932C_SCRATCH + 0x340u);
                }
            }
#if defined(WM_9932C_MUTANT_SKIPPED_ORIGINS)
            else {
                wm_9932c_set_cell_origins(
                    x, wm_9932c_lhu(WM_9932C_SCRATCH + 0x32Cu));
            }
#endif

            wm_9932c_sh(WM_9932C_SCRATCH + 0x328u, (u16)(x + 0x800u));
        }
        wm_9932c_sh(WM_9932C_SCRATCH + 0x32Cu,
                    (u16)(wm_9932c_lhu(WM_9932C_SCRATCH + 0x32Cu) - 0x800u));
    }
}
