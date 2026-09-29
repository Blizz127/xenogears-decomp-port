/* Focused production certificate for the retail mode-17 command stream. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827c8.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800B0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 1
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define SCRIPT     UINT32_C(0x8009A758)
#define COMMANDS   UINT32_C(0x800B1000)
#define POSITION_X UINT32_C(0x8009C5AC)
#define POSITION_Y UINT32_C(0x8009C5B0)
#define POSITION_Z UINT32_C(0x8009C5B4)
#define ANGLES     UINT32_C(0x1F8000A0)
#define GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define GLOBAL_D554 UINT32_C(0x8009D554)
#define GLOBAL_D7CC UINT32_C(0x8009D7CC)

enum EventKind {
    EV_CLAIM = 1,
    EV_MARKER_CREATE,
    EV_MARKER_CLEAR,
    EV_GROUP_CLEAR,
    EV_FADE,
    EV_SOUND,
    EV_SOUND_CONTROL
};

typedef struct Event {
    int kind;
    uintptr_t pointer;
    s32 a;
    s32 b;
    s32 c;
} Event;

static Event events[32];
static u32 event_count;
static int failures;
static uint8_t manager_storage;
static uint8_t seds[32];

void *D_80062528 = &manager_storage;
void *D_8006259C = seds;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void trace(int kind, uintptr_t pointer, s32 a, s32 b, s32 c)
{
    check(event_count < 32u, "trace.capacity");
    if (event_count < 32u) {
        events[event_count].kind = kind;
        events[event_count].pointer = pointer;
        events[event_count].a = a;
        events[event_count].b = b;
        events[event_count].c = c;
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

static void write_command(u32 address, u16 opcode, s16 a1, s16 a2, s16 a3)
{
    u32 word = (u32)opcode | ((u32)(u16)a1 << 16u);
    write32(address, word);
    write16(address + 4u, (u16)a2);
    write16(address + 6u, (u16)a3);
}

static void seed(void)
{
    u16 bank = UINT16_C(0x1234);

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(events, 0, sizeof(events));
    memset(seds, 0, sizeof(seds));
    memcpy(seds + 0x14u, &bank, sizeof(bank));
    write32(POOL_PTR, POOL);
    event_count = 0u;
}

s32 wm_80097770(u32 slot_index, s32 value)
{
    trace(EV_CLAIM, 0u, (s32)slot_index, value, 0);
    return 1;
}

void wm_80089160(u32 id, u32 vector, u32 flags)
{
    trace(EV_MARKER_CREATE, 0u, (s32)id, (s32)vector, (s32)flags);
}

void wm_800894C8(u32 record_index)
{
    trace(EV_MARKER_CLEAR, 0u, (s32)record_index, 0, 0);
}

void wm_80089514(u32 group_index)
{
    trace(EV_GROUP_CLEAR, 0u, (s32)group_index, 0, 0);
}

void func_8003A89C(void *manager, s32 level, s32 steps)
{
    trace(EV_FADE, (uintptr_t)manager, level, steps, 0);
}

void func_80039E60(s32 packed_id)
{
    trace(EV_SOUND, 0u, packed_id, 0, 0);
}

void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps)
{
    trace(EV_SOUND_CONTROL, 0u, packed_id, pitch, steps);
}

static void test_initializer(void)
{
    seed();
    check(wm_800827C8(SLOT_INDEX) == 1, "init.return");
    check(read32(SLOT + 0x50u) == SCRIPT, "init.script_pointer");
    check(event_count == 0u, "init.no_side_effects");
}

static void test_timer(void)
{
    u32 timer = COMMANDS + 0x200u;

    seed();
    write32(SLOT + 0x50u, timer);
    write32(GLOBAL_D554, 7u);
    write32(GLOBAL_D7CC, 8u);
    write_command(timer, 1u, 2, 0, 0);
    write_command(timer + 4u, 0u, 0, 0, 0);

    (void)wm_80076B34(SLOT_INDEX);
    check(read16(SLOT + 0x22u) == 2u &&
          read32(SLOT + 0x50u) == timer,
          "timer.seed");
    (void)wm_80076B34(SLOT_INDEX);
    check(read16(SLOT + 0x22u) == 1u &&
          read32(SLOT + 0x50u) == timer,
          "timer.hold");
    (void)wm_80076B34(SLOT_INDEX);
    check(read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x50u) == timer + 4u &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u,
          "timer.expiry");
}

static void test_complete_dispatch(void)
{
    u32 cursor = COMMANDS;
    u32 final_command;
    int events_ok;

    seed();
    write32(SLOT + 0x50u, COMMANDS);
    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 2u);

    write_command(cursor, 2u, 5, -6, 0); cursor += 8u;
    write_command(cursor, 3u, -2, 3, -4); cursor += 8u;
    write_command(cursor, 4u, 0x111, -0x222, 0x333); cursor += 8u;
    write_command(cursor, 5u, 7, 0, 0); cursor += 4u;
    write_command(cursor, 6u, 8, 0, 0); cursor += 4u;
    write_command(cursor, 7u, 9, 0, 0); cursor += 4u;
    write_command(cursor, 8u, 10, 11, 12); cursor += 8u;
    write_command(cursor, 9u, 12, 0, 0); cursor += 4u;
    write_command(cursor, 10u, 13, 14, 15); cursor += 8u;
    write_command(cursor, 11u, 16, 17, 0); cursor += 8u;
    final_command = cursor;
    write_command(cursor, 0u, 0, 0, 0);

    check(wm_80076B34(SLOT_INDEX) == 1, "update.return");
    check(read32(SLOT + 0x50u) == final_command,
          "update.advance_scale");
    check(read32(POSITION_X) == UINT32_C(0xFFFFE000) &&
          read32(POSITION_Y) == UINT32_C(0x00003000) &&
          read32(POSITION_Z) == UINT32_C(0xFFFFC000),
          "command3.position");
    check(read16(ANGLES + 0u) == UINT16_C(0x0111) &&
          read16(ANGLES + 2u) == UINT16_C(0xFDDE) &&
          read16(ANGLES + 4u) == UINT16_C(0x0333),
          "command4.angles");
    check(read32(GLOBAL_CCA4) == 16u && read32(GLOBAL_D3CC) == 17u,
          "command11.state");
    check(read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u,
          "command0.exit");

    events_ok = event_count == 7u;
    if (events_ok != 0) {
        events_ok = events[0].kind == EV_CLAIM &&
                    events[0].a == 5 && events[0].b == -6 &&
                    events[1].kind == EV_MARKER_CREATE &&
                    events[1].a == 7 &&
                    events[1].b == (s32)ANGLES && events[1].c == 0 &&
                    events[2].kind == EV_MARKER_CLEAR && events[2].a == 8 &&
                    events[3].kind == EV_GROUP_CLEAR && events[3].a == 9 &&
                    events[4].kind == EV_FADE &&
                    events[4].pointer == (uintptr_t)D_80062528 &&
                    events[4].a == 10 && events[4].b == 11 &&
                    events[5].kind == EV_SOUND &&
                    events[5].a == (s32)UINT32_C(0x1234000C) &&
                    events[6].kind == EV_SOUND_CONTROL &&
                    events[6].a == (s32)UINT32_C(0x1234000D) &&
                    events[6].b == 14 && events[6].c == 15;
    }
    check(events_ok != 0, "dispatch.mapping");
    check(event_count > 1u && events[1].b == (s32)ANGLES,
          "command5.angle_source");
    check(event_count > 5u &&
          events[5].a == (s32)UINT32_C(0x1234000C),
          "command9.sound_bank");
    check(event_count > 6u && events[6].b == 14 && events[6].c == 15,
          "command10.sound_control");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x800827C8));
    write32(SLOT + 0x1Cu, UINT32_C(0x80076B34));
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read32(SLOT + 0x50u) == SCRIPT,
          "scheduler.init_resolution");

    write32(SLOT + 0x50u, COMMANDS);
    write_command(COMMANDS, 0u, 0, 0, 0);
    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 2u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_timer();
    test_complete_dispatch();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N111 MODE17 COMMAND CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N111 MODE17 COMMAND CERTIFICATE PASS");
    return 0;
}
