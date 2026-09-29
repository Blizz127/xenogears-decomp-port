/* Focused production certificate for retail 0x8008032C/0x80080370. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8032c.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void *D_8006259C;

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  4
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define COMMANDS    UINT32_C(0x8009A698)
#define TIMERS      UINT32_C(0x8009A6AC)
#define GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define GLOBAL_D554 UINT32_C(0x8009D554)
#define GLOBAL_D7CC UINT32_C(0x8009D7CC)

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
    write16(COMMANDS + 0u, 1u);
    write16(COMMANDS + 2u, 2u);
    write16(TIMERS + 0u, 135u);
    write16(TIMERS + 2u, 15u);
    reset_trace();
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    check(claim_count < 8u, "trace.claim_capacity");
    if (claim_count < 8u) {
        claims[claim_count].slot = slot_idx;
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
    check(wm_8008032C(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 135u &&
          read32(SLOT + 0x50u) == 0u,
          "init.script_seed");

    write16(SLOT + 0x22u, 1u);
    check(wm_80080370(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x50u) == 0u,
          "timer.zero_is_live");
    check(wm_80080370(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 135u &&
          read32(SLOT + 0x50u) == 1u,
          "timer.replays_entry_zero");
}

static void test_claim_commands(void)
{
    seed();
    set_command(2u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 3u, 1) && read16(SLOT + 0x20u) == 1u,
          "command2.claim");

    set_command(3u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 2u, 2) && read16(SLOT + 0x20u) == 1u,
          "command3.claim");

    set_command(4u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 0u, 13) && read32(GLOBAL_CCA4) == 1u &&
          read32(GLOBAL_D3CC) == 128u && read16(SLOT + 0x20u) == 1u,
          "command4.state");

    set_command(5u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 2u &&
          claim_is(0u, 0u, 12) && claim_is(1u, 4u, 1) &&
          read32(GLOBAL_CCA4) == 1u && read32(GLOBAL_D3CC) == 128u &&
          read16(SLOT + 0x20u) == 1u,
          "command5.claims_state");

    set_command(6u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 2u, 3) && read16(SLOT + 0x20u) == 1u,
          "command6.claim");

    set_command(7u);
    check(wm_80080370(SLOT_INDEX) == 1 && claim_count == 1u &&
          claim_is(0u, 0u, 13) && read32(GLOBAL_CCA4) == 2u &&
          read32(GLOBAL_D3CC) == 4u && read16(SLOT + 0x20u) == 1u,
          "command7.state");
}

static void test_sound_and_exit(void)
{
    seed();
    set_command(8u);
    check(wm_80080370(SLOT_INDEX) == 1 && sound_count == 3u &&
          sounds[0] == UINT32_C(0x43210016) &&
          sounds[1] == UINT32_C(0x43210017) &&
          sounds[2] == UINT32_C(0x43210018) &&
          read16(SLOT + 0x20u) == 1u,
          "command8.sounds");

    write32(GLOBAL_D554, 1u);
    write32(GLOBAL_D7CC, 1u);
    set_command(64u);
    check(wm_80080370(SLOT_INDEX) == 1 &&
          read32(GLOBAL_D554) == 0u && read32(GLOBAL_D7CC) == 0u &&
          read16(SLOT + 0x20u) == 0u,
          "command64.exit");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8008032C));
    write32(SLOT + 0x1Cu, UINT32_C(0x80080370));
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
          wm_sched_get_invalid_hits() == 0 && claim_count == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer_and_timer();
    test_claim_commands();
    test_sound_and_exit();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N90 MODE13 SCRIPTED CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N90 MODE13 SCRIPTED CONTROL CERTIFICATE PASS");
    return 0;
}
