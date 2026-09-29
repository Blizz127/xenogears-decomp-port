/* W34N46 production-linked certificate for retail wm_800740B8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_740b8.h"

#define VERTICES     0x8009A340u
#define POSE         0x8009D55Cu
#define HEADING      0x8009BD3Au
#define SCALE        0x8009BCDCu
#define SCROLL       0x8009BE0Cu
#define BUFFER_INDEX 0x8009D7F0u
#define DRAW_GLOBAL  0x8009BE3Cu
#define DRAW_RECORD  0x800D0000u
#define OT_ROOT      0x800D1000u
#define PACKET4      0x8009C664u
#define MODE_PACKET  0x8009C5A0u
#define ICON_PACKET  0x8009C898u
#define FINAL_PACKET 0x8009C5C0u
#define ICON_MASK    0x8006F160u
#define DEFAULT_X    0x8009B6F4u
#define DEFAULT_Y    0x8009B6F6u
#define SPECIAL_24   0x8006EE60u
#define SPECIAL_25   0x8006EE82u
#define SPECIAL_26   0x8006EE78u
#define SC_ANGLES    0x1F8000B8u
#define SC_MATRIX    0x1F8000F0u

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static int s_rot_calls;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_project_calls;
static int s_link_calls;
static u32 s_rot_angles;
static u32 s_rot_matrix;
static u32 s_project_v[4][3];
static u32 s_project_packet[4];
static u32 s_link_ot[20];
static u32 s_link_packet[20];

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

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

void wm_740b8_test_rot_matrix(u32 angles, u32 matrix)
{
    MATRIX *output = (MATRIX *)PSX_ADDR(matrix);

    s_rot_angles = angles;
    s_rot_matrix = matrix;
    memset(output, 0, sizeof(*output));
    output->m[0][0] = 0x1000;
    output->m[1][1] = 0x1000;
    output->m[2][2] = 0x1000;
    s_rot_calls++;
}

void wm_740b8_test_set_rot(u32 matrix)
{
    check("matrix-install", matrix == SC_MATRIX);
    s_set_rot_calls++;
}

void wm_740b8_test_set_trans(u32 matrix)
{
    check("matrix-install", matrix == SC_MATRIX);
    s_set_trans_calls++;
}

void wm_740b8_test_project3(u32 v0, u32 v1, u32 v2, u32 packet)
{
    int call = s_project_calls;

    if (call < 4) {
        s_project_v[call][0] = v0;
        s_project_v[call][1] = v1;
        s_project_v[call][2] = v2;
        s_project_packet[call] = packet;
    }
    st32(packet + 8u, UINT32_C(0x11110000) + (u32)call);
    st32(packet + 16u, UINT32_C(0x22220000) + (u32)call);
    st32(packet + 24u, UINT32_C(0x33330000) + (u32)call);
    s_project_calls++;
}

void wm_740b8_test_link(u32 ot, u32 packet)
{
    int call = s_link_calls;

    if (call < 20) {
        s_link_ot[call] = ot;
        s_link_packet[call] = packet;
    }
    s_link_calls++;
}

static u16 special_expected(u16 source, u32 multiplier, u16 offset)
{
    u32 value = source;
    u32 high = (u32)(((uint64_t)value * (uint64_t)multiplier) >> 32);

    value = high + ((value - high) >> 1u);
    return (u16)((value >> 8u) + offset);
}

static void seed(void)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_project_v, 0, sizeof(s_project_v));
    memset(s_project_packet, 0, sizeof(s_project_packet));
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));
    s_rot_calls = 0;
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_project_calls = 0;
    s_link_calls = 0;

    st16(HEADING, UINT16_C(0x0345));
    st32(POSE + 0u, 0u);
    st32(POSE + 8u, 0u);
    st32(SCALE, 777u);
    st32(SCROLL, 9u);
    st32(BUFFER_INDEX, 1u);
    st32(DRAW_GLOBAL, DRAW_RECORD);
    st32(DRAW_RECORD + 0x70u, OT_ROOT);
    st32(ICON_MASK, UINT32_C(0x07000001));

    for (i = 0u; i < 32u; i++) {
        st16(DEFAULT_X + i * 4u, (u16)(10u + i));
        st16(DEFAULT_Y + i * 4u, (u16)(20u + i));
    }
    st16(SPECIAL_24 + 0u, UINT16_C(0x1000));
    st16(SPECIAL_24 + 4u, UINT16_C(0x2000));
    st16(SPECIAL_25 + 0u, UINT16_C(0x3000));
    st16(SPECIAL_25 + 4u, UINT16_C(0x4000));
    st16(SPECIAL_26 + 0u, UINT16_C(0x5000));
    st16(SPECIAL_26 + 2u, UINT16_C(0x6000));
}

static void test_renderer(void)
{
    MATRIX *matrix;
    u32 packet_base = PACKET4 + 112u;
    u32 icon_base = ICON_PACKET + 512u;
    u32 expected_links[13];
    u32 i;

    seed();
    wm_800740B8();
    matrix = (MATRIX *)PSX_ADDR(SC_MATRIX);

    check("rotation-source", s_rot_calls == 1 &&
          s_rot_angles == SC_ANGLES && s_rot_matrix == SC_MATRIX &&
          ld16(SC_ANGLES + 0u) == 0u && ld16(SC_ANGLES + 2u) == 0u &&
          ld16(SC_ANGLES + 4u) == UINT16_C(0x0345));
    check("matrix-translation", matrix->t[0] == 48 &&
          matrix->t[1] == 111 && matrix->t[2] == 777);
    check("matrix-install", s_set_rot_calls == 1 && s_set_trans_calls == 1);
    check("project-count", s_project_calls == 4);
    check("project-vertex-stride",
          s_project_v[0][0] == VERTICES &&
          s_project_v[1][0] == VERTICES + 24u &&
          s_project_v[3][0] == VERTICES + 72u);
    check("project-vertex-order",
          s_project_v[0][1] == VERTICES + 8u &&
          s_project_v[0][2] == VERTICES + 16u);
    check("project-packet-stride",
          s_project_packet[0] == packet_base &&
          s_project_packet[1] == packet_base + 28u &&
          s_project_packet[3] == packet_base + 84u);

    expected_links[0] = packet_base + 0u;
    expected_links[1] = packet_base + 28u;
    expected_links[2] = packet_base + 56u;
    expected_links[3] = packet_base + 84u;
    expected_links[4] = MODE_PACKET;
    expected_links[5] = icon_base + 0u * 16u;
    expected_links[6] = MODE_PACKET;
    expected_links[7] = icon_base + 24u * 16u;
    expected_links[8] = MODE_PACKET;
    expected_links[9] = icon_base + 25u * 16u;
    expected_links[10] = MODE_PACKET;
    expected_links[11] = icon_base + 26u * 16u;
    expected_links[12] = FINAL_PACKET + 40u;
    check("link-count-and-order", s_link_calls == 13);
    for (i = 0u; i < 13u && i < (u32)s_link_calls; i++) {
        check("link-count-and-order", s_link_ot[i] == OT_ROOT &&
              s_link_packet[i] == expected_links[i]);
    }

    check("default-coordinate",
          ld16(icon_base + 8u) == 218u &&
          ld16(icon_base + 10u) == 140u);
    check("special-coordinates",
          ld16(icon_base + 24u * 16u + 8u) ==
              special_expected(UINT16_C(0x1000),
                               UINT32_C(0xA01A01A1), 207u) &&
          ld16(icon_base + 25u * 16u + 10u) ==
              special_expected(UINT16_C(0x4000),
                               UINT32_C(0x80601807), 119u) &&
          ld16(icon_base + 26u * 16u + 8u) ==
              special_expected(UINT16_C(0x5000),
                               UINT32_C(0xA01A01A1), 207u) &&
          ld16(icon_base + 26u * 16u + 10u) ==
              special_expected(UINT16_C(0x6000),
                               UINT32_C(0x80601807), 119u));
}

int main(void)
{
    test_renderer();
    if (s_failures != 0)
        return 1;
    puts("W34N46 0x800740B8 full-body certificate PASS");
    return 0;
}
