/*
 * Shared world-context link helper 0x800848B4.
 *
 * Retail computes two wrapping record offsets, loads the live C620 root, and
 * publishes a guest pointer in the JR delay slot. It has no other side
 * effects.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_848b4.h"

#define WM_848B4_CONTEXT_PTR  0x8009C620u
#define WM_848B4_RECORD_STRIDE 0x54u
#define WM_848B4_LINK_OFFSET    0x50u

/* Keep each guest access opaque to the optimizer so the retail access order
 * remains explicit in ordinary optimized builds as well as trace builds. */
static u32 wm_848b4_load_u32(u32 address) __attribute__((noinline));
static u32 wm_848b4_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_848b4_store_u32(u32 address, u32 value)
    __attribute__((noinline));
static void wm_848b4_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

/* 0x800848B4..0x800848D4 and 0x800848E0: retail's shift/add chain
 * computes index * 0x54 with wrapping 32-bit arithmetic. */
static u32 wm_848b4_record_offset(s32 index)
{
    u32 bits = (u32)index;
    return (bits << 6) + (bits << 4) + (bits << 2);
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    u32 destination_offset = wm_848b4_record_offset(destination_record);
    u32 source_offset = wm_848b4_record_offset(source_record);
    u32 root = wm_848b4_load_u32(WM_848B4_CONTEXT_PTR); /* 0x800848DC */
    u32 destination = root + destination_offset + WM_848B4_LINK_OFFSET;
    u32 source = root + source_offset;

    /* Retail's sole store is the JR RA delay slot at 0x800848F0. */
    wm_848b4_store_u32(destination, source);
}
