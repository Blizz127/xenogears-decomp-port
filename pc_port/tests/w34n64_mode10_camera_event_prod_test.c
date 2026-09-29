/* Focused production certificate for retail 0x80078E2C..0x800794D8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_78e2c.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL         0x800A0000u
#define POOL_PTR     0x8009BE24u
#define SLOT_INDEX   2
#define SLOT         (POOL + (u32)SLOT_INDEX * 0x80u)
#define GEOM_Y       0x8009BE0Cu
#define ANGLES       0x8009BD38u
#define CAMERA_INPUT 0x8009BD40u
#define POSITION     0x8009BE28u
#define MODE_STATE   0x8009D144u
#define HEIGHT       0x8009D3F0u
#define CCA4         0x8009CCA4u
#define D3CC         0x8009D3CCu
#define D554         0x8009D554u
#define D7CC         0x8009D7CCu

static uint8_t sound_authority[32];
void* D_8006259C = sound_authority;

static int failures;
static int view_calls;
static u32 view_out;
static u32 view_position;
static s32 view_height;
static u32 view_angles;
static int claim_calls;
static u32 claim_slots[16];
static s32 claim_values[16];
static int sound_start_calls;
static u32 sound_start_ids[8];
static int sound_stop_calls;
static u32 sound_stop_ids[8];
static s32 sound_stop_pitch[8];
static s32 sound_stop_steps[8];
static int random_values[8];
static int random_count;
static int random_index;

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

static s16 read_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void seed(void)
{
    u16 bank = 0x1234u;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    memset(sound_authority, 0, sizeof(sound_authority));
    memcpy(sound_authority + 0x14u, &bank, sizeof(bank));
    write32(POOL_PTR, POOL);
    view_calls = 0;
    view_out = 0u;
    view_position = 0u;
    view_height = 0;
    view_angles = 0u;
    claim_calls = 0;
    memset(claim_slots, 0, sizeof(claim_slots));
    memset(claim_values, 0, sizeof(claim_values));
    sound_start_calls = 0;
    memset(sound_start_ids, 0, sizeof(sound_start_ids));
    sound_stop_calls = 0;
    memset(sound_stop_ids, 0, sizeof(sound_stop_ids));
    memset(sound_stop_pitch, 0, sizeof(sound_stop_pitch));
    memset(sound_stop_steps, 0, sizeof(sound_stop_steps));
    memset(random_values, 0, sizeof(random_values));
    random_count = 0;
    random_index = 0;
}

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 rot_svec)
{
    view_calls++;
    view_out = out_matrix;
    view_position = pos_vec;
    view_height = height;
    view_angles = rot_svec;
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    if (claim_calls < 16) {
        claim_slots[claim_calls] = slot_idx;
        claim_values[claim_calls] = value;
    }
    claim_calls++;
    return 1;
}

void func_80039E60(s32 packed_id)
{
    if (sound_start_calls < 8)
        sound_start_ids[sound_start_calls] = (u32)packed_id;
    sound_start_calls++;
}

void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps)
{
    if (sound_stop_calls < 8) {
        sound_stop_ids[sound_stop_calls] = (u32)packed_id;
        sound_stop_pitch[sound_stop_calls] = pitch;
        sound_stop_steps[sound_stop_calls] = steps;
    }
    sound_stop_calls++;
}

int rand(void)
{
    int value = 0;
    if (random_index < random_count)
        value = random_values[random_index];
    random_index++;
    return value;
}

static void set_state(u16 state, u16 timer)
{
    write16(SLOT + 0x20u, state);
    write16(SLOT + 0x22u, timer);
}

static void check_view(int expected, const char* name)
{
    check(view_calls == expected &&
          (expected == 0 ||
           (view_out == CAMERA_INPUT && view_position == POSITION &&
            view_height == (s32)read32(HEIGHT) && view_angles == ANGLES)),
          name);
}

static void test_init(void)
{
    seed();
    write32(SLOT + 0x50u, 1u);
    write32(SLOT + 0x54u, 2u);
    write32(SLOT + 0x58u, 3u);
    write32(SLOT + 0x74u, 4u);
    check(func_80078E2C(SLOT_INDEX) == 1, "init.return");
    check(read32(GEOM_Y) == 120u && read32(HEIGHT) == 0x00200000u &&
          read32(MODE_STATE) == 0u,
          "init.world_state");
    check(read16(SLOT + 0x20u) == 16u &&
          read16(SLOT + 0x22u) == 64u &&
          read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 0u &&
          read32(SLOT + 0x58u) == 0u &&
          read32(SLOT + 0x74u) == 0u,
          "init.state16");
    check(read_s16(ANGLES + 0u) == -192 &&
          read_s16(ANGLES + 2u) == 1664 &&
          read_s16(ANGLES + 4u) == 0,
          "init.angles");
}

static void test_early_sequence(void)
{
    seed();
    write32(HEIGHT, 0x00200000u);
    set_state(0u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 1u && read16(SLOT + 0x22u) == 32u &&
          claim_calls == 1 && claim_slots[0] == 5u && claim_values[0] == 1,
          "state0.claim");
    check(sound_start_calls == 2 &&
          sound_start_ids[0] == 0x12340062u &&
          sound_start_ids[1] == 0x12340063u,
          "state0.sounds");
    check_view(1, "state0.view");

    seed();
    set_state(1u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 2u && read16(SLOT + 0x22u) == 32u &&
          claim_calls == 1 && claim_slots[0] == 3u,
          "state1.transition");

    seed();
    set_state(2u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 3u && read16(SLOT + 0x22u) == 48u &&
          claim_calls == 3 && claim_slots[0] == 4u &&
          claim_slots[1] == 2u && claim_slots[2] == 6u,
          "state2.claims");

    seed();
    set_state(3u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 4u && read16(SLOT + 0x22u) == 40u,
          "state3.enter_jitter");
    check_view(0, "state3.skips_view");
}

static void seed_camera_halves(u16 value)
{
    write16(CAMERA_INPUT + 0u, value);
    write16(CAMERA_INPUT + 2u, value);
    write16(CAMERA_INPUT + 8u, value);
    write16(CAMERA_INPUT + 0x0Au, value);
}

static void test_jitter_states(void)
{
    seed();
    write32(HEIGHT, 0x00200000u);
    set_state(4u, 2u);
    seed_camera_halves(1000u);
    random_values[0] = 11;
    random_values[1] = 0;
    random_count = 2;
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 4u && read16(SLOT + 0x22u) == 1u &&
          read16(CAMERA_INPUT + 0u) == 1005u &&
          read16(CAMERA_INPUT + 8u) == 1005u &&
          read16(CAMERA_INPUT + 2u) == 994u &&
          read16(CAMERA_INPUT + 0x0Au) == 994u && random_index == 2,
          "jitter.state4");
    check_view(1, "jitter.view_once");

    seed();
    set_state(4u, 1u);
    random_values[0] = 6;
    random_values[1] = 6;
    random_count = 2;
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 5u && read16(SLOT + 0x22u) == 120u &&
          sound_start_calls == 1 && sound_start_ids[0] == 0x12340079u &&
          sound_stop_calls == 2 && sound_stop_ids[0] == 0x12340062u &&
          sound_stop_ids[1] == 0x12340063u &&
          sound_stop_steps[0] == 256 && sound_stop_steps[1] == 256,
          "jitter.state4_exit_audio");

    seed();
    set_state(5u, 1u);
    random_values[0] = 3;
    random_values[1] = 0;
    random_count = 2;
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 6u && read16(SLOT + 0x22u) == 90u,
          "jitter.state5_exit");
    check_view(2, "jitter.state5_expiry_view_twice");
}

static void test_exit_sequence(void)
{
    seed();
    set_state(6u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 7u && read16(SLOT + 0x22u) == 80u &&
          claim_calls == 1 && claim_slots[0] == 0u && claim_values[0] == 13 &&
          sound_stop_calls == 1 && sound_stop_ids[0] == 0x12340079u &&
          read32(CCA4) == 2u && read32(D3CC) == 4u,
          "state6.globals");

    seed();
    set_state(7u, 1u);
    write32(D554, 7u);
    write32(D7CC, 9u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x22u) == 0u &&
          read32(D554) == 0u && read32(D7CC) == 0u,
          "state7.world_exit");
}

static void test_entry_sequence(void)
{
    seed();
    set_state(16u, 7u);
    write16(SLOT + 4u, 0u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 16u && read16(SLOT + 0x22u) == 7u &&
          claim_calls == 0 && sound_stop_calls == 0,
          "entry.flag_gate");

    seed();
    set_state(16u, 7u);
    write16(SLOT + 4u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 17u && read16(SLOT + 0x22u) == 8u &&
          read16(SLOT + 4u) == 0u && claim_calls == 1 &&
          claim_slots[0] == 0u && claim_values[0] == 13 &&
          read32(CCA4) == 1u && read32(D3CC) == 32u &&
          sound_stop_calls == 1 && sound_stop_ids[0] == 0x12340036u &&
          sound_stop_pitch[0] == 0 && sound_stop_steps[0] == 8,
          "entry.state16_transition");

    seed();
    set_state(17u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 18u && read16(SLOT + 0x22u) == 16u &&
          read32(HEIGHT) == 0x00960000u &&
          read_s16(ANGLES + 0u) == -48 &&
          read_s16(ANGLES + 2u) == 64 && read_s16(ANGLES + 4u) == 0,
          "entry.state17_camera");

    seed();
    set_state(18u, 1u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 0u && read16(SLOT + 0x22u) == 1u &&
          claim_calls == 1 && claim_slots[0] == 0u && claim_values[0] == 12 &&
          read32(CCA4) == 1u && read32(D3CC) == 128u,
          "entry.loop_reset");
}

static void test_default_and_scheduler(void)
{
    seed();
    set_state(8u, 0x1234u);
    (void)wm_80078EA4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 8u && read16(SLOT + 0x22u) == 0x1234u,
          "default.read_only");
    check_view(1, "default.view");

    seed();
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(SLOT + 0x18u, 0x80078E2Cu);
    write32(SLOT + 0x1Cu, 0x80078EA4u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 2 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT) == 1u && read16(SLOT + 0x20u) == 16u,
          "scheduler.pair_resolution");
}

int main(void)
{
    test_init();
    test_early_sequence();
    test_jitter_states();
    test_exit_sequence();
    test_entry_sequence();
    test_default_and_scheduler();
    if (failures != 0) {
        fprintf(stderr, "W34N64 MODE10 CAMERA/EVENT CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N64 MODE10 CAMERA/EVENT CERTIFICATE PASS");
    return 0;
}
