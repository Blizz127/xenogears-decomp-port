/* Focused production certificate for retail 0x800795E4..0x8007A06C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_795e4.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          0x800A0000u
#define POOL_PTR      0x8009BE24u
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * 0x80u)
#define POSITION      0x8009BE28u
#define RENDER_Z      0x8009BBBCu
#define CONTEXT_PTR   0x8009C620u
#define CONTEXT       0x800A8000u
#define RESET_POS     0x8009C5ACu
#define SCRATCH_ROT_A 0x1F8000A0u
#define SCRATCH_MAT_A 0x800000F0u
#define SCRATCH_MAT_B 0x80000110u
#define SCRATCH_MAT_C 0x80000130u
#define SCRATCH_MAT_D 0x80000150u

static uint8_t sound_authority[32];
void* D_8006259C = sound_authority;

static int failures;
static int link_calls;
static s32 link_sources[16];
static s32 link_destinations[16];
static int sound_calls;
static u32 sound_ids[8];
static int presence_calls;
static u32 presence_ids[8];
static s16 presence_xyz[8][3];
static int clear_calls;
static u32 clear_ids[8];
static int claim_calls;
static u32 claim_slots[8];
static s32 claim_values[8];
static int wrap_calls;
static u32 wrap_address;
static u32 wrap_delta;
static int rot_calls;
static s16 rot_angles[8][3];
static u32 rot_destinations[8];

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

static u32 pointer_guest_address(const void* pointer)
{
    uintptr_t value = (uintptr_t)pointer;
    uintptr_t ram = (uintptr_t)g_PsxRam;
    uintptr_t scratch = (uintptr_t)g_PsxScratchpad;

    if (value >= scratch && value < scratch + sizeof(g_PsxScratchpad))
        return 0x1F800000u + (u32)(value - scratch);
    return 0x80000000u + (u32)(value - ram);
}

static u32 rot_word(int call_index, u32 word_index)
{
    return 0xA0000000u + ((u32)(call_index + 1) << 20u) + word_index;
}

static void reset_observers(void)
{
    link_calls = 0;
    memset(link_sources, 0, sizeof(link_sources));
    memset(link_destinations, 0, sizeof(link_destinations));
    sound_calls = 0;
    memset(sound_ids, 0, sizeof(sound_ids));
    presence_calls = 0;
    memset(presence_ids, 0, sizeof(presence_ids));
    memset(presence_xyz, 0, sizeof(presence_xyz));
    clear_calls = 0;
    memset(clear_ids, 0, sizeof(clear_ids));
    claim_calls = 0;
    memset(claim_slots, 0, sizeof(claim_slots));
    memset(claim_values, 0, sizeof(claim_values));
    wrap_calls = 0;
    wrap_address = 0u;
    wrap_delta = 0u;
    rot_calls = 0;
    memset(rot_angles, 0, sizeof(rot_angles));
    memset(rot_destinations, 0, sizeof(rot_destinations));
}

static void seed(void)
{
    u16 bank = 0x1234u;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    memset(sound_authority, 0, sizeof(sound_authority));
    memcpy(sound_authority + 0x14u, &bank, sizeof(bank));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    reset_observers();
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    if (link_calls < 16) {
        link_sources[link_calls] = source_record;
        link_destinations[link_calls] = destination_record;
    }
    link_calls++;
}

void func_80039E60(s32 packed_id)
{
    if (sound_calls < 8)
        sound_ids[sound_calls] = (u32)packed_id;
    sound_calls++;
}

void wm_80089160(u32 record, u32 vector, u32 third)
{
    (void)third;
    if (presence_calls < 8) {
        presence_ids[presence_calls] = record;
        presence_xyz[presence_calls][0] = read_s16(vector + 0u);
        presence_xyz[presence_calls][1] = read_s16(vector + 2u);
        presence_xyz[presence_calls][2] = read_s16(vector + 4u);
    }
    presence_calls++;
}

void wm_800894C8(u32 record)
{
    if (clear_calls < 8)
        clear_ids[clear_calls] = record;
    clear_calls++;
}

s32 wm_80097770(u32 slot_index, s32 value)
{
    if (claim_calls < 8) {
        claim_slots[claim_calls] = slot_index;
        claim_values[claim_calls] = value;
    }
    claim_calls++;
    return 1;
}

void wm_80093354(u32 address)
{
    wrap_calls++;
    wrap_address = address;
    write32(address, read32(address) + wrap_delta);
}

MATRIX* RotMatrixYXZ(SVECTOR* rotation, MATRIX* matrix)
{
    u32 index;
    int call_index = rot_calls;

    if (call_index < 8) {
        memcpy(rot_angles[call_index], rotation,
               sizeof(rot_angles[call_index]));
        rot_destinations[call_index] = pointer_guest_address(matrix);
    }
    for (index = 0u; index < 8u; index++) {
        u32 word = rot_word(call_index, index);
        memcpy((uint8_t*)matrix + index * 4u, &word, sizeof(word));
    }
    rot_calls++;
    return matrix;
}

static void set_state(u16 state)
{
    write16(SLOT + 0x20u, state);
}

static void seed_tail(void)
{
    write32(SLOT + 0x28u, 0xFFF23000u);
    write32(SLOT + 0x2Cu, 0x00145000u);
    write32(SLOT + 0x30u, 0x01500000u);
    write32(SLOT + 0x34u, 0x44444444u);
    write32(SLOT + 0x50u, 0x000008F0u);
    write32(SLOT + 0x54u, 0x00000010u);
    write32(SLOT + 0x58u, 0x00000FF0u);
    write32(SLOT + 0x5Cu, 0x00000030u);
    write32(SLOT + 0x60u, 0x00000FD0u);
    write32(SLOT + 0x64u, 0x00000050u);
    write16(CONTEXT + 0x18u, (u16)-256);
    write16(CONTEXT + 0x1Au, 0u);
    write16(CONTEXT + 0x1Cu, (u16)-64);
}

static void check_matrix(u32 address, int call_index, const char* name)
{
    u32 index;
    int correct = 1;
    for (index = 0u; index < 8u; index++) {
        if (read32(address + index * 4u) != rot_word(call_index, index))
            correct = 0;
    }
    check(correct, name);
}

static void test_init(void)
{
    u32 index;
    seed();
    write32(RESET_POS + 0u, 0x11111111u);
    write32(RESET_POS + 4u, 0x22222222u);
    write32(RESET_POS + 8u, 0x33333333u);
    write32(RESET_POS + 0x0Cu, 0x44444444u);

    check(wm_800795E4(SLOT_INDEX) == 1, "init.return");
    check(link_calls == 13, "init.links");
    for (index = 0u; index < 13u; index++)
        check(link_sources[index] == 0 &&
              link_destinations[index] == (s32)(index + 1u),
              "init.link_order");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x28u) == 0x11111111u &&
          read32(SLOT + 0x2Cu) == 0x22222222u &&
          read32(SLOT + 0x30u) == 0x33333333u &&
          read32(SLOT + 0x34u) == 0x44444444u,
          "init.slot_position");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 64u &&
          read32(SLOT + 0x58u) == 512u &&
          read32(SLOT + 0x5Cu) == 96u &&
          read32(SLOT + 0x60u) == 768u &&
          read32(SLOT + 0x64u) == 80u,
          "init.phases");
    check(memcmp(PSX_ADDR(POSITION), PSX_ADDR(RESET_POS), 16u) == 0,
          "init.position_publication");
    check(read_s16(CONTEXT + 0x18u) == -256 &&
          read_s16(CONTEXT + 0x1Au) == 0 &&
          read_s16(CONTEXT + 0x1Cu) == -64,
          "init.context_angles");
    check(sound_calls == 1 && sound_ids[0] == 0x12340036u,
          "init.sound");
}

static void test_state0(void)
{
    seed();
    seed_tail();
    set_state(0u);
    write32(SLOT + 0x28u, 0x00123000u);
    write32(SLOT + 0x2Cu, 0xFFFE4000u);
    write32(SLOT + 0x30u, 0x00090000u);
    write32(RENDER_Z, 0x00100000u);

    (void)wm_80079778(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 1u &&
          read16(SLOT + 0x22u) == 32u &&
          read32(SLOT + 0x2Cu) == 0xFFFE8000u &&
          read32(SLOT + 0x30u) == 0x00088000u &&
          read32(RENDER_Z) == 0x000F8000u &&
          read32(SLOT + 0x3Cu) == 0xFFFFC000u,
          "state0.transition");
    check(presence_calls == 2 && presence_ids[0] == 11u &&
          presence_ids[1] == 12u &&
          presence_xyz[0][0] == 0x0123 &&
          presence_xyz[0][1] == -28 &&
          presence_xyz[0][2] == 0x0090 &&
          memcmp(presence_xyz[0], presence_xyz[1],
                 sizeof(presence_xyz[0])) == 0,
          "state0.presence_reuse");
    check(sound_calls == 1 && sound_ids[0] == 0x12340071u,
          "state0.sound");
}

static void test_state1(void)
{
    seed();
    seed_tail();
    set_state(1u);
    write16(SLOT + 0x22u, 1u);
    write32(SLOT + 0x2Cu, 0xFFFEC000u);
    write32(SLOT + 0x30u, 0x00100000u);
    write32(SLOT + 0x3Cu, 0xFFFFC000u);
    write32(RENDER_Z, 0x00200000u);

    (void)wm_80079778(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 2u &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x2Cu) == 0xFFFE8000u &&
          read32(SLOT + 0x3Cu) == 0xFFFFC100u &&
          read32(SLOT + 0x30u) == 0x000F8000u &&
          read32(RENDER_Z) == 0x001F8000u &&
          read32(SLOT + 0x40u) == 0xFFFF8000u,
          "state1.motion");
    check(read_s16(CONTEXT + 0x18u) == -252 &&
          read_s16(CONTEXT + 0x1Cu) == -62,
          "state1.angles");
    check(clear_calls == 1 && clear_ids[0] == 12u &&
          presence_calls == 3 && presence_ids[0] == 11u &&
          presence_ids[1] == 12u && presence_ids[2] == 12u &&
          memcmp(presence_xyz[0], presence_xyz[2],
                 sizeof(presence_xyz[0])) == 0,
          "state1.presence_order");
}

static void test_state2(void)
{
    seed();
    seed_tail();
    set_state(2u);
    write32(SLOT + 0x30u, 0x01588000u);
    write32(SLOT + 0x40u, 0xFFFF8000u);
    write32(RENDER_Z, 0x02000000u);

    (void)wm_80079778(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 3u &&
          read16(SLOT + 0x22u) == 8u &&
          read32(SLOT + 0x30u) == 0x01580000u &&
          read32(SLOT + 0x40u) == 0xFFFF8100u &&
          read32(RENDER_Z) == 0x01FF8000u,
          "state2.transition");
    check(read_s16(CONTEXT + 0x18u) == -258 &&
          read_s16(CONTEXT + 0x1Cu) == -65,
          "state2.angles");
    check(clear_calls == 1 && clear_ids[0] == 12u &&
          claim_calls == 1 && claim_slots[0] == 1u &&
          claim_values[0] == 1,
          "state2.claim");
}

static void test_state3(void)
{
    seed();
    seed_tail();
    set_state(3u);
    write16(SLOT + 0x22u, 1u);
    write32(SLOT + 0x30u, 0x01000000u);
    write32(SLOT + 0x40u, 0xFFFFFC00u);

    (void)wm_80079778(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 4u &&
          read16(SLOT + 0x22u) == 0u &&
          read32(SLOT + 0x30u) == 0x00FFFC00u &&
          read32(SLOT + 0x40u) == 0xFFFFFD00u &&
          claim_calls == 1 && claim_slots[0] == 1u,
          "state3.transition");
}

static void test_state4(void)
{
    u32 index;
    int all_set = 1;
    seed();
    seed_tail();
    set_state(4u);
    write16(SLOT + 4u, 1u);

    (void)wm_80079778(SLOT_INDEX);
    for (index = 0u; index < 14u; index++) {
        if (read16(CONTEXT + index * 0x54u) != 1u)
            all_set = 0;
    }
    check(read16(SLOT + 4u) == 0u && all_set, "state4.flags");
    check(clear_calls == 1 && clear_ids[0] == 11u, "state4.clear");
}

static void test_common_tail(void)
{
    seed();
    seed_tail();
    set_state(9u);
    wrap_delta = 0x00001000u;

    check(wm_80079778(SLOT_INDEX) == 1, "tail.return");
    check(wrap_calls == 1 && wrap_address == SLOT + 0x28u &&
          read32(CONTEXT + 0x08u) == 0xFFFFFF24u &&
          read32(POSITION + 0u) == 0xFFF24000u,
          "tail.wrap_order");
    check(read32(CONTEXT + 0x0Cu) == 0x00000145u &&
          read32(CONTEXT + 0x10u) == 0x00001500u &&
          read32(POSITION + 4u) == 0x00145000u &&
          read32(POSITION + 8u) == 0x01500000u &&
          read32(POSITION + 0x0Cu) == 0x44444444u,
          "tail.position");
    check(read32(SLOT + 0x50u) == 0x900u &&
          read32(SLOT + 0x58u) == 0x020u &&
          read32(SLOT + 0x60u) == 0x020u,
          "tail.phases");
    check(rot_calls == 5 &&
          rot_destinations[0] == SCRATCH_MAT_A &&
          rot_destinations[1] == SCRATCH_MAT_B &&
          rot_destinations[2] == SCRATCH_MAT_C &&
          rot_destinations[3] == SCRATCH_MAT_D &&
          rot_destinations[4] == SCRATCH_MAT_A,
          "tail.rotation_destinations");
    check(rot_angles[0][0] == 0 && rot_angles[0][1] == 0x900 &&
          rot_angles[0][2] == 0 &&
          rot_angles[1][0] == 0 && rot_angles[1][1] == 0x020 &&
          rot_angles[1][2] == 0 &&
          rot_angles[2][0] == 0 && rot_angles[2][1] == 0x020 &&
          rot_angles[2][2] == 0 &&
          rot_angles[3][0] == 0 && rot_angles[3][1] == 0 &&
          rot_angles[3][2] == 0x020 &&
          rot_angles[4][0] == -256 && rot_angles[4][1] == 0 &&
          rot_angles[4][2] == -64,
          "tail.rotation_angles");

    check_matrix(CONTEXT + 0x11Cu, 3, "tail.matrix_d_chain");
    check_matrix(CONTEXT + 0x0C8u, 3, "tail.matrix_d_chain");
    check_matrix(CONTEXT + 0x074u, 3, "tail.matrix_d_chain");
    check_matrix(CONTEXT + 0x1C4u, 2, "tail.matrix_c_chain");
    check_matrix(CONTEXT + 0x170u, 2, "tail.matrix_c_chain");
    check_matrix(CONTEXT + 0x410u, 0, "tail.matrix_a_chain");
    check_matrix(CONTEXT + 0x2C0u, 0, "tail.matrix_a_chain");
    check_matrix(CONTEXT + 0x368u, 0, "tail.matrix_a_chain");
    check_matrix(CONTEXT + 0x218u, 0, "tail.matrix_a_chain");
    check_matrix(CONTEXT + 0x464u, 1, "tail.matrix_b_chain");
    check_matrix(CONTEXT + 0x314u, 1, "tail.matrix_b_chain");
    check_matrix(CONTEXT + 0x3BCu, 1, "tail.matrix_b_chain");
    check_matrix(CONTEXT + 0x26Cu, 1, "tail.matrix_b_chain");
    check_matrix(CONTEXT + 0x020u, 4, "tail.view_matrix");
}

static void dispatch_one(u32 cb0, u32 cb1, s16 scheduler_state)
{
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(SLOT + 0x18u, cb0);
    write32(SLOT + 0x1Cu, cb1);
    write16(SLOT + 0u, (u16)scheduler_state);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.resolution");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(RESET_POS + 0u, 0x11110000u);
    write32(RESET_POS + 4u, 0x22220000u);
    write32(RESET_POS + 8u, 0x33330000u);
    dispatch_one(0x800795E4u, 0x80079778u, 0);

    seed();
    dispatch_one(0x800795E4u, 0x80079778u, 1);
}

int main(void)
{
    test_init();
    test_state0();
    test_state1();
    test_state2();
    test_state3();
    test_state4();
    test_common_tail();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N65 MODE10 SEQUENCE/CONTEXT CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N65 MODE10 SEQUENCE/CONTEXT CERTIFICATE PASS");
    return 0;
}
