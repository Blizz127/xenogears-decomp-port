/* Focused production certificate for retail 0x80081174/0x800811C0. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_81174.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void *D_80062528;
void *D_8006259C;

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  1
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define COMMANDS    UINT32_C(0x8009A6C0)
#define TIMERS      UINT32_C(0x8009A70C)
#define GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define GLOBAL_D554 UINT32_C(0x8009D554)
#define GLOBAL_D7CC UINT32_C(0x8009D7CC)
#define AUDIO_MANAGER ((void *)(uintptr_t)UINT32_C(0x12345678))

typedef struct ClaimCall {
    u32 slot;
    s32 value;
} ClaimCall;

static int failures;
static uint8_t sound_bank[32];
static ClaimCall claims[8];
static unsigned claim_count;
static u32 sounds[3];
static unsigned sound_count;
static unsigned fade_count;
static void *fade_manager;
static s32 fade_level;
static s32 fade_steps;

static void check(int condition, const char *name)
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

static void reset_trace(void)
{
    memset(claims, 0, sizeof(claims));
    memset(sounds, 0, sizeof(sounds));
    claim_count = 0u;
    sound_count = 0u;
    fade_count = 0u;
    fade_manager = NULL;
    fade_level = 0;
    fade_steps = 0;
}

static void seed(void)
{
    u16 bank = UINT16_C(0x2468);

    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    memset(sound_bank, 0, sizeof(sound_bank));
    memcpy(sound_bank + 0x14u, &bank, sizeof(bank));
    D_80062528 = AUDIO_MANAGER;
    D_8006259C = sound_bank;
    write32(POOL_PTR, POOL);
    write16(COMMANDS + 0u, 1u);
    write16(COMMANDS + 2u, 8u);
    write16(TIMERS + 0u, 10u);
    write16(TIMERS + 2u, 170u);
    reset_trace();
}

s32 wm_80097770(u32 slot_index, s32 value)
{
    check(claim_count < 8u, "trace.claim_capacity");
    if (claim_count < 8u) {
        claims[claim_count].slot = slot_index;
        claims[claim_count].value = value;
    }
    claim_count++;
    return 1;
}

void func_80039E60(s32 packed_id)
{
    check(sound_count < 3u, "trace.sound_capacity");
    if (sound_count < 3u)
        sounds[sound_count] = (u32)packed_id;
    sound_count++;
}

void func_8003A89C(void *manager, s32 level, s32 steps)
{
    fade_count++;
    fade_manager = manager;
    fade_level = level;
    fade_steps = steps;
}

static void set_command(u16 command)
{
    write16(SLOT + 0x20u, command);
    reset_trace();
}

static int claim_is(unsigned index, u32 slot, s32 value)
{
    return index < claim_count && claims[index].slot == slot &&
           claims[index].value == value;
}

static void test_initializer_and_timer(void)
{
    seed();
    write32(SLOT + 0x50u, UINT32_C(0xDEADBEEF));
    check(wm_80081174(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 10u &&
          read32(SLOT + 0x50u) == 1u,
          "init.script_seed");

    write16(SLOT + 0x22u, 1u);
    check(wm_800811C0(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x50u) == 1u,
          "timer.zero_is_live");
    check(wm_800811C0(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 8u &&
          read16(SLOT + 0x22u) == 170u &&
          read32(SLOT + 0x50u) == 2u,
          "timer.advance_after_negative");
}

static void test_claim_commands(void)
{
    static const s32 values[6] = {1, 0, 2, 3, 4, 5};
    unsigned index;

    seed();
    for (index = 0u; index < 6u; index++) {
        set_command((u16)(index + 2u));
        check(wm_800811C0(SLOT_INDEX) == 1 && claim_count == 1u &&
              claim_is(0u, 6u, values[index]) &&
              read16(SLOT + 0x20u) == 1u,
              index == 1u ? "command3.claim" : "commands2_7.claims");
    }

    set_command(16u);
    check(wm_800811C0(SLOT_INDEX) == 1 && claim_count == 2u &&
          claim_is(0u, 3u, 1) && claim_is(1u, 4u, 1) &&
          read16(SLOT + 0x20u) == 1u,
          "command16.claims");

    set_command(17u);
    check(wm_800811C0(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 3u, 2) && read16(SLOT + 0x20u) == 1u,
          "command17.claim");
}

static void test_sound_fade_and_exit(void)
{
    seed();
    set_command(8u);
    check(wm_800811C0(SLOT_INDEX) == 1 && sound_count == 3u &&
          sounds[0] == UINT32_C(0x2468002E) &&
          sounds[1] == UINT32_C(0x2468002F) &&
          sounds[2] == UINT32_C(0x24680030) &&
          read16(SLOT + 0x20u) == 1u,
          "command8.sounds");

    set_command(63u);
    check(wm_800811C0(SLOT_INDEX) == 1 && fade_count == 1u &&
          fade_manager == AUDIO_MANAGER && fade_level == 0 &&
          fade_steps == 240 && claim_count == 1u &&
          claim_is(0u, 0u, 13) && read32(GLOBAL_CCA4) == 2u &&
          read32(GLOBAL_D3CC) == 4u && read16(SLOT + 0x20u) == 1u,
          "command63.fade_state");

    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 1u);
    set_command(64u);
    check(wm_800811C0(SLOT_INDEX) == 1 &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u &&
          read16(SLOT + 0x20u) == 0u,
          "command64.exit");
}

static void test_bounds_and_scheduler_resolution(void)
{
    seed();
    set_command(UINT16_C(0xFFFF));
    check(wm_800811C0(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == UINT16_C(0xFFFF) &&
          claim_count == 0u && sound_count == 0u && fade_count == 0u,
          "dispatch.negative_ignored");

    write32(SLOT + 0x18u, UINT32_C(0x80081174));
    write32(SLOT + 0x1Cu, UINT32_C(0x800811C0));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    set_command(2u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && claim_count == 1u &&
          claim_is(0u, 6u, 1),
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer_and_timer();
    test_claim_commands();
    test_sound_fade_and_exit();
    test_bounds_and_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N103 MODE16 SCRIPTED CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N103 MODE16 SCRIPTED CONTROL CERTIFICATE PASS");
    return 0;
}
