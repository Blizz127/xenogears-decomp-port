/* Retail signed-default terminal lane and shared terminal epilogue. */
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_762fc.h"
#include "world_map_terminal_default_71264.h"

typedef struct Wm71264Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm71264Rect;

extern void ChangeGameState(unsigned int state);
extern void MainLoop(int error_code);
extern int ClearImage(Wm71264Rect* rect, u8 r, u8 g, u8 b);
extern int DrawSync(int mode);
extern u8 D_800591AE;

#if defined(WM_71264_TEST_HOOKS)
extern void wm_71264_test_event(int event);
#define WM_71264_EVENT(event) wm_71264_test_event((event))
#else
#define WM_71264_EVENT(event) ((void)0)
#endif

void wm_71034_run_terminal_default_lane(void)
{
    Wm71264Rect clear_rect;

    WM_71264_EVENT(1);
#if defined(W34N32_MUTANT_WRONG_STATE)
    ChangeGameState(1u);
#else
    ChangeGameState(0u);
#endif

#if defined(W34N32_MUTANT_WRONG_ORIGIN)
    clear_rect.x = 1;
#else
    clear_rect.x = 0;
#endif
    clear_rect.y = 0;
#if defined(W34N32_MUTANT_WRONG_EXTENT)
    clear_rect.w = 320;
    clear_rect.h = 432;
#else
    clear_rect.w = 319;
    clear_rect.h = 431;
#endif

    WM_71264_EVENT(2);
#if defined(W34N32_MUTANT_WRONG_BLUE)
    (void)ClearImage(&clear_rect, 0u, 0u, 0u);
#else
    (void)ClearImage(&clear_rect, 0u, 0u, 64u);
#endif

    WM_71264_EVENT(3);
#if !defined(W34N32_MUTANT_SKIP_DIRECT_DRAW_SYNC)
    (void)DrawSync(0);
#endif

    WM_71264_EVENT(4);
#if defined(W34N32_MUTANT_GUEST_SYSTEM_BYTE)
    *(u8*)PSX_ADDR(0x800591AEu) = 0u;
#else
    D_800591AE = 0u;
#endif

#if defined(W34N32_MUTANT_SWAP_EPILOGUE)
    WM_71264_EVENT(6);
    MainLoop(0);
    WM_71264_EVENT(5);
    wm_800762FC();
#else
    WM_71264_EVENT(5);
    wm_800762FC();
    WM_71264_EVENT(6);
#if defined(W34N32_MUTANT_WRONG_MAIN_ARG)
    MainLoop(1);
#else
    MainLoop(0);
#endif
#endif
}
