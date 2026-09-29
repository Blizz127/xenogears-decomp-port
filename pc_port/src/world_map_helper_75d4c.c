/* Retail world-map helper [0x80075D4C, 0x80075E7C). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_75d4c.h"

#define WM_75D4C_POOL_PTR       0x8009BE24u
#define WM_75D4C_OLD_PRESENCE   0x8006EE70u
#define WM_75D4C_RESYNC_TIMER   0x8006EF8Eu
#define WM_75D4C_PRIMARY        0x8006F368u
#define WM_75D4C_NEW_PRESENCE   0x8006F8E5u
#define WM_75D4C_INPUT_FLAGS    0x8006EE68u
#define WM_75D4C_MODE           0x8009BE10u

static u8 wm75_lbu(u32 address)
{
    return *(const u8 *)PSX_ADDR(address);
}

static u16 wm75_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm75_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm75_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm75_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_80075D4C(void)
{
    u32 pool = wm75_lw(WM_75D4C_POOL_PTR);
    u32 channel;
    u32 active_count = 0u;

    for (channel = 0u; channel < 3u; channel++) {
#if defined(W34N14_MUTANT_WRONG_POOL_STRIDE)
        u32 slot = pool + channel * 0x40u;
#else
        u32 slot = pool + channel * 0x80u;
#endif
        u32 old_presence = (u32)wm75_lbu(WM_75D4C_OLD_PRESENCE + channel * 2u);
        u32 new_presence = (u32)wm75_lbu(WM_75D4C_NEW_PRESENCE + channel);

        if (old_presence == new_presence)
            continue;

        if (new_presence == 0u) {
#if defined(W34N14_MUTANT_WRONG_REMOVAL_DIRECTION)
            wm75_sw(slot + 0x228u, wm75_lw(slot + 0xA8u));
            wm75_sw(slot + 0x22Cu, wm75_lw(slot + 0xACu));
            wm75_sw(slot + 0x230u, wm75_lw(slot + 0xB0u));
            wm75_sw(slot + 0x258u, wm75_lw(slot + 0xD8u));
#else
            wm75_sw(slot + 0xA8u, wm75_lw(slot + 0x228u));
            wm75_sw(slot + 0xACu, wm75_lw(slot + 0x22Cu));
            wm75_sw(slot + 0xB0u, wm75_lw(slot + 0x230u));
            wm75_sw(slot + 0xD8u, wm75_lw(slot + 0x258u));
#endif
        } else {
#if !defined(W34N14_MUTANT_SKIP_RESYNC_TIMER)
            wm75_sh(WM_75D4C_RESYNC_TIMER + channel * 6u, 0x400u);
#endif
#if !defined(W34N14_MUTANT_SKIP_SLOT_CONTROL_CLEAR)
            wm75_sh(slot + 0x224u, 0u);
#endif
            wm75_sw(slot + 0x228u, wm75_lw(slot + 0xA8u));
            wm75_sw(slot + 0x22Cu, wm75_lw(slot + 0xACu));
            wm75_sw(slot + 0x230u, wm75_lw(slot + 0xB0u));
            wm75_sw(slot + 0x258u, wm75_lw(slot + 0xD8u));
        }
    }

    for (channel = 0u; channel < 3u; channel++) {
#if defined(W34N14_MUTANT_WRONG_ACTIVE_PREDICATE)
        if (wm75_lbu(WM_75D4C_PRIMARY + channel) == 0xFFu &&
#else
        if (wm75_lbu(WM_75D4C_PRIMARY + channel) != 0xFFu &&
#endif
            wm75_lbu(WM_75D4C_NEW_PRESENCE + channel) == 1u)
            active_count++;
    }

    {
        u16 input_flags = wm75_lhu(WM_75D4C_INPUT_FLAGS);
#if defined(W34N14_MUTANT_IGNORE_INPUT_FREEZE)
        input_flags = 0u;
#endif
        if ((input_flags & 0x4000u) == 0u)
            wm75_sw(WM_75D4C_MODE, active_count != 0u ? 2u : 1u);
    }
}
