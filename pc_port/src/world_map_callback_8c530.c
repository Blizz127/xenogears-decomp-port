/*
 * World-map scheduler callback 0x8008C530.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008C530, 0x8008C6EC).  C364 initializes the channel-zero object and
 * position before this callback clears its private fields.  Signed world
 * mode then selects a common tail, an extended terrain/history publication,
 * or the mode-4-through-7 slot state.  Every path returns C364's result.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8c530.h"
#include "world_map_helper_c364.h"
#include "world_map_terrain_sampler.h"

#define WM_C530_POOL_PTR          0x8009BE24u
#define WM_C530_MODE              0x8009BE10u
#define WM_C530_HEADING           0x8006EE5Au
#define WM_C530_CHANNEL0_FLAG     0x8006F8E5u
#define WM_C530_SHARED_X          0x8009C5ACu
#define WM_C530_SHARED_Z          0x8009C5B4u
#define WM_C530_SHARED_HEADING    0x8009C584u
#define WM_C530_SNAPSHOT          0x8009D55Cu
#define WM_C530_COMMON_CLEAR      0x8009D154u
#define WM_C530_PUBLISHED_HEADING 0x8009D52Cu
#define WM_C530_HISTORY           0x8009CEC4u
#define WM_C530_PUBLISHED_X       0x8006EF90u
#define WM_C530_PUBLISHED_Z       0x8006EF92u

#define WM_C530_SLOT_CONTROL      0x20u
#define WM_C530_SLOT_FLAG         0x24u
#define WM_C530_SLOT_X            0x28u
#define WM_C530_SLOT_Y            0x2Cu
#define WM_C530_SLOT_Z            0x30u
#define WM_C530_SLOT_AUX          0x34u
#define WM_C530_SLOT_CLEAR38      0x38u
#define WM_C530_SLOT_CLEAR3C      0x3Cu
#define WM_C530_SLOT_CLEAR40      0x40u
#define WM_C530_SLOT_HEADING      0x48u
#define WM_C530_SLOT_CONST        0x4Au
#define WM_C530_SLOT_SIGNED_HEAD  0x5Cu

#define WM_C530_HISTORY_COUNT     32u
#define WM_C530_HISTORY_STRIDE    0x14u

enum wm_c530_mode_family {
    WM_C530_MODE_NONPOSITIVE = 0,
    WM_C530_MODE_1_TO_3,
    WM_C530_MODE_4_TO_7,
    WM_C530_MODE_8_OR_GREATER
};

static u8 wm_c530_load_u8(u32 address)
{
    return *(volatile const u8*)PSX_ADDR(address);
}

static u16 wm_c530_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_c530_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_c530_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_c530_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_c530_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_c530_s32_as_u32(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static s32 wm_c530_sign_extend_u16(u16 value)
{
    if (value <= 0x7FFFu)
        return (s32)value;
    return (s32)(u32)value - 0x10000;
}

/* MIPS SRA by 12, expressed entirely with defined unsigned C operations. */
static u32 wm_c530_sra12(u32 value)
{
    u32 result = value >> 12;
    if ((value & 0x80000000u) != 0u)
        result |= 0xFFF00000u;
    return result;
}

/* Retail's BLEZ/SLTI chain partitions the signed mode before any optional
 * global access.  Keep that distinction explicit even though the <=0 and
 * >=8 families currently converge on the same common-tail effects. */
static enum wm_c530_mode_family wm_c530_classify_mode(s32 mode)
{
    if (mode <= 0)
        return WM_C530_MODE_NONPOSITIVE;
    if (mode < 4)
        return WM_C530_MODE_1_TO_3;
    if (mode < 8)
        return WM_C530_MODE_4_TO_7;
    return WM_C530_MODE_8_OR_GREATER;
}

static void wm_c530_extended_path(u32 slot_addr)
{
    u32 x_bits;
    u32 z_bits;
    u32 terrain_bits;
    u32 heading_bits;
    u32 snapshot_x;
    u32 snapshot_y;
    u32 snapshot_z;
    u32 snapshot_aux;
    u16 snapshot_heading;
    u32 record_addr;
    u32 row;

    wm_c530_store_u16(slot_addr + WM_C530_SLOT_CONTROL, 1u);

    x_bits = wm_c530_load_u32(WM_C530_SHARED_X);
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_X, x_bits);
    z_bits = wm_c530_load_u32(WM_C530_SHARED_Z);
    x_bits = wm_c530_load_u32(slot_addr + WM_C530_SLOT_X);
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_Z, z_bits);
    terrain_bits = wm_c530_s32_as_u32(
        wm_80093978(wm_c530_u32_as_s32(x_bits),
                    wm_c530_u32_as_s32(z_bits)));

    /* Retail reloads C584 before publishing the terrain return. */
    heading_bits = wm_c530_load_u32(WM_C530_SHARED_HEADING);
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_Y, terrain_bits);
    wm_c530_store_u16(slot_addr + WM_C530_SLOT_HEADING,
                      (u16)heading_bits);

    snapshot_x = wm_c530_load_u32(slot_addr + WM_C530_SLOT_X);
    snapshot_y = wm_c530_load_u32(slot_addr + WM_C530_SLOT_Y);
    snapshot_z = wm_c530_load_u32(slot_addr + WM_C530_SLOT_Z);
    wm_c530_store_u32(WM_C530_SNAPSHOT + 0x00u, snapshot_x);
    wm_c530_store_u32(WM_C530_SNAPSHOT + 0x04u, snapshot_y);
    wm_c530_store_u32(WM_C530_SNAPSHOT + 0x08u, snapshot_z);
    snapshot_aux = wm_c530_load_u32(slot_addr + WM_C530_SLOT_AUX);
    wm_c530_store_u32(WM_C530_SNAPSHOT + 0x0Cu, snapshot_aux);

    snapshot_heading = wm_c530_load_u16(slot_addr + WM_C530_SLOT_HEADING);
    wm_c530_store_u16(WM_C530_COMMON_CLEAR, 0u);
    wm_c530_store_u16(WM_C530_PUBLISHED_HEADING, snapshot_heading);

    record_addr = WM_C530_HISTORY;
    for (row = 0u; row < WM_C530_HISTORY_COUNT; row++) {
        u32 record_x = wm_c530_load_u32(slot_addr + WM_C530_SLOT_X);
        u32 record_y = wm_c530_load_u32(slot_addr + WM_C530_SLOT_Y);
        u32 record_z = wm_c530_load_u32(slot_addr + WM_C530_SLOT_Z);
        u32 record_aux = wm_c530_load_u32(slot_addr + WM_C530_SLOT_AUX);
        u16 record_heading;

        wm_c530_store_u32(record_addr + 0x00u, record_x);
        wm_c530_store_u32(record_addr + 0x04u, record_y);
        wm_c530_store_u32(record_addr + 0x08u, record_z);
        wm_c530_store_u32(record_addr + 0x0Cu, record_aux);
        record_heading =
            wm_c530_load_u16(slot_addr + WM_C530_SLOT_HEADING);
        wm_c530_store_u16(record_addr + 0x10u, record_heading);
        record_addr += WM_C530_HISTORY_STRIDE;
    }
}

s32 wm_8008C530(s32 slot_index)
{
    u32 slot_offset = (u32)slot_index << 7;
    u32 slot_addr = wm_c530_load_u32(WM_C530_POOL_PTR) + slot_offset;
    s32 return_state = wm_8008C364(slot_addr, 0);
    u16 heading;
    s32 signed_heading;
    s32 mode;

    wm_c530_store_u32(slot_addr + WM_C530_SLOT_CLEAR40, 0u);
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_CLEAR3C, 0u);
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_CLEAR38, 0u);

    heading = wm_c530_load_u16(WM_C530_HEADING);
    wm_c530_store_u16(slot_addr + WM_C530_SLOT_HEADING, heading);
    signed_heading = wm_c530_sign_extend_u16(
        wm_c530_load_u16(slot_addr + WM_C530_SLOT_HEADING));
    wm_c530_store_u16(slot_addr + WM_C530_SLOT_CONST, 12u);
    mode = wm_c530_u32_as_s32(wm_c530_load_u32(WM_C530_MODE));
    wm_c530_store_u32(slot_addr + WM_C530_SLOT_SIGNED_HEAD,
                      wm_c530_s32_as_u32(signed_heading));

    switch (wm_c530_classify_mode(mode)) {
    case WM_C530_MODE_1_TO_3:
        if (wm_c530_load_u8(WM_C530_CHANNEL0_FLAG) == 1u)
            wm_c530_extended_path(slot_addr);
        break;
    case WM_C530_MODE_4_TO_7:
        wm_c530_store_u16(slot_addr + WM_C530_SLOT_CONTROL, 2u);
        wm_c530_store_u16(slot_addr + WM_C530_SLOT_FLAG, 1u);
        break;
    case WM_C530_MODE_NONPOSITIVE:
    case WM_C530_MODE_8_OR_GREATER:
        break;
    }

    wm_c530_store_u16(WM_C530_PUBLISHED_X,
                      (u16)wm_c530_sra12(
                          wm_c530_load_u32(slot_addr + WM_C530_SLOT_X)));
    wm_c530_store_u16(WM_C530_PUBLISHED_Z,
                      (u16)wm_c530_sra12(
                          wm_c530_load_u32(slot_addr + WM_C530_SLOT_Z)));
    heading = wm_c530_load_u16(slot_addr + WM_C530_SLOT_HEADING);
    wm_c530_store_u16(WM_C530_HEADING, heading);

    return return_state;
}
