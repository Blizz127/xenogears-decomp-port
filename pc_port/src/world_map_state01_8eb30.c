/*
 * Exact native transcription of wm_8008E76C states 0 and 1.
 * Retail boundary: [0x8008EB30, 0x8008EB64).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_8bfd4.h"
#include "world_map_state01_8eb30.h"
#include "world_map_terrain_sampler.h"
#include "world_map_vehicle_tail_90620.h"

static u32 state01_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 state01_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void state01_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

s32 wm_8008E76C_state01(s32 slot_index, u32 slot)
{
    s32 terrain = wm_80093978(state01_as_s32(state01_lw(slot + 0x28u)),
                               state01_as_s32(state01_lw(slot + 0x30u)));

    state01_sw(slot + 0x2Cu, (u32)terrain - UINT32_C(0x1000));
    wm_8008BFD4((u16)slot_index, slot + 0x28u, UINT16_C(64), 1024);
    return wm_8008E76C_shared_tail(slot);
}
