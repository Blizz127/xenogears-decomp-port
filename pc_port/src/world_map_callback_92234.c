/*
 * World-map scheduler callback 0x80092234.
 *
 * The leaf initializes slot 11's shared scalar state for the signed world
 * mode and returns scheduler state 1.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_92234.h"

#define WM_92234_POOL_PTR 0x8009BE24u
#define WM_92234_MODE     0x8009BE10u
#define WM_92234_SCALAR   0x8009BE0Cu

#define WM_92234_SLOT_CONTROL 0x20u
#define WM_92234_SLOT_VALUE   0x50u

/* Opaque wrappers preserve retail guest-access order under optimization and
 * expose both reads and writes to the production-linked certificate. */
static u32 wm_92234_load_u32(u32 pc, u32 address)
    __attribute__((noinline));
static u32 wm_92234_load_u32(u32 pc, u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_92234_TEST_TRACE)
    wm_92234_test_trace(pc, WM_92234_TRACE_LW, address, 4u, value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_92234_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_92234_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92234_TEST_TRACE)
    wm_92234_test_trace(pc, WM_92234_TRACE_SH, address, 2u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_92234_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_92234_store_u32(u32 pc, u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92234_TEST_TRACE)
    wm_92234_test_trace(pc, WM_92234_TRACE_SW, address, 4u, value);
#else
    (void)pc;
#endif
}

static s32 wm_92234_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

s32 wm_80092234(s32 slot_index)
{
    u32 pool;
    u32 slot;
    u32 scalar;
    s32 mode;

    /* Retail reads BE24 at 0x8009223C, then signed BE10 at 0x80092244. */
    pool = wm_92234_load_u32(0x8009223Cu, WM_92234_POOL_PTR);
    mode = wm_92234_u32_as_s32(
        wm_92234_load_u32(0x80092244u, WM_92234_MODE));
    slot = pool + ((u32)slot_index << 7);

    if (mode >= 1 && mode <= 5) {
        wm_92234_store_u32(0x80092264u, WM_92234_SCALAR, 140u);
    } else if (mode >= 6 && mode <= 7) {
        wm_92234_store_u32(0x80092284u, WM_92234_SCALAR, 120u);
        wm_92234_store_u16(0x8009228Cu,
                           slot + WM_92234_SLOT_CONTROL, 1u);
    }

    /* Retail performs a real fresh BE0C load at 0x80092294 on every path. */
    scalar = wm_92234_load_u32(0x80092294u, WM_92234_SCALAR);
    wm_92234_store_u32(0x800922A0u, slot + WM_92234_SLOT_VALUE,
                       scalar << 12);

    return 1;
}
