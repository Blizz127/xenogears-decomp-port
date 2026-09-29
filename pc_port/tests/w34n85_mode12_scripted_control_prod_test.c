/* Focused production certificate for retail 0x8007C36C/0x8007C3B8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7c36c.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void *D_8006259C;

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  2
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define COMMANDS    UINT32_C(0x8009A4D8)
#define TIMERS      UINT32_C(0x8009A4E8)
#define WORLD_X     UINT32_C(0x8009BE28)
#define WORLD_Z     UINT32_C(0x8009BE30)
#define GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define GLOBAL_D554 UINT32_C(0x8009D554)
#define GLOBAL_D7CC UINT32_C(0x8009D7CC)
#define SCRATCH_VEC UINT32_C(0x1F8000A0)

typedef struct ClaimCall {
    u32 slot;
    s32 value;
} ClaimCall;

static int failures;
static uint8_t sound_bank[32];
static ClaimCall claims[16];
static unsigned claim_count;
static u32 sounds[16];
static unsigned sound_count;
static unsigned marker_count;
static u32 marker_id;
static u32 marker_vector;
static u32 marker_flags;

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
    claim_count = 0u;
    sound_count = 0u;
    marker_count = 0u;
    marker_id = 0u;
    marker_vector = 0u;
    marker_flags = 0u;
    memset(claims, 0, sizeof(claims));
    memset(sounds, 0, sizeof(sounds));
}

static void seed(void)
{
    u16 bank = UINT16_C(0x1234);

    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    memset(sound_bank, 0, sizeof(sound_bank));
    memcpy(sound_bank + 0x14u, &bank, sizeof(bank));
    D_8006259C = sound_bank;
    write32(POOL_PTR, POOL);
    write16(COMMANDS + 0u, 1u);
    write16(TIMERS + 0u, 2u);
    write16(COMMANDS + 2u, 22u);
    write16(TIMERS + 2u, 7u);
    reset_trace();
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    check(claim_count < 16u, "trace.claim_capacity");
    if (claim_count < 16u) {
        claims[claim_count].slot = slot_idx;
        claims[claim_count].value = value;
        claim_count++;
    }
    return 1;
}

void func_80039E60(s32 packed_id)
{
    check(sound_count < 16u, "trace.sound_capacity");
    if (sound_count < 16u) {
        sounds[sound_count] = (u32)packed_id;
        sound_count++;
    }
}

void wm_80089160(u32 id, u32 vector, u32 flags)
{
    marker_count++;
    marker_id = id;
    marker_vector = vector;
    marker_flags = flags;
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
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

static int sounds_are(u16 first)
{
    return sound_count == 3u &&
           sounds[0] == (UINT32_C(0x12340000) | (u32)first) &&
           sounds[1] == (UINT32_C(0x12340000) | (u32)(first + 1u)) &&
           sounds[2] == (UINT32_C(0x12340000) | (u32)(first + 2u));
}

static void test_initializer(void)
{
    seed();
    write32(SLOT + 0x50u, UINT32_C(0xDEADBEEF));
    write16(SLOT + 0x20u, UINT16_C(0x7777));
    write16(SLOT + 0x22u, UINT16_C(0x8888));

    check(wm_8007C36C(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 1u && read16(SLOT + 0x22u) == 2u &&
          read32(SLOT + 0x50u) == 1u,
          "init.script_seed");
}

static void test_timer_boundaries(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write16(SLOT + 0x22u, 1u);
    write32(SLOT + 0x50u, 1u);
    check(wm_8007C3B8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x50u) == 1u,
          "timer.zero_is_live");

    check(wm_8007C3B8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 22u &&
          read16(SLOT + 0x22u) == 7u &&
          read32(SLOT + 0x50u) == 2u,
          "timer.advance_after_negative");
}

static void test_command_two(void)
{
    seed();
    set_command(2u);
    check(wm_8007C3B8(SLOT_INDEX) == 1, "command2.return");
    check(claim_count == 1u && claim_is(0u, 2u, 1),
          "command2.claim");
    check(read16(SLOT + 0x20u) == 1u, "command2.state");
    check(sounds_are(13u), "command2.sounds");
}

static void test_command_sixteen(void)
{
    seed();
    write32(WORLD_X, UINT32_C(0xFFF00000));
    write32(WORLD_Z, UINT32_C(0x00400000));
    set_command(16u);
    check(wm_8007C3B8(SLOT_INDEX) == 1, "command16.return");
    check(marker_count == 1u && marker_id == 20u &&
          marker_vector == SCRATCH_VEC && marker_flags == 0u,
          "command16.marker");
    check(read16(SCRATCH_VEC + 0u) == UINT16_C(0xFF00) &&
          read16(SCRATCH_VEC + 2u) == 0u &&
          read16(SCRATCH_VEC + 4u) == UINT16_C(0x0400),
          "command16.vector");
    check(sounds_are(16u) && read16(SLOT + 0x20u) == 1u,
          "command16.sounds_state");
}

static void test_command_seventeen(void)
{
    seed();
    set_command(17u);
    check(wm_8007C3B8(SLOT_INDEX) == 1, "command17.return");
    check(claim_count == 2u && claim_is(0u, 2u, 2) &&
          claim_is(1u, 0u, 13), "command17.claims");
    check(read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 64u,
          "command17.globals");
    check(sounds_are(19u) && read16(SLOT + 0x20u) == 1u,
          "command17.sounds_state");
}

static void test_command_eighteen(void)
{
    seed();
    set_command(18u);
    check(wm_8007C3B8(SLOT_INDEX) == 1, "command18.return");
    check(claim_count == 7u && claim_is(0u, 2u, 3) &&
          claim_is(1u, 3u, 1) && claim_is(2u, 4u, 1) &&
          claim_is(3u, 5u, 1) && claim_is(4u, 6u, 1) &&
          claim_is(5u, 7u, 1) && claim_is(6u, 0u, 12),
          "command18.claims");
    check(read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 1u &&
          read16(SLOT + 0x20u) == 1u,
          "command18.globals_state");
}

static void test_commands_nineteen_and_twenty(void)
{
    seed();
    set_command(19u);
    check(wm_8007C3B8(SLOT_INDEX) == 1 && claim_count == 2u &&
          claim_is(0u, 2u, 4) && claim_is(1u, 9u, 1) &&
          read16(SLOT + 0x20u) == 1u,
          "command19.claims");

    set_command(20u);
    check(wm_8007C3B8(SLOT_INDEX) == 1 && claim_count == 3u &&
          claim_is(0u, 2u, 6) && claim_is(1u, 3u, 2) &&
          claim_is(2u, 9u, 2) && read16(SLOT + 0x20u) == 1u,
          "command20.claims");
}

static void test_command_twenty_two(void)
{
    seed();
    set_command(22u);
    check(wm_8007C3B8(SLOT_INDEX) == 1, "command22.return");
    check(claim_count == 1u && claim_is(0u, 0u, 13),
          "command22.claim");
    check(read32(GLOBAL_CCA4) == 2u && read32(GLOBAL_D3CC) == 4u &&
          read16(SLOT + 0x20u) == 1u,
          "command22.globals_state");
}

static void test_command_sixty_four_and_default(void)
{
    seed();
    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 2u);
    set_command(64u);
    check(wm_8007C3B8(SLOT_INDEX) == 1 &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u &&
          read16(SLOT + 0x20u) == 1u,
          "command64.exit");

    write32(GLOBAL_D554, 3u);
    write32(GLOBAL_D7CC, 4u);
    set_command(21u);
    check(wm_8007C3B8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 21u && claim_count == 0u &&
          sound_count == 0u && marker_count == 0u &&
          read32(GLOBAL_D554) == 3u && read32(GLOBAL_D7CC) == 4u,
          "default.no_effect");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007C36C));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007C3B8));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    reset_trace();
    write16(SLOT + 0u, 1u);
    write16(SLOT + 0x20u, 21u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_timer_boundaries();
    test_command_two();
    test_command_sixteen();
    test_command_seventeen();
    test_command_eighteen();
    test_commands_nineteen_and_twenty();
    test_command_twenty_two();
    test_command_sixty_four_and_default();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N85 MODE12 SCRIPTED CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N85 MODE12 SCRIPTED CONTROL CERTIFICATE PASS");
    return 0;
}
