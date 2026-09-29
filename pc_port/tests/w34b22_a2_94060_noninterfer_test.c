/*
 * Natural noninterference certificate for 0x80094060.
 *
 * Uses the accepted controller-idle D_800AFE9C / pad schedule.  Does not
 * force the helper, 0x80095414, 0x8008A72C, a PC, or a resolver.  The
 * helper must not run.  Frontiers stay at 0x8008A72C / 0x80071490.
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_driver.h"
#include "world_map_helper_94060.h"
#include "world_map_scheduler.h"

#define TEST_POOL        0x800A8000u
#define TEST_OT0         0x800F0000u
#define TEST_OT1         0x800F1000u
#define POOL_PTR         0x8009BE24u
#define ENVREC0          0x8009BBC8u
#define ENVREC1          0x8009BC40u
#define CONTEXT_PTR      0x8009BE3Cu
#define INDEX_ADDR       0x8009D7F0u
#define D554             0x8009D554u
#define OT_OFF           0x70u
#define AFE9C            0x800AFE9Cu

#define CB0_SLOT0        0x800923A8u
#define CB1_SLOT0        0x800925A0u
#define CB0_SLOT1        0x8008A2C8u
#define CB1_SLOT1        0x8008A72Cu

u16 g_C1ButtonState;
u16 g_C2ButtonState;
u16 g_C1ButtonStateReleased;
u16 g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce;
u16 g_C2ButtonStatePressedOnce;

static int s_fail;

static void check(const char *name, int cond)
{
    if (cond) {
        printf("  PASS: %s\n", name);
        return;
    }
    s_fail++;
    printf("  FAIL: %s\n", name);
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 load_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

int ControllerPopState(void)
{
    return 0;
}

int VSync(int mode)
{
    (void)mode;
    return 0;
}

/* Link-only providers for the 0x80071490 prologue's GPU/system calls. The
 * test asserts dispatcher/scheduler/frontier state, never GPU effects;
 * return values are discarded at every call site. */
int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

void GameCheckAndHandleSoftReset(void)
{
}

void PutDispEnv(void *env)
{
    (void)env;
}

void PutDrawEnv(void *env)
{
    (void)env;
}

int CdSync(int mode, u8 *result)
{
    (void)mode;
    (void)result;
    return 0;
}

u32 *ClearOTagR(u32 *ot, int n)
{
    (void)n;
    return ot;
}

void func_800250E0(int context)
{
    (void)context;
}

void func_8001D468(void)
{
}

u32 wm_800967E4_dispatch_cd_work(void)
{
    return 0u;
}

static s16 accepted_cb(int slot)
{
    (void)slot;
    return 1;
}

static void plant_slot(int index, s16 state, u32 cb0, u32 cb1)
{
    u32 slot = TEST_POOL + (u32)index * WM_SCHED_SLOT_STRIDE;
    memset(PSX_ADDR(slot), 0, WM_SCHED_SLOT_STRIDE);
    store_u16(slot + WM_SCHED_OFF_STATE, (u16)state);
    store_u32(slot + WM_SCHED_OFF_CB0, cb0);
    store_u32(slot + WM_SCHED_OFF_CB1, cb1);
}

int main(void)
{
    PsxMemory_Init();
    wm_80094060_reset_exec_count();

    store_u16(AFE9C, 0);
    g_C1ButtonState = 0;
    g_C2ButtonState = 0;
    g_C1ButtonStateReleased = 0;
    g_C2ButtonStateReleased = 0;
    g_C1ButtonStatePressedOnce = 0;
    g_C2ButtonStatePressedOnce = 0;

    store_u32(POOL_PTR, TEST_POOL);
    store_u32(ENVREC0 + OT_OFF, TEST_OT0);
    store_u32(ENVREC1 + OT_OFF, TEST_OT1);
    store_u32(CONTEXT_PTR, ENVREC0);
    store_u32(INDEX_ADDR, 0u);
    store_u32(D554, 0u);
    memset(PSX_ADDR(TEST_POOL), 0,
           (size_t)WM_SCHED_SLOT_COUNT * WM_SCHED_SLOT_STRIDE);
    plant_slot(0, 0, CB0_SLOT0, CB1_SLOT0);
    plant_slot(1, 0, CB0_SLOT1, CB1_SLOT1);

    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_fp_reset();
    wm_sched_callback_register(CB0_SLOT0, accepted_cb);
    wm_sched_callback_register(CB0_SLOT1, accepted_cb);
    wm_sched_callback_register(CB1_SLOT0, accepted_cb);

    wm_80097800();
    check("first-pass-states-to-1",
          load_s16(TEST_POOL + WM_SCHED_OFF_STATE) == 1 &&
              load_s16(TEST_POOL + WM_SCHED_SLOT_STRIDE +
                       WM_SCHED_OFF_STATE) == 1);

    wm_800712D0_frame_prologue();

    check("helper-never-ran", wm_80094060_get_exec_count() == 0u);
    check("callback-frontier-8008A72C",
          wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
              wm_sched_get_frontier_pc() == CB1_SLOT1);
    check("frame-frontier-80071490",
          wm_fp_get_cut_pc() == 0x80071490u);

    if (s_fail != 0) {
        printf("NONINTERFER FAIL\n");
        return 1;
    }
    printf("NONINTERFER PASS exec=0 cb=0x8008A72C frame=0x80071490\n");
    return 0;
}
