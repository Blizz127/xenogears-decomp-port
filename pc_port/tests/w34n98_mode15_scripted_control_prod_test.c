/* Focused production certificate for retail 0x8007DE14/0x8007DE98. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7de14.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void *D_8006259C;

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    1
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define SELECTOR      UINT32_C(0x8009D3D4)
#define SCRIPT_TABLES UINT32_C(0x8009A65C)
#define COMMANDS      UINT32_C(0x8009A700)
#define TIMERS        UINT32_C(0x8009A740)
#define SOUND_TABLE   UINT32_C(0x8009A5A0)
#define PARTICLE_PTR  UINT32_C(0x8009BDF4)
#define PARTICLES     UINT32_C(0x800A4000)
#define GLOBAL_CCA4   UINT32_C(0x8009CCA4)
#define GLOBAL_D3CC   UINT32_C(0x8009D3CC)
#define GLOBAL_D554   UINT32_C(0x8009D554)
#define GLOBAL_D7CC   UINT32_C(0x8009D7CC)

typedef struct ClaimCall {
    u32 slot;
    s32 value;
} ClaimCall;

static int failures;
static uint8_t sound_bank[32];
static ClaimCall claims[16];
static unsigned claim_count;
static u32 sounds[8];
static unsigned sound_count;

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
    memset(claims, 0, sizeof(claims));
    memset(sounds, 0, sizeof(sounds));
}

static void seed(void)
{
    u16 bank = UINT16_C(0x4321);

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(sound_bank, 0, sizeof(sound_bank));
    memcpy(sound_bank + 0x14u, &bank, sizeof(bank));
    D_8006259C = sound_bank;
    write32(POOL_PTR, POOL);
    write32(SELECTOR, 1u);
    write32(SCRIPT_TABLES + 8u, COMMANDS);
    write32(SCRIPT_TABLES + 12u, TIMERS);
    write16(COMMANDS + 0u, 1u);
    write16(COMMANDS + 2u, 62u);
    write16(TIMERS + 0u, 90u);
    write16(TIMERS + 2u, 60u);
    write32(PARTICLE_PTR, PARTICLES);
    write16(SOUND_TABLE + 0u, 0x0101u);
    write16(SOUND_TABLE + 2u, 0x0102u);
    write16(SOUND_TABLE + 4u, 0x0103u);
    write16(SOUND_TABLE + 6u, 0x0201u);
    write16(SOUND_TABLE + 8u, 0x0202u);
    write16(SOUND_TABLE + 10u, 0x0203u);
    write16(SOUND_TABLE + 12u, 0x0301u);
    reset_trace();
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    check(claim_count < 16u, "trace.claim_capacity");
    if (claim_count < 16u) {
        claims[claim_count].slot = slot_idx;
        claims[claim_count].value = value;
    }
    claim_count++;
    return 1;
}

void func_80039E60(s32 packed_id)
{
    check(sound_count < 8u, "trace.sound_capacity");
    if (sound_count < 8u)
        sounds[sound_count] = (u32)packed_id;
    sound_count++;
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

static void seed_particle(u32 record_index, s16 owner,
                          u16 live_value, s16 live_gate)
{
    u32 record = PARTICLES + record_index * UINT32_C(0x4C);
    write16(record + 0u, (u16)owner);
    write16(record + 4u, live_value);
    write16(record + 6u, (u16)live_gate);
}

static void test_initializer_and_timer(void)
{
    seed();
    check(wm_8007DE14(SLOT_INDEX) == 1, "init.return");
    check(read32(SLOT + 0x54u) == COMMANDS &&
          read32(SLOT + 0x58u) == TIMERS &&
          read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 90u &&
          read32(SLOT + 0x50u) == 1u,
          "init.script_seed");

    write16(SLOT + 0x22u, 1u);
    check(wm_8007DE98(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x50u) == 1u,
          "timer.zero_is_live");
    check(wm_8007DE98(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 62u &&
          read16(SLOT + 0x22u) == 60u &&
          read32(SLOT + 0x50u) == 2u,
          "timer.next_entry");
}

static void test_command2_and_particles(void)
{
    seed();
    set_command(2u);
    check(wm_8007DE98(SLOT_INDEX) == 1 && sound_count == 3u &&
          sounds[0] == UINT32_C(0x4321001C) &&
          sounds[1] == UINT32_C(0x4321001D) &&
          sounds[2] == UINT32_C(0x4321001E) &&
          claim_count == 7u && claim_is(0u, 2u, 2) &&
          claim_is(1u, 3u, 2) && claim_is(2u, 4u, 3) &&
          claim_is(6u, 8u, 3) && read16(SLOT + 0x20u) == 1u,
          "command2.sound_claims");

    seed_particle(0u, (s16)(34 * 8 + 2), 0x7777u, 1);
    seed_particle(1u, (s16)(35 * 8 + 5), 0x6666u, 1);
    seed_particle(2u, (s16)(36 * 8 + 7), 0x5555u, 1);
    seed_particle(3u, (s16)(37 * 8), 0x4444u, 1);
    set_command(3u);
    check(wm_8007DE98(SLOT_INDEX) == 1 &&
          read16(PARTICLES + 4u) == 0u &&
          read16(PARTICLES + UINT32_C(0x4C) + 4u) == 0u &&
          read16(PARTICLES + UINT32_C(0x98) + 4u) == 0u &&
          read16(PARTICLES + UINT32_C(0xE4) + 4u) == 0x4444u &&
          claim_count == 7u && claim_is(0u, 2u, 3) &&
          claim_is(6u, 8u, 3),
          "command3.particle_release");
}

static void test_middle_commands(void)
{
    seed();
    set_command(4u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 7u && claim_is(0u, 2u, 4) &&
          claim_is(1u, 3u, 4) && claim_is(6u, 8u, 3),
          "command4.claims");

    set_command(5u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 2u, 5), "command5.claim");

    set_command(6u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 0u, 13) &&
          read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 64u,
          "command6.state");

    set_command(7u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 7u && claim_is(0u, 3u, 5) &&
          claim_is(1u, 4u, 4) && claim_is(5u, 8u, 4) &&
          claim_is(6u, 0u, 12) && read32(GLOBAL_CCA4) == 1u &&
          read32(GLOBAL_D3CC) == 64u, "command7.claims_state");

    set_command(8u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 2u, 6), "command8.claim");

    set_command(9u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 0u, 13) &&
          read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 128u,
          "command9.state");

    set_command(10u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 3u && claim_is(0u, 0u, 12) &&
          claim_is(1u, 9u, 1) && claim_is(2u, 2u, 7) &&
          read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 128u,
          "command10.claims_state");
}

static void test_variant_commands(void)
{
    seed();
    set_command(16u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 7u && claim_is(0u, 2u, 4) &&
          claim_is(1u, 3u, 16) && claim_is(6u, 8u, 3),
          "command16.claims");
    set_command(17u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 2u, 16), "command17.claim");
    set_command(18u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 2u, 17), "command18.claim");
    set_command(24u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 7u && claim_is(0u, 2u, 4) &&
          claim_is(1u, 3u, 24) && claim_is(6u, 8u, 3),
          "command24.claims");
    set_command(25u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 2u, 24), "command25.claim");
}

static void test_sounds_state_and_exit(void)
{
    seed();
    set_command(61u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(sound_count == 3u && sounds[0] == UINT32_C(0x43210201) &&
          sounds[1] == UINT32_C(0x43210202) &&
          sounds[2] == UINT32_C(0x43210203), "command61.sound_table");

    set_command(62u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(sound_count == 3u && sounds[0] == UINT32_C(0x43210019) &&
          sounds[1] == UINT32_C(0x4321001A) &&
          sounds[2] == UINT32_C(0x4321001B), "command62.sounds");

    set_command(63u);
    (void)wm_8007DE98(SLOT_INDEX);
    check(claim_count == 1u && claim_is(0u, 0u, 13) &&
          read32(GLOBAL_CCA4) == 2u && read32(GLOBAL_D3CC) == 4u,
          "command63.state");

    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 2u);
    set_command(64u);
    check(wm_8007DE98(SLOT_INDEX) == 1 &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u &&
          read16(SLOT + 0x20u) == 0u, "command64.exit");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007DE14));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007DE98));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    set_command(62u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && sound_count == 3u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer_and_timer();
    test_command2_and_particles();
    test_middle_commands();
    test_variant_commands();
    test_sounds_state_and_exit();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N98 MODE15 SCRIPTED CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N98 MODE15 SCRIPTED CONTROL CERTIFICATE PASS");
    return 0;
}
