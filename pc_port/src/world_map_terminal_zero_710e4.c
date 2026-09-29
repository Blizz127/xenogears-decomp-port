/*
 * Retail WorldMapMain D7CC==0 terminal lane [0x800710E4, 0x800711B0),
 * followed by the shared terminal epilogue [0x800712A0, 0x800712B8).
 *
 * Main-executable globals use their compiled native authorities in the port;
 * world-overlay values remain guest addresses in g_PsxRam.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_762fc.h"
#include "world_map_helper_94364.h"
#include "world_map_terminal_zero_710e4.h"

extern void* LoadGameStateOverlay(unsigned int overlay_index);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(int error_code);

extern u8 D_800591AE;
extern u16 D_8006F94E;
extern u16 D_8006F950;
extern u16 D_8006F954;

#define WM_710E4_BBC4       0x8009BBC4u
#define WM_710E4_BD0C       0x8009BD0Cu
#define WM_710E4_BD3A       0x8009BD3Au
#define WM_710E4_D55C       0x8009D55Cu
#define WM_710E4_D7D8       0x8009D7D8u
#define WM_710E4_EF64       0x8006EF64u
#define WM_710E4_EF68       0x8006EF68u

#if defined(WM_710E4_TEST_HOOKS)
extern void wm_710e4_test_event(int event);
#define WM_710E4_EVENT(event) wm_710e4_test_event((event))
#else
#define WM_710E4_EVENT(event) ((void)0)
#endif

static u32 wm_710e4_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_710e4_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_710e4_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_710e4_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_71034_run_terminal_zero_lane(void)
{
    u32 record;

#if defined(W34N28_MUTANT_SWAP_OVERLAY_STATE)
    WM_710E4_EVENT(2);
    ChangeGameState(1u);
    WM_710E4_EVENT(1);
    (void)LoadGameStateOverlay(1u);
#else
    WM_710E4_EVENT(1);
    (void)LoadGameStateOverlay(1u);
    WM_710E4_EVENT(2);
    ChangeGameState(1u);
#endif

#if defined(W34N28_MUTANT_INVERT_BBC4_GUARD)
    if (wm_710e4_lw(WM_710E4_BBC4) != 0u) {
#else
    if (wm_710e4_lw(WM_710E4_BBC4) == 0u) {
#endif
        record = wm_710e4_lw(WM_710E4_D7D8);
#if defined(W34N28_MUTANT_IGNORE_RECORD_TYPE)
        (void)wm_710e4_lh(record + 0xEu);
        if (1) {
#else
        if (wm_710e4_lh(record + 0xEu) == 3) {
#endif
#if !defined(W34N28_MUTANT_SKIP_HELPER)
            WM_710E4_EVENT(3);
#if defined(W34N28_MUTANT_WRONG_HELPER_ARG)
            (void)wm_80094364(WM_710E4_D55C, 2u,
                              (s32)wm_710e4_lhu(WM_710E4_EF64));
#else
            (void)wm_80094364(WM_710E4_D55C, 3u,
                              (s32)wm_710e4_lhu(WM_710E4_EF64));
#endif
#endif
        }

#if !defined(W34N28_MUTANT_STALE_RECORD)
        record = wm_710e4_lw(WM_710E4_D7D8);
#endif
#if defined(W34N28_MUTANT_GUEST_OUTPUTS)
        WM_710E4_EVENT(4);
        wm_710e4_sh(0x8006F950u, wm_710e4_lhu(WM_710E4_BD3A));
        WM_710E4_EVENT(5);
        wm_710e4_sh(0x8006F94Eu, wm_710e4_lhu(record + 8u));
        WM_710E4_EVENT(6);
        wm_710e4_sh(0x8006F954u, wm_710e4_lhu(record + 0xAu));
#else
        WM_710E4_EVENT(4);
        D_8006F950 = wm_710e4_lhu(WM_710E4_BD3A);
        WM_710E4_EVENT(5);
        D_8006F94E = wm_710e4_lhu(record + 8u);
        WM_710E4_EVENT(6);
        D_8006F954 = wm_710e4_lhu(record + 0xAu);
#endif
    }

    WM_710E4_EVENT(7);
#if defined(W34N28_MUTANT_WRONG_EF68_OFFSET)
    wm_710e4_sh(WM_710E4_EF68,
                (u16)(wm_710e4_lw(WM_710E4_BD0C) + 0x200u));
#else
    wm_710e4_sh(WM_710E4_EF68,
                (u16)(wm_710e4_lw(WM_710E4_BD0C) + 0x400u));
#endif

    WM_710E4_EVENT(8);
#if !defined(W34N28_MUTANT_SKIP_SYSTEM_BYTE_CLEAR)
    D_800591AE = 0u;
#endif

#if defined(W34N28_MUTANT_SWAP_EPILOGUE)
    WM_710E4_EVENT(10);
    MainLoop(0);
    WM_710E4_EVENT(9);
    wm_800762FC();
#else
    WM_710E4_EVENT(9);
    wm_800762FC();
    WM_710E4_EVENT(10);
    MainLoop(0);
#endif
}
