#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_input.h"

#define TEST_INPUT_MAX_STEPS 128u

/* DIAGNOSTIC / TEST TOOLING: the field schedule drives long scripted scenes
 * (the Map 4 prologue needs hundreds of confirm presses), so it gets its own
 * larger capacity instead of the 128-step interactive limit. */
#define FIELD_TEST_INPUT_MAX_STEPS 4096u

typedef struct TestInputStep {
    uint32_t frame;
    uint16_t value;
} TestInputStep;

static TestInputStep s_steps[TEST_INPUT_MAX_STEPS];
static size_t s_step_count;
static size_t s_current_step;
static uint32_t s_frame;
static int s_enabled;
static int s_initialized;

static TestInputStep s_world_steps[TEST_INPUT_MAX_STEPS];
static size_t s_world_step_count;
static size_t s_world_current_step;
static uint32_t s_world_frame;
static uint16_t s_world_previous;
static int s_world_enabled;
static int s_world_initialized;

static int test_input_error(const char *reason)
{
    fprintf(stderr, "[test-input] invalid XENO_TEST_INPUT: %s\n", reason);
    return -1;
}

int PcPort_TestInputInit(void)
{
    const char *schedule;
    const char *cursor;
    uint32_t previous_frame = 0u;

    if (s_initialized != 0)
        return 0;
    s_initialized = 1;
    s_frame = UINT32_MAX;
    schedule = getenv("XENO_TEST_INPUT");
    if (schedule == NULL) {
        s_enabled = 0;
        return 0;
    }
    if (*schedule == '\0')
        return test_input_error("empty schedule");

    cursor = schedule;
    while (*cursor != '\0') {
        char *end;
        unsigned long frame;
        unsigned long value;

        if (s_step_count == TEST_INPUT_MAX_STEPS)
            return test_input_error("too many frame/value pairs");
        errno = 0;
        frame = strtoul(cursor, &end, 10);
        if (errno != 0 || end == cursor || *end != ':' || frame > UINT32_MAX)
            return test_input_error("invalid frame boundary");
        cursor = end + 1;
        errno = 0;
        value = strtoul(cursor, &end, 0);
        if (errno != 0 || end == cursor || value > UINT16_MAX)
            return test_input_error("invalid input value");
        if (*end != '\0' && *end != ',')
            return test_input_error("expected comma between pairs");
        if (s_step_count == 0u && frame != 0u)
            return test_input_error("first frame must be zero");
        if (s_step_count != 0u && frame <= previous_frame)
            return test_input_error("frame boundaries must increase");

        s_steps[s_step_count].frame = (uint32_t)frame;
        s_steps[s_step_count].value = (uint16_t)value;
        s_step_count++;
        previous_frame = (uint32_t)frame;
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return test_input_error("trailing comma");
    }

    s_enabled = 1;
    fprintf(stderr, "[test-input] enabled steps=%zu\n", s_step_count);
    return 0;
}

static TestInputStep s_field_steps[FIELD_TEST_INPUT_MAX_STEPS];
static size_t s_field_step_count;
static size_t s_field_current_step;
static uint32_t s_field_frame;
static uint16_t s_field_previous;
static int s_field_enabled;
static int s_field_initialized;

static int field_test_input_error(const char *reason)
{
    fprintf(stderr, "[field-test-input] invalid XENO_FIELD_TEST_INPUT: %s\n",
            reason);
    return -1;
}

/* DIAGNOSTIC / TEST TOOLING -- see include/test_input.h.  Same schedule syntax
 * as XENO_WORLD_TEST_INPUT ("frame:value,frame:value,...", first frame 0). */
int PcPort_FieldTestInputInit(void)
{
    const char *schedule;
    const char *cursor;
    uint32_t previous_frame = 0u;

    if (s_field_initialized != 0)
        return 0;
    s_field_initialized = 1;
    schedule = getenv("XENO_FIELD_TEST_INPUT");
    if (schedule == NULL) {
        s_field_enabled = 0;
        return 0;
    }
    if (*schedule == '\0')
        return field_test_input_error("empty schedule");

    cursor = schedule;
    while (*cursor != '\0') {
        char *end;
        unsigned long frame;
        unsigned long value;

        if (s_field_step_count == FIELD_TEST_INPUT_MAX_STEPS)
            return field_test_input_error("too many frame/value pairs");
        errno = 0;
        frame = strtoul(cursor, &end, 10);
        if (errno != 0 || end == cursor || *end != ':' || frame > UINT32_MAX)
            return field_test_input_error("invalid frame boundary");
        cursor = end + 1;
        errno = 0;
        value = strtoul(cursor, &end, 0);
        if (errno != 0 || end == cursor || value > UINT16_MAX)
            return field_test_input_error("invalid input value");
        if (*end != '\0' && *end != ',')
            return field_test_input_error("expected comma between pairs");
        if (s_field_step_count == 0u && frame != 0u)
            return field_test_input_error("first frame must be zero");
        if (s_field_step_count != 0u && frame <= previous_frame)
            return field_test_input_error("frame boundaries must increase");

        s_field_steps[s_field_step_count].frame = (uint32_t)frame;
        s_field_steps[s_field_step_count].value = (uint16_t)value;
        s_field_step_count++;
        previous_frame = (uint32_t)frame;
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return field_test_input_error("trailing comma");
    }

    s_field_enabled = 1;
    fprintf(stderr, "[field-test-input] enabled steps=%zu\n",
            s_field_step_count);
    return 0;
}

/* DIAGNOSTIC / TEST TOOLING.  Held bits come straight from the schedule;
 * pressed/released edges are derived from schedule transitions so scripts that
 * gate on OP31 (held) and on the released accumulator both see the input. */
/* HARNESS: console `hold <mask> <frames>` (cheat_console.c) holds a button
 * mask for N field frames on top of any schedule.  Inert until called. */
static uint16_t s_hold_value, s_hold_previous;
static uint32_t s_hold_frames;

void PcPort_FieldTestInputHold(unsigned value, unsigned frames)
{
    s_hold_value = (uint16_t)value;
    s_hold_frames = frames;
}

void PcPort_FieldTestInputMerge(u16 *held_buttons, u16 *pressed_edges,
                                u16 *released_edges)
{
    uint16_t value;
    uint16_t rising;
    uint16_t falling;
    size_t previous_step;

    /* The step-pad clock advances here because this is the one call made
     * after every FieldPollControllers drain (see PcPort_FieldStepPadApply). */
    {
        extern void PcPort_FieldStepPadNoteDrain(void);
        PcPort_FieldStepPadNoteDrain();
    }
    if (s_hold_frames != 0u || s_hold_previous != 0u) {
        uint16_t v = s_hold_frames != 0u ? s_hold_value : (uint16_t)0;
        *held_buttons = (u16)(*held_buttons | v);
        *pressed_edges = (u16)(*pressed_edges | (uint16_t)(v & (uint16_t)~s_hold_previous));
        *released_edges = (u16)(*released_edges | (uint16_t)(s_hold_previous & (uint16_t)~v));
        s_hold_previous = v;
        if (s_hold_frames != 0u)
            s_hold_frames--;
    }
    if (s_field_enabled == 0)
        return;

    previous_step = s_field_current_step;
    while (s_field_current_step + 1u < s_field_step_count &&
           s_field_frame >= s_field_steps[s_field_current_step + 1u].frame)
        s_field_current_step++;
    value = s_field_steps[s_field_current_step].value;
    rising = (uint16_t)(value & (uint16_t)~s_field_previous);
    falling = (uint16_t)(s_field_previous & (uint16_t)~value);

    *held_buttons = (u16)(*held_buttons | value);
    *pressed_edges = (u16)(*pressed_edges | rising);
    *released_edges = (u16)(*released_edges | falling);

    if (s_field_current_step != previous_step) {
        fprintf(stderr,
                "[field-test-input] frame=%u held=0x%04x rising=0x%04x "
                "falling=0x%04x\n",
                s_field_frame, (unsigned int)value, (unsigned int)rising,
                (unsigned int)falling);
    }
    s_field_previous = value;
    s_field_frame++;
}

static int world_test_input_error(const char *reason)
{
    fprintf(stderr, "[world-test-input] invalid XENO_WORLD_TEST_INPUT: %s\n",
            reason);
    return -1;
}

int PcPort_WorldTestInputInit(void)
{
    const char *schedule;
    const char *cursor;
    uint32_t previous_frame = 0u;

    if (s_world_initialized != 0)
        return 0;
    s_world_initialized = 1;
    schedule = getenv("XENO_WORLD_TEST_INPUT");
    if (schedule == NULL) {
        s_world_enabled = 0;
        return 0;
    }
    if (*schedule == '\0')
        return world_test_input_error("empty schedule");

    cursor = schedule;
    while (*cursor != '\0') {
        char *end;
        unsigned long frame;
        unsigned long value;

        if (s_world_step_count == TEST_INPUT_MAX_STEPS)
            return world_test_input_error("too many frame/value pairs");
        errno = 0;
        frame = strtoul(cursor, &end, 10);
        if (errno != 0 || end == cursor || *end != ':' || frame > UINT32_MAX)
            return world_test_input_error("invalid frame boundary");
        cursor = end + 1;
        errno = 0;
        value = strtoul(cursor, &end, 0);
        if (errno != 0 || end == cursor || value > UINT16_MAX)
            return world_test_input_error("invalid input value");
        if (*end != '\0' && *end != ',')
            return world_test_input_error("expected comma between pairs");
        if (s_world_step_count == 0u && frame != 0u)
            return world_test_input_error("first frame must be zero");
        if (s_world_step_count != 0u && frame <= previous_frame)
            return world_test_input_error("frame boundaries must increase");

        s_world_steps[s_world_step_count].frame = (uint32_t)frame;
        s_world_steps[s_world_step_count].value = (uint16_t)value;
        s_world_step_count++;
        previous_frame = (uint32_t)frame;
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return world_test_input_error("trailing comma");
    }

    s_world_enabled = 1;
    fprintf(stderr, "[world-test-input] enabled steps=%zu\n",
            s_world_step_count);
    return 0;
}

void PcPort_TestInputAdvanceFrame(void)
{
    if (s_enabled != 0) {
        size_t previous_step = s_current_step;

        s_frame++;
        while (s_current_step + 1u < s_step_count &&
               s_frame >= s_steps[s_current_step + 1u].frame)
            s_current_step++;
        if (s_frame == 0u || s_current_step != previous_step)
            fprintf(stderr, "[test-input] frame=%u value=0x%04x\n",
                    s_frame, (unsigned int)s_steps[s_current_step].value);
    }
}

void PcPort_TestInputInject(u16 *held_buttons)
{
    if (s_enabled == 0)
        return;

#if defined(XENO_TEST_INPUT_MUTANT_DROP_HOLD)
    if (s_frame == s_steps[s_current_step].frame)
        *held_buttons = s_steps[s_current_step].value;
    else
        *held_buttons = 0u;
#else
    *held_buttons = s_steps[s_current_step].value;
#endif
}

void PcPort_WorldTestInputMerge(u16 *held_buttons, u16 *pressed_edges,
                                u16 *repeat_edges)
{
    uint16_t value;
    uint16_t rising;
    size_t previous_step;

    if (s_world_enabled == 0)
        return;

    previous_step = s_world_current_step;
    while (s_world_current_step + 1u < s_world_step_count &&
           s_world_frame >= s_world_steps[s_world_current_step + 1u].frame)
        s_world_current_step++;
    value = s_world_steps[s_world_current_step].value;
    rising = (uint16_t)(value & (uint16_t)~s_world_previous);
#if defined(XENO_WORLD_TEST_INPUT_MUTANT_NO_RISING)
    rising = 0u;
#endif
    *held_buttons = (u16)(*held_buttons | value);
    *pressed_edges = (u16)(*pressed_edges | rising);
    *repeat_edges = (u16)(*repeat_edges | rising);
    if (s_world_frame == 0u || s_world_current_step != previous_step ||
            rising != 0u) {
        fprintf(stderr,
                "[world-test-input] frame=%u schedule_frame=%u "
                "held=0x%04x rising=0x%04x\n",
                s_world_frame + 1u, s_world_frame, (unsigned int)value,
                (unsigned int)rising);
    }
    s_world_previous = value;
    s_world_frame++;
}

#if defined(XENO_TEST_INPUT_CERTIFICATE)
void PcPort_TestInputResetForCertificate(void)
{
    size_t i;

    for (i = 0u; i < TEST_INPUT_MAX_STEPS; i++) {
        s_steps[i].frame = 0u;
        s_steps[i].value = 0u;
    }
    s_step_count = 0u;
    s_current_step = 0u;
    s_frame = UINT32_MAX;
    s_enabled = 0;
    s_initialized = 0;
}

void PcPort_WorldTestInputResetForCertificate(void)
{
    size_t i;

    for (i = 0u; i < TEST_INPUT_MAX_STEPS; i++) {
        s_world_steps[i].frame = 0u;
        s_world_steps[i].value = 0u;
    }
    s_world_step_count = 0u;
    s_world_current_step = 0u;
    s_world_frame = 0u;
    s_world_previous = 0u;
    s_world_enabled = 0;
    s_world_initialized = 0;
}
#endif

/* DIAGNOSTIC / TEST TOOLING: XENO_FIELD_STEP_PAD_INPUT ("step:value,...",
 * first step 0, game CTRL_BTN_* bits) is a field-step-clocked schedule that
 * presses buttons on the PHYSICAL pad, before ControllerPoll, unlike
 * XENO_FIELD_TEST_INPUT, which ORs into the accumulators after both control
 * masks.  Retail remap, edge/repeat derivation, the 16-state FIFO, the
 * per-state g_FieldControl.controllerBtnMask and the D_800ADB00 mask all
 * still apply.
 *
 * Step k is "after k completed FieldPollControllers drains since arming".
 * Every vblank state queued between drain k-1 and drain k carries the step-k
 * value, so drain k sees held == value(k) (ORed with any real pad, then
 * masked) no matter how many vblanks the field loop spans. A drain that saw
 * no vblank at all is counted and reported, never filled in.
 *
 * XENO_FIELD_STEP_PAD_MAP=<map> arms on the first vblank in that field
 * (low 12 bits of g_GameSceneMapNum); unset arms on the first vblank.
 * It refuses to combine with XENO_FIELD_TEST_INPUT, and with
 * XENO_PAD_TEST_INPUT unless that boot schedule hands over: it retires at
 * XENO_PAD_TEST_STOP_FIELD, which must equal XENO_FIELD_STEP_PAD_MAP.
 * Otherwise both would inject on the same paths and make the receipt
 * ambiguous. */
static TestInputStep s_step_pad_steps[FIELD_TEST_INPUT_MAX_STEPS];
static size_t s_step_pad_count;
static size_t s_step_pad_current;
static uint32_t s_step_pad_step;          /* completed drains since arming */
static uint32_t s_step_pad_vblanks;       /* injections since the last drain */
static uint32_t s_step_pad_empty;         /* drains that saw no injection */
static int s_step_pad_map = -1;
static int s_step_pad_state = -1;         /* -1 unparsed, 0 off, 1 waiting, 2 armed */
/* XENO_FIELD_STEP_PAD_SUSPENDED="u:value,..." (first u 0, game bits): while
 * the field loop is suspended -- MenuMain runs synchronously inside the
 * field menu opener, so no FieldPollControllers drain happens and the step
 * clock stops -- the pad follows this schedule instead, clocked by u = vblanks
 * since the last drain minus XENO_FIELD_STEP_PAD_SUSPEND_AFTER (default 8;
 * the live field drains every 2 under the test clock).  It replays from u=0
 * on every suspension, and only applies while the field latch is set and a
 * drain has happened since arming, so loads never get scripted presses. */
static TestInputStep s_susp_steps[TEST_INPUT_MAX_STEPS];
static size_t s_susp_count;
static uint32_t s_susp_after = 8u;
static uint16_t s_susp_logged = 0xFFFFu;

static int step_pad_error(const char *reason)
{
    fprintf(stderr, "[field-step-pad] invalid XENO_FIELD_STEP_PAD_INPUT: %s\n",
            reason);
    s_step_pad_state = 0;
    return -1;
}

int PcPort_FieldStepPadInit(void)
{
    const char *schedule;
    const char *cursor;
    const char *map;
    uint32_t previous = 0u;
    int handover;

    if (s_step_pad_state >= 0)
        return 0;
    s_step_pad_state = 0;
    schedule = getenv("XENO_FIELD_STEP_PAD_INPUT");
    if (schedule == NULL)
        return 0;
    if (getenv("XENO_FIELD_TEST_INPUT") != NULL)
        return step_pad_error("XENO_FIELD_TEST_INPUT is also set (post-mask path)");
    /* XENO_PAD_TEST_INPUT may only hand over: the boot schedule retires when
     * XENO_PAD_TEST_STOP_FIELD loads, and this probe must arm on that same
     * map (checked below), so the two never inject together. */
    handover = getenv("XENO_PAD_TEST_INPUT") != NULL;
    if (*schedule == '\0')
        return step_pad_error("empty schedule");

    cursor = schedule;
    while (*cursor != '\0') {
        char *end;
        unsigned long step;
        unsigned long value;

        if (s_step_pad_count == FIELD_TEST_INPUT_MAX_STEPS)
            return step_pad_error("too many step/value pairs");
        errno = 0;
        step = strtoul(cursor, &end, 10);
        if (errno != 0 || end == cursor || *end != ':' || step > UINT32_MAX)
            return step_pad_error("invalid step boundary");
        cursor = end + 1;
        errno = 0;
        value = strtoul(cursor, &end, 0);
        if (errno != 0 || end == cursor || value > UINT16_MAX)
            return step_pad_error("invalid input value");
        if (*end != '\0' && *end != ',')
            return step_pad_error("expected comma between pairs");
        if (s_step_pad_count == 0u && step != 0u)
            return step_pad_error("first step must be zero");
        if (s_step_pad_count != 0u && step <= previous)
            return step_pad_error("step boundaries must increase");
        s_step_pad_steps[s_step_pad_count].frame = (uint32_t)step;
        s_step_pad_steps[s_step_pad_count].value = (uint16_t)value;
        s_step_pad_count++;
        previous = (uint32_t)step;
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return step_pad_error("trailing comma");
    }

    {
        const char *susp = getenv("XENO_FIELD_STEP_PAD_SUSPENDED");
        const char *after = getenv("XENO_FIELD_STEP_PAD_SUSPEND_AFTER");
        uint32_t prev = 0u;

        if (after != NULL && after[0] != '\0')
            s_susp_after = (uint32_t)strtoul(after, NULL, 10);
        cursor = susp;
        while (cursor != NULL && *cursor != '\0') {
            char *end;
            unsigned long u, value;

            if (s_susp_count == TEST_INPUT_MAX_STEPS)
                return step_pad_error("too many suspended pairs");
            errno = 0;
            u = strtoul(cursor, &end, 10);
            if (errno != 0 || end == cursor || *end != ':' || u > UINT32_MAX)
                return step_pad_error("invalid suspended boundary");
            cursor = end + 1;
            value = strtoul(cursor, &end, 0);
            if (end == cursor || value > UINT16_MAX || (*end != '\0' && *end != ','))
                return step_pad_error("invalid suspended value");
            if ((s_susp_count == 0u && u != 0u) || (s_susp_count != 0u && u <= prev))
                return step_pad_error("suspended boundaries must start at 0 and increase");
            s_susp_steps[s_susp_count].frame = (uint32_t)u;
            s_susp_steps[s_susp_count].value = (uint16_t)value;
            s_susp_count++;
            prev = (uint32_t)u;
            cursor = (*end == ',') ? end + 1 : end;
        }
    }

    map = getenv("XENO_FIELD_STEP_PAD_MAP");
    if (map != NULL) {
        char *end;
        long parsed;

        errno = 0;
        parsed = strtol(map, &end, 0);
        if (errno != 0 || end == map || *end != '\0' || parsed < 0 || parsed > 0xFFF)
            return step_pad_error("XENO_FIELD_STEP_PAD_MAP must be 0..0xFFF");
        s_step_pad_map = (int)parsed;
    }
    if (handover) {
        const char *stop = getenv("XENO_PAD_TEST_STOP_FIELD");
        if (stop == NULL || stop[0] == '\0' || s_step_pad_map < 0 ||
            (atoi(stop) & 0xFFF) != s_step_pad_map)
            return step_pad_error("XENO_PAD_TEST_INPUT is also set (vblank-clocked path) "
                                  "without XENO_PAD_TEST_STOP_FIELD == XENO_FIELD_STEP_PAD_MAP");
        fprintf(stderr, "[field-step-pad] hand-over: boot pad schedule retires at map %d\n",
                s_step_pad_map);
    }
    s_step_pad_state = 1;
    fprintf(stderr, "[field-step-pad] enabled steps=%zu map=%d\n",
            s_step_pad_count, s_step_pad_map);
    return 0;
}

/* Once per dispatched vblank, after the real pad has been read into the
 * buffer and before ControllerPoll derives state from it.  `pad` is the
 * controller-1 buffer (active-low button bytes at +2/+3), `to_physical`
 * undoes the retail remap the way PcPort_PadTestGameToPhysical does. */
int PcPort_FieldStepPadApply(unsigned char *pad,
                             unsigned short (*to_physical)(unsigned short),
                             int map)
{
    unsigned short value;
    size_t previous;

    if (s_step_pad_state < 0 && PcPort_FieldStepPadInit() != 0)
        return -1;
    if (s_step_pad_state == 1 && (s_step_pad_map < 0 || (map & 0xFFF) == s_step_pad_map)) {
        s_step_pad_state = 2;
        fprintf(stderr, "[field-step-pad] armed map=%d step=0 held=0x%04x\n",
                map & 0xFFF, (unsigned int)s_step_pad_steps[0].value);
    }
    if (s_step_pad_state != 2)
        return 0;

    previous = s_step_pad_current;
    while (s_step_pad_current + 1u < s_step_pad_count &&
           s_step_pad_step >= s_step_pad_steps[s_step_pad_current + 1u].frame)
        s_step_pad_current++;
    value = s_step_pad_steps[s_step_pad_current].value;
    if (s_susp_count != 0u && s_step_pad_step > 0u && s_step_pad_vblanks >= s_susp_after) {
        extern int PcPort_QuickCheckpointFieldIsActive(void) __attribute__((weak));
        if (!PcPort_QuickCheckpointFieldIsActive || PcPort_QuickCheckpointFieldIsActive()) {
            uint32_t u = s_step_pad_vblanks - s_susp_after;
            size_t k = 0u;
            while (k + 1u < s_susp_count && u >= s_susp_steps[k + 1u].frame)
                k++;
            value = s_susp_steps[k].value;
            if (value != s_susp_logged) {
                fprintf(stderr, "[field-step-pad] suspended u=%u held=0x%04x step=%u\n",
                        u, (unsigned int)value, s_step_pad_step);
                s_susp_logged = value;
            }
        }
    } else {
        s_susp_logged = 0xFFFFu;
    }
    if (s_step_pad_current != previous)
        fprintf(stderr, "[field-step-pad] step=%u held=0x%04x\n",
                s_step_pad_step, (unsigned int)value);
    value = to_physical(value);
    pad[0x2] &= (unsigned char)~(unsigned char)(value >> 8);
    pad[0x3] &= (unsigned char)~(unsigned char)(value & 0xFF);
    s_step_pad_vblanks++;
    return 0;
}

/* Once per FieldPollControllers, after its drain. */
void PcPort_FieldStepPadNoteDrain(void)
{
    extern void PcPort_DetVblankNoteDrain(void) __attribute__((weak));
    if (PcPort_DetVblankNoteDrain)
        PcPort_DetVblankNoteDrain();
    if (s_step_pad_state != 2)
        return;
    if (s_step_pad_vblanks == 0u) {
        s_step_pad_empty++;
        fprintf(stderr, "[field-step-pad] step=%u drained no vblank (empty=%u)\n",
                s_step_pad_step, s_step_pad_empty);
    }
    s_step_pad_vblanks = 0u;
    s_step_pad_step++;
}

/* For the port's step-keyed evidence hooks (psyq_compat.c,
 * game_overrides.c): 1 once armed, with the completed-drain step count. */
int PcPort_FieldStepPadArmedStep(unsigned *step)
{
    *step = s_step_pad_step;
    return s_step_pad_state == 2;
}

/* Receipt for drivers: completed steps and empty drains since arming. */
void PcPort_FieldStepPadCounts(unsigned *steps, unsigned *empty)
{
    *steps = s_step_pad_step;
    *empty = s_step_pad_empty;
}

#if defined(XENO_TEST_INPUT_CERTIFICATE)
void PcPort_FieldStepPadResetForCertificate(void)
{
    s_step_pad_count = s_step_pad_current = 0u;
    s_step_pad_step = s_step_pad_vblanks = s_step_pad_empty = 0u;
    s_step_pad_map = -1;
    s_step_pad_state = -1;
    s_susp_count = 0u;
    s_susp_after = 8u;
}
#endif
