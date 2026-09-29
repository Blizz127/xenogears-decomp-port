/* Focused production-linked certificate for retail wm_80085CDC. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_85cdc.h"

#define POOL       0x800D0000u
#define POOL_PTR   0x8009BE24u
#define CAMERA_POS 0x8009BE28u
#define ENV_PTR    0x8009BE3Cu
#define HEADING    0x8009BD3Au
#define CAMERA     0x8009C808u
#define ENV        0x8009BC40u
#define OT         0x800A4000u
#define SLOT(i)    (POOL + (u32)(i) * 0x80u)

static u8 s_objects[7][0xC0];
static s32 s_depths[7];
static u32 s_project_count;
static u32 s_matrix_count;
static u32 s_render_count;
static u32 s_direction_count;
static u32 s_tick_count;
static void *s_render_object[7];
static void *s_render_ot[7];
static s16 s_direction[7];
static char s_events[32];
static size_t s_event_count;
static int s_failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static void wr16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wr32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 rd32(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 native_bits(void *pointer)
{
    uintptr_t bits = (uintptr_t)pointer;
    check(bits <= UINT32_MAX, "fixture.native_pointer_fits_retail_word");
    return (u32)bits;
}

static int object_index(void *object)
{
    ptrdiff_t delta = (u8 *)object - &s_objects[0][0];
    check(delta >= 0 && delta % (ptrdiff_t)sizeof(s_objects[0]) == 0,
          "object.pointer.known");
    return (int)(delta / (ptrdiff_t)sizeof(s_objects[0]));
}

s32 wm_85cdc_test_project_depth(void *object)
{
    int index = object_index(object);
    s_project_count++;
    return s_depths[index];
}

void wm_85cdc_test_set_render_matrix(MATRIX *matrix)
{
    check(matrix == (MATRIX *)PSX_ADDR(CAMERA), "render_matrix.source");
    s_events[s_event_count++] = 'M';
    s_matrix_count++;
}

void wm_85cdc_test_render(void *object, void *ot)
{
    if (s_render_count < 7u) {
        s_render_object[s_render_count] = object;
        s_render_ot[s_render_count] = ot;
    }
    s_events[s_event_count++] = 'R';
    s_render_count++;
}

void wm_85cdc_test_set_direction(void *object, s16 direction)
{
    int index = object_index(object);
    s_direction[index] = direction;
    s_events[s_event_count++] = 'D';
    s_direction_count++;
}

void wm_85cdc_test_animation_tick(void *object)
{
    (void)object_index(object);
    s_events[s_event_count++] = 'T';
    s_tick_count++;
}

void wm_80093484(u32 vector)
{
    check(vector == 0x1F800008u, "wrap.scratch.argument");
}

void SetRotMatrix(MATRIX *matrix)
{
    check(matrix == (MATRIX *)PSX_ADDR(CAMERA), "gte.rotation.camera");
}

void SetTransMatrix(MATRIX *matrix)
{
    check(matrix == (MATRIX *)PSX_ADDR(CAMERA), "gte.translation.camera");
}

static void configure_slot(int index, s16 state, s32 depth,
                           s16 target, s32 current)
{
    u32 slot = SLOT(index);
    void *object = s_objects[index];

    s_depths[index] = depth;
    wr16(slot + 4u, 0u);
    wr16(slot + 0x24u, (u16)state);
    wr32(slot + 0x28u, (u32)(0x120 + index * 7));
    wr32(slot + 0x2Cu, (u32)(0x40 + index));
    wr32(slot + 0x30u, (u32)(0x260 + index * 11));
    wr16(slot + 0x48u, (u16)target);
    wr32(slot + 0x4Cu, state == 2 ? 0u : native_bits(object));
    wr32(slot + 0x5Cu, (u32)current);
}

int main(void)
{
    u32 i;

    PsxMemory_Init();
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(s_objects, 0xA5, sizeof(s_objects));
    memset(s_direction, 0, sizeof(s_direction));
    wr32(POOL_PTR, POOL);
    wr32(CAMERA_POS + 0u, 0x20u);
    wr32(CAMERA_POS + 8u, 0x30u);
    wr32(ENV_PTR, ENV);
    wr32(ENV + 0x70u, OT);
    wr16(HEADING, 0x80u);

    configure_slot(0, 0, 0x2A0, 0x300, 0x000);
    configure_slot(1, 0, 0x120, 0xF80, 0x080);
    configure_slot(2, 0, 0x090, 0x150, 0x100);
    configure_slot(3, 0, -16, 0x700, 0x100);
    configure_slot(4, 0, 0xB00, 0x600, 0x100);
    configure_slot(5, 1, 0x100, 0x600, 0x100);
    configure_slot(6, 2, 0x100, 0x600, 0x100);
    for (i = 7u; i < 64u; i++) {
        wr16(SLOT(i) + 4u, 1u);
        wr16(SLOT(i) + 0x24u, 1u);
    }

    wm_80085CDC();

    check(s_matrix_count == 1u, "render_matrix.once");
    check(s_project_count == 5u, "projection.active_nonnull_only");
    check(s_render_count == 4u, "render.signed_depth_gate");
    check(s_direction_count == 4u, "direction.eligible_count");
    check(s_tick_count == 4u, "animation_tick.once_per_eligible_object");
    check(s_event_count == 13u &&
              memcmp(s_events, "MRDTRDTRDTRDT", 13u) == 0,
          "order.matrix_then_render_direction_tick");
    check(s_render_object[0] == s_objects[0] &&
              s_render_object[1] == s_objects[1] &&
              s_render_object[2] == s_objects[2] &&
              s_render_object[3] == s_objects[3],
          "render.object.order");
    check(s_render_ot[0] == (u8 *)PSX_ADDR(OT) + 0xA8,
          "ot_bucket.depth_shift_then_word_scale");
    check(s_render_ot[3] == (u8 *)PSX_ADDR(OT) - 4,
          "ot_bucket.signed_negative_depth");
    check(rd32(SLOT(0) + 0x5Cu) == 0x100,
          "heading.positive_step_0x100");
    check(rd32(SLOT(1) + 0x5Cu) == -0x80,
          "heading.negative_step_0x100");
    check(rd32(SLOT(2) + 0x5Cu) == 0x150,
          "heading.snap_inside_0x100");
    check(s_direction[0] == (s16)0xC80,
          "direction.subtract_global_and_quarter_turn");
    check(rd32(SLOT(4) + 0x5Cu) == 0x100,
          "depth_ceiling.excluded");
    check(rd32(SLOT(5) + 0x5Cu) == 0x100,
          "state.excluded");
    check(rd32(SLOT(6) + 0x5Cu) == 0x100,
          "null_object.excluded");

    check(*(s32 *)&s_objects[0][0] == (0x120 - 0x20) * 16,
          "phase1.native_object_x");
    check(*(s32 *)&s_objects[0][4] == 0x40 * 16,
          "phase1.native_object_y");
    check(*(s32 *)&s_objects[0][8] == -(0x260 - 0x30) * 16,
          "phase1.native_object_z");

    if (s_failures != 0) {
        fprintf(stderr, "%d assertion(s) failed\n", s_failures);
        return 1;
    }
    puts("W34C1 animation stepper certificate PASS");
    return 0;
}
