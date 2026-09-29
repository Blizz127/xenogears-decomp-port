/*
 * World-map channel object/position helper 0x8008C364.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008C364, 0x8008C530).  A bounded version of the retail eight-entry
 * jump table selects one missing/resource path, one channel-position path, or
 * one shared-position path.  At most one object is allocated per invocation.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_c28c.h"
#include "world_map_helper_c364.h"
#include "world_map_terrain_sampler.h"

#define WM_C364_PRIMARY_BASE       0x8006F368u
#define WM_C364_POSITION_BASE      0x8006EF8Eu
#define WM_C364_FLAG_BASE          0x8006F8E5u
#define WM_C364_GEAR_BASE          0x8006D940u
#define WM_C364_SHARED_X           0x8009C5ACu
#define WM_C364_SHARED_Z           0x8009C5B4u
#define WM_C364_SLOT_CONTROL       0x24u
#define WM_C364_SLOT_X             0x28u
#define WM_C364_SLOT_Y             0x2Cu
#define WM_C364_SLOT_Z             0x30u

static u8 wm_c364_load_u8(u32 address)
{
    return *(volatile const u8*)PSX_ADDR(address);
}

static u16 wm_c364_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_c364_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_c364_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_c364_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_c364_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_c364_s32_as_u32(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_c364_channel_stride6(u32 channel)
{
    return ((channel << 1) + channel) << 1;
}

static u32 wm_c364_character_stride164(u32 character)
{
    u32 offset = (character << 2) + character;
    offset <<= 3;
    offset += character;
    return offset << 2;
}

static void wm_c364_store_missing_position(u32 slot_addr)
{
    wm_c364_store_u16(slot_addr + WM_C364_SLOT_CONTROL, 1u);
    wm_c364_store_u32(slot_addr + WM_C364_SLOT_Y, 0u);
    wm_c364_store_u32(slot_addr + WM_C364_SLOT_Z, 0u);
    wm_c364_store_u32(slot_addr + WM_C364_SLOT_X, 0u);
}

s32 wm_8008C364(u32 slot_addr, s32 channel)
{
    u32 channel_bits = (u32)channel;
    u32 channel_offset = wm_c364_channel_stride6(channel_bits);
    u8 primary = wm_c364_load_u8(WM_C364_PRIMARY_BASE + channel_bits);
    u16 raw_position =
        wm_c364_load_u16(WM_C364_POSITION_BASE + channel_offset);
    u32 dispatch = primary != 0xFFu ? 1u : 0u;
    u8 flag;

    if (((u32)raw_position & 0x3FFFu) >= 0x0400u)
        dispatch |= 2u;

    flag = wm_c364_load_u8(WM_C364_FLAG_BASE + channel_bits);
    if (flag == 1u)
        dispatch |= 4u;

    /* Preserve the retail unsigned guard before its computed dispatch. */
    if (dispatch >= 8u)
        return 1;

    switch (dispatch) {
    case 1u: {
        u32 fresh_primary =
            (u32)wm_c364_load_u8(WM_C364_PRIMARY_BASE + channel_bits);
        u32 gear_address = WM_C364_GEAR_BASE +
                           wm_c364_character_stride164(fresh_primary);

        if (wm_c364_load_u8(gear_address) == 0xFFu) {
            wm_c364_store_missing_position(slot_addr);
            return 3;
        }

        wm_8008C28C(slot_addr, channel);
        wm_c364_store_missing_position(slot_addr);
        return 1;
    }

    case 3u: {
        u32 x;
        u32 z;
        u32 terrain_x;
        s32 terrain_y;

        wm_8008C28C(slot_addr, channel);
        wm_c364_store_u16(slot_addr + WM_C364_SLOT_CONTROL, 0u);

        x = (u32)wm_c364_load_u16(WM_C364_POSITION_BASE + 2u +
                                  channel_offset) << 12;
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_X, x);
        z = (u32)wm_c364_load_u16(WM_C364_POSITION_BASE + 4u +
                                  channel_offset) << 12;
        terrain_x = wm_c364_load_u32(slot_addr + WM_C364_SLOT_X);
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_Z, z);
        terrain_y = wm_80093978(wm_c364_u32_as_s32(terrain_x),
                                wm_c364_u32_as_s32(z));
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_Y,
                          wm_c364_s32_as_u32(terrain_y));
        return 1;
    }

    case 5u:
    case 7u: {
        u32 x;
        u32 z;
        u32 terrain_x;
        s32 terrain_y;

        wm_8008C28C(slot_addr, channel);
        wm_c364_store_u16(slot_addr + WM_C364_SLOT_CONTROL, 0u);

        x = wm_c364_load_u32(WM_C364_SHARED_X);
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_X, x);
        z = wm_c364_load_u32(WM_C364_SHARED_Z);
        terrain_x = wm_c364_load_u32(slot_addr + WM_C364_SLOT_X);
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_Z, z);
        terrain_y = wm_80093978(wm_c364_u32_as_s32(terrain_x),
                                wm_c364_u32_as_s32(z));
        wm_c364_store_u32(slot_addr + WM_C364_SLOT_Y,
                          wm_c364_s32_as_u32(terrain_y));
        wm_c364_store_u16(WM_C364_POSITION_BASE + channel_offset, 0x0400u);
        return 1;
    }

    case 0u:
    case 2u:
    case 4u:
    case 6u:
        wm_c364_store_missing_position(slot_addr);
        return 3;

    default:
        /* The guarded three-bit retail index makes this unreachable. */
        return 1;
    }
}
