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
void PcPort_FieldTestInputMerge(u16 *held_buttons, u16 *pressed_edges,
                                u16 *released_edges)
{
    uint16_t value;
    uint16_t rising;
    uint16_t falling;
    size_t previous_step;

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
