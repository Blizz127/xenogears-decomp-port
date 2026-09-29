/* W34N50 production-linked certificate for retail wm_80091FF8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_91ff8.h"

#define SCRATCH       UINT32_C(0x1F800000)
#define ROT_TABLE     UINT32_C(0x80010000)
#define HEIGHT_TABLE  UINT32_C(0x80011000)
#define SAMPLE_BASE   UINT32_C(0x80060000)
#define BD40           UINT32_C(0x8009BD40)
#define BE28           UINT32_C(0x8009BE28)
#define BE30           UINT32_C(0x8009BE30)
#define B214           UINT32_C(0x8009B214)
#define B234           UINT32_C(0x8009B234)

enum SampleMode {
    SAMPLE_NEGATIVE_CANDIDATES,
    SAMPLE_POSITIVE_ONLY
};

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static enum SampleMode s_mode;
static u32 s_view_calls;
static u32 s_sample_calls;
static u32 s_transform_calls;
static u32 s_view_out[3];
static u32 s_view_pos[3];
static s32 s_view_height[3];
static u32 s_view_rot[3];
static u16 s_view_angle[3];
static s32 s_sample_x[108];
static s32 s_sample_z[108];

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static u16 ld16(u32 address)
{
    u16 value;
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

void wm_80096F18(u32 output_matrix, u32 position, s32 height,
                  u32 rotation)
{
    u32 index = s_view_calls;

    if (index < 3u) {
        s_view_out[index] = output_matrix;
        s_view_pos[index] = position;
        s_view_height[index] = height;
        s_view_rot[index] = rotation;
        s_view_angle[index] = ld16(rotation);
    }
    s_view_calls++;
    st16(output_matrix, UINT16_C(0x0120));
    st16(output_matrix + 4u, (u16)(s16)-0x140);
}

void wm_80093354(u32 position)
{
    check("terrain-transform-uses-scratch", position == SCRATCH);
    s_transform_calls++;
}

u32 wm_80093660(s32 x, s32 z)
{
    u32 index = s_sample_calls;
    u32 candidate = index / 36u;
    u32 within = index % 36u;
    u32 result = SAMPLE_BASE + index * 4u;
    s8 height = 5;

    if (index < 108u) {
        s_sample_x[index] = x;
        s_sample_z[index] = z;
    }
    if (s_mode == SAMPLE_NEGATIVE_CANDIDATES && within == 7u)
        height = (s8)(-10 * (s32)(candidate + 1u));
    *(s8 *)PSX_ADDR(result) = height;
    s_sample_calls++;
    return result;
}

static void seed(enum SampleMode mode)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_view_out, 0, sizeof(s_view_out));
    memset(s_view_pos, 0, sizeof(s_view_pos));
    memset(s_view_height, 0, sizeof(s_view_height));
    memset(s_view_rot, 0, sizeof(s_view_rot));
    memset(s_view_angle, 0, sizeof(s_view_angle));
    memset(s_sample_x, 0, sizeof(s_sample_x));
    memset(s_sample_z, 0, sizeof(s_sample_z));
    s_mode = mode;
    s_view_calls = 0u;
    s_sample_calls = 0u;
    s_transform_calls = 0u;

    st16(UINT32_C(0x8009BD3A), UINT16_C(0x0456));
    st16(UINT32_C(0x8009BD3C), UINT16_C(0x0789));
    st32(UINT32_C(0x8009D560), UINT32_C(0x00020000));
    st32(UINT32_C(0x8009BCDC), UINT32_C(0xFFFFFFFD));
    st32(BE28, UINT32_C(0x01234567));
    st32(BE30, UINT32_C(0x07654321));
    for (i = 0u; i < 3u; i++) {
        st16(ROT_TABLE + i * 2u, (u16)(UINT16_C(0x0111) + i * 0x111u));
        st16(HEIGHT_TABLE + i * 2u, 0u);
        st32(B214 + i * 4u, UINT32_C(0x00100000) * (i + 1u));
    }
    for (i = 0u; i < 8u; i++)
        st16(B234 + i * 2u, UINT16_C(0x2000));
}

static void test_three_candidates_and_grid(void)
{
    const s32 expected_height[3] = {0x00101000, 0x00201000, 0x00301000};
    const u16 expected_angle[3] = {0x0111u, 0x0222u, 0x0333u};
    const u32 x_start = UINT32_C(0x01180000);
    const u32 z_start = UINT32_C(0x07600000);
    int x_ok = 1;
    int z_ok = 1;
    u32 index;
    s32 result;

    seed(SAMPLE_NEGATIVE_CANDIDATES);
    result = wm_80091FF8(5, ROT_TABLE, HEIGHT_TABLE);

    check("three-candidate-loop", result == 3 && s_view_calls == 3u &&
          s_sample_calls == 108u && s_transform_calls == 108u);
    for (index = 0u; index < 3u; index++) {
        check("view-call-arguments",
              s_view_out[index] == BD40 && s_view_pos[index] == BE28 &&
              s_view_height[index] == expected_height[index] &&
              s_view_rot[index] == SCRATCH + 0xA8u &&
              s_view_angle[index] == expected_angle[index]);
    }
    for (index = 0u; index < 108u; index++) {
        u32 within = index % 36u;
        u32 row = within / 6u;
        u32 column = within % 6u;
        s32 expected_x = (s32)(x_start + column * UINT32_C(0x80000));
        s32 expected_z = (s32)(z_start + row * UINT32_C(0x80000));

        x_ok = x_ok && s_sample_x[index] == expected_x;
        z_ok = z_ok && s_sample_z[index] == expected_z;
    }
    check("grid-x-uses-world-base", x_ok);
    check("grid-z-subtracts-view-offset", z_ok);
    check("heading-direction-scratch",
          ld16(SCRATCH + 0xAAu) == UINT16_C(0x0456) &&
          ld16(SCRATCH + 0xACu) == UINT16_C(0x0789));
}

static void test_zero_floor_and_threshold_break(void)
{
    s32 result;

    seed(SAMPLE_POSITIVE_ONLY);
    st16(HEIGHT_TABLE, (u16)(s16)-112); /* threshold = 0 */
    st16(HEIGHT_TABLE + 2u, (u16)(s16)-113); /* threshold = -1 */
    result = wm_80091FF8(5, ROT_TABLE, HEIGHT_TABLE);

    check("zero-floor-minimum",
          result == 1 && s_view_calls == 2u && s_sample_calls == 72u);
}

static void test_proximity_snap(void)
{
    s32 result;

    seed(SAMPLE_POSITIVE_ONLY);
    st16(HEIGHT_TABLE, (u16)(s16)-113); /* fail with minimum floor 0 */
    st16(B234 + 2u * 2u, (u16)(s16)-112); /* exact final difference 0 */
    result = wm_80091FF8(2, ROT_TABLE, HEIGHT_TABLE);

    check("proximity-snaps-to-requested-count",
          result == 2 && s_view_calls == 1u && s_sample_calls == 36u);
}

int main(void)
{
    test_three_candidates_and_grid();
    test_zero_floor_and_threshold_break();
    test_proximity_snap();
    if (s_failures != 0)
        return 1;
    puts("W34N50 0x80091FF8 full-body certificate PASS");
    return 0;
}
