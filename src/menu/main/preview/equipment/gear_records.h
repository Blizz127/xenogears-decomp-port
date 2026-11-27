#ifndef XENO_PC_GEAR_RECORDS_H
#define XENO_PC_GEAR_RECORDS_H

#include <stdint.h>
#include "common.h"
#include "system/menu.h"

static inline u8 *gear_record(u8 id)
{
    return (u8 *)&g_GameState.gears[id];
}

static inline u16 gear_u16(const u8 *bytes)
{
    return (u16)(bytes[0] | ((u16)bytes[1] << 8));
}

static inline void gear_put16(u8 *bytes, u16 value)
{
    bytes[0] = (u8)value;
    bytes[1] = (u8)(value >> 8);
}

/* MenuUnk6 retains four-byte table slots even on a 64-bit host. */
static inline const u8 *gear_table(void *context, unsigned slot)
{
    MenuUnk6 *resources = context;
    const u8 *word = &resources->unk8[slot - 8];
    u32 address = gear_u16(word) | ((u32)gear_u16(word + 2) << 16);
    return (const u8 *)(uintptr_t)address;
}

#endif
