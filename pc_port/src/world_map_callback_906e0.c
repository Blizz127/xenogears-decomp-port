/*
 * World-map scheduler callback 0x800906E0.
 *
 * The callback links context records two and three to record zero, copies
 * their position words into the selected scheduler slot, applies the signed
 * mode 4..7 arm, and returns scheduler state 1 on every path.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_906e0.h"
#include "world_map_helper_848b4.h"

#define WM_906E0_POOL_PTR    0x8009BE24u
#define WM_906E0_MODE        0x8009BE10u
#define WM_906E0_CONTEXT_PTR 0x8009C620u

#define WM_906E0_CONTEXT_RECORD2 0xA8u
#define WM_906E0_CONTEXT_RECORD3 0xFCu
#define WM_906E0_CONTEXT_X       0x08u
#define WM_906E0_CONTEXT_Y       0x0Cu
#define WM_906E0_CONTEXT_Z       0x10u
#define WM_906E0_CONTEXT_W       0x14u

#define WM_906E0_SLOT_CONTROL 0x20u
#define WM_906E0_SLOT_X2      0x28u
#define WM_906E0_SLOT_Y2      0x2Cu
#define WM_906E0_SLOT_Z2      0x30u
#define WM_906E0_SLOT_W2      0x34u
#define WM_906E0_SLOT_X3      0x38u
#define WM_906E0_SLOT_Y3      0x3Cu
#define WM_906E0_SLOT_Z3      0x40u
#define WM_906E0_SLOT_W3      0x44u
#define WM_906E0_SLOT_CLEAR50 0x50u
#define WM_906E0_SLOT_CONST54 0x54u
#define WM_906E0_SLOT_CONST58 0x58u
#define WM_906E0_SLOT_CONST5C 0x5Cu

/* Retail order is authoritative even where final RAM would be identical.
 * Opaque wrappers prevent optimized host builds from reordering accesses. */
static u32 wm_906e0_load_u32(u32 address) __attribute__((noinline));
static u32 wm_906e0_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_906e0_store_u16(u32 address, u16 value)
    __attribute__((noinline));
static void wm_906e0_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_906e0_store_u32(u32 address, u32 value)
    __attribute__((noinline));
static void wm_906e0_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_906e0_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

s32 wm_800906E0(s32 slot_index)
{
    u32 slot_offset;
    u32 pool;
    u32 context;
    u32 slot;
    u32 x;
    u32 y;
    u32 z;
    u32 w;
    s32 mode;

    /* The retail JAL delay slots at 0x800906F8 and 0x80090704 supply the
     * destination record indices. Both calls precede all local slot work. */
    wm_800848B4(0, 2);
    wm_800848B4(0, 3);

    slot_offset = (u32)slot_index << 7;
    pool = wm_906e0_load_u32(WM_906E0_POOL_PTR);       /* 0x80090710 */
    context = wm_906e0_load_u32(WM_906E0_CONTEXT_PTR); /* 0x80090718 */
    slot = pool + slot_offset;

    wm_906e0_store_u16(slot + WM_906E0_SLOT_CONTROL, 0u);

    /* Retail loads all four record-two words before publishing any of them. */
    x = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD2 +
                          WM_906E0_CONTEXT_X);
    y = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD2 +
                          WM_906E0_CONTEXT_Y);
    z = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD2 +
                          WM_906E0_CONTEXT_Z);
    w = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD2 +
                          WM_906E0_CONTEXT_W);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_X2, x);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_Y2, y);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_Z2, z);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_W2, w);

    /* 0x80090748 is a mandatory fresh C620 reload. */
    context = wm_906e0_load_u32(WM_906E0_CONTEXT_PTR);
    x = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD3 +
                          WM_906E0_CONTEXT_X);
    y = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD3 +
                          WM_906E0_CONTEXT_Y);
    z = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD3 +
                          WM_906E0_CONTEXT_Z);
    w = wm_906e0_load_u32(context + WM_906E0_CONTEXT_RECORD3 +
                          WM_906E0_CONTEXT_W);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_X3, x);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_Y3, y);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_Z3, z);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_W3, w);

    mode = wm_906e0_u32_as_s32(wm_906e0_load_u32(WM_906E0_MODE));
    wm_906e0_store_u32(slot + WM_906E0_SLOT_CLEAR50, 0u);
    wm_906e0_store_u32(slot + WM_906E0_SLOT_CONST54, 0x40u);

    if (mode >= 4 && mode < 8) {
        wm_906e0_store_u16(slot + WM_906E0_SLOT_CONTROL, 3u);
        /* Retail PCs 0x800907A4 then 0x800907A8: +5C precedes +58. */
        wm_906e0_store_u32(slot + WM_906E0_SLOT_CONST5C, 0x80u);
        wm_906e0_store_u32(slot + WM_906E0_SLOT_CONST58, 0x80u);
    }

    return 1;
}
