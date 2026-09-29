/* Focused production-linked oracle for retail world helper 0x8007528C. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_7528c.h"

#define TIMERS       0x8009C854u
#define TIMER_COUNT  0x8009BCC4u
#define TIMER_RANGE  0x8009BE40u
#define COUNTDOWN    0x8009D64Cu
#define EXPIRED      0x8009D80Cu

static const int *s_random_values;
static size_t s_random_count;
static size_t s_random_index;
static int s_random_log[32];
static int s_failures;

int rand(void)
{
    int value;
    if (s_random_index >= s_random_count) {
        fprintf(stderr, "ASSERTION rand/sequence exhausted at call %zu\n",
                s_random_index);
        exit(2);
    }
    value = s_random_values[s_random_index];
    if (s_random_index < sizeof(s_random_log) / sizeof(s_random_log[0]))
        s_random_log[s_random_index] = value;
    s_random_index++;
    return value;
}

static void store_s32(u32 address, s32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 load_s32(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void check_int(const char *name, long long got, long long expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=%lld expected=%lld\n",
                name, got, expected);
        s_failures++;
    }
}

static void reset_fixture(s32 countdown, s32 count, s32 range,
                          s32 expired, const int *random_values,
                          size_t random_count)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    store_s32(COUNTDOWN, countdown);
    store_s32(TIMER_COUNT, count);
    store_s32(TIMER_RANGE, range);
    store_s32(EXPIRED, expired);
    s_random_values = random_values;
    s_random_count = random_count;
    s_random_index = 0u;
    memset(s_random_log, 0, sizeof(s_random_log));
}

static void test_countdown_and_existing_timers(void)
{
    reset_fixture(3, 3, 8, 77, NULL, 0);
    store_u16(TIMERS + 0u, 5u);
    store_u16(TIMERS + 2u, 0u);
    store_u16(TIMERS + 4u, 2u);
    wm_8007528C();
    check_int("A countdown decremented", load_s32(COUNTDOWN), 2);
    check_int("A no rand calls", (long long)s_random_index, 0);
    check_int("H zero timer wraps and is not counted", load_u16(TIMERS + 2u),
              0xFFFF);
    check_int("I timer decrements but remains nonzero", load_u16(TIMERS), 4);
    check_int("I second nonzero timer", load_u16(TIMERS + 4u), 1);
    check_int("L expired reset before counting", load_s32(EXPIRED), 0);
}

static void test_zero_boundary_not_off_by_one(void)
{
    reset_fixture(2, 1, 9, 0, NULL, 0);
    store_u16(TIMERS, 3u);
    wm_8007528C();
    check_int("B zero test exact", load_s32(COUNTDOWN), 1);
    check_int("B zero test consumes no rand", (long long)s_random_index, 0);
    check_int("B existing timer decremented", load_u16(TIMERS), 2);
}

static void test_generated_minimum(void)
{
    static const int sequence[] = {0};
    reset_fixture(1, 1, 4, 23, sequence, 1);
    store_u16(TIMERS, 0x7777u);
    wm_8007528C();
    check_int("B countdown reload source", load_s32(COUNTDOWN), 4);
    check_int("C one generated timer final value", load_u16(TIMERS), 0);
    check_int("F minimum rand maps to one", load_s32(EXPIRED), 1);
    check_int("F minimum rand call count", (long long)s_random_index, 1);
    check_int("F minimum rand call order", s_random_log[0], 0);
}

static void test_generated_maximum(void)
{
    static const int sequence[] = {3};
    reset_fixture(1, 1, 4, 0, sequence, 1);
    wm_8007528C();
    check_int("G maximum rand maps to upper bound", load_u16(TIMERS), 3);
    check_int("G maximum rand call count", (long long)s_random_index, 1);
    check_int("G maximum rand call order", s_random_log[0], 3);
}

static void test_multiple_unique_with_retry(void)
{
    static const int sequence[] = {1, 1, 4, 0};
    reset_fixture(1, 3, 5, 99, sequence, 4);
    store_u16(TIMERS + 6u, 0xBEEFu);
    wm_8007528C();
    check_int("D generated timer zero", load_u16(TIMERS + 0u), 1);
    check_int("E uniqueness retry timer one", load_u16(TIMERS + 2u), 4);
    check_int("E uniqueness retry timer two", load_u16(TIMERS + 4u), 0);
    check_int("E uniqueness retry call count", (long long)s_random_index, 4);
    check_int("E uniqueness retry call zero", s_random_log[0], 1);
    check_int("E uniqueness retry duplicate", s_random_log[1], 1);
    check_int("E uniqueness retry replacement", s_random_log[2], 4);
    check_int("E uniqueness retry next entry", s_random_log[3], 0);
    check_int("D exact halfword stride canary", load_u16(TIMERS + 6u), 0xBEEF);
    check_int("D one generated timer expires", load_s32(EXPIRED), 1);
}

static void test_multiple_zero_crossings(void)
{
    reset_fixture(9, 4, 12, 1234, NULL, 0);
    store_u16(TIMERS + 0u, 1u);
    store_u16(TIMERS + 2u, 2u);
    store_u16(TIMERS + 4u, 1u);
    store_u16(TIMERS + 6u, 0u);
    wm_8007528C();
    check_int("J first timer reaches exactly zero", load_u16(TIMERS), 0);
    check_int("I middle timer remains nonzero", load_u16(TIMERS + 2u), 1);
    check_int("J second timer reaches exactly zero", load_u16(TIMERS + 4u), 0);
    check_int("H zero timer not a crossing", load_u16(TIMERS + 6u), 0xFFFF);
    check_int("K simultaneous zero crossings", load_s32(EXPIRED), 2);
}

int main(void)
{
    test_countdown_and_existing_timers();
    test_zero_boundary_not_off_by_one();
    test_generated_minimum();
    test_generated_maximum();
    test_multiple_unique_with_retry();
    test_multiple_zero_crossings();
    if (s_failures != 0) {
        fprintf(stderr, "W34B21-C6B FAIL assertions=%d\n", s_failures);
        return 1;
    }
    printf("W34B21-C6B 0x8007528C focused oracle PASS\n");
    return 0;
}
