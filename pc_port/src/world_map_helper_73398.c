/*
 * Retail world-map restore-entry helper 0x80073398.
 *
 * Transcribed from disc/world_map.bin [0x80073398, 0x80073448).  The
 * session mode is read before EE6A is cleared.  Modes 1/2 restore the
 * direct halfword record; modes 4/5/7 rebuild the fixed-point vector;
 * modes 3/6 and values outside 1..7 only consume the flag.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73398.h"
#include "world_map_helper_8dff4.h"

#define WM_73398_MODE       0x8009BE10u
#define WM_73398_FLAG       0x8006EE6Au
#define WM_73398_DIRECT_X   0x8006EE54u
#define WM_73398_DIRECT_Z   0x8006EE56u
#define WM_73398_DIRECT_ID  0x8006EE58u
#define WM_73398_DFF_ID     0x8006EE66u
#define WM_73398_STATE_ID   0x8009C584u
#define WM_73398_POS_X      0x8009C5ACu
#define WM_73398_POS_Y      0x8009C5B0u
#define WM_73398_POS_Z      0x8009C5B4u

static u16 wm_73398_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_73398_TEST_HOOKS)
    wm_73398_test_event(WM_73398_TEST_READ, address, value);
#endif
    return value;
}

static u32 wm_73398_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_73398_TEST_HOOKS)
    wm_73398_test_event(WM_73398_TEST_READ, address, value);
#endif
    return value;
}

#if !defined(W34N25_MUTANT_SKIP_CLEAR) || \
    defined(W34N25_MUTANT_CLEAR_BEFORE_MODE)
static void wm_73398_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_73398_TEST_HOOKS)
    wm_73398_test_event(WM_73398_TEST_WRITE, address, value);
#endif
}
#endif

static void wm_73398_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_73398_TEST_HOOKS)
    wm_73398_test_event(WM_73398_TEST_WRITE, address, value);
#endif
}

void wm_80073398(void)
{
    u32 mode;

#if defined(W34N25_MUTANT_CLEAR_BEFORE_MODE)
    wm_73398_sh(WM_73398_FLAG, 0u);
    mode = wm_73398_lw(WM_73398_MODE);
#else
    mode = wm_73398_lw(WM_73398_MODE);
#if !defined(W34N25_MUTANT_SKIP_CLEAR)
    wm_73398_sh(WM_73398_FLAG, 0u);
#endif
#endif

    if ((mode - 1u) >= 7u)
        return;

#if defined(W34N25_MUTANT_WRONG_GROUP)
    if (mode == 1u || mode == 3u) {
#else
    if (mode == 1u || mode == 2u) {
#endif
        u16 direct_x = wm_73398_lhu(WM_73398_DIRECT_X);
        u16 direct_id = wm_73398_lhu(WM_73398_DIRECT_ID);
        u16 direct_z;

#if defined(W34N25_MUTANT_SIGNED_DIRECT)
        wm_73398_sw(WM_73398_POS_X,
                    (u32)(s32)(s16)direct_x << 12);
#else
        wm_73398_sw(WM_73398_POS_X, (u32)direct_x << 12);
#endif
        direct_z = wm_73398_lhu(WM_73398_DIRECT_Z);
#if defined(W34N25_MUTANT_SWAP_DIRECT)
        wm_73398_sw(WM_73398_STATE_ID, direct_z);
#else
        wm_73398_sw(WM_73398_STATE_ID, direct_id);
#endif
#if defined(W34N25_MUTANT_SWAP_DIRECT)
        wm_73398_sw(WM_73398_POS_Z, (u32)direct_id << 12);
#elif defined(W34N25_MUTANT_SIGNED_DIRECT)
        wm_73398_sw(WM_73398_POS_Z,
                    (u32)(s32)(s16)direct_z << 12);
#else
        wm_73398_sw(WM_73398_POS_Z, (u32)direct_z << 12);
#endif
#if defined(W34N25_MUTANT_WRITE_C5B0)
        wm_73398_sw(WM_73398_POS_Y, 0u);
#endif
        return;
    }

    if (mode == 4u || mode == 5u || mode == 7u) {
#if !defined(W34N25_MUTANT_SKIP_DFF4)
#if defined(WM_73398_TEST_HOOKS)
        wm_73398_test_event(WM_73398_TEST_DFF4, WM_73398_POS_X, 0u);
#endif
        wm_8008DFF4(WM_73398_POS_X);
#endif
        wm_73398_sw(WM_73398_STATE_ID,
                    wm_73398_lhu(WM_73398_DFF_ID));
    }
}
