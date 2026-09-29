/* Focused production certificate for retail 0x8007EBBC/7ECA4/7EE34. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7eca4.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800A0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 3
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT    UINT32_C(0x800B0000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define RESET_POS  UINT32_C(0x8009C5AC)
#define SELECTOR   UINT32_C(0x8009D3D4)
#define TARGETS    UINT32_C(0x8009A674)
#define DESC0      UINT32_C(0x800B1000)
#define DESC1      UINT32_C(0x800B1100)
#define DESC2      UINT32_C(0x800B1200)
#define SRC0       UINT32_C(0x800B2000)
#define SRC1       UINT32_C(0x800B2200)
#define SRC2       UINT32_C(0x800B2400)
#define DST0       UINT32_C(0x800B3000)
#define DST1       UINT32_C(0x800B3200)
#define DST2       UINT32_C(0x800B3400)

static int failures;
static unsigned link_count;
static s32 link_src[4];
static s32 link_dst[4];
static unsigned rot_calls;
static unsigned mul_calls;
static unsigned scale_calls;
static unsigned wrap_calls;
static unsigned angle_calls;
static unsigned marker_count;
static u32 marker_id[16];
static u32 marker_pos[16];
static u32 marker_rot[16];
static unsigned release_count;
static u32 release_id;
static unsigned claim_count;
static u32 claim_slot[16];
static s32 claim_value[16];
static u16 mul_left_m00;
static s32 terrain_value = INT32_C(0x00123000);

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u8 read8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
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
    link_count = 0u;
    rot_calls = 0u;
    mul_calls = 0u;
    scale_calls = 0u;
    wrap_calls = 0u;
    angle_calls = 0u;
    marker_count = 0u;
    release_count = 0u;
    release_id = 0u;
    claim_count = 0u;
    mul_left_m00 = 0u;
}

static void seed_owner(u32 owner, u32 descriptor, u32 source,
                       u32 destination, u16 count, u8 seed_byte)
{
    u32 index;
    write32(owner + 0x48u, source);
    write32(owner + 0x4Cu, destination);
    write16(descriptor + 4u, count);
    for (index = 0u; index < (u32)count * 40u; index++)
        write8(source + index, (u8)(seed_byte + (u8)index));
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write32(RESET_POS + 0u, UINT32_C(0x03000000));
    write32(RESET_POS + 4u, UINT32_C(0xFFF80000));
    write32(RESET_POS + 8u, UINT32_C(0x06000000));
    write32(SELECTOR, 2u);
    write16(TARGETS + 16u, 123);
    write16(TARGETS + 20u, (u16)(s16)-456);

    write32(CONTEXT + 0x94u, DESC0);
    write32(CONTEXT + 0x9Cu, SRC0);
    write32(CONTEXT + 0xE8u, DESC1);
    write32(CONTEXT + 0xF0u, SRC1);
    write32(CONTEXT + 0x13Cu, DESC2);
    write32(CONTEXT + 0x144u, SRC2);
    seed_owner(CONTEXT + 0x54u, DESC0, SRC0, DST0, 2u, 0x10u);
    seed_owner(CONTEXT + 0xA8u, DESC1, SRC1, DST1, 1u, 0x30u);
    seed_owner(CONTEXT + 0xFCu, DESC2, SRC2, DST2, 1u, 0x50u);
    reset_trace();
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    check(tp == 0 && x == 768 && y == 256, "primitive.tpage_args");
    return (u16)(UINT32_C(0x1200) + (u32)abr);
}

u16 GetClut(int x, int y)
{
    check(x == 0 && y == 511, "primitive.clut_args");
    return UINT16_C(0x2345);
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    link_src[link_count] = source_record;
    link_dst[link_count] = destination_record;
    link_count++;
}

MATRIX *RotMatrixYXZ(SVECTOR *rotation, MATRIX *matrix)
{
    (void)rotation;
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = 4096;
    matrix->m[1][1] = 4096;
    matrix->m[2][2] = 4096;
    rot_calls++;
    return matrix;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output)
{
    (void)right;
    mul_left_m00 = (u16)left->m[0][0];
    memcpy(output, left, sizeof(*output));
    mul_calls++;
    return output;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    (void)scale;
    scale_calls++;
    return matrix;
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output)
{
    (void)left;
    (void)right;
    output->vx = 4096;
    output->vy = 0;
    output->vz = 0;
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    if (input != output)
        memcpy(output, input, sizeof(*output));
    return 0;
}

int ratan2(int y, int x)
{
    check(y == 2138 && x == 3494, "render.ratan2_args");
    return 0x123;
}

void wm_80093354(u32 address)
{
    check(address == SLOT + 0x28u, "update.wrap_arg");
    wrap_calls++;
}

void wm_80097070(u32 matrix, u32 angles)
{
    check(matrix == UINT32_C(0x1F800150) &&
          angles == UINT32_C(0x1F8000A8), "aligned.angle_args");
    write16(angles + 0u, 1u);
    write16(angles + 2u, 2u);
    write16(angles + 4u, 3u);
    angle_calls++;
}

s32 wm_80097770(u32 slot_index, s32 value)
{
    claim_slot[claim_count] = slot_index;
    claim_value[claim_count] = value;
    claim_count++;
    return 1;
}

s32 wm_80093978(s32 x, s32 z)
{
    check(x == INT32_C(0x01498000) && z == INT32_C(0x04AF2000),
          "state2.terrain_args");
    return terrain_value;
}

void wm_80089160(u32 id, u32 position, u32 rotation)
{
    marker_id[marker_count] = id;
    marker_pos[marker_count] = position;
    marker_rot[marker_count] = rotation;
    marker_count++;
}

void wm_800894C8(u32 id)
{
    release_count++;
    release_id = id;
}

static void test_primitive_helper(void)
{
    seed();
    wm_8007EBBC(CONTEXT + 0x54u, SRC0, 2u, 3u);
    check(read8(SRC0 + 3u) == 9u && read8(SRC0 + 4u) == 192u &&
          read8(SRC0 + 5u) == 60u && read8(SRC0 + 6u) == 60u &&
          read8(SRC0 + 7u) == 46u && read16(SRC0 + 14u) == 0x2345u &&
          read16(SRC0 + 22u) == 0x1203u,
          "primitive.fields");
    check(memcmp(PSX_ADDR(SRC0), PSX_ADDR(DST0), 80u) == 0,
          "primitive.copy");
}

static void test_initializer(void)
{
    u32 vx = UINT32_C(0xFFFFF7A6);
    u32 vz = UINT32_C(0x00000DA6);

    seed();
    check(wm_8007ECA4(SLOT_INDEX) == 1, "init.return");
    check(link_count == 2u && link_src[0] == 1 && link_dst[0] == 2 &&
          link_src[1] == 1 && link_dst[1] == 3, "init.context_links");
    check(read32(SLOT + 0x38u) == vx && read32(SLOT + 0x3Cu) == 0u &&
          read32(SLOT + 0x40u) == vz && read16(SLOT + 0x20u) == 0u,
          "init.velocity_state");
    check(read32(SLOT + 0x50u) == (123u << 12u) &&
          read32(SLOT + 0x54u) == ((u32)(s32)-456 << 12u),
          "init.target_table");
    check(read32(SLOT + 0x28u) ==
              read32(RESET_POS) - ((vx * 107u) << 7u) &&
          read32(SLOT + 0x2Cu) == read32(RESET_POS + 4u) &&
          read32(SLOT + 0x30u) ==
              read32(RESET_POS + 8u) - ((vz * 107u) << 7u),
          "init.position_distance");
    check(rot_calls == 1u && read16(CONTEXT + 0x54u) == 0u &&
          read16(CONTEXT + 0x6Cu) == 0u, "init.object_rotation");
}

static void prepare_update(void)
{
    seed();
    (void)wm_8007ECA4(SLOT_INDEX);
    reset_trace();
    write16(SLOT + 4u, 0u);
}

static void test_natural_update(void)
{
    u32 old_x;
    u32 old_z;

    prepare_update();
    old_x = read32(SLOT + 0x28u);
    old_z = read32(SLOT + 0x30u);
    check(wm_8007EE34(SLOT_INDEX) == 1, "update.return");
    check(read32(SLOT + 0x28u) == old_x +
              (UINT32_C(0xFFFFF7A6) << 7u) &&
          read32(SLOT + 0x30u) == old_z + (UINT32_C(0x00000DA6) << 7u),
          "update.velocity_integration");
    check(wrap_calls == 1u && marker_count == 1u && marker_id[0] == 34u &&
          marker_pos[0] == UINT32_C(0x1F8000A0) &&
          marker_rot[0] == UINT32_C(0x1F8000A8),
          "update.fixed_render");
    check(mul_calls == 1u && scale_calls == 1u && mul_left_m00 == 3494u,
          "render.fixed_matrix");
}

static void test_transitions(void)
{
    prepare_update();
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x40u, 0u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x28u, 99u);
    write32(SLOT + 0x30u, 201u);
    write32(SLOT + 0x50u, 100u);
    write32(SLOT + 0x54u, 200u);
    (void)wm_8007EE34(SLOT_INDEX);
    check(read32(SLOT + 0x28u) == 100u && read32(SLOT + 0x30u) == 200u &&
          claim_count == 5u && claim_slot[0] == 4u && claim_slot[4] == 8u &&
          claim_value[0] == 2 && claim_value[4] == 2,
          "state0.claim_range");

    prepare_update();
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x40u, 0u);
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x28u, UINT32_C(0x01F9E000));
    write32(SLOT + 0x30u, UINT32_C(0x05998001));
    (void)wm_8007EE34(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 1u && marker_count == 1u &&
          marker_id[0] == 34u, "state1.strict_limit");

    prepare_update();
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x40u, 0u);
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x28u, UINT32_C(0x01F9DFFF));
    write32(SLOT + 0x30u, UINT32_C(0x05998001));
    (void)wm_8007EE34(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 64u &&
          read32(SLOT + 0x28u) == UINT32_C(0x01F9E000) &&
          marker_count == 1u && marker_id[0] == 32u,
          "state1.transition");
}

static void test_ballistic_and_release(void)
{
    prepare_update();
    write16(SLOT + 4u, 5u);
    write32(SLOT + 0x2Cu, UINT32_C(0x00090000));
    (void)wm_8007EE34(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 3u && marker_count == 4u &&
          marker_id[0] == 37u && marker_id[1] == 38u &&
          marker_id[2] == 39u && marker_id[3] == 34u &&
          angle_calls == 1u, "state2.height_transition");

    prepare_update();
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x3Cu, 0u);
    write32(SLOT + 0x40u, 0u);
    write16(SLOT + 0x20u, 65u);
    check(wm_8007EE34(SLOT_INDEX) == 3 && release_count == 1u &&
          release_id == 34u, "state65.release_return");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007ECA4));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007EE34));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 && read16(SLOT) == 1u,
          "scheduler.init_resolution");
    write16(SLOT + 4u, 0u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_primitive_helper();
    test_initializer();
    test_natural_update();
    test_transitions();
    test_ballistic_and_release();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N100 MODE15 MOVING OBJECT CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N100 MODE15 MOVING OBJECT CERTIFICATE PASS");
    return 0;
}
