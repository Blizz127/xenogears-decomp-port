/* Focused production certificate for retail 0x800834D0..0x8008355C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_834d0.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800B0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 8
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define POSITION   UINT32_C(0x8009C5AC)

enum EventKind {
    EV_DRAW_SYNC = 1,
    EV_VSYNC,
    EV_RESET,
    EV_POSITION,
    EV_CD_WORK,
    EV_DISTANCE
};

typedef struct Event {
    int kind;
    u32 value;
} Event;

static Event events[32];
static u32 event_count;
static u32 distance_count;
static int failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void trace(int kind, u32 value)
{
    check(event_count < 32u, "trace.capacity");
    if (event_count < 32u) {
        events[event_count].kind = kind;
        events[event_count].value = value;
    }
    event_count++;
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_trace(void)
{
    memset(events, 0, sizeof(events));
    event_count = 0u;
    distance_count = 0u;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    reset_trace();
}

int DrawSync(int mode)
{
    trace(EV_DRAW_SYNC, (u32)mode);
    return 0;
}

int Vsync(int mode)
{
    trace(EV_VSYNC, (u32)mode);
    return 0;
}

void wm_80097D64(void)
{
    trace(EV_RESET, 0u);
}

void wm_80097BC0(u32 position)
{
    trace(EV_POSITION, position);
}

u32 wm_800967E4(void)
{
    trace(EV_CD_WORK, 0u);
    return 0u;
}

u32 wm_80096668_circular_distance(void)
{
    static const u32 values[] = {2u, 1u, 0u};
    u32 index = distance_count;
    u32 value = index < 3u ? values[index] : 0u;

    distance_count++;
    trace(EV_DISTANCE, value);
    return value;
}

static void test_initializer(void)
{
    seed();
    check(func_800834D0() == 1, "init.return");
    check(event_count == 0u, "init.no_side_effects");
}

static void test_latched_update(void)
{
    static const int expected[] = {
        EV_DRAW_SYNC, EV_VSYNC, EV_RESET, EV_POSITION,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE
    };
    u32 index;
    int order_ok = 1;

    seed();
    write16(SLOT + 0x04u, 1u);
    check(wm_800834D8(SLOT_INDEX) == 1, "update.return");
    check(event_count != 0u, "update.latch_gate");
    check(read16(SLOT + 0x04u) == 0u, "update.latch_clear");
    if (event_count != (u32)(sizeof(expected) / sizeof(expected[0])))
        order_ok = 0;
    else {
        for (index = 0u; index < event_count; index++)
            if (events[index].kind != expected[index])
                order_ok = 0;
    }
    check(order_ok != 0, "update.order");
    check(event_count > 3u && events[3].kind == EV_POSITION &&
          events[3].value == POSITION,
          "update.position");
    check(distance_count == 3u &&
          events[6].value == 2u && events[9].value == 1u &&
          events[12].value == 0u,
          "update.drain_loop");
}

static void test_idle_update(void)
{
    seed();
    write16(SLOT + 0x04u, 0u);
    check(wm_800834D8(SLOT_INDEX) == 1 && event_count == 0u,
          "update.idle_noop");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x800834D0));
    write32(SLOT + 0x1Cu, UINT32_C(0x800834D8));
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0x00u) == 1u,
          "scheduler.init_resolution");

    reset_trace();
    write16(SLOT + 0x04u, 1u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && distance_count == 3u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_latched_update();
    test_idle_update();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N110 MODE17 TRANSITION CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N110 MODE17 TRANSITION CERTIFICATE PASS");
    return 0;
}
