/* Production-linked certificate for WorldMapMain's retail D7CC==0 lane. */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_capture.h"
#include "world_map_frame_driver_712d0.h"
#include "world_map_main_loop_71034.h"
#include "world_map_terminal_zero_710e4.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u8 D_800591AE;
u16 D_8006F94E;
u16 D_8006F950;
u16 D_8006F954;
u16 D_800AFE9C;

#define BBC4       0x8009BBC4u
#define BD0C       0x8009BD0Cu
#define BD3A       0x8009BD3Au
#define D7CC       0x8009D7CCu
#define D7D8       0x8009D7D8u
#define EF64       0x8006EF64u
#define EF68       0x8006EF68u
#define OLD_RECORD 0x800B1000u
#define NEW_RECORD 0x800B1100u

static int failures;
static int events[32];
static int event_count;
static int helper_calls;
static u32 helper_a0;
static u32 helper_a1;
static s32 helper_a2;
static int helper_moves_record;
static int overlay_calls;
static unsigned int overlay_arg;
static int state_calls;
static unsigned int state_arg;
static int sync_calls;
static int main_loop_calls;
static int main_loop_arg;
static int slot1_calls;
static int slot2_calls;
static int scheduler_calls;
static int driver_calls;
static int capture_resets;
static int capture_finishes;
static int outer_stage;
static int outer_order_errors;
static int terminal_default_calls;
static s32 driver_exit_state;
static int observed_frame_limit;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_counters(void)
{
    memset(events, 0, sizeof(events));
    event_count = 0;
    helper_calls = 0;
    helper_a0 = 0u;
    helper_a1 = 0u;
    helper_a2 = 0;
    helper_moves_record = 0;
    overlay_calls = 0;
    overlay_arg = 0u;
    state_calls = 0;
    state_arg = 0u;
    sync_calls = 0;
    main_loop_calls = 0;
    main_loop_arg = -1;
    slot1_calls = 0;
    slot2_calls = 0;
    scheduler_calls = 0;
    driver_calls = 0;
    capture_resets = 0;
    capture_finishes = 0;
    outer_stage = 0;
    outer_order_errors = 0;
    terminal_default_calls = 0;
    driver_exit_state = 0;
    observed_frame_limit = -99;
}

static void seed(u32 bbc4, s16 type)
{
    memset(g_PsxRam, 0xCD, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    reset_counters();
    sw(BBC4, bbc4);
    sw(BD0C, 0x1234F900u);
    sh(BD3A, 0x2468u);
    sh(EF64, 0x1357u);
    sw(D7D8, OLD_RECORD);
    sh(OLD_RECORD + 8u, 0x1111u);
    sh(OLD_RECORD + 0xAu, 0x2222u);
    sh(OLD_RECORD + 0xEu, (u16)type);
    sh(NEW_RECORD + 8u, 0x3333u);
    sh(NEW_RECORD + 0xAu, 0x4444u);
    sh(NEW_RECORD + 0xEu, 3u);
    sh(EF68, 0xAAAAu);

    D_8006F950 = 0xA950u;
    D_8006F94E = 0xA94Eu;
    D_8006F954 = 0xA954u;
    D_800591AE = 0x7Bu;
    sh(0x8006F950u, 0xB950u);
    sh(0x8006F94Eu, 0xB94Eu);
    sh(0x8006F954u, 0xB954u);
    *(u8*)PSX_ADDR(0x800591AEu) = 0x6Au;
}

void wm_710e4_test_event(int event)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0])))
        events[event_count] = event;
    event_count++;
}

void* LoadGameStateOverlay(unsigned int overlay_index)
{
    overlay_calls++;
    overlay_arg = overlay_index;
    if (outer_stage == 7)
        outer_stage = 8;
    return PSX_ADDR(0x800C0000u);
}

void ChangeGameState(unsigned int state)
{
    state_calls++;
    state_arg = state;
    if (outer_stage == 8)
        outer_stage = 9;
}

void MainLoop(int error_code)
{
    main_loop_calls++;
    main_loop_arg = error_code;
    if (outer_stage == 9)
        outer_stage = 10;
}

s32 wm_80094364(u32 a0, u32 a1, s32 a2)
{
    helper_calls++;
    helper_a0 = a0;
    helper_a1 = a1;
    helper_a2 = a2;
    if (helper_moves_record != 0)
        sw(D7D8, NEW_RECORD);
    return helper_moves_record;
}

void wm_800762FC(void)
{
    sync_calls++;
}

static void check_events(const int* expected, int count, const char* name)
{
    int i;
    if (event_count != count) {
        check(0, name);
        return;
    }
    for (i = 0; i < count; i++) {
        if (events[i] != expected[i]) {
            check(0, name);
            return;
        }
    }
}

static void check_common(void)
{
    check(overlay_calls == 1 && overlay_arg == 1u,
          "overlay.argument.once");
    check(state_calls == 1 && state_arg == 1u,
          "state.argument.once");
    check(lhu(EF68) == (u16)(0x1234F900u + 0x400u),
          "ef68.bd0c.plus.400");
    check(D_800591AE == 0u && *(u8*)PSX_ADDR(0x800591AEu) == 0x6Au,
          "system.byte.native.authority");
    check(sync_calls == 1 && main_loop_calls == 1 && main_loop_arg == 0,
          "epilogue.sync.before.mainloop");
}

static void test_guard_skip(void)
{
    static const int expected[] = {1, 2, 7, 8, 9, 10};

    seed(1u, 3);
    sw(D7D8, 0xFFFFFFFFu);
    wm_71034_run_terminal_zero_lane();
    check(helper_calls == 0, "guard.bbc4.skips.region");
    check(D_8006F950 == 0xA950u && D_8006F94E == 0xA94Eu &&
          D_8006F954 == 0xA954u, "guard.bbc4.skips.outputs");
    check_events(expected, 6, "order.guard.skip");
    check_common();
}

static void test_type3_reload_and_outputs(void)
{
    static const int expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    seed(0u, 3);
    helper_moves_record = 1;
    wm_71034_run_terminal_zero_lane();
    check(helper_calls == 1, "guard.type3.calls.helper");
    check(helper_a0 == 0x8009D55Cu && helper_a1 == 3u &&
          helper_a2 == 0x1357, "helper.retail.arguments");
    check(D_8006F950 == 0x2468u && D_8006F94E == 0x3333u &&
          D_8006F954 == 0x4444u, "outputs.reload.helper.record");
    check(lhu(0x8006F950u) == 0xB950u &&
          lhu(0x8006F94Eu) == 0xB94Eu &&
          lhu(0x8006F954u) == 0xB954u,
          "outputs.native.authority");
    check_events(expected, 10, "order.region.arm");
    check_common();
}

static void test_non_type3_publishes_without_helper(void)
{
    static const int expected[] = {1, 2, 4, 5, 6, 7, 8, 9, 10};

    seed(0u, 2);
    wm_71034_run_terminal_zero_lane();
    check(helper_calls == 0, "guard.non3.skips.helper");
    check(D_8006F950 == 0x2468u && D_8006F94E == 0x1111u &&
          D_8006F954 == 0x2222u, "outputs.current.record");
    check_events(expected, 9, "order.region.no.helper");
    check_common();
}

/* Main-loop dependencies for the product integration check. */
void PcPort_WorldCaptureReset(void) { capture_resets++; }
void PcPort_WorldCaptureSetFrame(int frame) { (void)frame; }
int PcPort_WorldCaptureRequest(int frame, const char* path)
{ (void)frame; (void)path; return 0; }
int PcPort_WorldCaptureFrameComplete(int frame) { (void)frame; return 0; }
int PcPort_WorldCaptureFinish(void) { capture_finishes++; return 0; }
void PcPort_WorldCaptureFulfillAtPresent(void) {}
int PcPort_WorldCapturePending(void) { return 0; }
int PcPort_WorldCaptureLastFulfilledFrame(void) { return 0; }
void PcPort_TestInputAdvanceFrame(void) {}
void PcPort_TestInputInject(u16* held_buttons) { (void)held_buttons; }
void PsyX_EndScene(void) {}

int wm_80072238(void)
{
    slot1_calls++;
    if (outer_stage != 0)
        outer_order_errors++;
    outer_stage = 1;
    return 0;
}

int wm_80077214(void) { return 0; }
void wm_80077480(void) {}
int wm_80077A64(void) { return 0; }
void wm_80077CC0(void) {}

void wm_80097800(void)
{
    scheduler_calls++;
    if (outer_stage != 1)
        outer_order_errors++;
    outer_stage = 2;
}

void DrawSync(void (*func)(unsigned long))
{
    (void)func;
    if (outer_stage != 2)
        outer_order_errors++;
    outer_stage = 3;
}

void Vsync(long mode)
{
    (void)mode;
    if (outer_stage != 3)
        outer_order_errors++;
    outer_stage = 4;
}

void ControllerResetState(void)
{
    if (outer_stage != 4)
        outer_order_errors++;
    outer_stage = 5;
}

Wm712D0RunResult wm_800712D0_run_bounded(Wm712D0BoundedRun* run)
{
    observed_frame_limit = run->frame_limit;
    driver_calls++;
    if (outer_stage != 5)
        outer_order_errors++;
    outer_stage = 6;
    sw(D7CC, (u32)driver_exit_state);
    return WM_712D0_RUN_NATURAL_EXIT;
}

void wm_8007299C(void)
{
    slot2_calls++;
    if (outer_stage != 6)
        outer_order_errors++;
    outer_stage = 7;
}

void wm_71034_run_terminal_one_lane(void) { abort(); }
void wm_71034_run_terminal_default_lane(void)
{
    terminal_default_calls++;
    if (outer_stage != 7)
        outer_order_errors++;
    outer_stage = 10;
}

static void test_main_loop_integration(void)
{
    seed(1u, 3);
    sw(D7D8, 0xFFFFFFFFu);
    sw(0x8009C5A8u, 0u);
    sw(D7CC, 2u);
    sw(0x8009A05Cu, 0x80072238u);
    sw(0x8009A060u, 0x8007299Cu);
    if (setenv("XENO_WORLD_FRAME_LIMIT", "120", 1) != 0) {
        perror("setenv");
        exit(EXIT_FAILURE);
    }

    wm_80071034();
    check(slot1_calls == 1 && scheduler_calls == 1 && driver_calls == 1 &&
          slot2_calls == 1, "integration.session.order.counts");
    check(outer_order_errors == 0 && outer_stage == 10,
          "integration.slot2.to.terminal.order");
    check(capture_resets == 1 && capture_finishes == 1,
          "integration.capture.closed.before.terminal");
    check(main_loop_calls == 1, "integration.d7cc0.dispatches.terminal");

    seed(1u, 3);
    sw(0x8009C5A8u, 0u);
    sw(D7CC, 2u);
    sw(0x8009A05Cu, 0x80072238u);
    sw(0x8009A060u, 0x8007299Cu);
    driver_exit_state = -1;
    wm_80071034();
    check(slot1_calls == 1 && scheduler_calls == 1 && driver_calls == 1 &&
          slot2_calls == 1, "integration.default.session.order.counts");
    check(outer_order_errors == 0 && outer_stage == 10,
          "integration.default.slot2.to.terminal.order");
    check(terminal_default_calls == 1 && main_loop_calls == 0,
          "integration.signed.default.dispatches.terminal");
    check(observed_frame_limit == 120,
          "integration.explicit.bound.forwarded");

    if (unsetenv("XENO_WORLD_FRAME_LIMIT") != 0) {
        perror("unsetenv");
        exit(EXIT_FAILURE);
    }
    seed(1u, 3);
    sw(0x8009C5A8u, 0u);
    sw(D7CC, 2u);
    sw(0x8009A05Cu, 0x80072238u);
    sw(0x8009A060u, 0x8007299Cu);
    driver_exit_state = -1;
    wm_80071034();
    check(observed_frame_limit == 0,
          "integration.absent.bound.means.unbounded");
    check(terminal_default_calls == 1,
          "integration.unbounded.natural.exit.reaches.terminal");
}

int main(void)
{
    test_guard_skip();
    test_type3_reload_and_outputs();
    test_non_type3_publishes_without_helper();
    test_main_loop_integration();

    if (failures != 0) {
        fprintf(stderr, "W34N28 TERMINAL CERTIFICATE: %d failure(s)\n",
                failures);
        return EXIT_FAILURE;
    }
    puts("W34N28 terminal zero certificate PASS");
    return EXIT_SUCCESS;
}
