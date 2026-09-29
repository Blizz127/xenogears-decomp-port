/* W34B26 production-linked certificate for retail 0x80071490..0x800714C4.
 * The test exercises the real frame-prologue object and verifies the exact
 * post-pass call order plus PSX_ADDR mapping of both environment pointers. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
u16 g_C1ButtonState;
u16 g_C2ButtonState;
u16 g_C1ButtonStateReleased;
u16 g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce;
u16 g_C2ButtonStatePressedOnce;
u32 g_ArchiveDebugTable;

#define ENVREC0 0x8009BBC8u
#define ENVREC1 0x8009BC40u
#define DB_PTR  0x8009BE3Cu
#define INDEX   0x8009D7F0u
#define D554    0x8009D554u
#define CDSYNC  0x8009C588u
#define OT_OFF  0x70u
#define ENV_STRIDE 0x78u

enum {
    CALL_DRAWSYNC = 1,
    CALL_VSYNC = 2,
    CALL_SOFT_RESET = 3,
    CALL_PUT_DISP = 4,
    CALL_PUT_DRAW = 5
};

static int s_pass;
static int s_fail;
static int s_total;
static int s_order[8];
static int s_order_n;
static int s_drawsync_mode;
static int s_vsync_mode;
static int s_cdsync_mode;
static u8 *s_cdsync_result;
static void *s_put_disp;
static void *s_put_draw;
static int s_scheduler_calls;
static int s_gamecheck_calls;
/* (W34B38: ClearOTagR call-spy state removed; size verified in guest RAM.) */
static int s_250e0_calls;
static int s_1d468_calls;

void wm_712d0_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    (void)pc;
    (void)kind;
    (void)address;
    (void)width;
    (void)value;
}

static void check_case(const char *name, const char *assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", name, assertion);
    }
}

static void record_call(int call)
{
    if (s_order_n < (int)(sizeof(s_order) / sizeof(s_order[0])))
        s_order[s_order_n] = call;
    s_order_n++;
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

int ControllerPopState(void)
{
    return 0;
}

int VSync(int mode)
{
    s_vsync_mode = mode;
    record_call(CALL_VSYNC);
    return 0;
}

int CdSync(int mode, u8 *result)
{
    s_cdsync_mode = mode;
    s_cdsync_result = result;
    return 0;
}

u32 *ClearOTagR(u32 *ot, int n)
{
    (void)ot;
    (void)n;
    return ot;
}

void func_800250E0(int context)
{
    (void)context;
    s_250e0_calls++;
}

void func_8001D468(void)
{
    s_1d468_calls++;
}

u32 wm_800967E4_dispatch_cd_work(void)
{
    return 0;
}

void wm_80097800(void)
{
    s_scheduler_calls++;
}

int DrawSync(int mode)
{
    s_drawsync_mode = mode;
    record_call(CALL_DRAWSYNC);
    return 0;
}

void GameCheckAndHandleSoftReset(void)
{
    s_gamecheck_calls++;
    record_call(CALL_SOFT_RESET);
}

void PutDispEnv(void *env)
{
    s_put_disp = env;
    record_call(CALL_PUT_DISP);
}

void PutDrawEnv(void *env)
{
    s_put_draw = env;
    record_call(CALL_PUT_DRAW);
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(s_order, 0, sizeof(s_order));
    s_order_n = 0;
    s_drawsync_mode = -1;
    s_vsync_mode = -1;
    s_cdsync_mode = -1;
    s_cdsync_result = NULL;
    s_put_disp = NULL;
    s_put_draw = NULL;
    s_scheduler_calls = 0;
    s_gamecheck_calls = 0;
    /* (W34B38: no ClearOTagR spy state to reset.) */
    s_250e0_calls = 0;
    s_1d468_calls = 0;
    g_C1ButtonState = 0;
    g_C2ButtonState = 0;
    g_C1ButtonStateReleased = 0;
    g_C2ButtonStateReleased = 0;
    g_C1ButtonStatePressedOnce = 0;
    g_C2ButtonStatePressedOnce = 0;
    store_u32(ENVREC0 + OT_OFF, 0x800F0000u);
    store_u32(ENVREC1 + OT_OFF, 0x800F1000u);
    wm_fp_reset();
}

static void run_fixture(const char *name)
{
    reset_fixture();
    store_u32(DB_PTR, 0x13572468u);
    store_u32(INDEX, 0x24681357u);
    store_u32(D554, 0x89ABCDEFu);
    store_u32(CDSYNC, 0xDEADBEEFu);
    wm_800712D0_frame_prologue();

    check_case(name, "cut-advances-to-0x800714D4",
               wm_fp_get_cut_pc() == 0x800714D4u);
    check_case(name, "scheduler-second-pass-once", s_scheduler_calls == 1);
    check_case(name, "drawsync-mode-zero", s_drawsync_mode == 0);
    check_case(name, "vsync-mode-two", s_vsync_mode == 2);
    check_case(name, "soft-reset-called-once", s_gamecheck_calls == 1);
    check_case(name, "cdsync-mode-one", s_cdsync_mode == 1);
    check_case(name, "cdsync-buffer-is-guest-mapped",
               s_cdsync_result == PSX_ADDR(CDSYNC));
    {
        /* W34B38: the prologue clears via the guest-native path, not the
         * PSYQ ClearOTagR call, so the call spy never fires. Verify the
         * retail size (0x400 entries) and link pattern in guest RAM
         * instead. env provably stays at ENVREC0 (put-disp assertion). */
        u32 ot = 0x800F0000u;
        u32 i;
        int cleared = load_u32(ot) == 0x00FFFFFFu;
        for (i = 1u; cleared && i < 0x400u; i++)
            cleared = load_u32(ot + i * 4u) ==
                      ((ot + (i - 1u) * 4u) & 0x00FFFFFFu);
        check_case(name, "clear-otag-retail-size", cleared);
    }
    check_case(name, "put-disp-uses-env-plus-0x5c",
               s_put_disp == (void *)PSX_ADDR(ENVREC0 + 0x5Cu));
    check_case(name, "put-draw-uses-env-base",
               s_put_draw == (void *)PSX_ADDR(ENVREC0));
    check_case(name, "retail-call-order",
               s_order_n == 5 && s_order[0] == CALL_DRAWSYNC &&
                   s_order[1] == CALL_VSYNC && s_order[2] == CALL_SOFT_RESET &&
                   s_order[3] == CALL_PUT_DISP && s_order[4] == CALL_PUT_DRAW);
    check_case(name, "pre-frontier-state-preserved",
               load_u32(INDEX) == 0u && load_u32(D554) == 1u);
}

int main(void)
{
    printf("=== W34B26 0x80071490 post-pass continuation ===\n");
    printf("RETAIL_BOUNDARY [0x80071490,0x800714D4) bytes=68 insns=17\n");
    printf("SUBJECT pc_port/src/world_map_frame_driver.c::wm_800712D0_frame_prologue\n");
    run_fixture("asymmetric-env-and-sync-fixture");
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
