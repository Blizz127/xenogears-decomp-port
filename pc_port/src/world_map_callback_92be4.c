/*
 * World-map scheduler callback 0x80092BE4.
 *
 * This linear callback initializes the fixed D498 text/render object through
 * two accepted helpers. It performs a mandatory fresh D4A8 flags load after
 * construction, publishes byte/halfword state in retail order, and returns
 * scheduler state 1. The scheduler-supplied slot index is unused by retail.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_92be4.h"

#define WM_92BE4_TEXT_OBJECT 0x8009D498u
#define WM_92BE4_TEXT_FLAGS  0x8009D4A8u
#define WM_92BE4_TEXT_STATE  0x8009D500u
#define WM_92BE4_SELECTION   0x8009BD24u

/* The accepted host helper has one extra dead C-ABI parameter between the
 * six retail geometry values and height. Zero is a host-only placeholder;
 * the seven meaningful retail values remain D498,960,384,160,120,32,1. */
void func_80032F54(void* object, s32 tpage_x, s32 tpage_y,
                   s32 x, s32 y, s32 width, s32 host_dead_mode,
                   s32 height);
void func_80034614(void* object);

/* Opaque wrappers preserve the retail typed guest-access sequence under
 * optimization and expose it to the production-linked certificate. */
static u16 wm_92be4_load_u16(u32 pc, u32 address)
    __attribute__((noinline));
static u16 wm_92be4_load_u16(u32 pc, u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_92BE4_TEST_TRACE)
    wm_92be4_test_trace(pc, WM_92BE4_TRACE_LHU, address, 2u,
                        (u32)value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_92be4_store_u8(u32 pc, u32 address, u8 value)
    __attribute__((noinline));
static void wm_92be4_store_u8(u32 pc, u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92BE4_TEST_TRACE)
    wm_92be4_test_trace(pc, WM_92BE4_TRACE_SB, address, 1u,
                        (u32)value);
#else
    (void)pc;
#endif
}

static void wm_92be4_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_92be4_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_92BE4_TEST_TRACE)
    wm_92be4_test_trace(pc, WM_92BE4_TRACE_SH, address, 2u,
                        (u32)value);
#else
    (void)pc;
#endif
}

s32 wm_80092BE4(s32 slot_index)
{
    void* const object = PSX_ADDR(WM_92BE4_TEXT_OBJECT);
    u16 flags;

    (void)slot_index;

    wm_92be4_store_u16(0x80092C00u, WM_92BE4_SELECTION, 0xFFFFu);
    func_80032F54(object, 960, 384, 160, 120, 32, 0, 1);

    /* Retail requires this observable post-constructor LHU. */
    flags = wm_92be4_load_u16(0x80092C34u, WM_92BE4_TEXT_FLAGS);
    wm_92be4_store_u8(0x80092C40u, WM_92BE4_TEXT_STATE, 8u);
    wm_92be4_store_u16(0x80092C4Cu, WM_92BE4_TEXT_FLAGS,
                       (u16)(flags | 2u));

    func_80034614(object);
    return 1;
}
