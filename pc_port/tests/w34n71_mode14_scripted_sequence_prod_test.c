/* Focused production certificate for retail 0x8007A9B4/0x8007A9F8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7a9b4.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  3
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define SOUND_PTR   UINT32_C(0x8006259C)
#define SOUND       UINT32_C(0x800A8000)
#define STATE_TABLE UINT32_C(0x8009A450)
#define TIMER_TABLE UINT32_C(0x8009A46C)

enum TraceKind { TRACE_CLAIM, TRACE_MARKER, TRACE_SOUND60, TRACE_SOUND18 };
typedef struct Trace {
    enum TraceKind kind;
    u32 a;
    s32 b;
    u32 c;
} Trace;

static int failures;
static Trace trace[32];
static unsigned trace_count;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
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

static void append(enum TraceKind kind, u32 a, s32 b, u32 c)
{
    check(trace_count < (sizeof(trace) / sizeof(trace[0])), "trace.capacity");
    if (trace_count < (sizeof(trace) / sizeof(trace[0]))) {
        trace[trace_count].kind = kind;
        trace[trace_count].a = a;
        trace[trace_count].b = b;
        trace[trace_count].c = c;
        trace_count++;
    }
}

static void seed(void)
{
    static const u16 states[] = { 1u, 2u, 3u, 8u, 4u, 5u,
                                  6u, 8u, 9u, 7u, 10u, 11u };
    static const u16 timers[] = { 30u, 160u, 8u, 30u, 20u, 60u,
                                  8u, 30u, 120u, 76u, 72u, 1u };
    unsigned i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(SOUND_PTR, SOUND);
    write16(SOUND + 0x14u, 0x3456u);
    for (i = 0u; i < 12u; i++) {
        write16(STATE_TABLE + (u32)i * 2u, states[i]);
        write16(TIMER_TABLE + (u32)i * 2u, timers[i]);
    }
    trace_count = 0u;
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    append(TRACE_CLAIM, slot_idx, value, 0u);
    return 1;
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    append(TRACE_MARKER, a0, (s32)a1, a2);
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
}

void func_80039E60(s32 packed_id)
{
    append(TRACE_SOUND60, 0u, packed_id, 0u);
}

void func_80039E18(s32 packed_id)
{
    append(TRACE_SOUND18, 0u, packed_id, 0u);
}

static void set_state(u16 state)
{
    write16(SLOT + 0x20u, state);
}

static void expect_trace(unsigned index, enum TraceKind kind, u32 a, s32 b,
                         u32 c, const char* name)
{
    check(index < trace_count && trace[index].kind == kind &&
          trace[index].a == a && trace[index].b == b &&
          trace[index].c == c, name);
}

static void test_initializer_and_timing(void)
{
    seed();
    write32(SLOT + 0x50u, UINT32_C(0xDEADBEEF));
    check(wm_8007A9B4(SLOT_INDEX) == 1, "init.return");
    check(read32(SLOT + 0x50u) == 0u && read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 30u,
          "init.tables");

    write16(SLOT + 0x22u, 1u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x22u) == 0u && read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x50u) == 0u,
          "timer.zero_is_live");

    write16(SLOT + 0x22u, 0u);
    write32(SLOT + 0x50u, 3u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 8u && read16(SLOT + 0x22u) == 30u &&
          read32(SLOT + 0x50u) == 4u,
          "timer.table_transition");
}

static void test_marker_and_sound_states(void)
{
    seed();
    set_state(2u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 && read16(SLOT + 0x20u) == 1u,
          "state2.return");
    check(trace_count == 2u, "state2.trace_count");
    expect_trace(0u, TRACE_MARKER, 17u, 0, 0u, "state2.marker");
    expect_trace(1u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x34560001), 0u,
                 "state2.sound");

    seed();
    set_state(3u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 && read16(SLOT + 0x20u) == 1u,
          "state3.return");
    check(trace_count == 6u, "state3.trace_count");
    expect_trace(0u, TRACE_CLAIM, 2u, 2, 0u, "state3.claim");
    expect_trace(1u, TRACE_MARKER, 15u, 0, 0u, "state3.marker15");
    expect_trace(2u, TRACE_MARKER, 16u, 0, 0u, "state3.marker16");
    expect_trace(3u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x34560004), 0u,
                 "state3.sound4");
    expect_trace(4u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x34560005), 0u,
                 "state3.sound5");
    expect_trace(5u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x34560006), 0u,
                 "state3.sound6");

    seed();
    set_state(6u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 && trace_count == 4u,
          "state6.return_and_count");
    expect_trace(0u, TRACE_CLAIM, 2u, 4, 0u, "state6.claim");
    expect_trace(1u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x3456000A), 0u,
                 "state6.sound10");
    expect_trace(2u, TRACE_SOUND18, 0u, (s32)UINT32_C(0x3456000B), 0u,
                 "state6.sound11_function");
    expect_trace(3u, TRACE_SOUND18, 0u, (s32)UINT32_C(0x3456000C), 0u,
                 "state6.sound12_function");
}

static void test_claim_states(void)
{
    static const u16 states[] = { 4u, 7u, 8u, 9u };
    static const u32 count[] = { 2u, 1u, 1u, 2u };
    static const u32 a0[] = { 2u, 2u, 2u, 6u };
    static const s32 b0[] = { 3, 5, 1, 1 };
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        seed();
        set_state(states[i]);
        check(wm_8007A9F8(SLOT_INDEX) == 1 &&
              read16(SLOT + 0x20u) == 1u && trace_count == count[i],
              "claim_state.return_count");
        expect_trace(0u, TRACE_CLAIM, a0[i], b0[i], 0u,
                     "claim_state.first");
    }

    seed();
    set_state(5u);
    check(wm_8007A9F8(SLOT_INDEX) == 1 && trace_count == 6u,
          "state5.trace_count");
    expect_trace(0u, TRACE_CLAIM, 2u, 6, 0u, "state5.claim2");
    expect_trace(1u, TRACE_CLAIM, 4u, 1, 0u, "state5.claim4");
    expect_trace(2u, TRACE_CLAIM, 5u, 1, 0u, "state5.claim5");
    expect_trace(5u, TRACE_SOUND60, 0u, (s32)UINT32_C(0x34560009), 0u,
                 "state5.sound9");
}

static void test_terminal_states(void)
{
    seed();
    set_state(10u);
    write32(UINT32_C(0x8009D3CC), UINT32_C(0xAAAAAAAA));
    check(wm_8007A9F8(SLOT_INDEX) == 1 && trace_count == 1u &&
          read32(UINT32_C(0x8009D3CC)) == 4u &&
          read16(SLOT + 0x20u) == 1u,
          "state10.transition");
    expect_trace(0u, TRACE_CLAIM, 0u, 13, 0u, "state10.claim");

    seed();
    set_state(11u);
    write32(UINT32_C(0x8009D554), UINT32_C(0x11111111));
    write32(UINT32_C(0x8009D7CC), UINT32_C(0x22222222));
    check(wm_8007A9F8(SLOT_INDEX) == 1 &&
          read32(UINT32_C(0x8009D554)) == 0u &&
          read32(UINT32_C(0x8009D7CC)) == 0u &&
          read16(SLOT + 0x20u) == 1u,
          "state11.terminal_clear");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007A9B4));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007A9F8));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    write16(SLOT + 0u, 1u);
    set_state(0u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer_and_timing();
    test_marker_and_sound_states();
    test_claim_states();
    test_terminal_states();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N71 MODE14 SCRIPTED SEQUENCE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N71 MODE14 SCRIPTED SEQUENCE CERTIFICATE PASS");
    return 0;
}
