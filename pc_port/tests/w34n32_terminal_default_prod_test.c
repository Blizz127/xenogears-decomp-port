/* Production-linked certificate for WorldMapMain's signed-default lane. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_terminal_default_71264.h"

typedef struct Wm71264Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm71264Rect;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
u8 D_800591AE;

static int failures;
static int events[8];
static int event_count;
static int state_calls;
static unsigned int state_arg;
static int clear_calls;
static Wm71264Rect observed_rect;
static unsigned int observed_r;
static unsigned int observed_g;
static unsigned int observed_b;
static int direct_sync_calls;
static int helper_calls;
static int main_calls;
static int main_arg;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

void wm_71264_test_event(int event)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0])))
        events[event_count] = event;
    event_count++;
}

void ChangeGameState(unsigned int state)
{
    state_calls++;
    state_arg = state;
}

int ClearImage(Wm71264Rect* rect, u8 r, u8 g, u8 b)
{
    clear_calls++;
    observed_rect = *rect;
    observed_r = r;
    observed_g = g;
    observed_b = b;
    return 0;
}

int DrawSync(int mode)
{
    check(mode == 0, "draw_sync.argument.zero");
    direct_sync_calls++;
    return 0;
}

void wm_800762FC(void) { helper_calls++; }

void MainLoop(int error_code)
{
    main_calls++;
    main_arg = error_code;
}

static void seed(void)
{
    memset(g_PsxRam, 0xCD, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    memset(events, 0, sizeof(events));
    event_count = 0;
    state_calls = 0;
    state_arg = 99u;
    clear_calls = 0;
    memset(&observed_rect, 0xA5, sizeof(observed_rect));
    observed_r = 99u;
    observed_g = 99u;
    observed_b = 99u;
    direct_sync_calls = 0;
    helper_calls = 0;
    main_calls = 0;
    main_arg = -1;
    D_800591AE = 0x5Bu;
    *(u8*)PSX_ADDR(0x800591AEu) = 0x6Au;
}

static void check_order(void)
{
    int i;
    check(event_count == 6, "order.event.count");
    if (event_count != 6)
        return;
    for (i = 0; i < 6; i++) {
        if (events[i] != i + 1) {
            check(0, "order.retail.sequence");
            return;
        }
    }
}

int main(void)
{
    seed();
    wm_71034_run_terminal_default_lane();

    check(state_calls == 1 && state_arg == 0u,
          "state.change.zero");
    check(clear_calls == 1, "clear.once");
    check(observed_rect.x == 0 && observed_rect.y == 0,
          "clear.origin.zero");
    check(observed_rect.w == 319 && observed_rect.h == 431,
          "clear.extent.319.431");
    check(observed_r == 0u && observed_g == 0u && observed_b == 64u,
          "clear.color.0.0.64");
    check(direct_sync_calls == 1, "draw_sync.direct.once");
    check(D_800591AE == 0u &&
          *(u8*)PSX_ADDR(0x800591AEu) == 0x6Au,
          "system.byte.native.authority");
    check(helper_calls == 1 && main_calls == 1,
          "epilogue.calls.once");
    check(main_arg == 0, "main_loop.argument.zero");
    check_order();

    if (failures != 0) {
        fprintf(stderr, "W34N32 DEFAULT TERMINAL CERTIFICATE: %d failure(s)\n",
                failures);
        return EXIT_FAILURE;
    }
    puts("W34N32 terminal default certificate PASS");
    return EXIT_SUCCESS;
}
