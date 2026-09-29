/* W34N50 production-linked certificate for retail wm_80091C18. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_91c18.h"

#define POOL         UINT32_C(0x80070000)
#define SLOT         (POOL + 10u * UINT32_C(0x80))
#define ROT_TABLE    UINT32_C(0x8009B224)
#define HEIGHT_TABLE UINT32_C(0x8009B234)
#define D144         UINT32_C(0x8009D144)
#define D3F0         UINT32_C(0x8009D3F0)
#define BD38         UINT32_C(0x8009BD38)

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static s32 s_sampler_result;
static u32 s_sampler_calls;
static s32 s_sampler_max;
static u32 s_sampler_rot;
static u32 s_sampler_height;
static u32 s_matrix_calls;
static u32 s_matrix_out;
static u32 s_matrix_position;
static s32 s_matrix_height;
static u32 s_matrix_rotation;

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static s16 ld16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 ld32(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

s32 wm_80091FF8(s32 max_candidates, u32 rot_table, u32 height_table)
{
    s_sampler_calls++;
    s_sampler_max = max_candidates;
    s_sampler_rot = rot_table;
    s_sampler_height = height_table;
    return s_sampler_result;
}

void wm_80096F18(u32 output_matrix, u32 position, s32 height,
                  u32 rotation)
{
    s_matrix_calls++;
    s_matrix_out = output_matrix;
    s_matrix_position = position;
    s_matrix_height = height;
    s_matrix_rotation = rotation;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    st32(UINT32_C(0x8009BE24), POOL);
    st32(SLOT + 0x64u, ROT_TABLE);
    st32(SLOT + 0x68u, HEIGHT_TABLE);
    s_sampler_result = 0;
    s_sampler_calls = 0u;
    s_sampler_max = 0;
    s_sampler_rot = 0u;
    s_sampler_height = 0u;
    s_matrix_calls = 0u;
    s_matrix_out = 0u;
    s_matrix_position = 0u;
    s_matrix_height = 0;
    s_matrix_rotation = 0u;
}

static void check_matrix_call(s32 height)
{
    check("matrix-call-arguments",
          s_matrix_calls == 1u &&
          s_matrix_out == UINT32_C(0x8009BD40) &&
          s_matrix_position == UINT32_C(0x8009BE28) &&
          s_matrix_height == height && s_matrix_rotation == BD38);
}

static void test_reset_target(void)
{
    seed();
    st16(SLOT + 4u, 14u);
    st32(SLOT + 0x50u, 1u);
    s_sampler_result = 1;
    st32(D144, UINT32_C(0x12345678));
    st32(D3F0, UINT32_C(0x00200000));

    check("return-constant", wm_80091C18(10) == 1);
    check("reset-clears-d144-not-camera-height",
          ld32(D144) == 0 && ld32(D3F0) == 0x00200000);
    check("reset-enters-state-zero-sampler",
          s_sampler_calls == 1u && s_sampler_max == 1 &&
          s_sampler_rot == ROT_TABLE && s_sampler_height == HEIGHT_TABLE);
    check_matrix_call(0x00200000);
}

static void test_live_positive_interpolation(void)
{
    seed();
    st16(SLOT + 0x20u, 1u);
    st16(SLOT + 0x22u, 0u);
    st32(SLOT + 0x50u, 0u);
    st32(SLOT + 0x54u, UINT32_C(0x00180000));
    st32(SLOT + 0x58u, 100u);
    st32(SLOT + 0x60u, 0u);
    st32(D3F0, UINT32_C(0x00100000));
    st16(BD38, 0u);

    (void)wm_80091C18(10);

    check("positive-zoom-cap-is-0x8000",
          ld32(D3F0) == 0x00108000);
    check("zero-candidate-heading-step",
          ld32(SLOT + 0x60u) == 0x0F32 && ld16(BD38) == 0);
    check("frame-counter-increments", ld16(SLOT + 0x22u) == 1);
    check_matrix_call(0x00108000);
}

static void test_nonzero_candidate_heading_step(void)
{
    seed();
    st16(SLOT + 0x20u, 1u);
    st32(SLOT + 0x50u, 2u);
    st32(SLOT + 0x54u, UINT32_C(0x00100000));
    st32(SLOT + 0x58u, 100u);
    st32(SLOT + 0x60u, 0u);
    st32(D3F0, UINT32_C(0x00100000));

    (void)wm_80091C18(10);
    check("nonzero-candidate-heading-step",
          ld32(SLOT + 0x60u) == 0x0799);
}

static void test_state_two_skips_matrix(void)
{
    seed();
    st16(SLOT + 0x20u, 2u);
    st32(UINT32_C(0x8009BE2C), UINT32_C(0x12345000));
    st16(UINT32_C(0x8009BD48), UINT16_C(0xAAAA));
    st16(UINT32_C(0x8009BD4A), UINT16_C(0xBBBB));
    st16(UINT32_C(0x8009BD4C), UINT16_C(0xCCCC));

    (void)wm_80091C18(10);
    check("state-two-skips-matrix", s_matrix_calls == 0u);
    check("state-two-camera-position",
          ld16(UINT32_C(0x8009BD48)) == 0 &&
          ld16(UINT32_C(0x8009BD4A)) == 0x2345 &&
          ld16(UINT32_C(0x8009BD4C)) == 0);
}

static void test_periodic_candidate_refresh(void)
{
    seed();
    st16(SLOT + 0x20u, 1u);
    st16(SLOT + 0x22u, 5u);
    st32(SLOT + 0x50u, 2u);
    st32(BD38, 0u);
    st32(D3F0, UINT32_C(0x00200000));
    st32(UINT32_C(0x8009B218), UINT32_C(0x00200000));
    st16(ROT_TABLE + 2u, 0u);
    s_sampler_result = 1;

    (void)wm_80091C18(10);
    check("periodic-sampler-arguments",
          s_sampler_calls == 1u && s_sampler_max == 2 &&
          s_sampler_rot == ROT_TABLE && s_sampler_height == HEIGHT_TABLE);
    check("periodic-selection-publication",
          ld32(SLOT + 0x50u) == 1 &&
          ld32(SLOT + 0x54u) == 0x00200000 &&
          ld32(SLOT + 0x58u) == 0 && ld16(SLOT + 0x22u) == 1);
}

int main(void)
{
    test_reset_target();
    test_live_positive_interpolation();
    test_nonzero_candidate_heading_step();
    test_state_two_skips_matrix();
    test_periodic_candidate_refresh();
    if (s_failures != 0)
        return 1;
    puts("W34N50 0x80091C18 full-body certificate PASS");
    return 0;
}
