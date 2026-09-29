/* Focused production certificate for retail 0x8007BB60/0x8007BBEC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7bb60.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007BB60(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       UINT32_C(0x800A0000)
#define POOL_PTR   UINT32_C(0x8009BE24)
#define SLOT_INDEX 6
#define SLOT       (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT    UINT32_C(0x800A8000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define PATH       UINT32_C(0x8009A490)
#define SC_FIRST   UINT32_C(0x1F800000)
#define SC_SECOND  UINT32_C(0x1F800010)
#define SC_UP      UINT32_C(0x1F800020)
#define SC_CROSS   UINT32_C(0x1F800030)
#define SC_MARKER  UINT32_C(0x1F8000A0)
#define SC_ANGLES  UINT32_C(0x1F8000A8)
#define SC_MATRIX  UINT32_C(0x1F8000F0)

typedef struct BlendCall {
    s32 parameter;
    u32 a;
    u32 b;
    u32 c;
    u32 output;
} BlendCall;

static int failures;
static int link_count;
static s32 link_source[4];
static s32 link_destination[4];
static int rot_count;
static const SVECTOR *rot_input[4];
static MATRIX *rot_output[4];
static BlendCall blend_calls[4];
static int blend_count;
static int normal_count;
static int outer_count;
static int apply_count;
static int angle_calls;
static u32 angle_matrix;
static u32 angle_output;
static int marker_calls;
static u32 marker_id;
static u32 marker_vector;
static u32 marker_angles;
static s16 first_y;

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

static void write_vector(u32 address, s32 x, s32 y, s32 z)
{
    write32(address + 0u, (u32)x);
    write32(address + 4u, (u32)y);
    write32(address + 8u, (u32)z);
}

static void write_blend_output(u32 output)
{
    if (output == SC_FIRST) {
        write32(output + 0u, UINT32_C(0x00120000));
        write32(output + 4u, (u32)(u16)first_y << 16u);
        write32(output + 8u, UINT32_C(0x00340000));
    } else {
        write32(output + 0u, UINT32_C(0x00220000));
        write32(output + 4u,
                ((u32)(u16)first_y << 16u) + UINT32_C(0x00100000));
        write32(output + 8u, UINT32_C(0x00440000));
    }
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    first_y = -127;
    link_count = 0;
    rot_count = 0;
    blend_count = 0;
    normal_count = 0;
    outer_count = 0;
    apply_count = 0;
    angle_calls = 0;
    angle_matrix = 0u;
    angle_output = 0u;
    marker_calls = 0;
    marker_id = 0u;
    marker_vector = 0u;
    marker_angles = 0u;
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    check(link_count < 4, "trace.link_capacity");
    if (link_count < 4) {
        link_source[link_count] = source_record;
        link_destination[link_count] = destination_record;
        link_count++;
    }
}

MATRIX *RotMatrixYXZ(SVECTOR *rotation, MATRIX *matrix)
{
    check(rot_count < 4, "trace.rot_capacity");
    if (rot_count < 4) {
        rot_input[rot_count] = rotation;
        rot_output[rot_count] = matrix;
        rot_count++;
    }
    memset(matrix, 0x55, sizeof(*matrix));
    return matrix;
}

void wm_80076858(s32 parameter, u32 vector_a, u32 vector_b,
                 u32 vector_c, u32 output)
{
    check(blend_count < 4, "trace.blend_capacity");
    if (blend_count < 4) {
        blend_calls[blend_count].parameter = parameter;
        blend_calls[blend_count].a = vector_a;
        blend_calls[blend_count].b = vector_b;
        blend_calls[blend_count].c = vector_c;
        blend_calls[blend_count].output = output;
        blend_count++;
    }
    write_blend_output(output);
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    (void)input;
    normal_count++;
    if (normal_count == 1)
        write_vector(SC_FIRST, 101, 102, 103);
    else if (normal_count == 2)
        write_vector(SC_UP, 201, 202, 203);
    else
        write_vector(SC_SECOND, 301, 302, 303);
    (void)output;
    return 1;
}

int ratan2(int y, int x)
{
    check(y == 101 && x == 103, "basis.ratan_arguments");
    return 0x1234;
}

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output)
{
    apply_count++;
    check(matrix == (MATRIX *)PSX_ADDR(SC_MATRIX) &&
          input == (SVECTOR *)PSX_ADDR(SC_MARKER) &&
          output == (VECTOR *)PSX_ADDR(SC_SECOND),
          "basis.apply_addresses");
    check(input->vx == 0 && input->vy == -4096 && input->vz == 0,
          "basis.apply_down_vector");
    write_vector(SC_SECOND, 401, 402, 403);
    return output;
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output)
{
    (void)left;
    (void)right;
    outer_count++;
    write_vector(SC_CROSS, 501 + outer_count, 502 + outer_count,
                 503 + outer_count);
    (void)output;
}

void wm_80097070(u32 matrix, u32 angles)
{
    angle_calls++;
    angle_matrix = matrix;
    angle_output = angles;
    write16(angles + 0u, UINT16_C(0x0111));
    write16(angles + 2u, UINT16_C(0x0222));
    write16(angles + 4u, UINT16_C(0x0333));
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    marker_calls++;
    marker_id = a0;
    marker_vector = a1;
    marker_angles = a2;
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
}

static void test_initializer(void)
{
    seed();
    write32(SLOT + 0x50u, UINT32_C(0xAAAAAAAA));
    write32(SLOT + 0x54u, UINT32_C(0xBBBBBBBB));
    write32(SLOT + 0x58u, UINT32_C(0xCCCCCCCC));
    memset(PSX_ADDR(CONTEXT + 0x18u), 0x7E, 0x28u);

    check(func_8007BB60(SLOT_INDEX) == 3, "init.return");
    /* func_800848B4 stores record src's guest address at record dst +0x50 */
    check(read32(CONTEXT + 1u * 0x54u + 0x50u) == CONTEXT + 0u * 0x54u &&
          read32(CONTEXT + 2u * 0x54u + 0x50u) == CONTEXT + 0u * 0x54u &&
          read32(CONTEXT + 3u * 0x54u + 0x50u) == CONTEXT + 0u * 0x54u,
          "init.links");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 256u &&
          read32(SLOT + 0x58u) == 256u,
          "init.motion_seeds");
    check(read16(CONTEXT + 0x18u) == 0u &&
          read16(CONTEXT + 0x1Au) == 0u &&
          read16(CONTEXT + 0x1Cu) == 0u,
          "init.zero_angles");
    check(rot_count == 1 &&
          rot_input[0] == (SVECTOR *)PSX_ADDR(CONTEXT + 0x18u) &&
          rot_output[0] == (MATRIX *)PSX_ADDR(CONTEXT + 0x20u),
          "init.rotation_destination");
}

static void test_motion_families(void)
{
    seed();
    first_y = -128;
    write32(SLOT + 0x50u, 0u);
    write32(SLOT + 0x54u, 256u);
    write32(SLOT + 0x58u, 256u);
    check(wm_8007BBEC(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == 256u &&
          read32(SLOT + 0x58u) == 256u &&
          read32(SLOT + 0x54u) == 260u,
          "motion.acceleration_family");

    seed();
    first_y = -128;
    write32(SLOT + 0x50u, UINT32_C(2) << 12u);
    write32(SLOT + 0x54u, 100u);
    write32(SLOT + 0x58u, 132u);
    check(wm_8007BBEC(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == (UINT32_C(2) << 12u) + 132u &&
          read32(SLOT + 0x58u) == 128u &&
          read32(SLOT + 0x54u) == 104u,
          "motion.low_clamp");

    seed();
    first_y = -128;
    write32(SLOT + 0x50u, UINT32_C(5) << 12u);
    write32(SLOT + 0x54u, 100u);
    write32(SLOT + 0x58u, 252u);
    check(wm_8007BBEC(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == (UINT32_C(5) << 12u) + 252u &&
          read32(SLOT + 0x58u) == 256u &&
          read32(SLOT + 0x54u) == 84u,
          "motion.high_clamp");

    seed();
    first_y = -128;
    write32(SLOT + 0x50u, UINT32_C(8) << 12u);
    write32(SLOT + 0x54u, 77u);
    write32(SLOT + 0x58u, 99u);
    check(wm_8007BBEC(SLOT_INDEX) == 3 &&
          read32(SLOT + 0x50u) == (UINT32_C(8) << 12u) &&
          read32(SLOT + 0x54u) == 77u &&
          read32(SLOT + 0x58u) == 99u,
          "motion.segment8_return");
}

static void test_sentinel(void)
{
    seed();
    first_y = -128;
    write32(SLOT + 0x58u, 0u);
    write16(PATH + 0x16u, UINT16_C(0xFFFF));
    write32(SC_FIRST + 0u, UINT32_C(0x00120000));
    write32(SC_FIRST + 4u, UINT32_C(0xFF800000));
    write32(SC_FIRST + 8u, UINT32_C(0x00340000));

    check(wm_8007BBEC(SLOT_INDEX) == 1 && blend_count == 1 &&
          blend_calls[0].parameter == 128 &&
          blend_calls[0].output == SC_SECOND,
          "sample.sentinel_skip");
    check(read32(CONTEXT + 0x08u) == 18u &&
          read32(CONTEXT + 0x0Cu) == UINT32_C(0xFFFFFF80) &&
          read32(CONTEXT + 0x10u) == 52u,
          "sample.sentinel_preserves_first");
}

static void check_matrix(void)
{
    static const u16 expected[9] = {
        201u, 301u, 101u,
        202u, 302u, 102u,
        203u, 303u, 103u
    };
    static const u32 offsets[9] = {
        0u, 2u, 4u, 6u, 8u, 10u, 12u, 14u, 16u
    };
    unsigned i;

    for (i = 0u; i < 9u; i++)
        if (read16(CONTEXT + 0x20u + offsets[i]) != expected[i]) {
            check(0, "matrix.transpose");
            return;
        }
    check(1, "matrix.transpose");
}

static void test_full_path_and_marker(void)
{
    seed();
    write32(SLOT + 0x58u, 0u);
    write32(SLOT + 0x54u, 256u);

    check(wm_8007BBEC(SLOT_INDEX) == 1, "path.return");
    check(blend_count == 2 &&
          blend_calls[0].parameter == 0 &&
          blend_calls[0].a == PATH &&
          blend_calls[0].b == PATH + 8u &&
          blend_calls[0].c == PATH + 16u &&
          blend_calls[0].output == SC_FIRST &&
          blend_calls[1].parameter == 128 &&
          blend_calls[1].output == SC_SECOND,
          "path.blend_arguments");
    check(read32(CONTEXT + 0x08u) == 18u &&
          read32(CONTEXT + 0x0Cu) == UINT32_C(0xFFFFFF81) &&
          read32(CONTEXT + 0x10u) == 52u,
          "path.context_position");
    check(normal_count == 3 && outer_count == 2 && apply_count == 1,
          "basis.call_sequence");
    check(rot_count == 1 &&
          rot_input[0] == (SVECTOR *)PSX_ADDR(SC_MARKER) &&
          rot_output[0] == (MATRIX *)PSX_ADDR(SC_MATRIX) &&
          read_s16(SC_MARKER + 0u) == 18 &&
          read_s16(SC_MARKER + 2u) == -127 &&
          read_s16(SC_MARKER + 4u) == 52,
          "basis.rotation_and_marker_vector");
    check_matrix();
    check(angle_calls == 1 && angle_matrix == SC_MATRIX &&
          angle_output == SC_ANGLES,
          "angles.arguments");
    check(marker_calls == 1 && marker_id == 18u &&
          marker_vector == SC_MARKER && marker_angles == SC_ANGLES &&
          read16(SC_ANGLES + 0u) == UINT16_C(0x0111) &&
          read16(SC_ANGLES + 2u) == UINT16_C(0x0222) &&
          read16(SC_ANGLES + 4u) == UINT16_C(0xFCCD),
          "marker.visibility");

    seed();
    first_y = -128;
    write32(SLOT + 0x58u, 0u);
    (void)wm_8007BBEC(SLOT_INDEX);
    check(marker_calls == 0, "marker.hidden_below_threshold");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007BB60));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007BBEC));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    write16(SLOT + 0u, 1u);
    write32(SLOT + 0x50u, UINT32_C(8) << 12u);
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
    test_motion_families();
    test_sentinel();
    test_full_path_and_marker();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N72 MODE14 PATH CAMERA CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N72 MODE14 PATH CAMERA CERTIFICATE PASS");
    return 0;
}
