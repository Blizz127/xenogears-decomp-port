/* W34N6 — retail world queue-drain barrier 0x80096694 certificate. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_96130.h"

#define HEAD_ADDR 0x8009BE44u
#define TAIL_ADDR 0x8009BCB8u
#define C624_BASE 0x8009C624u

static int s_failures;
static int s_vsync_calls;
static int s_dispatch_polls;
static int s_loader_calls;
static char s_events[64];
static size_t s_event_count;

#define ASSERT_MSG(condition, name, ...) do { \
    if (!(condition)) { \
        s_failures++; \
        fprintf(stderr, "ASSERTION %s FAILED: ", name); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } \
} while (0)

static void sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void event(char value)
{
    if (s_event_count < sizeof(s_events))
        s_events[s_event_count++] = value;
}

u32 func_8002C3D8(void)
{
    s_dispatch_polls++;
    return 1u; /* selects retail's C624 path */
}

int Vsync(int mode)
{
    ASSERT_MSG(mode == 0, "vsync_mode", "mode=%d", mode);
    s_vsync_calls++;
    event('V');
    return 0;
}

void wm_800966CC(u32 record)
{
    ASSERT_MSG(record != 0u, "nonzero_record", "record=0x%08x", record);
    s_loader_calls++;
    event('L');
}

void wm_8009699C(u32 record)
{
    (void)record;
    ASSERT_MSG(0, "wrong_loader", "CD loader reached on forced C624 path");
}

u32 wm_80096668_circular_distance(void)
{
    s32 distance = (s32)(lw(HEAD_ADDR) - lw(TAIL_ADDR));
    if (distance < 0)
        distance += 16;
    return (u32)distance;
}

static void seed(u32 head, u32 tail)
{
    u32 i;
    PsxMemory_Init();
    sw(HEAD_ADDR, head);
    sw(TAIL_ADDR, tail);
    for (i = 0u; i < 16u; i++)
        sw(C624_BASE + i * 4u, 0u);
    s_vsync_calls = 0;
    s_dispatch_polls = 0;
    s_loader_calls = 0;
    s_event_count = 0u;
    memset(s_events, 0, sizeof(s_events));
}

static void check_empty_do_while(void)
{
    seed(4u, 4u);
    wm_80096694();
    ASSERT_MSG(s_vsync_calls == 1, "empty_do_while_vsync",
               "vsync=%d", s_vsync_calls);
    ASSERT_MSG(s_dispatch_polls == 2, "empty_do_while_dispatch",
               "func_8002C3D8 calls=%d", s_dispatch_polls);
    ASSERT_MSG(s_loader_calls == 0, "empty_no_loader",
               "loaders=%d", s_loader_calls);
    ASSERT_MSG(s_event_count == 1u && s_events[0] == 'V',
               "empty_event_order", "events=%.*s", (int)s_event_count,
               s_events);
}

static void check_linear_drain(void)
{
    seed(3u, 0u);
    sw(C624_BASE + 0u * 4u, 0x800A1000u);
    sw(C624_BASE + 1u * 4u, 0x800A2000u);
    sw(C624_BASE + 2u * 4u, 0x800A3000u);
    wm_80096694();
    ASSERT_MSG(lw(TAIL_ADDR) == 3u, "linear_tail_drained",
               "tail=%u", lw(TAIL_ADDR));
    ASSERT_MSG(s_vsync_calls == 3 && s_loader_calls == 3,
               "linear_iteration_count", "vsync=%d loaders=%d",
               s_vsync_calls, s_loader_calls);
    ASSERT_MSG(s_event_count == 6u && memcmp(s_events, "VLVLVL", 6u) == 0,
               "linear_vsync_before_dispatch", "events=%.*s",
               (int)s_event_count, s_events);
}

static void check_wraparound_drain(void)
{
    seed(1u, 15u);
    sw(C624_BASE + 15u * 4u, 0x800AF000u);
    sw(C624_BASE + 0u * 4u, 0x800A0000u);
    wm_80096694();
    ASSERT_MSG(lw(TAIL_ADDR) == 1u, "wrap_tail_drained",
               "tail=%u", lw(TAIL_ADDR));
    ASSERT_MSG(s_vsync_calls == 2 && s_loader_calls == 2,
               "wrap_iteration_count", "vsync=%d loaders=%d",
               s_vsync_calls, s_loader_calls);
    ASSERT_MSG(s_event_count == 4u && memcmp(s_events, "VLVL", 4u) == 0,
               "wrap_event_order", "events=%.*s", (int)s_event_count,
               s_events);
}

int main(void)
{
    check_empty_do_while();
    check_linear_drain();
    check_wraparound_drain();
    if (s_failures != 0) {
        fprintf(stderr, "W34N6 FAIL failures=%d\n", s_failures);
        return 1;
    }
    printf("W34N6 QUEUE BARRIER CERTIFICATE PASS\n");
    return 0;
}
