/*
 * World-map scheduler callback 0x8008DD6C.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008DD6C, 0x8008DE9C).  C364 initializes the channel-two object and
 * position before this callback clears its private fields.  Signed world
 * mode and the channel-two flag select optional terrain/position work; every
 * path publishes the current position and returns C364's result unchanged.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8dd6c.h"
#include "world_map_helper_c364.h"
#include "world_map_terrain_sampler.h"

#define WM_DD6C_POOL_PTR       0x8009BE24u
#define WM_DD6C_MODE           0x8009BE10u
#define WM_DD6C_HEADING        0x8006EE5Eu
#define WM_DD6C_CHANNEL2_FLAG  0x8006F8E7u
#define WM_DD6C_SHARED_X       0x8009C5ACu
#define WM_DD6C_SHARED_Z       0x8009C5B4u
#define WM_DD6C_SHARED_HEADING 0x8009C584u
#define WM_DD6C_PUBLISHED_X    0x8006EF9Cu
#define WM_DD6C_PUBLISHED_Z    0x8006EF9Eu

#define WM_DD6C_SLOT_CONTROL     0x20u
#define WM_DD6C_SLOT_FLAG        0x24u
#define WM_DD6C_SLOT_X           0x28u
#define WM_DD6C_SLOT_Y           0x2Cu
#define WM_DD6C_SLOT_Z           0x30u
#define WM_DD6C_SLOT_CLEAR38     0x38u
#define WM_DD6C_SLOT_CLEAR3C     0x3Cu
#define WM_DD6C_SLOT_CLEAR40     0x40u
#define WM_DD6C_SLOT_HEADING     0x48u
#define WM_DD6C_SLOT_CONST       0x4Au
#define WM_DD6C_SLOT_AUX_CONST   0x58u
#define WM_DD6C_SLOT_SIGNED_HEAD 0x5Cu

enum wm_dd6c_mode_family {
    WM_DD6C_MODE_NONPOSITIVE = 0,
    WM_DD6C_MODE_1_TO_3,
    WM_DD6C_MODE_4_TO_7,
    WM_DD6C_MODE_8_OR_GREATER
};

static u8 wm_dd6c_load_u8(u32 address)
{
    return *(volatile const u8*)PSX_ADDR(address);
}

static u16 wm_dd6c_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_dd6c_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_dd6c_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_dd6c_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_dd6c_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_dd6c_s32_as_u32(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static s32 wm_dd6c_sign_extend_u16(u16 value)
{
    if (value <= 0x7FFFu)
        return (s32)value;
    return (s32)(u32)value - 0x10000;
}

/* MIPS SRA by 12, expressed entirely with defined unsigned C operations. */
static u32 wm_dd6c_sra12(u32 value)
{
    u32 result = value >> 12;
    if ((value & 0x80000000u) != 0u)
        result |= 0xFFF00000u;
    return result;
}

static enum wm_dd6c_mode_family wm_dd6c_classify_mode(s32 mode)
{
    if (mode <= 0)
        return WM_DD6C_MODE_NONPOSITIVE;
    if (mode < 4)
        return WM_DD6C_MODE_1_TO_3;
    if (mode < 8)
        return WM_DD6C_MODE_4_TO_7;
    return WM_DD6C_MODE_8_OR_GREATER;
}

static void wm_dd6c_extended_path(u32 slot_addr)
{
    u32 x_bits;
    u32 z_bits;
    u32 terrain_bits;
    u32 heading_bits;

    wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_CONTROL, 1u);
    x_bits = wm_dd6c_load_u32(WM_DD6C_SHARED_X);
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_X, x_bits);
    z_bits = wm_dd6c_load_u32(WM_DD6C_SHARED_Z);
    x_bits = wm_dd6c_load_u32(slot_addr + WM_DD6C_SLOT_X);

    /* Retail publishes Z in the terrain JAL delay slot. */
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_Z, z_bits);
    terrain_bits = wm_dd6c_s32_as_u32(
        wm_80093978(wm_dd6c_u32_as_s32(x_bits),
                    wm_dd6c_u32_as_s32(z_bits)));

    /* Retail freshly reads C584 before publishing the terrain result. */
    heading_bits = wm_dd6c_load_u32(WM_DD6C_SHARED_HEADING);
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_Y, terrain_bits);
    wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_HEADING,
                      (u16)heading_bits);
}

s32 wm_8008DD6C(s32 slot_index)
{
    u32 slot_offset = (u32)slot_index << 7;
    u32 slot_addr = wm_dd6c_load_u32(WM_DD6C_POOL_PTR) + slot_offset;
    s32 return_state = wm_8008C364(slot_addr, 2);
    u16 heading;
    s32 signed_heading;
    s32 mode;

    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_CLEAR40, 0u);
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_CLEAR3C, 0u);
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_CLEAR38, 0u);

    heading = wm_dd6c_load_u16(WM_DD6C_HEADING);
    wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_CONST, 12u);
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_AUX_CONST, 31u);
    mode = wm_dd6c_u32_as_s32(wm_dd6c_load_u32(WM_DD6C_MODE));
    wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_HEADING, heading);
    signed_heading = wm_dd6c_sign_extend_u16(
        wm_dd6c_load_u16(slot_addr + WM_DD6C_SLOT_HEADING));
    wm_dd6c_store_u32(slot_addr + WM_DD6C_SLOT_SIGNED_HEAD,
                      wm_dd6c_s32_as_u32(signed_heading));

    switch (wm_dd6c_classify_mode(mode)) {
    case WM_DD6C_MODE_1_TO_3:
        if (wm_dd6c_load_u8(WM_DD6C_CHANNEL2_FLAG) == 1u)
            wm_dd6c_extended_path(slot_addr);
        break;
    case WM_DD6C_MODE_4_TO_7:
        wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_CONTROL, 2u);
        wm_dd6c_store_u16(slot_addr + WM_DD6C_SLOT_FLAG, 1u);
        break;
    case WM_DD6C_MODE_NONPOSITIVE:
    case WM_DD6C_MODE_8_OR_GREATER:
        break;
    }

    wm_dd6c_store_u16(WM_DD6C_PUBLISHED_X,
                      (u16)wm_dd6c_sra12(
                          wm_dd6c_load_u32(slot_addr + WM_DD6C_SLOT_X)));
    wm_dd6c_store_u16(WM_DD6C_PUBLISHED_Z,
                      (u16)wm_dd6c_sra12(
                          wm_dd6c_load_u32(slot_addr + WM_DD6C_SLOT_Z)));
    heading = wm_dd6c_load_u16(slot_addr + WM_DD6C_SLOT_HEADING);
    wm_dd6c_store_u16(WM_DD6C_HEADING, heading);

    return return_state;
}
