/* W34N47 production-linked certificate for retail wm_80086798. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_86798.h"

#undef rsin
#undef rcos

#define SC             0x1F800000u
#define MID_SOURCE     0x8009ADB0u
#define FAR_SOURCE     0x8009AD50u
#define UV_SOURCE      0x8009AD40u
#define BASE_MATRIX    0x8009A180u
#define CAMERA_MATRIX  0x8009C808u
#define ANGLE_X        0x8009BD38u
#define HEADING        0x8009BD3Au
#define OFFSET_X       0x8009BD40u
#define OFFSET_Z       0x8009BD44u
#define CAMERA_X       0x8009BE28u
#define CAMERA_Z       0x8009BE30u
#define DRAW_GLOBAL    0x8009BE3Cu
#define CALLBACK       0x8009CD40u
#define AUX_GLOBAL     0x8009CEB4u
#define RECORD_GLOBAL  0x8009D150u
#define BUFFER_INDEX   0x8009D7F0u
#define PACKET_GLOBAL  0x8009D7F8u

#define RECORDS        0x80070000u
#define AUXILIARY      0x80071000u
#define PACKETS        0x80072000u
#define DRAW_RECORD    0x80076000u
#define OT_ROOT        0x80077000u

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static u32 s_allowed_records;
static u32 s_nclip_calls;
static u16 s_origin_depth;
static u32 s_origin_flag;
static u32 s_origin_calls;
static u16 s_quad_depth;
static u32 s_quad_flag3;
static u32 s_quad_flag1;
static int s_quad_offscreen;
static u32 s_quad_calls;
static u32 s_bounds_calls;
static u32 s_link_calls;
static u32 s_link_ot[400];
static u32 s_link_packet[400];
static u32 s_quad_vertex[400];
static s16 s_quad_coords[400][4][3];
static u32 s_rot_x_calls;
static u32 s_rot_y_calls;
static u32 s_mul_calls;
static u32 s_comp_calls;
static u32 s_set_rot_calls;
static u32 s_set_trans_calls;
static s32 s_rot_x_angle;
static s32 s_rot_y_angle[2];
static u32 s_comp_left;
static u32 s_comp_right;
static u32 s_comp_output;
static s32 s_comp_translation[3];

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

int rsin(int angle)
{
    return angle & 0x0FFF;
}

int rcos(int angle)
{
    return (angle + 0x400) & 0x0FFF;
}

void wm_86798_test_rot_x(s32 angle, u32 matrix)
{
    check("rotation-sequence", matrix == SC + 0x250u);
    s_rot_x_angle = angle;
    s_rot_x_calls++;
}

void wm_86798_test_rot_y(s32 angle, u32 matrix)
{
    if (s_rot_y_calls < 2u) {
        s_rot_y_angle[s_rot_y_calls] = angle;
        check("rotation-sequence",
              matrix == SC + (s_rot_y_calls == 0u ? 0x270u : 0x290u));
    }
    s_rot_y_calls++;
}

void wm_86798_test_mul(u32 left, u32 right, u32 output)
{
    if (s_mul_calls == 0u) {
        check("rotation-sequence", left == SC + 0x250u &&
              right == SC + 0x270u && output == SC + 0x2B0u);
    } else if (s_mul_calls == 1u) {
        check("rotation-sequence", left == SC + 0x290u &&
              right == SC + 0x2B0u && output == SC + 0x250u);
    }
    memcpy(PSX_ADDR(output), PSX_ADDR(right), 32u);
    s_mul_calls++;
}

void wm_86798_test_comp(u32 left, u32 right, u32 output)
{
    MATRIX *matrix = (MATRIX *)PSX_ADDR(right);
    if (s_comp_calls == 0u) {
        s_comp_left = left;
        s_comp_right = right;
        s_comp_output = output;
        s_comp_translation[0] = matrix->t[0];
        s_comp_translation[1] = matrix->t[1];
        s_comp_translation[2] = matrix->t[2];
    }
    memcpy(PSX_ADDR(output), PSX_ADDR(right), 32u);
    s_comp_calls++;
}

void wm_86798_test_set_rot(u32 matrix)
{
    check("matrix-install", matrix == SC + 0x270u);
    s_set_rot_calls++;
}

void wm_86798_test_set_trans(u32 matrix)
{
    check("matrix-install", matrix == SC + 0x270u);
    s_set_trans_calls++;
}

s32 wm_86798_test_nclip(u32 xy0, u32 xy1, u32 xy2)
{
    s32 result = s_nclip_calls < s_allowed_records * 2u ? 0 : 1;
    (void)xy0;
    (void)xy1;
    (void)xy2;
    s_nclip_calls++;
    return result;
}

void wm_86798_test_origin(u32 vertex, u32 *flag, u16 *depth)
{
    check("origin-vector", vertex == SC + 0x1F0u &&
          ld16(vertex) == 0u && ld16(vertex + 2u) == 0u &&
          ld16(vertex + 4u) == 0u);
    *flag = s_origin_flag;
    *depth = s_origin_depth;
    s_origin_calls++;
}

static u32 pack_xy(u16 x, u16 y)
{
    return (u32)x | ((u32)y << 16u);
}

void wm_86798_test_quad(u32 vertex, u32 xy[4], u16 depth[4],
                         u32 *flag3, u32 *flag1)
{
    u32 call = s_quad_calls;
    u32 i;
    if (call < 400u) {
        s_quad_vertex[call] = vertex;
        for (i = 0u; i < 4u; i++) {
            s_quad_coords[call][i][0] = (s16)ld16(vertex + i * 8u + 0u);
            s_quad_coords[call][i][1] = (s16)ld16(vertex + i * 8u + 2u);
            s_quad_coords[call][i][2] = (s16)ld16(vertex + i * 8u + 4u);
        }
    }
    for (i = 0u; i < 4u; i++) {
        xy[i] = s_quad_offscreen ? pack_xy(400u, 300u) :
                                   pack_xy((u16)(40u + i),
                                           (u16)(50u + i));
        depth[i] = (u16)(s_quad_depth + (u16)i);
    }
    *flag3 = s_quad_flag3;
    *flag1 = s_quad_flag1;
    s_quad_calls++;
}

void wm_86798_test_bounds(u32 vertex, u32 *flag1, u32 *flag3)
{
    check("near-bounds-source", vertex == SC + 0x1F0u);
    *flag1 = 0u;
    *flag3 = 0u;
    s_bounds_calls++;
}

void PcPort_AddPrimDomainAware(void *ot_pointer, void *prim_pointer)
{
    u32 *ot = (u32 *)ot_pointer;
    u32 *prim = (u32 *)prim_pointer;
    u32 guest = PsxMemory_GuestAddr(prim_pointer);
    if (s_link_calls < 400u) {
        s_link_ot[s_link_calls] = PsxMemory_GuestAddr(ot_pointer);
        s_link_packet[s_link_calls] = guest;
    }
    *prim = (*prim & UINT32_C(0xFF000000)) |
            (*ot & UINT32_C(0x00FFFFFF));
    *ot = (*ot & UINT32_C(0xFF000000)) |
          (guest & UINT32_C(0x00FFFFFF));
    s_link_calls++;
}

static void seed(void)
{
    u32 i;
    MATRIX identity;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(&identity, 0, sizeof(identity));
    identity.m[0][0] = 0x1000;
    identity.m[1][1] = 0x1000;
    identity.m[2][2] = 0x1000;
    memcpy(PSX_ADDR(BASE_MATRIX), &identity, 32u);
    memcpy(PSX_ADDR(CAMERA_MATRIX), &identity, 32u);

    s_allowed_records = 1u;
    s_nclip_calls = 0u;
    s_origin_depth = 1500u;
    s_origin_flag = 0u;
    s_origin_calls = 0u;
    s_quad_depth = 2500u;
    s_quad_flag3 = 0u;
    s_quad_flag1 = 0u;
    s_quad_offscreen = 0;
    s_quad_calls = 0u;
    s_bounds_calls = 0u;
    s_link_calls = 0u;
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));
    memset(s_quad_vertex, 0, sizeof(s_quad_vertex));
    memset(s_quad_coords, 0, sizeof(s_quad_coords));
    s_rot_x_calls = 0u;
    s_rot_y_calls = 0u;
    s_mul_calls = 0u;
    s_comp_calls = 0u;
    s_set_rot_calls = 0u;
    s_set_trans_calls = 0u;
    s_rot_x_angle = 0;
    memset(s_rot_y_angle, 0, sizeof(s_rot_y_angle));
    s_comp_left = 0u;
    s_comp_right = 0u;
    s_comp_output = 0u;
    memset(s_comp_translation, 0, sizeof(s_comp_translation));

    st32(CALLBACK, UINT32_C(0x80086700));
    st32(RECORD_GLOBAL, RECORDS);
    st32(AUX_GLOBAL, AUXILIARY);
    st32(PACKET_GLOBAL, PACKETS);
    st32(PACKET_GLOBAL + 4u, PACKETS + 0x4000u);
    st32(BUFFER_INDEX, 0u);
    st32(DRAW_GLOBAL, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_ROOT);
    st32(CAMERA_X, 0x00100000u);
    st32(CAMERA_Z, 0x00180000u);
    st16(ANGLE_X, 0x0200u);
    st16(HEADING, 0x0100u);
    st16(OFFSET_X, 10u);
    st16(OFFSET_Z, 20u);

    for (i = 0u; i < 80u; i++) {
        st32(RECORDS + i * 16u + 0u, 0x00100000u);
        st32(RECORDS + i * 16u + 4u, 3u << 12u);
        st32(RECORDS + i * 16u + 8u, 0x00180000u);
        st16(AUXILIARY + i * 8u + 2u, 1u);
    }
    st16(AUXILIARY + 0u, 0x1000u);
    st16(AUXILIARY + 4u, 0xF000u);

    for (i = 0u; i < 48u * 8u; i++)
        *((u8 *)PSX_ADDR(MID_SOURCE + i)) = (u8)(i ^ 0x5Au);
    for (i = 0u; i < 12u * 8u; i++)
        *((u8 *)PSX_ADDR(FAR_SOURCE + i)) = (u8)(i ^ 0xA5u);
    st16(FAR_SOURCE + 0u, (u16)(s16)-100);
    st16(FAR_SOURCE + 2u, 20u);
    st16(FAR_SOURCE + 4u, 300u);
    for (i = 0u; i < 8u; i++)
        st16(UV_SOURCE + i * 2u, (u16)(0x0100u + i));
    for (i = 0u; i < 400u; i++) {
        st32(PACKETS + i * 40u, UINT32_C(0x09000000));
        *((u8 *)PSX_ADDR(PACKETS + i * 40u + 7u)) = 0x2Eu;
    }
    st32(PACKETS - 4u, UINT32_C(0xA1B2C3D4));
    st32(PACKETS + 288u * 40u, UINT32_C(0xD4C3B2A1));
}

static void test_far_and_shared_state(void)
{
    seed();
    wm_80086798();
    check("callback-advance",
          ld32(RECORDS) == 0x00101000u &&
          ld32(RECORDS + 8u) == 0x0017F000u);
    check("template-copies",
          memcmp(PSX_ADDR(SC + 0x60u), PSX_ADDR(MID_SOURCE), 48u * 8u) == 0 &&
          memcmp(PSX_ADDR(SC), PSX_ADDR(FAR_SOURCE), 12u * 8u) == 0 &&
          memcmp(PSX_ADDR(SC + 0x1E0u), PSX_ADDR(UV_SOURCE), 16u) == 0);
    check("camera-tile-slots",
          ld32(SC + 0x220u) == 0x100u && ld32(SC + 0x228u) == 0x180u);
    check("rotation-sequence",
          s_rot_x_calls == 1u && s_rot_y_calls == 2u &&
          s_mul_calls == 2u && s_rot_x_angle == 0xC0 &&
          s_rot_y_angle[0] == -0x100 && s_rot_y_angle[1] == 0x100);
    check("composition-order",
          s_comp_calls == 1u && s_comp_left == CAMERA_MATRIX &&
          s_comp_right == SC + 0x250u && s_comp_output == SC + 0x270u &&
          s_comp_translation[0] == 1 && s_comp_translation[1] == 3 &&
          s_comp_translation[2] == 1);
    check("matrix-install", s_set_rot_calls == 1u && s_set_trans_calls == 1u);
    check("far-path-shape",
          s_origin_calls == 1u && s_quad_calls == 3u &&
          ld32(SC + 0x2F0u) == 3u && ld32(SC + 0x2F4u) == 3u);
    check("far-links", s_link_calls == 3u &&
          s_link_ot[0] == OT_ROOT + ((2503u >> 4u) * 4u));
    check("packet-stride",
          s_link_packet[0] == PACKETS &&
          s_link_packet[1] == PACKETS + 40u &&
          s_link_packet[2] == PACKETS + 80u);
    check("far-uv-layout",
          ld16(PACKETS + 0x0Cu) == 0x0104u &&
          ld16(PACKETS + 0x14u) == 0x013Fu &&
          ld16(PACKETS + 0x1Cu) == 0x3F04u &&
          ld16(PACKETS + 0x24u) == 0x3F3Fu &&
          ld16(PACKETS + 40u + 0x0Cu) == 0x0105u);
    check("packet-tag-and-xy",
          (ld32(PACKETS) >> 24u) == 9u &&
          ld32(PACKETS + 8u) == pack_xy(40u, 50u) &&
          *((u8 *)PSX_ADDR(PACKETS + 7u)) == 0x2Eu);
}

static void test_mid_path(void)
{
    seed();
    s_origin_depth = 1200u;
    s_quad_depth = 1100u;
    wm_80086798();
    check("mid-path-shape", s_quad_calls == 12u &&
          ld32(SC + 0x2F0u) == 12u && ld32(SC + 0x2F4u) == 12u &&
          s_quad_vertex[0] == SC + 0x60u &&
          s_quad_vertex[11] == SC + 0x1C0u);
    check("mid-uv-layout",
          ld16(PACKETS + 0u * 40u + 0x0Cu) == 0x0104u &&
          ld16(PACKETS + 1u * 40u + 0x0Cu) == 0x0124u &&
          ld16(PACKETS + 2u * 40u + 0x0Cu) == 0x2104u &&
          ld16(PACKETS + 3u * 40u + 0x0Cu) == 0x2124u);
}

static void test_near_path(void)
{
    seed();
    s_origin_depth = 1000u;
    s_quad_depth = 800u;
    wm_80086798();
    check("near-path-shape", s_bounds_calls == 3u && s_quad_calls == 48u &&
          ld32(SC + 0x2F0u) == 48u && ld32(SC + 0x2F4u) == 48u);
    check("near-grid-step",
          s_quad_coords[0][0][0] == -100 &&
          s_quad_coords[0][1][0] == -4 &&
          s_quad_coords[0][2][2] == 204 &&
          s_quad_coords[1][0][0] == -4 &&
          s_quad_coords[4][0][2] == 204 &&
          s_quad_coords[16][0][1] == 12);
    check("near-uv-layout",
          ld16(PACKETS + 0u * 40u + 0x0Cu) == 0x0104u &&
          ld16(PACKETS + 1u * 40u + 0x0Cu) == 0x0114u &&
          ld16(PACKETS + 4u * 40u + 0x0Cu) == 0x1104u);
}

static void test_path_specific_flags(void)
{
    seed();
    s_quad_flag3 = 0x00002000u;
    wm_80086798();
    check("far-sign-only-flag", s_link_calls == 3u);

    seed();
    s_origin_depth = 1000u;
    s_quad_flag3 = 0x00002000u;
    wm_80086798();
    check("near-strict-flag", s_link_calls == 0u && s_quad_calls == 48u);
}

static void test_packet_ceiling(void)
{
    seed();
    s_allowed_records = 7u;
    s_origin_depth = 1000u;
    s_quad_depth = 800u;
    wm_80086798();
    check("packet-ceiling", s_origin_calls == 6u && s_link_calls == 288u &&
          ld32(SC + 0x2F0u) == 288u &&
          ld32(PACKETS - 4u) == UINT32_C(0xA1B2C3D4) &&
          ld32(PACKETS + 288u * 40u) == UINT32_C(0xD4C3B2A1));
}

int main(void)
{
    test_far_and_shared_state();
    test_mid_path();
    test_near_path();
    test_path_specific_flags();
    test_packet_ceiling();
    if (s_failures != 0)
        return 1;
    puts("W34N47 0x80086798 full-body certificate PASS");
    return 0;
}
