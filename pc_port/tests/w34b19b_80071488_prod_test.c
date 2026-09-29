/*
 * W34B19-B production-linked certificate for the second scheduler jal
 * at 0x80071488 / nop 0x8007148C.
 *
 * Retail slice [0x80071488, 0x80071490):
 *   80071488  0C025E00  jal 0x80097800
 *   8007148C  00000000  nop
 *   8 bytes, 2 instructions
 *   SHA-256 f082c48aadb535cf17202501b97d36df5f30688c9ecdaac5f5fc41f66b197893
 *
 * The accepted prologue is the input. This rung executes the jal once,
 * then hard-cuts before DrawSync at 0x80071490. 0x800925A0 is selected
 * and classified MISSING; its body is not implemented and must not run.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u16 g_C1ButtonState;
u16 g_C2ButtonState;
u16 g_C1ButtonStateReleased;
u16 g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce;
u16 g_C2ButtonStatePressedOnce;
u32 g_ArchiveDebugTable;

extern u32 wm_800967E4_dispatch_cd_work(void);
extern void wm_800712D0_frame_prologue(void);

#define TEST_POOL           0x800A0000u
#define TEST_OT0            0x800F0000u
#define TEST_OT1            0x800F1000u
#define OR_ENVREC0          0x8009BBC8u
#define OR_ENVREC1          0x8009BC40u
#define OR_DB_PTR           0x8009BE3Cu
#define OR_INDEX            0x8009D7F0u
#define OR_D554             0x8009D554u
#define OR_OT_OFF           0x70u
#define OR_ENV_STRIDE       0x78u
/* Retail Table A record 0 at 0x80099E8C (world_map.bin, load 0x8006FAF0):
 *   +0x00 cb0 = 0x800923A8
 *   +0x04 cb1 = 0x800925A0
 * RETAIL_DERIVED — not copied from production macros. */
#define RETAIL_TABLE_A0_CB0 0x800923A8u
#define RETAIL_TABLE_A0_CB1 0x800925A0u
#define CB1_WRONG           0x8008A72Cu

enum {
    TR_CALL = 5
};

enum {
    CALL_1D468 = 7,
    CALL_97800 = 8,
    CALL_DRAWSYNC = 9
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 address;
    u32 value;
} TraceEvent;

#define TRACE_CAP 64
static TraceEvent s_got[TRACE_CAP];
static size_t s_got_n;

static int s_pass;
static int s_fail;
static int s_total;

static int s_pop_calls;
static int s_vsync_calls;
static int s_cdsync_calls;
static int s_clear_calls;
static int s_250e0_calls;
static int s_1d468_calls;
static int s_drawsync_calls;
static int s_925a0_body_calls;

void wm_712d0_test_trace(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    (void)width;
    if (s_got_n < TRACE_CAP) {
        s_got[s_got_n].pc = pc;
        s_got[s_got_n].kind = kind;
        s_got[s_got_n].address = address;
        s_got[s_got_n].value = value;
    }
    s_got_n++;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

int ControllerPopState(void)
{
    s_pop_calls++;
    return 0;
}

int VSync(int mode)
{
    (void)mode;
    s_vsync_calls++;
    return 0;
}

int CdSync(int mode, u8* result)
{
    (void)mode;
    (void)result;
    s_cdsync_calls++;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    s_drawsync_calls++;
    return 0;
}

u32* ClearOTagR(u32* ot, int n)
{
    (void)n;
    s_clear_calls++;
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

static s16 pass1_923a8_stub(int slot_index)
{
    (void)slot_index;
    return 1;
}

#if defined(WM_71488_MUTANT_M6)
static s16 fake_925a0_resolve(int slot_index)
{
    (void)slot_index;
    s_925a0_body_calls++;
    return 1;
}
#endif

#if defined(WM_71488_MUTANT_M10)
void wm_71488_mutant_fake_925a0(void)
{
    s_925a0_body_calls++;
}
#endif

static void check_case(const char* fixture, const char* assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", fixture, assertion);
    }
}

static int event_index(u32 pc, u32 kind, u32 address)
{
    size_t i;
    for (i = 0; i < s_got_n && i < TRACE_CAP; i++) {
        if (s_got[i].pc == pc && s_got[i].kind == kind &&
            s_got[i].address == address)
            return (int)i;
    }
    return -1;
}

static u8* slot_ptr(int index)
{
    return (u8*)PSX_ADDR(TEST_POOL + (u32)index * WM_SCHED_SLOT_STRIDE);
}

static void plant_slot(int index, s16 state, u32 cb0, u32 cb1)
{
    u8* slot = slot_ptr(index);
    u32 base = TEST_POOL + (u32)index * WM_SCHED_SLOT_STRIDE;
    memset(slot, 0, WM_SCHED_SLOT_STRIDE);
    memcpy(slot + WM_SCHED_OFF_STATE, &state, sizeof(state));
    store_u32(base + WM_SCHED_OFF_CB0, cb0);
    store_u32(base + WM_SCHED_OFF_CB1, cb1);
}

static void seed_accepted_prologue_ram(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    store_u32(OR_ENVREC0 + OR_OT_OFF, TEST_OT0);
    store_u32(OR_ENVREC0 + OR_ENV_STRIDE + OR_OT_OFF, TEST_OT1);
    store_u32(OR_DB_PTR, 0x11111111u);
    store_u32(OR_INDEX, 0x22222222u);
    store_u32(OR_D554, 0x33333333u);
    store_u32(WM_SCHED_POOL_PTR, TEST_POOL);
}

static void reset_counts(void)
{
    s_pop_calls = s_vsync_calls = s_cdsync_calls = 0;
    s_clear_calls = s_250e0_calls = s_1d468_calls = 0;
    s_drawsync_calls = s_925a0_body_calls = 0;
    s_got_n = 0;
    g_C1ButtonState = g_C2ButtonState = 0;
    g_C1ButtonStateReleased = g_C2ButtonStateReleased = 0;
    g_C1ButtonStatePressedOnce = g_C2ButtonStatePressedOnce = 0;
    wm_fp_reset();
    wm_sched_reset();
    wm_sched_callback_registry_clear();
}

static void run_rung(const char* name)
{
    int i1d;
    int isched;
    int inspect_slot;
    s16 state_after_pass1;
    s16 state_after_pass2;
    u32 cb1;
    u32 expect_target;

#if defined(WM_71488_MUTANT_M9)
    inspect_slot = 1;
#else
    inspect_slot = 0;
#endif

#if defined(WM_71488_MUTANT_M7)
    cb1 = CB1_WRONG;
    expect_target = RETAIL_TABLE_A0_CB1;
#else
    cb1 = RETAIL_TABLE_A0_CB1;
    expect_target = RETAIL_TABLE_A0_CB1;
#endif

    seed_accepted_prologue_ram();
    reset_counts();
    plant_slot(inspect_slot, 0, RETAIL_TABLE_A0_CB0, cb1);

    /* Pass 1: register only the retail Table-A cb0 stub. Do not register
     * 0x800925A0. The stub returns 1 so slot state is produced by the
     * real scheduler, not planted. */
    wm_sched_callback_register(RETAIL_TABLE_A0_CB0, pass1_923a8_stub);
    wm_80097800();
    memcpy(&state_after_pass1,
           slot_ptr(inspect_slot) + WM_SCHED_OFF_STATE,
           sizeof(state_after_pass1));
    check_case(name, "pass1-publishes-state-0-to-1",
               wm_sched_get_completed_passes() == 1 &&
                   state_after_pass1 == 1 &&
                   wm_sched_get_entry() == 1);

#if defined(WM_71488_MUTANT_M8)
    {
        s16 forced = 0;
        memcpy(slot_ptr(inspect_slot) + WM_SCHED_OFF_STATE, &forced,
               sizeof(forced));
        state_after_pass1 = 0;
    }
#endif

#if defined(WM_71488_MUTANT_M6)
    wm_sched_callback_register(RETAIL_TABLE_A0_CB1, fake_925a0_resolve);
#endif

    wm_fp_reset();
    s_got_n = 0;
    wm_800712D0_frame_prologue();

    memcpy(&state_after_pass2,
           slot_ptr(inspect_slot) + WM_SCHED_OFF_STATE,
           sizeof(state_after_pass2));
    i1d = event_index(0x80071480u, TR_CALL, CALL_1D468);
    isched = event_index(0x80071488u, TR_CALL, CALL_97800);

    /* STRUCTURAL / PRODUCTION_DIAGNOSTIC */
    check_case(name, "func_8001D468-once", s_1d468_calls == 1);
    /* STRUCTURAL: this rung's jal happens once inside the frame helper. */
    check_case(name, "frame-driver-scheduler-once",
               wm_fp_get_scheduler_calls() == 1);
    /* RUNTIME_OBSERVED: pass 1 + this jal => entry 2. */
    check_case(name, "scheduler-entry-is-2",
               wm_sched_get_entry() == 2);
    /* INDEPENDENT_ORACLE: event order after accepted 1D468. */
    check_case(name, "scheduler-after-func_8001D468",
               i1d >= 0 && isched >= 0 && isched == i1d + 1);
    /* RETAIL_DERIVED: hard cut is the jal return PC 0x80071490. */
    check_case(name, "hard-cut-pc-0x80071490",
               wm_fp_get_cut_pc() == 0x80071490u);
    check_case(name, "no-DrawSync-beyond-0x80071490",
               s_drawsync_calls == 0);
    /* RETAIL_DERIVED: scheduler walks slot 0 first. */
    check_case(name, "slot0-first-occupied",
               wm_sched_get_last_slot() == 0);
    /* RETAIL_DERIVED: state 1 selects +0x1C / cb1. */
    check_case(name, "slot0-state-1-selects-cb1",
               wm_sched_get_last_callback_state() == 1);
    /* RETAIL_DERIVED: Table A record 0 cb1. */
    check_case(name, "target-0x800925A0",
               wm_sched_get_last_callback() == expect_target);
    /* STRUCTURAL: 925A0 is known-missing, no production body. */
    check_case(name, "resolver-missing-unresolved",
               wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                   wm_sched_get_missing_hits() == 1);
    /* MUTATION_META / STRUCTURAL */
    check_case(name, "925A0-body-not-executed",
               s_925a0_body_calls == 0 &&
                   wm_sched_get_callbacks_executed() == 1);
    check_case(name, "slot0-state-unchanged",
               state_after_pass2 == 1);
    check_case(name, "callback-frontier-0x800925A0",
               wm_sched_get_frontier_pc() == RETAIL_TABLE_A0_CB1);
    check_case(name, "no-second-scheduler-later-in-rung",
               wm_fp_get_scheduler_calls() == 1);
}

int main(void)
{
    printf("=== W34B19-B 0x80071488 second scheduler jal ===\n");
    printf("RETAIL_71488_BOUNDARY [0x80071488,0x80071490) bytes=8 insns=2\n");
    printf("RETAIL_71488_SHA256 f082c48aadb535cf17202501b97d36df5f30688c9ecdaac5f5fc41f66b197893\n");
    printf("SUBJECT_71488 pc_port/src/world_map_frame_driver.c::wm_800712D0_frame_prologue\n");
    printf("CALLBACK_FRONTIER 0x800925A0\n");
    printf("FRAME_DRIVER_FRONTIER 0x80071490\n");

    run_rung("natural-pass2-missing-925A0");

    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
