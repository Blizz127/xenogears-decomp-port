/*
 * World-map scheduler callback 0x80091B54.
 *
 * The leaf selects one of two static camera-table pairs for slot 10 and
 * returns scheduler state 1. Modes outside the two retail arms are no-ops.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_91b54.h"

#define WM_91B54_POOL_PTR 0x8009BE24u
#define WM_91B54_MODE     0x8009BE10u
#define WM_91B54_D3F0     0x8009D3F0u
#define WM_91B54_BD38     0x8009BD38u

#define WM_91B54_SLOT_SELECTOR 0x50u
#define WM_91B54_SLOT_TABLE_A  0x64u
#define WM_91B54_SLOT_TABLE_B  0x68u

/* Opaque wrappers preserve the retail guest-access order under optimization
 * and expose that order to the production-linked certificate. */
static u32 wm_91b54_load_u32(u32 pc, u32 address)
    __attribute__((noinline));
static u32 wm_91b54_load_u32(u32 pc, u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_91B54_TEST_TRACE)
    wm_91b54_test_trace(pc, WM_91B54_TRACE_LW, address, 4u, value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_91b54_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_91b54_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_91B54_TEST_TRACE)
    wm_91b54_test_trace(pc, WM_91B54_TRACE_SH, address, 2u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_91b54_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_91b54_store_u32(u32 pc, u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_91B54_TEST_TRACE)
    wm_91b54_test_trace(pc, WM_91B54_TRACE_SW, address, 4u, value);
#else
    (void)pc;
#endif
}

static s32 wm_91b54_u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

s32 wm_80091B54(s32 slot_index)
{
    u32 pool;
    u32 slot;
    s32 mode;

    /* Retail reads BE24 at 0x80091B5C, then signed BE10 at 0x80091B64. */
    pool = wm_91b54_load_u32(0x80091B5Cu, WM_91B54_POOL_PTR);
    mode = wm_91b54_u32_as_s32(
        wm_91b54_load_u32(0x80091B64u, WM_91B54_MODE));
    slot = pool + ((u32)slot_index << 7);

    if (mode >= 1 && mode <= 5) {
        wm_91b54_store_u32(0x80091BA0u,
                           slot + WM_91B54_SLOT_SELECTOR, 3u);
        wm_91b54_store_u32(0x80091BACu, WM_91B54_D3F0,
                           0x00460000u);
        wm_91b54_store_u16(0x80091BB8u, WM_91B54_BD38, 0xFDA0u);
        wm_91b54_store_u32(0x80091BC4u,
                           slot + WM_91B54_SLOT_TABLE_A, 0x8009B224u);
        /* Retail publishes +68 in the 0x80091BD0 jump delay slot. */
        wm_91b54_store_u32(0x80091BD4u,
                           slot + WM_91B54_SLOT_TABLE_B, 0x8009B234u);
    } else if (mode == 7) {
        wm_91b54_store_u32(0x80091BDCu,
                           slot + WM_91B54_SLOT_SELECTOR, 3u);
        wm_91b54_store_u32(0x80091BE8u, WM_91B54_D3F0,
                           0x00280000u);
        wm_91b54_store_u16(0x80091BF4u, WM_91B54_BD38, 0xFDA0u);
        wm_91b54_store_u32(0x80091C00u,
                           slot + WM_91B54_SLOT_TABLE_A, 0x8009B22Cu);
        wm_91b54_store_u32(0x80091C0Cu,
                           slot + WM_91B54_SLOT_TABLE_B, 0x8009B23Cu);
    }

    return 1;
}
