/* Unit proof for XENO_FIELD_STEP_PAD_INPUT (pc_port/src/test_input.c).
 *
 * Links the real retail controller.c (ControllerPoll / ControllerPushState /
 * ControllerPopState) and the real test_input.c.  The runner lifts the
 * production FieldPollControllers drain (src/field/main/misc2.c) and
 * PcPort_PadTestGameToPhysical (pc_port/src/psyq_compat.c) verbatim into
 * field_step_pad_lifted.h, so the masks and the remap tested here are the
 * shipped ones.  Each simulated vblank runs the order of func_8003634C:
 * real pad read -> step-pad apply -> ControllerPoll -> ControllerPushState.
 *
 * This is not runtime evidence: no game, no field loop, no timing.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/controller.h"

/* controller.c state */
u_short g_C1ButtonState, g_C1ButtonStatePressedOnce, g_C1ButtonStateReleased;
u_short g_C2ButtonState, g_C2ButtonStatePressedOnce, g_C2ButtonStateReleased;
u_short g_C1ButtonStatesPressed[16], g_C1ButtonStatesPressedOnce[16];
u_short g_C1ButtonStatesReleased[16], g_C2ButtonStatesPressed[16];
u_short g_C2ButtonStatesPressedOnce[16], g_C2ButtonStatesReleased[16];
int g_ControllerCurStateWriteIndex, g_ControllerCurStateReadIndex;
int g_ControllerIsStateStackFull, D_80050200;
u_int g_ControllerNumStates;
short D_800594EC, D_800594E8, D_800594E0, D_800594DC, D_800595CC, D_800595C8;
u_char g_C1LeftStickXAxis, g_C1LeftStickYAxis, g_C1RightStickXAxis, g_C1RightStickYAxis;
u_char g_C2LeftStickXAxis, g_C2LeftStickYAxis, g_C2RightStickXAxis, g_C2RightStickYAxis;
int g_C1PrevButtonState, g_C2PrevButtonState, D_8005022C, D_80050230, D_80059488;
u_char g_ControllerStickToAnalogX[0x10], g_ControllerStickToAnalogY[0x10];
u_char g_ControllerType;
u_char g_C1Buffer[2 * CONTROLLER_BUFFER_SIZE];
u_char D_800625FD[2 * CONTROLLER_BUFFER_SIZE];
u_char g_C2Buffer[CONTROLLER_BUFFER_SIZE];
u8 D_80062600, D_80062601, D_80062602, D_80062603;
u8 D_80062622, D_80062623, D_80062624, D_80062625;
u_char g_ControllerButtonMappings[8];
u_short g_ControllerButtonMasks[8] = {
    CTRL_BTN_CIRCLE, CTRL_BTN_CROSS, CTRL_BTN_TRIANGLE, CTRL_BTN_SQUARE,
    CTRL_BTN_L1, CTRL_BTN_L2, CTRL_BTN_R1, CTRL_BTN_R2};

/* FieldPollControllers state */
u16 D_800AFE9C, D_800AFEA0, D_800C2694, D_800C38F8, D_800C3900, D_800C3908;
u16 D_800ADB00 = 0xFFFF;
struct { short controllerBtnMask; } g_FieldControl = {(short)0xFFFF};

extern void ControllerPoll(void);
extern void ControllerPushState(void);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int PcPort_FieldStepPadApply(unsigned char *, unsigned short (*)(unsigned short), int);
extern void PcPort_FieldStepPadCounts(unsigned *, unsigned *);

#include "field_step_pad_lifted.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "FIELD STEP PAD FAIL line=%d %s\n", __LINE__, #x); \
    exit(1); \
} } while (0)

static int s_map;
static unsigned short s_real;   /* game bits a real pad holds (physical = same here) */

static void pad_read(void)
{
    /* What PsyX_UpdateInput leaves: digital pad, all buttons up unless the
     * "real" player holds some; controller 2 absent. */
    memset(g_C1Buffer, 0xFF, sizeof(g_C1Buffer));
    g_C1Buffer[CONTROLLER_STATUS] = CONTROLLER_STATUS_SUCCESS;
    g_C1Buffer[CONTROLLER_TYPE] = 0x41;
    g_C1Buffer[0x2] &= (u_char)~(s_real >> 8);
    g_C1Buffer[0x3] &= (u_char)~(s_real & 0xFF);
}

static void vblank(void)
{
    D_80059488++;
    pad_read();
    CHECK(PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical, s_map) == 0);
    ControllerPoll();
    ControllerPushState();
}

static void reset_controller(void)
{
    ControllerResetState();
    g_C1PrevButtonState = g_C2PrevButtonState = 0;
    D_8005022C = D_80050230 = 0;
    D_80059488 = 0;
}

/* Schedule used by the main mode (game bits). */
#define SCHEDULE "0:0,3:0x2000,6:0,9:0x8020,11:0x0040,13:0"
static unsigned short schedule_value(unsigned step)
{
    if (step < 3) return 0;
    if (step < 6) return 0x2000;
    if (step < 9) return 0;
    if (step < 11) return 0x8020;
    if (step < 13) return 0x0040;
    return 0;
}

/* One pattern of vblanks per field step; 0 = a drain that saw no vblank. */
static const unsigned kPatterns[][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 3, 1, 4, 2, 1, 3, 2, 1, 4, 1, 2, 3, 1, 2, 1},
    {1, 2, 0, 3, 1, 0, 2, 1, 2, 0, 3, 1, 1, 0, 2, 1},
};

typedef struct { u16 held[16], pressed[16]; unsigned vb[16]; } Trace;

static void run_pattern(const unsigned *pattern, u16 btn_mask, u16 post_mask, Trace *out)
{
    unsigned step;

    g_FieldControl.controllerBtnMask = (short)btn_mask;
    D_800ADB00 = post_mask;
    for (step = 0; step < 16; step++) {
        for (unsigned v = 0; v < pattern[step]; v++)
            vblank();
        FieldPollControllers();
        out->held[step] = D_800AFE9C;
        out->pressed[step] = D_800C2694;
        out->vb[step] = pattern[step];
    }
}

/* I1/I2/I3: per-step held equals value(k) under masks for every cadence
 * (each pattern runs in its own process from step 0); press edges appear
 * once, at the first non-empty step of a transition; empty drains see
 * nothing and are counted. */
static void mode_main(int swapped, unsigned p)
{
    static const u_char identity[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    static const u_char usa[8] = {1, 0, 3, 2, 4, 5, 6, 7};
    unsigned steps, empty, expected_empty = 0;
    u16 previous = 0;
    Trace t;

    setenv("XENO_FIELD_STEP_PAD_INPUT", SCHEDULE, 1);
    memcpy(g_ControllerButtonMappings, swapped ? usa : identity, 8);
    reset_controller();
    run_pattern(kPatterns[p], 0xFFFF, 0xFFFF, &t);
    for (unsigned step = 0; step < 16; step++) {
        u16 want = schedule_value(step);
        if (t.vb[step] == 0) {
            CHECK(t.held[step] == 0 && t.pressed[step] == 0);
            expected_empty++;
            continue;
        }
        CHECK(t.held[step] == want);
        CHECK(t.pressed[step] == (u16)(want & ~previous));
        previous = want;
    }
    PcPort_FieldStepPadCounts(&steps, &empty);
    CHECK(steps == 16 && empty == expected_empty);
    CHECK(g_ControllerIsStateStackFull == 0);
    printf("FIELD STEP PAD PASS main layout=%s pattern=%u steps=%u empty=%u\n",
           swapped ? "usa-swap" : "identity", p, steps, empty);
}

/* I4: per-state and post-drain masks still remove scheduled bits. */
static void mode_masks(void)
{
    Trace t;

    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0x2020", 1);
    memcpy(g_ControllerButtonMappings, (u_char[8]){1, 0, 3, 2, 4, 5, 6, 7}, 8);
    reset_controller();
    run_pattern(kPatterns[1], (u16)~0x2000, 0xFFFF, &t);   /* per-state mask drops Right */
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0x0020);
    run_pattern(kPatterns[1], 0xFFFF, (u16)~0x0020, &t);   /* D_800ADB00 drops Circle */
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0x2000);
    printf("FIELD STEP PAD PASS masks: controllerBtnMask and D_800ADB00 applied\n");
}

/* Contrast: the existing XENO_FIELD_TEST_INPUT merge ORs AFTER the masks. */
static void mode_post_mask_contrast(void)
{
    extern void PcPort_FieldTestInputMerge(u16 *, u16 *, u16 *);
    (void)PcPort_FieldTestInputMerge;
    Trace t;

    setenv("XENO_FIELD_TEST_INPUT", "0:0x2000", 1);
    reset_controller();
    run_pattern(kPatterns[0], (u16)~0x2000, (u16)~0x2000, &t);
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0x2000);   /* masked bit still present: bypass */
    printf("FIELD STEP PAD PASS contrast: XENO_FIELD_TEST_INPUT bypasses both masks\n");
}

/* I5: FIFO overflow is preserved, not compensated. */
static void mode_overflow(void)
{
    static const unsigned burst[16] = {20};
    Trace t;

    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0x1000", 1);
    reset_controller();
    run_pattern(burst, 0xFFFF, 0xFFFF, &t);
    CHECK(g_ControllerIsStateStackFull == 1);
    CHECK(t.held[0] == 0x1000);
    printf("FIELD STEP PAD PASS overflow: 20 vblanks in one step keep FIFO overflow\n");
}

/* I6: arming waits for the requested map; drains before it are not steps. */
static void mode_map(void)
{
    unsigned steps, empty;
    Trace t;

    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0x2000", 1);
    setenv("XENO_FIELD_STEP_PAD_MAP", "14", 1);
    reset_controller();
    s_map = 3;
    run_pattern(kPatterns[1], 0xFFFF, 0xFFFF, &t);
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0);
    PcPort_FieldStepPadCounts(&steps, &empty);
    CHECK(steps == 0);
    s_map = 0x100E;   /* low 12 bits select field 14 */
    run_pattern(kPatterns[1], 0xFFFF, 0xFFFF, &t);
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0x2000);
    PcPort_FieldStepPadCounts(&steps, &empty);
    CHECK(steps == 16);
    printf("FIELD STEP PAD PASS map: armed only in field 14\n");
}

/* I7: unset leaves the pad buffer untouched; real input passes through. */
static void mode_unset(void)
{
    Trace t;

    reset_controller();
    s_real = 0x4000;
    run_pattern(kPatterns[1], 0xFFFF, 0xFFFF, &t);
    for (unsigned s = 0; s < 16; s++)
        CHECK(t.held[s] == 0x4000);
    pad_read();
    CHECK(PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical, 0) == 0);
    CHECK(g_C1Buffer[0x2] == (u_char)~0x40 && g_C1Buffer[0x3] == 0xFF);
    printf("FIELD STEP PAD PASS unset: pad untouched, real input ORed as before\n");
}

/* I9: suspended sub-schedule (field loop stopped inside MenuMain): after T
 * vblanks without a drain the pad follows XENO_FIELD_STEP_PAD_SUSPENDED by
 * u = vblanks - T, replays per suspension, and never applies before the
 * first drain (a load). */
static void suspension(unsigned vblanks, u16 *seen)
{
    unsigned v;
    for (v = 0; v < vblanks; v++) {
        vblank();
        seen[v] = g_C1ButtonState;
    }
}

static void mode_suspend(void)
{
    u16 seen[40];
    unsigned v, round;

    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0,2:0x10,4:0", 1);
    setenv("XENO_FIELD_STEP_PAD_SUSPENDED", "0:0,10:0x40,12:0", 1);
    setenv("XENO_FIELD_STEP_PAD_SUSPEND_AFTER", "8", 1);
    memcpy(g_ControllerButtonMappings, (u_char[8]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
    reset_controller();
    suspension(30, seen);                      /* before any drain: a load */
    for (v = 0; v < 30; v++)
        CHECK(seen[v] == 0);
    for (v = 0; v < 6; v++) {                  /* live field: drains every vblank */
        vblank();
        FieldPollControllers();
    }
    for (round = 0; round < 2; round++) {      /* two menu suspensions */
        suspension(30, seen);
        for (v = 0; v < 30; v++)
            CHECK(seen[v] == ((v >= 18 && v < 20) ? 0x40 : 0));
        FieldPollControllers();                /* the loop resumes */
    }
    printf("FIELD STEP PAD PASS suspend: sub-schedule after T, replayed per suspension, "
           "inert before the first drain\n");
}

/* I8: ambiguous or invalid configurations refuse. */
static int mode_refuse(const char *which)
{
    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0x2000", 1);
    if (strcmp(which, "field") == 0)
        setenv("XENO_FIELD_TEST_INPUT", "0:0", 1);
    else if (strcmp(which, "pad") == 0)
        setenv("XENO_PAD_TEST_INPUT", "0:0", 1);
    else if (strcmp(which, "schedule") == 0)
        setenv("XENO_FIELD_STEP_PAD_INPUT", "1:0x2000", 1);
    else if (strcmp(which, "map") == 0)
        setenv("XENO_FIELD_STEP_PAD_MAP", "0x1000", 1);
    else if (strcmp(which, "handover-mismatch") == 0) {
        setenv("XENO_PAD_TEST_INPUT", "0:0", 1);
        setenv("XENO_PAD_TEST_STOP_FIELD", "5", 1);
        setenv("XENO_FIELD_STEP_PAD_MAP", "4", 1);
    } else if (strcmp(which, "handover-nomap") == 0) {
        setenv("XENO_PAD_TEST_INPUT", "0:0", 1);
        setenv("XENO_PAD_TEST_STOP_FIELD", "4", 1);
    }
    pad_read();
    CHECK(PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical, 0) == -1);
    CHECK(g_C1Buffer[0x2] == 0xFF && g_C1Buffer[0x3] == 0xFF);
    printf("FIELD STEP PAD PASS refuse-%s\n", which);
    return 0;
}

/* The boot schedule may hand over when it retires at the probe's map:
 * accepted, and the probe stays inert until that map. */
static void mode_handover(void)
{
    setenv("XENO_FIELD_STEP_PAD_INPUT", "0:0x2000", 1);
    setenv("XENO_PAD_TEST_INPUT", "0:0", 1);
    setenv("XENO_PAD_TEST_STOP_FIELD", "4", 1);
    setenv("XENO_FIELD_STEP_PAD_MAP", "4", 1);
    pad_read();
    CHECK(PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical, 490) == 0);
    CHECK(g_C1Buffer[0x2] == 0xFF && g_C1Buffer[0x3] == 0xFF);
    pad_read();
    CHECK(PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical, 4) == 0);
    CHECK(g_C1Buffer[0x2] != 0xFF || g_C1Buffer[0x3] != 0xFF);
    printf("FIELD STEP PAD PASS handover: accepted only with STOP_FIELD == MAP, inert before\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "main-identity") && argc == 3) mode_main(0, (unsigned)atoi(argv[2]) % 3);
    else if (!strcmp(argv[1], "main-usa") && argc == 3) mode_main(1, (unsigned)atoi(argv[2]) % 3);
    else if (!strcmp(argv[1], "masks")) mode_masks();
    else if (!strcmp(argv[1], "contrast")) mode_post_mask_contrast();
    else if (!strcmp(argv[1], "overflow")) mode_overflow();
    else if (!strcmp(argv[1], "map")) mode_map();
    else if (!strcmp(argv[1], "unset")) mode_unset();
    else if (!strcmp(argv[1], "suspend")) mode_suspend();
    else if (!strcmp(argv[1], "handover")) mode_handover();
    else if (!strcmp(argv[1], "refuse") && argc == 3) return mode_refuse(argv[2]);
    else return 2;
    return 0;
}
