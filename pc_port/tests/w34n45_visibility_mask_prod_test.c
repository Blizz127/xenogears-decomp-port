/* W34N45 production-linked certificate for retail 983A0 + 987AC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_983a0.h"
#include "world_map_helper_987ac.h"

#define INPUT_ADDR 0x800B0000u
#define MATRIX_SRC 0x8009D534u
#define CAMERA     0x8009C808u
#define COARSE     0x8009D618u
#define FINE       0x8009D650u
#define TABLE      0x8009B7A8u
#define SC_MATRIX  0x1F8000F0u
#define SC_COMPOSE 0x1F800110u

uint8_t g_PsxRam[PSX_RAM_SIZE];

typedef struct {
    u32 p[4];
    u16 x[4];
    u16 z[4];
} ClassifyCall;

static int s_failures;
static int s_comp_calls;
static int s_rot_calls;
static int s_trans_calls;
static int s_classify_calls;
static int s_transform_calls;
static u32 s_comp_args[3];
static u32 s_rot_arg;
static u32 s_trans_arg;
static ClassifyCall s_classify[40];
static s32 s_transform_output[4][3];

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 ld16(u32 address)
{
    u16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

void wm_983a0_test_comp_matrix(u32 left, u32 right, u32 output)
{
    s_comp_args[0] = left;
    s_comp_args[1] = right;
    s_comp_args[2] = output;
    memcpy(PSX_ADDR(output), PSX_ADDR(right), 32u);
    s_comp_calls++;
}

void wm_983a0_test_set_rot_matrix(u32 matrix)
{
    s_rot_arg = matrix;
    s_rot_calls++;
}

void wm_983a0_test_set_trans_matrix(u32 matrix)
{
    s_trans_arg = matrix;
    s_trans_calls++;
}

s32 wm_983a0_test_classify(u32 p0, u32 p1, u32 p2, u32 p3)
{
    static const s32 scripted[] = { 0, -1, 0, 1, -1 };
    u32 points[4];
    int call = s_classify_calls;
    int i;

    points[0] = p0;
    points[1] = p1;
    points[2] = p2;
    points[3] = p3;
    if (call < 40) {
        for (i = 0; i < 4; i++) {
            s_classify[call].p[i] = points[i];
            s_classify[call].x[i] = ld16(points[i] + 0u);
            s_classify[call].z[i] = ld16(points[i] + 4u);
        }
    }
    s_classify_calls++;
    if (call < (int)(sizeof(scripted) / sizeof(scripted[0])))
        return scripted[call];
    return 1;
}

void wm_987ac_test_rot_trans(u32 input_addr, u32 output_addr)
{
    int call = s_transform_calls;

    (void)input_addr;
    if (call < 4) {
        st32(output_addr + 0u, (u32)s_transform_output[call][0]);
        st32(output_addr + 4u, (u32)s_transform_output[call][1]);
        st32(output_addr + 8u, (u32)s_transform_output[call][2]);
    }
    s_transform_calls++;
}

static void reset_all(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_classify, 0, sizeof(s_classify));
    memset(s_transform_output, 0, sizeof(s_transform_output));
    memset(s_comp_args, 0, sizeof(s_comp_args));
    s_comp_calls = 0;
    s_rot_calls = 0;
    s_trans_calls = 0;
    s_classify_calls = 0;
    s_transform_calls = 0;
}

static void seed_plane_factors(void)
{
    static const u32 first[] = {
        0x8009C828u, 0x8009C844u, 0x8009C878u, 0x8009C7F4u
    };
    static const u32 second[] = {
        0x8009C830u, 0x8009C84Cu, 0x8009C87Cu, 0x8009C7F8u
    };
    u32 i;

    for (i = 0u; i < 4u; i++) {
        st32(first[i], 0x1000u);
        st32(second[i], 0u);
    }
}

static void set_transform_values(s32 x0, s32 x1, s32 x2, s32 x3,
                                 s32 y0, s32 y1, s32 y2, s32 y3)
{
    s_transform_output[0][0] = x0;
    s_transform_output[1][0] = x1;
    s_transform_output[2][0] = x2;
    s_transform_output[3][0] = x3;
    s_transform_output[0][1] = y0;
    s_transform_output[1][1] = y1;
    s_transform_output[2][1] = y2;
    s_transform_output[3][1] = y3;
}

static void test_classifier(void)
{
    s32 result;

    reset_all();
    seed_plane_factors();
    set_transform_values(-1, -1, -1, -1, -1, -1, -1, -1);
    result = wm_800987AC(0x800C0000u, 0x800C0008u,
                         0x800C0010u, 0x800C0018u);
    check("classifier-outside-tristate", result == -1);
    check("classifier-four-transforms", s_transform_calls == 4);

    reset_all();
    seed_plane_factors();
    set_transform_values(1, 1, 1, 1, 1, 1, 1, 1);
    result = wm_800987AC(0x800C0000u, 0x800C0008u,
                         0x800C0010u, 0x800C0018u);
    check("classifier-inside-tristate", result == 1);

    reset_all();
    seed_plane_factors();
    /* X remains wholly inside. Y has only retail-adjacent corners 1/3
     * outside, proving both the Y component and 0,1,3,2 corner order. */
    set_transform_values(1, 1, 1, 1, 1, -1, 1, -1);
    result = wm_800987AC(0x800C0000u, 0x800C0008u,
                         0x800C0010u, 0x800C0018u);
    check("classifier-intersection-order", result == 0);
}

static void seed_mask_case(void)
{
    u32 i;

    for (i = 0u; i < 8u; i++)
        st32(MATRIX_SRC + i * 4u, UINT32_C(0x11110000) + i);
    for (i = 0u; i < 25u; i++)
        st16(COARSE + i * 2u, UINT16_C(0xEEEE));
    st32(INPUT_ADDR + 0u, UINT32_C(0x00500000));
    st32(INPUT_ADDR + 8u, UINT32_C(0x00200000));

    /* Four distinguishable 200-byte quadrant tables. */
    for (i = 0u; i < 25u; i++) {
        st32(TABLE + 0u * 200u + i * 8u, UINT32_C(0x01000000));
        st32(TABLE + 0u * 200u + i * 8u + 4u, UINT32_C(0x02000000));
        st32(TABLE + 1u * 200u + i * 8u, UINT32_C(0x00100000));
        st32(TABLE + 1u * 200u + i * 8u + 4u, UINT32_C(0x00200000));
        st32(TABLE + 2u * 200u + i * 8u, UINT32_C(0x00010000));
        st32(TABLE + 2u * 200u + i * 8u + 4u, UINT32_C(0x00020000));
        st32(TABLE + 3u * 200u + i * 8u, UINT32_C(0x00001000));
        st32(TABLE + 3u * 200u + i * 8u + 4u, UINT32_C(0x00002000));
    }
}

static void test_mask_producer(void)
{
    u32 i;

    reset_all();
    seed_mask_case();
    wm_800983A0(INPUT_ADDR);

    check("matrix-copy-source",
          memcmp(PSX_ADDR(SC_MATRIX), PSX_ADDR(MATRIX_SRC), 32u) == 0);
    check("matrix-compose-and-publish",
          s_comp_calls == 1 && s_comp_args[0] == CAMERA &&
          s_comp_args[1] == SC_MATRIX && s_comp_args[2] == SC_COMPOSE &&
          s_rot_calls == 1 && s_rot_arg == SC_COMPOSE &&
          s_trans_calls == 1 && s_trans_arg == SC_COMPOSE);
    check("classify-call-count", s_classify_calls == 29);
    check("coarse-mask-values", ld16(COARSE) == 0u &&
          ld16(COARSE + 2u) == 1u && ld16(COARSE + 48u) == 1u);

    check("grid-cell-layout",
          s_classify[0].x[0] == UINT16_C(0xEB00) &&
          s_classify[0].z[0] == UINT16_C(0x1200) &&
          s_classify[0].x[1] == UINT16_C(0xF300) &&
          s_classify[0].z[2] == UINT16_C(0x0A00) &&
          s_classify[5].x[0] == UINT16_C(0xF300) &&
          s_classify[5].z[0] == UINT16_C(0x1200));
    check("subdivision-corner-layout",
          s_classify[1].p[0] == UINT32_C(0x1F8000A0) &&
          s_classify[1].p[1] == UINT32_C(0x1F8000C0) &&
          s_classify[1].p[2] == UINT32_C(0x1F8000C8) &&
          s_classify[1].p[3] == UINT32_C(0x1F8000E0) &&
          s_classify[4].p[0] == UINT32_C(0x1F8000E0) &&
          s_classify[4].p[3] == UINT32_C(0x1F8000B8));

    check("boundary-table-selection",
          ld32(FINE + 0u) == UINT32_C(0x0010FFFF) &&
          ld32(FINE + 4u) == UINT32_C(0xFFFF0001));
    check("boundary-mask-or",
          ld32(FINE + 8u) == UINT32_C(0x00110001) &&
          ld32(FINE + 12u) == UINT32_C(0x00210001));

    for (i = 0u; i < 25u; i++)
        check("coarse-exact-write-set", ld16(COARSE + i * 2u) != UINT16_C(0xEEEE));
}

int main(void)
{
    test_classifier();
    test_mask_producer();
    if (s_failures != 0)
        return 1;
    puts("W34N45 0x800983A0/0x800987AC full-body certificate PASS");
    return 0;
}
