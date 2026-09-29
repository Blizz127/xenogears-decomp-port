/* Focused production certificate for retail 0x8007C724/0x8007C7D8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7c724.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define CAMERA_GATE   UINT32_C(0x8009D144)
#define RESET_X       UINT32_C(0x8009C5AC)
#define RESET_Y       UINT32_C(0x8009C5B0)
#define RESET_Z       UINT32_C(0x8009C5B4)
#define LIVE_X        UINT32_C(0x8009BE28)
#define LIVE_Y        UINT32_C(0x8009BE2C)
#define LIVE_Z        UINT32_C(0x8009BE30)
#define SHADOW_X      UINT32_C(0x8009D55C)
#define SHADOW_Y      UINT32_C(0x8009D560)
#define SHADOW_Z      UINT32_C(0x8009D564)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define CAMERA_MATRIX UINT32_C(0x8009C808)
#define WORLD_Z_ACCUM UINT32_C(0x8009BBBC)
#define PATH_NORMAL   UINT32_C(0x8009A4F8)
#define PATH_FINAL    UINT32_C(0x8009A568)
#define SCRATCH       UINT32_C(0x1F800000)
#define MARKER_VEC    UINT32_C(0x1F8000A0)

typedef struct BlendCall {
    s32 parameter;
    u32 a;
    u32 b;
    u32 c;
    u32 output;
} BlendCall;

static int failures;
static int wrap_calls;
static int generic_calls;
static u32 generic_out;
static u32 generic_pos;
static s32 generic_height;
static u32 generic_angles;
static int blend_calls;
static BlendCall last_blend;
static u32 blend_word0;
static u32 blend_word1;
static u32 blend_word2;
static int basis_calls;
static u32 basis_arg;
static uint8_t basis_snapshot[32];
static int euler_calls;
static u32 euler_matrix;
static u32 euler_angles;
static u32 marker_ids[8];
static unsigned marker_count;
static u32 destroy_ids[8];
static unsigned destroy_count;
static int random_value;

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
    wrap_calls = 0;
    generic_calls = 0;
    generic_out = 0u;
    generic_pos = 0u;
    generic_height = 0;
    generic_angles = 0u;
    blend_calls = 0;
    memset(&last_blend, 0, sizeof(last_blend));
    basis_calls = 0;
    basis_arg = 0u;
    memset(basis_snapshot, 0, sizeof(basis_snapshot));
    euler_calls = 0;
    euler_matrix = 0u;
    euler_angles = 0u;
    marker_count = 0u;
    destroy_count = 0u;
    memset(marker_ids, 0, sizeof(marker_ids));
    memset(destroy_ids, 0, sizeof(destroy_ids));
    blend_word0 = UINT32_C(0x12340000);
    blend_word1 = UINT32_C(0xFFFE0000);
    blend_word2 = UINT32_C(0x00030000);
    random_value = 0;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(RESET_X, UINT32_C(0x00110000));
    write32(RESET_Y, UINT32_C(0x00220000));
    write32(RESET_Z, UINT32_C(0x00330000));
    write32(SLOT + 0x7Cu, 4096u);
    write32(SLOT + 0x58u, 64u);
    write32(SLOT + 0x60u, 1u);
    write32(LIVE_Y, UINT32_C(0x00123000));
    write32(LIVE_Z, UINT32_C(0x00200000));
    write32(SHADOW_Z, UINT32_C(0x00201000));
    write32(CAMERA_GATE, 1u);
    reset_trace();
}

int rand(void)
{
    return random_value;
}

void wm_80093484(u32 vector)
{
    check(vector == SCRATCH, "trace.wrap_arg");
    wrap_calls++;
}

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 angles)
{
    generic_calls++;
    generic_out = out_matrix;
    generic_pos = pos_vec;
    generic_height = height;
    generic_angles = angles;
}

void wm_80076858(s32 parameter, u32 vector_a, u32 vector_b,
                 u32 vector_c, u32 output)
{
    blend_calls++;
    last_blend.parameter = parameter;
    last_blend.a = vector_a;
    last_blend.b = vector_b;
    last_blend.c = vector_c;
    last_blend.output = output;
    write32(output + 0u, blend_word0);
    write32(output + 4u, blend_word1);
    write32(output + 8u, blend_word2);
}

void wm_80097244(u32 input)
{
    basis_calls++;
    basis_arg = input;
    memcpy(basis_snapshot, PSX_ADDR(input), sizeof(basis_snapshot));
}

void wm_80097070(u32 matrix, u32 angles)
{
    euler_calls++;
    euler_matrix = matrix;
    euler_angles = angles;
}

void wm_80089160(u32 id, u32 vector, u32 flags)
{
    check(marker_count < 8u, "trace.marker_capacity");
    check(vector == MARKER_VEC && flags == 0u, "trace.marker_args");
    if (marker_count < 8u)
        marker_ids[marker_count++] = id;
}

void wm_800894C8(u32 id)
{
    check(destroy_count < 8u, "trace.destroy_capacity");
    if (destroy_count < 8u)
        destroy_ids[destroy_count++] = id;
}

static void set_sentinel(u32 table, u32 phase)
{
    u32 record = table + ((phase >> 12u) * 8u);
    write16(record + 0x16u, UINT16_C(0xFFFF));
}

static void test_initializer(void)
{
    seed();
    write32(VIEW_HEIGHT, UINT32_C(0xDEADBEEF));
    write32(CAMERA_HEIGHT, UINT32_C(0xDEADBEEF));
    write32(CAMERA_GATE, 0u);
    write16(ANGLES + 0u, UINT16_C(0x1111));
    write16(ANGLES + 2u, UINT16_C(0x2222));
    write16(ANGLES + 4u, UINT16_C(0x3333));
    write32(SLOT + 0x50u, UINT32_C(0xDEADBEEF));
    write32(SLOT + 0x58u, UINT32_C(0xDEADBEEF));
    write32(SLOT + 0x7Cu, UINT32_C(0xDEADBEEF));

    check(wm_8007C724(SLOT_INDEX) == 1, "init.return");
    check(read32(VIEW_HEIGHT) == 120u &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x00400000) &&
          read32(CAMERA_GATE) == 1u,
          "init.camera_constants");
    check(read16(ANGLES + 0u) == UINT16_C(0xFFC0) &&
          read16(ANGLES + 2u) == 0u && read16(ANGLES + 4u) == 0u,
          "init.angles");
    check(read32(LIVE_X) == read32(RESET_X) &&
          read32(LIVE_Y) == read32(RESET_Y) &&
          read32(LIVE_Z) == read32(RESET_Z) &&
          read32(SHADOW_X) == read32(RESET_X) &&
          read32(SHADOW_Y) == read32(RESET_Y) &&
          read32(SHADOW_Z) == read32(RESET_Z),
          "init.positions");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x58u) == 64u &&
          read32(SLOT + 0x7Cu) == 4096u,
          "init.slot_motion");
}

static void test_latches(void)
{
    seed();
    write16(SLOT + 0x04u, 1u);
    write16(SLOT + 0x20u, 9u);
    set_sentinel(PATH_NORMAL, 64u);
    check(wm_8007C7D8(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x50u) == 64u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x8200) &&
          read32(SLOT + 0x60u) == 1u,
          "latch.one");

    seed();
    write16(SLOT + 0x04u, 2u);
    set_sentinel(PATH_NORMAL, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x7Cu) == UINT32_C(0xF000),
          "latch.two_state3");

    seed();
    write16(SLOT + 0x04u, 4u);
    set_sentinel(PATH_NORMAL, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x58u) == 256u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0xA800) &&
          read32(SLOT + 0x60u) == 8u,
          "latch.four_state5");

    seed();
    write16(SLOT + 0x04u, 5u);
    write16(SLOT + 0x20u, 9u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read16(SLOT + 0x04u) == 5u &&
          read16(SLOT + 0x20u) == 9u,
          "latch.five_noop");

    seed();
    write16(SLOT + 0x04u, 6u);
    set_sentinel(PATH_FINAL, 64u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 6u &&
          read32(SLOT + 0x50u) == 64u &&
          read32(SLOT + 0x58u) == 64u,
          "latch.six");
}

static void test_state_boundaries(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, UINT32_C(0x81C0));
    write32(SLOT + 0x58u, 64u);
    write32(SLOT + 0x5Cu, UINT32_C(0x8200));
    set_sentinel(PATH_NORMAL, UINT32_C(0x8200));
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read32(SLOT + 0x50u) == UINT32_C(0x8200) &&
          read16(SLOT + 0x20u) == 1u,
          "state1.strict_threshold");

    seed();
    write16(SLOT + 0x20u, 2u);
    write32(SLOT + 0x50u, 100u);
    write32(SLOT + 0x58u, 3u);
    write32(SLOT + 0x60u, 4u);
    write32(SLOT + 0x5Cu, 1000u);
    set_sentinel(PATH_NORMAL, 100u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read32(SLOT + 0x58u) == 0u &&
          read32(SLOT + 0x50u) == 100u &&
          read16(SLOT + 0x20u) == 0u,
          "state2.negative_clamp");

    seed();
    write16(SLOT + 0x20u, 5u);
    set_sentinel(PATH_NORMAL, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read32(SLOT + 0x58u) == 256u &&
          read16(SLOT + 0x20u) == 1u,
          "state5.enters_state1");
}

static void test_state_four(void)
{
    seed();
    write16(SLOT + 0x20u, 4u);
    write32(SLOT + 0x7Cu, 4200u);
    write32(LIVE_X, UINT32_C(0x00123000));
    write32(LIVE_Z, UINT32_C(0x00456000));
    set_sentinel(PATH_NORMAL, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(marker_count == 2u && marker_ids[0] == 26u &&
          marker_ids[1] == 27u &&
          read16(MARKER_VEC + 0u) == UINT16_C(0x0123) &&
          read16(MARKER_VEC + 4u) == UINT16_C(0x0456),
          "state4.markers");
    check(read32(SLOT + 0x7Cu) == 4096u &&
          read16(SLOT + 0x20u) == 0u && destroy_count == 2u &&
          destroy_ids[0] == 26u && destroy_ids[1] == 27u,
          "state4.clamp_destroy");
}

static void test_state_six(void)
{
    u32 record = PATH_FINAL + 4u * 8u;

    seed();
    write16(SLOT + 0x20u, 6u);
    write32(SLOT + 0x50u, UINT32_C(0x4800));
    write32(SLOT + 0x58u, 1u);
    write32(SHADOW_X, UINT32_C(0x11110000));
    write32(SHADOW_Y, UINT32_C(0x22220000));
    write32(SHADOW_Z, UINT32_C(0x00300000));
    write32(LIVE_X, UINT32_C(0xAAAA0000));
    write32(LIVE_Y, UINT32_C(0xBBBB0000));
    write32(LIVE_Z, UINT32_C(0x002FF000));
    write32(WORLD_Z_ACCUM, 10u);
    write16(record + 0x16u, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read32(LIVE_X) == UINT32_C(0x11110000) &&
          read32(LIVE_Y) == UINT32_C(0x22220000) &&
          read32(LIVE_Z) == UINT32_C(0x00300000) &&
          read32(WORLD_Z_ACCUM) == UINT32_C(0x100A),
          "state6.shadow_restore");
    check(read32(SLOT + 0x50u) == UINT32_C(0x4801) &&
          read32(SLOT + 0x58u) == 0u,
          "state6.deceleration");
    check(blend_calls == 1 && last_blend.a == record &&
          last_blend.b == record + 8u && last_blend.c == record + 16u,
          "state6.final_table");
}

static void test_generic_camera_gate(void)
{
    seed();
    write32(CAMERA_GATE, 0u);
    write32(CAMERA_HEIGHT, UINT32_C(0xFFC00000));
    write16(SLOT + 0x20u, 9u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(generic_calls == 1 && generic_out == CAMERA_INPUT &&
          generic_pos == LIVE_X &&
          (u32)generic_height == UINT32_C(0xFFC00000) &&
          generic_angles == ANGLES,
          "camera.generic_gate");
    check(blend_calls == 0 && basis_calls == 0 && euler_calls == 0,
          "camera.invalid_state_skips_path");
}

static void test_camera_blend_and_sentinel(void)
{
    u32 record = PATH_NORMAL + 8u;
    u16 value;

    seed();
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, UINT32_C(0x1A34));
    write16(record + 0x16u, 0u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(blend_calls == 1 && last_blend.parameter == 0xA34 &&
          last_blend.a == record && last_blend.b == record + 8u &&
          last_blend.c == record + 16u && last_blend.output == SCRATCH,
          "camera.blend_args");
    memcpy(&value, basis_snapshot + 0x00u, sizeof(value));
    check(basis_calls == 1 && basis_arg == CAMERA_INPUT &&
          value == UINT16_C(0x1234),
          "camera.input_x");
    memcpy(&value, basis_snapshot + 0x02u, sizeof(value));
    check(value == UINT16_C(0xFFFE), "camera.input_y");
    memcpy(&value, basis_snapshot + 0x04u, sizeof(value));
    check(value == UINT16_C(0xFFFD), "camera.input_z");
    memcpy(&value, basis_snapshot + 0x0Au, sizeof(value));
    check(value == UINT16_C(0x0123), "camera.input_live_y");
    check(euler_calls == 1 && euler_matrix == CAMERA_MATRIX &&
          euler_angles == ANGLES,
          "camera.helper_order_outputs");

    seed();
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, UINT32_C(0x1A34));
    write16(record + 0x16u, UINT16_C(0xFFFF));
    memset(PSX_ADDR(CAMERA_INPUT), 0x6B, 32u);
    (void)wm_8007C7D8(SLOT_INDEX);
    check(blend_calls == 0 && basis_calls == 1 && euler_calls == 1 &&
          basis_snapshot[0] == UINT8_C(0x6B) &&
          basis_snapshot[31] == UINT8_C(0x6B),
          "camera.sentinel_preserves_input");
}

static void test_jitter(void)
{
    seed();
    write16(SLOT + 0x20u, 9u);
    write32(SLOT + 0x7Cu, UINT32_C(0xF000));
    write16(CAMERA_INPUT + 0x02u, 100u);
    write16(CAMERA_INPUT + 0x0Au, 200u);
    random_value = 10;
    (void)wm_8007C7D8(SLOT_INDEX);
    check(read16(MARKER_VEC + 2u) == 3u &&
          read16(CAMERA_INPUT + 0x02u) == 103u &&
          read16(CAMERA_INPUT + 0x0Au) == 203u,
          "jitter.mirrored_axes");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007C724));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007C7D8));
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
    write16(SLOT + 0x20u, 9u);
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
    test_latches();
    test_state_boundaries();
    test_state_four();
    test_state_six();
    test_generic_camera_gate();
    test_camera_blend_and_sentinel();
    test_jitter();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N86 MODE12 CAMERA CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N86 MODE12 CAMERA CONTROL CERTIFICATE PASS");
    return 0;
}
