/* Focused production certificate for retail callbacks 0x80078948/0x80078950. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_78948.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_80078948(void);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL             0x800A0000u
#define POOL_PTR         0x8009BE24u
#define SLOT_INDEX       2
#define SLOT             (POOL + (u32)SLOT_INDEX * 0x80u)
#define RENDER_POSITION  0x8009BBB4u
#define CAMERA_INPUT     0x8009BD40u
#define TERRAIN_POSITION 0x8009BE28u
#define CONTEXT_PTR      0x8009BE3Cu
#define PALETTE_INDEX    0x8009C5BCu
#define CAMERA_KIND      0x8009D144u
#define PAGING_FLAGS     0x8009D558u
#define CONTEXT          0x800A1000u

enum Event {
    EV_CAMERA_EULER,
    EV_CAMERA_LOOK,
    EV_PARTICLES,
    EV_SCALED_OBJECTS,
    EV_REGION_MODELS,
    EV_POSITION_WRAP,
    EV_TILE_WINDOW,
    EV_QUEUE_DRAIN,
    EV_TILE_REFILL,
    EV_VISIBILITY,
    EV_TERRAIN_CONTEXT,
    EV_TERRAIN_PACKETS,
    EV_SKY,
    EV_TILED_OBJECTS
};

typedef struct EventRecord {
    enum Event event;
    u32 a0;
    u32 a1;
    u32 a2;
} EventRecord;

static EventRecord events[32];
static int event_count;
static int failures;
static uint8_t ram_before[PSX_RAM_SIZE];

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void record_event(enum Event event, u32 a0, u32 a1, u32 a2)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0]))) {
        events[event_count].event = event;
        events[event_count].a0 = a0;
        events[event_count].a1 = a1;
        events[event_count].a2 = a2;
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

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static int has_event(enum Event event)
{
    int i;
    for (i = 0; i < event_count; i++)
        if (events[i].event == event)
            return 1;
    return 0;
}

static int event_sequence_is(const enum Event* expected, int count)
{
    int i;
    if (event_count != count)
        return 0;
    for (i = 0; i < count; i++)
        if (events[i].event != expected[i])
            return 0;
    return 1;
}

static const EventRecord* find_event(enum Event event)
{
    int i;
    for (i = 0; i < event_count; i++)
        if (events[i].event == event)
            return &events[i];
    return NULL;
}

void wm_80097440(u32 input_data)
{
    record_event(EV_CAMERA_EULER, input_data, 0u, 0u);
}

void wm_80097244(u32 input_data)
{
    record_event(EV_CAMERA_LOOK, input_data, 0u, 0u);
}

void wm_80089748(void)
{
    record_event(EV_PARTICLES, 0u, 0u, 0u);
}

void wm_80089C78(u32 input_addr)
{
    record_event(EV_SCALED_OBJECTS, input_addr, 0u, 0u);
}

void wm_800848F4(void)
{
    record_event(EV_REGION_MODELS, 0u, 0u, 0u);
}

void wm_800980D4(u32 pos_vec)
{
    record_event(EV_POSITION_WRAP, pos_vec, 0u, 0u);
}

void wm_800981C8(u32 pos_vec)
{
    record_event(EV_TILE_WINDOW, pos_vec, 0u, 0u);
}

void wm_80096130(void)
{
    record_event(EV_QUEUE_DRAIN, 0u, 0u, 0u);
}

void wm_80098CC0(void)
{
    record_event(EV_TILE_REFILL, 0u, 0u, 0u);
}

void wm_800983A0(u32 input_addr)
{
    record_event(EV_VISIBILITY, input_addr, 0u, 0u);
}

void wm_8009932C(u32 ot_base, u32 packet_base, u32 position)
{
    record_event(EV_TERRAIN_CONTEXT, ot_base, packet_base, position);
}

void wm_80073B04(void)
{
    record_event(EV_TERRAIN_PACKETS, 0u, 0u, 0u);
}

void wm_800737EC(void)
{
    record_event(EV_SKY, 0u, 0u, 0u);
}

void wm_80086798(void)
{
    record_event(EV_TILED_OBJECTS, 0u, 0u, 0u);
}

static void seed(void)
{
    memset(g_PsxRam, 0x5Au, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xC3, sizeof(g_PsxScratchpad));
    event_count = 0;
    write32(CONTEXT_PTR, CONTEXT);
    write32(CONTEXT + 0x70u, 0x800A2228u);
    write32(CONTEXT + 0x74u, 0x800B1000u);
    write32(PALETTE_INDEX, 0x120u);
    write32(CAMERA_KIND, 0u);
    write16(PAGING_FLAGS, 0u);
}

static void snapshot(void)
{
    memcpy(ram_before, g_PsxRam, sizeof(ram_before));
}

static void check_write_set(const char* name)
{
    size_t i;
    for (i = 0u; i < sizeof(g_PsxRam); i++) {
        u32 address = 0x80000000u + (u32)i;
        if (ram_before[i] != g_PsxRam[i] &&
                !(address >= PALETTE_INDEX &&
                  address < PALETTE_INDEX + 4u)) {
            fprintf(stderr, "ASSERTION %s FAILED ram+0x%zx\n", name, i);
            failures++;
            break;
        }
    }
}

static void test_no_paging(void)
{
    static const enum Event expected[] = {
        EV_CAMERA_EULER, EV_PARTICLES, EV_SCALED_OBJECTS,
        EV_REGION_MODELS, EV_POSITION_WRAP, EV_VISIBILITY,
        EV_TERRAIN_CONTEXT, EV_TERRAIN_PACKETS, EV_SKY,
        EV_TILED_OBJECTS
    };
    const EventRecord* record;
    s32 result;

    seed();
    snapshot();
    result = wm_80078950(47);
    check(result == 1, "no_paging.return_one");
    check(event_sequence_is(expected,
          (int)(sizeof(expected) / sizeof(expected[0]))),
          "no_paging.call_order");
    check(events[0].event == EV_CAMERA_EULER &&
          events[0].a0 == CAMERA_INPUT, "no_paging.camera_branch");
    check(has_event(EV_POSITION_WRAP), "no_paging.position_wrap");
    check(!has_event(EV_TILE_WINDOW) && !has_event(EV_QUEUE_DRAIN) &&
          !has_event(EV_TILE_REFILL), "no_paging.chain_skipped");
    record = find_event(EV_SCALED_OBJECTS);
    check(record != NULL && record->a0 == RENDER_POSITION,
          "no_paging.render_position");
    record = find_event(EV_VISIBILITY);
    check(record != NULL && record->a0 == TERRAIN_POSITION,
          "no_paging.visibility_position");
    record = find_event(EV_TERRAIN_CONTEXT);
    check(record != NULL && record->a0 == 0x800A2228u &&
          record->a1 == 0x800B1000u &&
          record->a2 == TERRAIN_POSITION,
          "no_paging.context_arguments");
    check(read32(PALETTE_INDEX) == 0x160u,
          "no_paging.palette_step");
    check_write_set("no_paging.write_set");
}

static void test_paging(void)
{
    static const enum Event expected[] = {
        EV_CAMERA_LOOK, EV_PARTICLES, EV_SCALED_OBJECTS,
        EV_REGION_MODELS, EV_POSITION_WRAP, EV_TILE_WINDOW,
        EV_QUEUE_DRAIN, EV_TILE_REFILL, EV_VISIBILITY,
        EV_TERRAIN_CONTEXT, EV_TERRAIN_PACKETS, EV_SKY,
        EV_TILED_OBJECTS
    };
    const EventRecord* record;

    seed();
    write32(CAMERA_KIND, 1u);
    write16(PAGING_FLAGS, 0x0004u);
    (void)wm_80078950(-1);
    check(event_sequence_is(expected,
          (int)(sizeof(expected) / sizeof(expected[0]))),
          "paging.call_order");
    check(events[0].event == EV_CAMERA_LOOK &&
          events[0].a0 == CAMERA_INPUT, "paging.camera_branch");
    check(has_event(EV_TILE_WINDOW) && has_event(EV_QUEUE_DRAIN) &&
          has_event(EV_TILE_REFILL), "paging.chain_present");
    record = find_event(EV_TILE_WINDOW);
    check(record != NULL && record->a0 == TERRAIN_POSITION,
          "paging.window_position");
    record = find_event(EV_VISIBILITY);
    check(record != NULL && record->a0 == TERRAIN_POSITION,
          "paging.visibility_position");
}

static void test_init_callback(void)
{
    seed();
    snapshot();
    check(func_80078948() == 1, "init.return_one");
    check(event_count == 0, "init.no_calls");
    check(memcmp(ram_before, g_PsxRam, sizeof(ram_before)) == 0,
          "init.no_writes");
}

static void test_scheduler_resolution(void)
{
    seed();
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(POOL_PTR, POOL);
    write32(SLOT + 0x18u, 0x80078948u);
    write32(SLOT + 0x1Cu, 0x80078950u);
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(read16(SLOT + 0x00u) == 1u && event_count == 0,
          "scheduler.cb0_resolution");
    wm_80097800();
    check(read16(SLOT + 0x00u) == 1u &&
          has_event(EV_TILED_OBJECTS), "scheduler.cb1_resolution");
    check(wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.no_resolution_abort");
}

int main(void)
{
    test_init_callback();
    test_no_paging();
    test_paging();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N58 SHARED MODE DRAW CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N58 SHARED MODE DRAW CERTIFICATE PASS");
    return 0;
}
