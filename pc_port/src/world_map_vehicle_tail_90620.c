/*
 * Exact native transcription of the shared wm_8008E76C tail.
 * Retail boundary: [0x80090620, 0x800906B4).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8e034.h"
#include "world_map_vehicle_tail_90620.h"

#define WM_VEHICLE_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_VEHICLE_HEAD_MIRROR UINT32_C(0x8006EE66)

static u16 vehicle_tail_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 vehicle_tail_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 vehicle_tail_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void vehicle_tail_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void vehicle_tail_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 vehicle_tail_sra12(u32 bits)
{
    u32 value = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= UINT32_C(0xFFF00000);
    return value;
}

s32 wm_8008E76C_shared_tail(u32 slot)
{
    u32 context = vehicle_tail_lw(WM_VEHICLE_CONTEXT_PTR);
    u32 x = vehicle_tail_sra12(vehicle_tail_lw(slot + 0x28u));
    u32 y = vehicle_tail_sra12(vehicle_tail_lw(slot + 0x2Cu));
    u32 z = vehicle_tail_sra12(vehicle_tail_lw(slot + 0x30u));
    s16 state;

    vehicle_tail_sw(context + 0x5Cu, x);
    vehicle_tail_sw(context + 0x08u, x);
    vehicle_tail_sw(context + 0x60u, y);
    vehicle_tail_sw(context + 0x0Cu, y);
    vehicle_tail_sw(context + 0x64u, z);
    vehicle_tail_sw(context + 0x10u, z);
    wm_8008E034(slot + 0x28u);
    vehicle_tail_sh(WM_VEHICLE_HEAD_MIRROR,
                    vehicle_tail_lhu(slot + 0x48u));

    state = vehicle_tail_lh(slot + 0x20u);
#if defined(W34N123_MUTANT_M10_WRONG_PUBLISH_STATES)
    if (state == 2 || state == 8)
#else
    if (state == 2 || state == 8 || state == 16)
#endif
        wm_80074794(2, slot + 0x28u);
    return 1;
}
