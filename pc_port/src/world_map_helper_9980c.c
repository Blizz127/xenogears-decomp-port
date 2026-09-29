/*
 * World-map helper 0x8009980C (terrain triangle submitter).
 * Retail boundary: [0x8009980C, 0x80099BFC).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_9980c.h"

#define WM_9980C_SCRATCH       0x1F800000u
#define WM_9980C_SHADE_TABLE   0x1F800288u
#define WM_9980C_COLOR_TABLE   0x1F800308u
#define WM_9980C_PACKET_COUNT  0x8009D7DCu

static u16 wm_9980c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_9980c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_9980c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_9980c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int wm_9980c_overlaps_screen(u32 xy0, u32 xy1, u32 xy2)
{
    u32 x0 = xy0 & 0xFFFFu;
    u32 x1 = xy1 & 0xFFFFu;
    u32 x2 = xy2 & 0xFFFFu;
    u32 y0 = (u32)(s32)(s16)(xy0 >> 16);
    u32 y1 = (u32)(s32)(s16)(xy1 >> 16);
    u32 y2 = (u32)(s32)(s16)(xy2 >> 16);

    return (x0 < 320u || x1 < 320u || x2 < 320u) &&
           (y0 < 216u || y1 < 216u || y2 < 216u);
}

static void wm_9980c_colors(u32 packed, u16 colors[4])
{
    u16 base = (u16)(((packed >> 12) & 0x00F0u) |
                     ((packed >> 8) & 0xF000u));

    switch ((packed >> 13) & 3u) {
    case 0u:
        colors[0] = base;
        colors[1] = (u16)(base + 0x000Fu);
        colors[2] = (u16)(base + 0x0F00u);
        colors[3] = (u16)(base + 0x0F0Fu);
        break;
    case 1u:
        colors[0] = (u16)(base + 0x000Fu);
        colors[1] = base;
        colors[2] = (u16)(base + 0x0F0Fu);
        colors[3] = (u16)(base + 0x0F00u);
        break;
    case 2u:
        colors[0] = (u16)(base + 0x0F00u);
        colors[1] = (u16)(base + 0x0F0Fu);
        colors[2] = base;
        colors[3] = (u16)(base + 0x000Fu);
        break;
    default:
        colors[0] = (u16)(base + 0x0F0Fu);
        colors[1] = (u16)(base + 0x0F00u);
        colors[2] = (u16)(base + 0x000Fu);
        colors[3] = base;
        break;
    }
}

static u32 wm_9980c_shaded_color(u32 packed, u16 base, int color_table)
{
    s32 ir0 = (s32)C2_IR0;
    u16 upper;

    if ((u32)ir0 >= 0x1000u)
        ir0 = 0x0FFF;

    if (color_table != 0) {
        u32 offset = (packed >> 7) & 0xEu;
        upper = wm_9980c_lhu(WM_9980C_COLOR_TABLE + offset);
    } else {
        u32 bank = (packed >> 5) & 0x40u;
        u32 shade_index = ((u32)ir0 >> 6) & 0x7Eu;
        upper = wm_9980c_lhu(WM_9980C_SHADE_TABLE + bank + shade_index);
    }
    return ((u32)upper << 16) | (u32)base;
}

static int wm_9980c_emit_triangle(u32 v0_address, u32 v1_address,
                                  u32 v2_address, u32 packed, u16 color0,
                                  u16 color1, u16 color2, u32 ot_base,
                                  u32 *packet_address, u32 *packet_count)
{
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long p = 0;
    long flag = 0;
    u16 max_depth;
    u32 ot_address;
    u32 old_tag;
    u32 packet;

    (void)RotTransPers3((SVECTOR *)PSX_ADDR(v0_address),
                        (SVECTOR *)PSX_ADDR(v1_address),
                        (SVECTOR *)PSX_ADDR(v2_address),
                        &xy0, &xy1, &xy2, &p, &flag);
    if ((s32)flag < 0) {
        return 0;
    }
    if (!wm_9980c_overlaps_screen((u32)xy0, (u32)xy1, (u32)xy2)) {
        return 0;
    }

    max_depth = (u16)C2_SZ1;
    if ((u16)C2_SZ2 > max_depth)
        max_depth = (u16)C2_SZ2;
    if ((u16)C2_SZ3 > max_depth)
        max_depth = (u16)C2_SZ3;
    if (max_depth >= 0x0F00u) {
        return 0;
    }
    if (NormalClip((int)(u32)xy0, (int)(u32)xy1, (int)(u32)xy2) <= 0) {
        return 0;
    }

    packet = *packet_address;
    ot_address = ot_base + ((u32)max_depth >> 4) * 4u;
    old_tag = wm_9980c_lw(ot_address);
    wm_9980c_sw(packet + 8u, (u32)xy0);
    wm_9980c_sw(packet + 0x10u, (u32)xy1);
    wm_9980c_sw(packet + 0x18u, (u32)xy2);
    wm_9980c_sw(packet + 0x0Cu,
                 wm_9980c_shaded_color(packed, color0, 0));
    wm_9980c_sw(packet + 0x14u,
                 wm_9980c_shaded_color(packed, color1, 1));
    wm_9980c_sh(packet + 0x1Cu, color2);
    wm_9980c_sw(packet, UINT32_C(0x07000000) | old_tag);
    wm_9980c_sw(ot_address, packet & 0x00FFFFFFu);
    *packet_address = packet + 0x20u;
    (*packet_count)++;
    return 1;
}

void wm_8009980C(u32 tile_data, u32 ot_base, u32 packet_base)
{
    u32 packet_count = wm_9980c_lw(WM_9980C_PACKET_COUNT);
    u32 packet = packet_base;
    u32 vertex = WM_9980C_SCRATCH;
    u32 source = tile_data;
    u32 row;

    for (row = 0u; row < 8u; row++) {
        u32 column;

        for (column = 0u; column < 8u; column++) {
            u32 packed = wm_9980c_lw(source);
            u16 colors[4];

            wm_9980c_colors(packed, colors);
            if (packet_count >= 0x7FEu)
                goto complete;
            /* Triangle A, retail 0x800998D8-0x8009990C: V0 = vertex,
             * V2 = vertex+0x48 (lwc2 $4/$5), V1 = vertex+0x50 when bit 15
             * is set (lwc2 $2/$3 at 0x800998F0) else vertex+8 (0x80099900).
             * Colours: +0xC = colors[0], +0x14 = colors[3] / colors[1],
             * +0x1C = colors[2]. */
            if ((packed & 0x8000u) != 0u) {
#if defined(WM_9980C_MUTANT_SWAPPED_FIRST)   /* pre-W34C7 shape */
                (void)wm_9980c_emit_triangle(
                    vertex, vertex + 0x48u, vertex + 0x50u, packed,
                    colors[0], colors[3], colors[2], ot_base, &packet,
                    &packet_count);
#else
                (void)wm_9980c_emit_triangle(
                    vertex, vertex + 0x50u, vertex + 0x48u, packed,
                    colors[0], colors[3], colors[2], ot_base, &packet,
                    &packet_count);
#endif
                (void)wm_9980c_emit_triangle(
                    vertex + 8u, vertex + 0x50u, vertex, packed,
                    colors[1], colors[3], colors[0], ot_base, &packet,
                    &packet_count);
            } else {
#if defined(WM_9980C_MUTANT_SWAPPED_FIRST)
                (void)wm_9980c_emit_triangle(
                    vertex, vertex + 0x48u, vertex + 8u, packed,
                    colors[0], colors[1], colors[2], ot_base, &packet,
                    &packet_count);
#else
                (void)wm_9980c_emit_triangle(
                    vertex, vertex + 8u, vertex + 0x48u, packed,
                    colors[0], colors[1], colors[2], ot_base, &packet,
                    &packet_count);
#endif
                (void)wm_9980c_emit_triangle(
                    vertex + 8u, vertex + 0x50u, vertex + 0x48u, packed,
                    colors[1], colors[3], colors[2], ot_base, &packet,
                    &packet_count);
            }
            source += 4u;
            vertex += 8u;
        }
        source += 4u;
        vertex += 8u;
    }

complete:
    wm_9980c_sw(WM_9980C_PACKET_COUNT, packet_count);
}
