/*
 * World-map 5x5 wrapped-FT4 dispatch 0x8008615C.
 * Retail boundary: [0x8008615C, 0x800863E0).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_8615c.h"
#include "world_map_helper_99bfc.h"

#define WM_8615C_SCRATCH       0x1F800000u
#define WM_8615C_MODEL         0x8009A180u
#define WM_8615C_CLUT_TABLE    0x8009D478u
#define WM_8615C_TILE_MAP      0x8009D570u
#define WM_8615C_VALIDITY      0x8009D618u
#define WM_8615C_PACKET_ROOTS  0x8009D7E8u
#define WM_8615C_BUFFER_INDEX  0x8009D7F0u
#define WM_8615C_TILE_RECORDS  0x8009C7ECu
#define WM_8615C_MAP_X         0x8009C838u
#define WM_8615C_MAP_Z         0x8009C83Cu
#define WM_8615C_CAMERA        0x8009C808u
#define WM_8615C_HEADING       0x8009BD3Cu
#define WM_8615C_OUTPUT_COUNT  0x8009BE04u
#define WM_8615C_DRAW_RECORD   0x8009BE3Cu

static u16 wm_8615c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_8615c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_8615c_lwu(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_8615c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_8008615C(void)
{
    u32 row;
    u32 column;

    /* Four shared vertices used by wm_80099BFC. */
    wm_8615c_sh(WM_8615C_SCRATCH + 0x00u, (u16)(s16)-24);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x02u, (u16)(s16)-72);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x04u, 0u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x08u, 24u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x0Au, (u16)(s16)-72);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x0Cu, 0u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x10u, (u16)(s16)-24);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x12u, 0u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x14u, 0u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x18u, 24u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x1Au, 0u);
    wm_8615c_sh(WM_8615C_SCRATCH + 0x1Cu, 0u);

    memcpy(PSX_ADDR(WM_8615C_SCRATCH + 0x28u), PSX_ADDR(WM_8615C_CAMERA),
           sizeof(MATRIX));
    memcpy(PSX_ADDR(WM_8615C_SCRATCH + 0x48u), PSX_ADDR(WM_8615C_MODEL),
           sizeof(MATRIX));
#if defined(WM_8615C_MUTANT_POSITIVE_HEADING)
    (void)RotMatrixZ((int)wm_8615c_lh(WM_8615C_HEADING),
                     (MATRIX*)PSX_ADDR(WM_8615C_SCRATCH + 0x48u));
#else
    (void)RotMatrixZ(-(int)wm_8615c_lh(WM_8615C_HEADING),
                     (MATRIX*)PSX_ADDR(WM_8615C_SCRATCH + 0x48u));
#endif

    for (column = 0u;
#if defined(WM_8615C_MUTANT_EIGHT_CLUTS)
         column < 8u;
#else
         column < 16u;
#endif
         column++) {
        wm_8615c_sh(WM_8615C_SCRATCH + 0x68u + column * 2u,
                    wm_8615c_lhu(WM_8615C_CLUT_TABLE + column * 2u));
    }

    wm_8615c_sh(WM_8615C_OUTPUT_COUNT, 0u);
    for (row = 0u;
#if defined(WM_8615C_MUTANT_FOUR_BY_FOUR)
         row < 4u;
#else
         row < 5u;
#endif
         row++) {
        for (column = 0u;
#if defined(WM_8615C_MUTANT_FOUR_BY_FOUR)
             column < 4u;
#else
             column < 5u;
#endif
             column++) {
            u32 validity_index = row *
#if defined(WM_8615C_MUTANT_VALIDITY_STRIDE_9)
                                 9u + column;
#else
                                 5u + column;
#endif
            s16 validity = wm_8615c_lh(WM_8615C_VALIDITY +
                                       validity_index * 2u);
            s32 map_x;
            s32 map_z;
            s32 map_index;
            s16 tile_id;
            u32 tile_records;
            u32 record;
            u32 entries;
            s32 count;
            u32 draw_record;
            u32 ot_base;
            u32 buffer;
            u32 packet_root;
            u32 packet;

#if !defined(WM_8615C_MUTANT_SKIP_VALIDITY_GATE)
            if (validity == -1)
                continue;
#else
            (void)validity;
#endif
            map_x = (s32)wm_8615c_lh(WM_8615C_MAP_X) + (s32)column;
            map_z = (s32)wm_8615c_lh(WM_8615C_MAP_Z) + (s32)row;
#if defined(WM_8615C_MUTANT_MAP_WIDTH_8)
            map_index = map_z * 8 + map_x;
#else
            map_index = map_z * 9 + map_x;
#endif
            tile_id = wm_8615c_lh(WM_8615C_TILE_MAP +
                                   (u32)map_index * 2u);
            tile_records = wm_8615c_lwu(WM_8615C_TILE_RECORDS);
            record = tile_records + (u32)(s32)tile_id * 8u;
            entries = wm_8615c_lwu(record + 0u);
            count = (s32)wm_8615c_lwu(record + 4u);
            if (count == 0)
                continue;

            draw_record = wm_8615c_lwu(WM_8615C_DRAW_RECORD);
            ot_base = wm_8615c_lwu(draw_record + 0x70u);
            buffer = wm_8615c_lwu(WM_8615C_BUFFER_INDEX);
#if defined(WM_8615C_MUTANT_FIXED_PACKET_ROOT)
            (void)buffer;
            packet_root = wm_8615c_lwu(WM_8615C_PACKET_ROOTS);
#else
            packet_root = wm_8615c_lwu(WM_8615C_PACKET_ROOTS + buffer * 4u);
#endif
            packet = packet_root +
                     (u32)(s32)wm_8615c_lh(WM_8615C_OUTPUT_COUNT) * 0x28u;
            wm_80099BFC(entries, count, ot_base, packet);
        }
    }
}
