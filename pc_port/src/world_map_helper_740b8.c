/* Retail world-map overlay renderer 0x800740B8. */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_740b8.h"

#define WM_740B8_VERTICES       0x8009A340u
#define WM_740B8_POSE           0x8009D55Cu
#define WM_740B8_HEADING        0x8009BD3Au
#define WM_740B8_SCALE          0x8009BCDCu
#define WM_740B8_SCROLL         0x8009BE0Cu
#define WM_740B8_BUFFER_INDEX   0x8009D7F0u
#define WM_740B8_DRAW_RECORD    0x8009BE3Cu
#define WM_740B8_PROJECT_PACKET 0x8009C664u
#define WM_740B8_MODE_PACKET    0x8009C5A0u
#define WM_740B8_ICON_PACKET    0x8009C898u
#define WM_740B8_FINAL_PACKET   0x8009C5C0u
#define WM_740B8_ICON_MASK      0x8006F160u
#define WM_740B8_DEFAULT_X      0x8009B6F4u
#define WM_740B8_DEFAULT_Y      0x8009B6F6u
#define WM_740B8_SPECIAL_24     0x8006EE60u
#define WM_740B8_SPECIAL_25     0x8006EE82u
#define WM_740B8_SPECIAL_26     0x8006EE78u

#define WM_740B8_SC_ANGLES      0x1F8000B8u
#define WM_740B8_SC_MATRIX      0x1F8000F0u

static u16 wm_740b8_lhu(u32 address)
{
    u16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_740b8_lw(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_740b8_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_740b8_sw(u32 address, u32 value) __attribute__((unused));
static void wm_740b8_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_740b8_as_s32(u32 bits)
{
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_740b8_sra(u32 bits, u32 amount)
{
    u32 shifted = bits >> amount;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_MAX << (32u - amount);
    return shifted;
}

static u32 wm_740b8_mul_hi_signed(u32 left, u32 right)
{
    int64_t product = (int64_t)wm_740b8_as_s32(left) *
                      (int64_t)wm_740b8_as_s32(right);

    return (u32)((uint64_t)product >> 32);
}

static u32 wm_740b8_mul_hi_unsigned(u32 left, u32 right)
{
    return (u32)(((uint64_t)left * (uint64_t)right) >> 32);
}

#if defined(WM_740B8_TEST_TRACE)
extern void wm_740b8_test_rot_matrix(u32 angles, u32 matrix);
extern void wm_740b8_test_set_rot(u32 matrix);
extern void wm_740b8_test_set_trans(u32 matrix);
extern void wm_740b8_test_project3(u32 v0, u32 v1, u32 v2,
                                  u32 packet);
extern void wm_740b8_test_link(u32 ot, u32 packet);
#define WM_740B8_ROT_MATRIX(a, m) wm_740b8_test_rot_matrix((a), (m))
#define WM_740B8_SET_ROT(m) wm_740b8_test_set_rot(m)
#define WM_740B8_SET_TRANS(m) wm_740b8_test_set_trans(m)
#define WM_740B8_PROJECT3(v0, v1, v2, packet) \
    wm_740b8_test_project3((v0), (v1), (v2), (packet))
#define WM_740B8_LINK(ot, packet) wm_740b8_test_link((ot), (packet))
#else
#define WM_740B8_ROT_MATRIX(a, m) \
    ((void)RotMatrix((SVECTOR *)PSX_ADDR(a), (MATRIX *)PSX_ADDR(m)))
#define WM_740B8_SET_ROT(m) SetRotMatrix((MATRIX *)PSX_ADDR(m))
#define WM_740B8_SET_TRANS(m) SetTransMatrix((MATRIX *)PSX_ADDR(m))
static void wm_740b8_project3(u32 v0, u32 v1, u32 v2, u32 packet)
{
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long p = 0;
    long flag = 0;

    (void)RotTransPers3((SVECTOR *)PSX_ADDR(v0),
                        (SVECTOR *)PSX_ADDR(v1),
                        (SVECTOR *)PSX_ADDR(v2),
                        &xy0, &xy1, &xy2, &p, &flag);
    wm_740b8_sw(packet + 8u, (u32)(unsigned long)xy0);
    wm_740b8_sw(packet + 16u, (u32)(unsigned long)xy1);
    wm_740b8_sw(packet + 24u, (u32)(unsigned long)xy2);
}
#define WM_740B8_PROJECT3(v0, v1, v2, packet) \
    wm_740b8_project3((v0), (v1), (v2), (packet))
#define WM_740B8_LINK(ot, packet) \
    PcPort_AddPrimDomainAware(PSX_ADDR(ot), PSX_ADDR(packet))
#endif

static u32 wm_740b8_position_x(u32 fixed_x)
{
    u32 units = wm_740b8_sra(fixed_x, 12u);
    u32 high = wm_740b8_mul_hi_signed(units, UINT32_C(0xD00D00D1));
    u32 value = wm_740b8_sra(high + units, 8u);

    value -= wm_740b8_sra(fixed_x, 31u);
#if defined(WM_740B8_MUTANT_POSITION_OFFSET)
    return value + 47u;
#else
    return value + 48u;
#endif
}

static u32 wm_740b8_position_y(u32 fixed_z)
{
    u32 units = wm_740b8_sra(fixed_z, 12u);
    u32 high = wm_740b8_mul_hi_signed(units, UINT32_C(0x300C0301));
    u32 value = wm_740b8_sra(high, 6u);

    value -= wm_740b8_sra(fixed_z, 31u);
    value += 120u;
    return value - wm_740b8_lw(WM_740B8_SCROLL);
}

static u16 wm_740b8_special_coord(u16 source, u32 multiplier, u16 offset)
{
    u32 value = source;
    u32 high = wm_740b8_mul_hi_unsigned(value, multiplier);

    value = high + ((value - high) >> 1u);
    value >>= 8u;
    return (u16)(value + offset);
}

static u32 wm_740b8_ot(void)
{
    u32 draw_record = wm_740b8_lw(WM_740B8_DRAW_RECORD);

    return wm_740b8_lw(draw_record + 0x70u);
}

void wm_800740B8(void)
{
    u32 buffer_index = wm_740b8_lw(WM_740B8_BUFFER_INDEX);
    u32 packet = WM_740B8_PROJECT_PACKET + buffer_index * 112u;
    u32 vertex = WM_740B8_VERTICES;
    u32 index;
    MATRIX *matrix = (MATRIX *)PSX_ADDR(WM_740B8_SC_MATRIX);

    wm_740b8_sh(WM_740B8_SC_ANGLES + 0u, 0u);
    wm_740b8_sh(WM_740B8_SC_ANGLES + 2u, 0u);
    wm_740b8_sh(WM_740B8_SC_ANGLES + 4u,
                 wm_740b8_lhu(WM_740B8_HEADING));
    WM_740B8_ROT_MATRIX(WM_740B8_SC_ANGLES, WM_740B8_SC_MATRIX);

    matrix->t[0] = wm_740b8_as_s32(
        wm_740b8_position_x(wm_740b8_lw(WM_740B8_POSE + 0u)));
    matrix->t[1] = wm_740b8_as_s32(
        wm_740b8_position_y(wm_740b8_lw(WM_740B8_POSE + 8u)));
    matrix->t[2] = wm_740b8_as_s32(wm_740b8_lw(WM_740B8_SCALE));
    WM_740B8_SET_ROT(WM_740B8_SC_MATRIX);
    WM_740B8_SET_TRANS(WM_740B8_SC_MATRIX);

    for (index = 0u; index < 4u; index++) {
#if defined(WM_740B8_MUTANT_VERTEX_STRIDE)
        u32 current_vertex = vertex + index * 16u;
#else
        u32 current_vertex = vertex + index * 24u;
#endif
#if defined(WM_740B8_MUTANT_PACKET_STRIDE)
        u32 current_packet = packet + index * 24u;
#else
        u32 current_packet = packet + index * 28u;
#endif
#if defined(WM_740B8_MUTANT_VERTEX_ORDER)
        WM_740B8_PROJECT3(current_vertex, current_vertex + 16u,
                          current_vertex + 8u, current_packet);
#else
        WM_740B8_PROJECT3(current_vertex, current_vertex + 8u,
                          current_vertex + 16u, current_packet);
#endif
#if !defined(WM_740B8_MUTANT_SKIP_PROJECTED_LINKS)
        WM_740B8_LINK(wm_740b8_ot(), current_packet);
#endif
    }

    {
        u32 mask = wm_740b8_lw(WM_740B8_ICON_MASK);
        u32 icon_packet = WM_740B8_ICON_PACKET + buffer_index * 512u;

        for (index = 0u; index < 32u; index++) {
            if ((mask & 1u) != 0u) {
                u16 x;
                u16 y;

#if !defined(WM_740B8_MUTANT_SKIP_MODE_PACKET)
                WM_740B8_LINK(wm_740b8_ot(), WM_740B8_MODE_PACKET);
#endif
                if (index == 24u) {
                    x = wm_740b8_special_coord(
                        wm_740b8_lhu(WM_740B8_SPECIAL_24 + 0u),
                        UINT32_C(0xA01A01A1), 207u);
                    y = wm_740b8_special_coord(
                        wm_740b8_lhu(WM_740B8_SPECIAL_24 + 4u),
                        UINT32_C(0x80601807), 119u);
                } else if (index == 25u) {
                    x = wm_740b8_special_coord(
                        wm_740b8_lhu(WM_740B8_SPECIAL_25 + 0u),
                        UINT32_C(0xA01A01A1), 207u);
                    y = wm_740b8_special_coord(
                        wm_740b8_lhu(WM_740B8_SPECIAL_25 + 4u),
                        UINT32_C(0x80601807), 119u);
                } else if (index == 26u) {
#if defined(WM_740B8_MUTANT_SPECIAL_SOURCE)
                    u32 source = WM_740B8_SPECIAL_25;
#else
                    u32 source = WM_740B8_SPECIAL_26;
#endif
                    x = wm_740b8_special_coord(wm_740b8_lhu(source + 0u),
                                                UINT32_C(0xA01A01A1), 207u);
                    y = wm_740b8_special_coord(wm_740b8_lhu(source + 2u),
                                                UINT32_C(0x80601807), 119u);
                } else {
#if defined(WM_740B8_MUTANT_DEFAULT_COORD)
                    x = (u16)(wm_740b8_lhu(WM_740B8_DEFAULT_X +
                                           index * 4u) + 207u);
#else
                    x = (u16)(wm_740b8_lhu(WM_740B8_DEFAULT_X +
                                           index * 4u) + 208u);
#endif
                    y = (u16)(wm_740b8_lhu(WM_740B8_DEFAULT_Y +
                                           index * 4u) + 120u);
                }
                wm_740b8_sh(icon_packet + 8u, x);
                wm_740b8_sh(icon_packet + 10u, y);
                WM_740B8_LINK(wm_740b8_ot(), icon_packet);
            }
#if defined(WM_740B8_MUTANT_MASK_LEFT)
            mask <<= 1u;
#else
            mask >>= 1u;
#endif
            icon_packet += 16u;
        }
    }

#if !defined(WM_740B8_MUTANT_SKIP_FINAL_PACKET)
    WM_740B8_LINK(wm_740b8_ot(),
                  WM_740B8_FINAL_PACKET + buffer_index * 40u);
#endif
}
