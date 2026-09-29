/*
 * Retail world-map tiled-object renderer 0x80086798.
 *
 * Structural transcription of world_map.bin [0x80086798, 0x80087710),
 * 0xF78 bytes / 990 instructions. The renderer advances the 80 live object
 * records, builds the camera-relative transform, selects one of three
 * distance-dependent quad grids, and publishes retail POLY_FT4 packets.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_86798.h"

#define WM_86798_SCRATCH       0x1F800000u
#define WM_86798_FAR_VERTICES  (WM_86798_SCRATCH + 0x000u)
#define WM_86798_MID_VERTICES  (WM_86798_SCRATCH + 0x060u)
#define WM_86798_UV_TABLE      (WM_86798_SCRATCH + 0x1E0u)
#define WM_86798_CELL_V0       (WM_86798_SCRATCH + 0x1F0u)
#define WM_86798_CELL_V1       (WM_86798_SCRATCH + 0x1F8u)
#define WM_86798_CELL_V2       (WM_86798_SCRATCH + 0x200u)
#define WM_86798_CELL_V3       (WM_86798_SCRATCH + 0x208u)
#define WM_86798_CAMERA_X      (WM_86798_SCRATCH + 0x210u)
#define WM_86798_CAMERA_Z      (WM_86798_SCRATCH + 0x218u)
#define WM_86798_CAMERA_TILE_X (WM_86798_SCRATCH + 0x220u)
#define WM_86798_CAMERA_TILE_Z (WM_86798_SCRATCH + 0x228u)
#define WM_86798_WEDGE_LEFT    (WM_86798_SCRATCH + 0x230u)
#define WM_86798_WEDGE_RIGHT   (WM_86798_SCRATCH + 0x234u)
#define WM_86798_WEDGE_X       (WM_86798_SCRATCH + 0x240u)
#define WM_86798_WEDGE_Z       (WM_86798_SCRATCH + 0x248u)
#define WM_86798_OBJECT_MATRIX (WM_86798_SCRATCH + 0x250u)
#define WM_86798_Y_NEG_MATRIX  (WM_86798_SCRATCH + 0x270u)
#define WM_86798_Y_POS_MATRIX  (WM_86798_SCRATCH + 0x290u)
#define WM_86798_TEMP_MATRIX   (WM_86798_SCRATCH + 0x2B0u)
#define WM_86798_UV_INDEX      (WM_86798_SCRATCH + 0x2D0u)
#define WM_86798_FLAG          (WM_86798_SCRATCH + 0x2D4u)
#define WM_86798_MAX_DEPTH     (WM_86798_SCRATCH + 0x2D8u)
#define WM_86798_ORIGIN_DEPTH  (WM_86798_SCRATCH + 0x2DCu)
#define WM_86798_PACKET_COUNT  (WM_86798_SCRATCH + 0x2F0u)
#define WM_86798_ATTEMPT_COUNT (WM_86798_SCRATCH + 0x2F4u)

#define WM_86798_FAR_SOURCE    0x8009AD50u
#define WM_86798_MID_SOURCE    0x8009ADB0u
#define WM_86798_UV_SOURCE     0x8009AD40u
#define WM_86798_BASE_MATRIX   0x8009A180u
#define WM_86798_CAMERA_MATRIX 0x8009C808u
#define WM_86798_ANGLE_X       0x8009BD38u
#define WM_86798_HEADING       0x8009BD3Au
#define WM_86798_OFFSET_X      0x8009BD40u
#define WM_86798_OFFSET_Z      0x8009BD44u
#define WM_86798_CAMERA_WORLD_X 0x8009BE28u
#define WM_86798_CAMERA_WORLD_Z 0x8009BE30u
#define WM_86798_DRAW_RECORD   0x8009BE3Cu
#define WM_86798_AUX_ROOT      0x8009CEB4u
#define WM_86798_CALLBACK      0x8009CD40u
#define WM_86798_RECORD_ROOT   0x8009D150u
#define WM_86798_BUFFER_INDEX  0x8009D7F0u
#define WM_86798_PACKET_ROOTS  0x8009D7F8u

#define WM_86798_RECORD_COUNT   80u
#define WM_86798_RECORD_STRIDE  16u
#define WM_86798_AUX_STRIDE      8u
#if defined(WM_86798_MUTANT_PACKET_STRIDE_36)
#define WM_86798_PACKET_STRIDE  36u
#else
#define WM_86798_PACKET_STRIDE  40u
#endif
#define WM_86798_PACKET_CEILING 241u
#define WM_86798_FLAG_SIGN      UINT32_C(0x80000000)
#define WM_86798_FLAG_STRICT    UINT32_C(0x7F85E000)

static u16 wm_86798_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_86798_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_86798_lwu(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm_86798_lw(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_86798_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_86798_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_86798_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_86798_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 wm_86798_add(s32 left, s32 right)
{
    return wm_86798_as_s32(wm_86798_as_u32(left) +
                           wm_86798_as_u32(right));
}

static s32 wm_86798_sub(s32 left, s32 right)
{
    return wm_86798_as_s32(wm_86798_as_u32(left) -
                           wm_86798_as_u32(right));
}

static s32 wm_86798_neg(s32 value)
{
    return wm_86798_as_s32(0u - wm_86798_as_u32(value));
}

static s32 wm_86798_sra(s32 value, u32 amount)
{
    u32 bits = wm_86798_as_u32(value) >> amount;
    if (value < 0)
        bits |= UINT32_MAX << (32u - amount);
    return wm_86798_as_s32(bits);
}

static s32 wm_86798_relative(s32 world, s32 camera)
{
    s32 relative = wm_86798_sra(wm_86798_sub(world, camera), 12u);
    if (relative < -4096)
        relative += 8192;
    else if (relative >= 4096)
        relative -= 8192;
    return relative;
}

static u32 wm_86798_pack_sxy(s32 x, s32 y)
{
    return (u32)(u16)x | ((u32)(u16)y << 16u);
}

static int wm_86798_screen_overlap(const u32 xy[4])
{
    u32 i;
    int x_inside = 0;
    int y_inside = 0;
    for (i = 0u; i < 4u; i++) {
        if ((u32)(u16)xy[i] < 320u)
            x_inside = 1;
        if ((u32)(u16)(xy[i] >> 16u) < 216u)
            y_inside = 1;
    }
    return x_inside && y_inside;
}

static u16 wm_86798_max_depth(const u16 depth[4])
{
    u16 maximum = depth[0];
    u32 i;
    for (i = 1u; i < 4u; i++) {
        if (depth[i] > maximum)
            maximum = depth[i];
    }
    return maximum;
}

#if defined(WM_86798_TEST_TRACE)
extern void wm_86798_test_rot_x(s32 angle, u32 matrix);
extern void wm_86798_test_rot_y(s32 angle, u32 matrix);
extern void wm_86798_test_mul(u32 left, u32 right, u32 output);
extern void wm_86798_test_comp(u32 left, u32 right, u32 output);
extern void wm_86798_test_set_rot(u32 matrix);
extern void wm_86798_test_set_trans(u32 matrix);
extern s32 wm_86798_test_nclip(u32 xy0, u32 xy1, u32 xy2);
extern void wm_86798_test_origin(u32 vertex, u32 *flag, u16 *depth);
extern void wm_86798_test_quad(u32 vertex, u32 xy[4], u16 depth[4],
                               u32 *flag3, u32 *flag1);
extern void wm_86798_test_bounds(u32 vertex, u32 *flag1, u32 *flag3);
#define WM_86798_ROT_X(a, m) wm_86798_test_rot_x((a), (m))
#define WM_86798_ROT_Y(a, m) wm_86798_test_rot_y((a), (m))
#define WM_86798_MUL(l, r, o) wm_86798_test_mul((l), (r), (o))
#define WM_86798_COMP(l, r, o) wm_86798_test_comp((l), (r), (o))
#define WM_86798_SET_ROT(m) wm_86798_test_set_rot(m)
#define WM_86798_SET_TRANS(m) wm_86798_test_set_trans(m)
#define WM_86798_NCLIP(a, b, c) wm_86798_test_nclip((a), (b), (c))
#else
#define WM_86798_ROT_X(a, m) \
    ((void)RotMatrixX((a), (MATRIX *)PSX_ADDR(m)))
#define WM_86798_ROT_Y(a, m) \
    ((void)RotMatrixY((a), (MATRIX *)PSX_ADDR(m)))
#define WM_86798_MUL(l, r, o) \
    ((void)MulMatrix0((MATRIX *)PSX_ADDR(l), (MATRIX *)PSX_ADDR(r), \
                      (MATRIX *)PSX_ADDR(o)))
#define WM_86798_COMP(l, r, o) \
    ((void)CompMatrix((MATRIX *)PSX_ADDR(l), (MATRIX *)PSX_ADDR(r), \
                      (MATRIX *)PSX_ADDR(o)))
#define WM_86798_SET_ROT(m) SetRotMatrix((MATRIX *)PSX_ADDR(m))
#define WM_86798_SET_TRANS(m) SetTransMatrix((MATRIX *)PSX_ADDR(m))
#define WM_86798_NCLIP(a, b, c) NormalClip((int)(a), (int)(b), (int)(c))
#endif

/* W34B25: exact retail callback [0x80086700, 0x80086798). */
static void wm_80086700_cd40(void) __attribute__((unused));
static void wm_80086700_cd40(void)
{
    u32 records = wm_86798_lwu(WM_86798_RECORD_ROOT);
    u32 auxiliary = wm_86798_lwu(WM_86798_AUX_ROOT);
    u32 index;
    for (index = 0u; index < WM_86798_RECORD_COUNT; index++) {
        s32 x = wm_86798_add(wm_86798_lw(records),
                             (s32)wm_86798_lh(auxiliary));
        s32 z = wm_86798_add(wm_86798_lw(records + 8u),
                             (s32)wm_86798_lh(auxiliary + 4u));
        if (x > 0x01FFFFFF)
            x = wm_86798_add(x, wm_86798_as_s32(UINT32_C(0xFE000000)));
        if (x < 0)
            x = wm_86798_add(x, 0x02000000);
        if (z > 0x01FFFFFF)
            z = wm_86798_add(z, wm_86798_as_s32(UINT32_C(0xFE000000)));
        if (z < 0)
            z = wm_86798_add(z, 0x02000000);
        wm_86798_sw(records, wm_86798_as_u32(x));
        wm_86798_sw(records + 8u, wm_86798_as_u32(z));
        records += WM_86798_RECORD_STRIDE;
        auxiliary += WM_86798_AUX_STRIDE;
    }
}

static int s_wm_86798_cd40_boundary_hits;

static void wm_86798_dispatch_callback(void)
{
    u32 callback = wm_86798_lwu(WM_86798_CALLBACK);
#if !defined(WM_86798_MUTANT_SKIP_CALLBACK)
    if (callback == UINT32_C(0x80086700)) {
        wm_80086700_cd40();
        return;
    }
#endif
    if (callback != 0u) {
        s_wm_86798_cd40_boundary_hits++;
        fprintf(stderr,
                "[worldmap-86798] BOUNDARY: CD40 callback 0x%08x "
                "has no native; skipped\n", callback);
    }
}

static void wm_86798_prepare_shared_state(void)
{
    s32 pitch;
    s16 heading;
    s32 trig;

    memcpy(PSX_ADDR(WM_86798_MID_VERTICES),
           PSX_ADDR(WM_86798_MID_SOURCE), 48u * 8u);
    memcpy(PSX_ADDR(WM_86798_FAR_VERTICES),
           PSX_ADDR(WM_86798_FAR_SOURCE), 12u * 8u);
    memcpy(PSX_ADDR(WM_86798_UV_TABLE),
           PSX_ADDR(WM_86798_UV_SOURCE), 8u * 2u);

    wm_86798_sw(WM_86798_CAMERA_X,
                wm_86798_lwu(WM_86798_CAMERA_WORLD_X) &
                UINT32_C(0x01FFFFFF));
    wm_86798_sw(WM_86798_CAMERA_Z,
                wm_86798_lwu(WM_86798_CAMERA_WORLD_Z) &
                UINT32_C(0x01FFFFFF));
    wm_86798_sw(WM_86798_CAMERA_TILE_X,
                (u32)wm_86798_sra(wm_86798_lw(WM_86798_CAMERA_WORLD_X),
                                  12u) & UINT32_C(0x7FF));
#if defined(WM_86798_MUTANT_CAMERA_Z_OVERWRITES_X)
    wm_86798_sw(WM_86798_CAMERA_TILE_X,
#else
    wm_86798_sw(WM_86798_CAMERA_TILE_Z,
#endif
                (u32)wm_86798_sra(wm_86798_lw(WM_86798_CAMERA_WORLD_Z),
                                  12u) & UINT32_C(0x7FF));

    memcpy(PSX_ADDR(WM_86798_OBJECT_MATRIX),
           PSX_ADDR(WM_86798_BASE_MATRIX), 32u);
    memcpy(PSX_ADDR(WM_86798_Y_NEG_MATRIX),
           PSX_ADDR(WM_86798_OBJECT_MATRIX), 32u);
    memcpy(PSX_ADDR(WM_86798_Y_POS_MATRIX),
           PSX_ADDR(WM_86798_OBJECT_MATRIX), 32u);

    pitch = (s32)wm_86798_lh(WM_86798_ANGLE_X) + 0x400;
    if (pitch < 0)
        pitch += 7;
    pitch = wm_86798_sra(pitch, 3u);
    WM_86798_ROT_X(pitch, WM_86798_OBJECT_MATRIX);
    heading = wm_86798_lh(WM_86798_HEADING);
    WM_86798_ROT_Y(-(s32)heading, WM_86798_Y_NEG_MATRIX);
    WM_86798_ROT_Y((s32)heading, WM_86798_Y_POS_MATRIX);
    WM_86798_MUL(WM_86798_OBJECT_MATRIX, WM_86798_Y_NEG_MATRIX,
                 WM_86798_TEMP_MATRIX);
    WM_86798_MUL(WM_86798_Y_POS_MATRIX, WM_86798_TEMP_MATRIX,
                 WM_86798_OBJECT_MATRIX);

    wm_86798_sw(WM_86798_WEDGE_LEFT,
                ((u32)(u16)rsin((s32)heading - 0x169) << 16u) |
                (u32)(u16)rcos((s32)heading - 0x169));
    wm_86798_sw(WM_86798_WEDGE_RIGHT,
                ((u32)(u16)rsin((s32)heading + 0x169) << 16u) |
                (u32)(u16)rcos((s32)heading + 0x169));
    trig = wm_86798_neg((s32)rcos((s32)heading));
    wm_86798_sw(WM_86798_WEDGE_X,
                wm_86798_as_u32((s32)wm_86798_lh(WM_86798_OFFSET_X) * 2 +
                                 wm_86798_sra(trig, 1u)));
    trig = wm_86798_neg((s32)rsin((s32)heading));
    wm_86798_sw(WM_86798_WEDGE_Z,
                wm_86798_as_u32((s32)wm_86798_lh(WM_86798_OFFSET_Z) * 2 +
                                 wm_86798_sra(trig, 1u)));
}

static int wm_86798_inside_wedge(s32 x, s32 z)
{
    u32 point = wm_86798_pack_sxy(
        wm_86798_sub(x, wm_86798_lw(WM_86798_WEDGE_X)),
        wm_86798_sub(z, wm_86798_lw(WM_86798_WEDGE_Z)));
    s32 clip = (s32)WM_86798_NCLIP(
        point, wm_86798_lwu(WM_86798_WEDGE_RIGHT), 0u);
    wm_86798_sw(WM_86798_FLAG, wm_86798_as_u32(clip));
    if (clip > 0)
        return 0;
    clip = (s32)WM_86798_NCLIP(
        0u, wm_86798_lwu(WM_86798_WEDGE_LEFT), point);
    wm_86798_sw(WM_86798_FLAG, wm_86798_as_u32(clip));
    return clip <= 0;
}

static u16 wm_86798_project_origin(u32 *flag_out)
{
#if defined(WM_86798_TEST_TRACE)
    u16 depth = 0u;
    wm_86798_test_origin(WM_86798_CELL_V0, flag_out, &depth);
    return depth;
#else
    int xy = 0;
    long p = 0;
    long flag = 0;
    (void)RotTransPers((SVECTOR *)PSX_ADDR(WM_86798_CELL_V0),
                       &xy, &p, &flag);
    *flag_out = (u32)(unsigned long)flag;
    return (u16)C2_SZ3;
#endif
}

static int wm_86798_project_quad(u32 vertex, u32 flag_mask,
                                  u32 xy[4], u16 depth[4])
{
    u32 flag3 = 0u;
    u32 flag1 = 0u;
#if defined(WM_86798_TEST_TRACE)
    wm_86798_test_quad(vertex, xy, depth, &flag3, &flag1);
#else
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    int xy3 = 0;
    long p = 0;
    long flag = 0;
    (void)RotTransPers3((SVECTOR *)PSX_ADDR(vertex),
                        (SVECTOR *)PSX_ADDR(vertex + 0x08u),
                        (SVECTOR *)PSX_ADDR(vertex + 0x10u),
                        &xy0, &xy1, &xy2, &p, &flag);
    xy[0] = (u32)(unsigned long)xy0;
    xy[1] = (u32)(unsigned long)xy1;
    xy[2] = (u32)(unsigned long)xy2;
    flag3 = (u32)(unsigned long)flag;
    if ((flag3 & flag_mask) != 0u) {
        wm_86798_sw(WM_86798_FLAG, flag3);
        return 0;
    }
    (void)RotTransPers((SVECTOR *)PSX_ADDR(vertex + 0x18u),
                       &xy3, &p, &flag);
    xy[3] = (u32)xy3;
    flag1 = (u32)(unsigned long)flag;
    depth[0] = (u16)C2_SZ0;
    depth[1] = (u16)C2_SZ1;
    depth[2] = (u16)C2_SZ2;
    depth[3] = (u16)C2_SZ3;
#endif
    wm_86798_sw(WM_86798_FLAG, flag1);
    return ((flag3 | flag1) & flag_mask) == 0u;
}

static int wm_86798_project_bounds(u32 flag_mask)
{
    u32 flag1 = 0u;
    u32 flag3 = 0u;
#if defined(WM_86798_TEST_TRACE)
    wm_86798_test_bounds(WM_86798_CELL_V0, &flag1, &flag3);
#else
    int xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long xy3 = 0;
    long p = 0;
    long flag = 0;
    (void)RotTransPers((SVECTOR *)PSX_ADDR(WM_86798_CELL_V0),
                       &xy0, &p, &flag);
    flag1 = (u32)(unsigned long)flag;
    if ((flag1 & flag_mask) != 0u) {
        wm_86798_sw(WM_86798_FLAG, flag1);
        return 0;
    }
    (void)RotTransPers3((SVECTOR *)PSX_ADDR(WM_86798_CELL_V1),
                        (SVECTOR *)PSX_ADDR(WM_86798_CELL_V2),
                        (SVECTOR *)PSX_ADDR(WM_86798_CELL_V3),
                        &xy1, &xy2, &xy3, &p, &flag);
    flag3 = (u32)(unsigned long)flag;
#endif
    wm_86798_sw(WM_86798_FLAG, flag3);
    return ((flag1 | flag3) & flag_mask) == 0u;
}

static void wm_86798_publish(u32 packet, u16 uv, u16 extent,
                              const u32 xy[4], u16 depth)
{
    u32 draw_record = wm_86798_lwu(WM_86798_DRAW_RECORD);
    u32 ot = wm_86798_lwu(draw_record + 0x70u) +
             ((u32)depth >> 4u) * 4u;
    wm_86798_sw(packet,
                (wm_86798_lwu(packet) & UINT32_C(0x00FFFFFF)) |
                UINT32_C(0x09000000));
#if !defined(WM_86798_MUTANT_NO_PUBLICATION)
    PcPort_AddPrimDomainAware(PSX_ADDR(ot), PSX_ADDR(packet));
#else
    (void)ot;
#endif
    wm_86798_sw(packet + 0x08u, xy[0]);
    wm_86798_sw(packet + 0x10u, xy[1]);
    wm_86798_sw(packet + 0x18u, xy[2]);
    wm_86798_sw(packet + 0x20u, xy[3]);
    wm_86798_sh(packet + 0x0Cu, uv);
    wm_86798_sh(packet + 0x14u, (u16)(uv | extent));
    wm_86798_sh(packet + 0x1Cu, (u16)(uv | (u16)(extent << 8u)));
    wm_86798_sh(packet + 0x24u,
                (u16)(uv | extent | (u16)(extent << 8u)));
}

static u32 wm_86798_render_far(u32 packet, u32 *packet_count,
                               u32 *attempt_count, s32 uv_index)
    __attribute__((unused));
static u32 wm_86798_render_far(u32 packet, u32 *packet_count,
                                u32 *attempt_count, s32 uv_index)
{
    u32 tile;
    u32 vertex = WM_86798_FAR_VERTICES;
    for (tile = 0u; tile < 3u; tile++) {
        u32 xy[4] = {0u, 0u, 0u, 0u};
        u16 depth[4] = {0u, 0u, 0u, 0u};
        u16 uv = wm_86798_lhu(WM_86798_UV_TABLE + (u32)uv_index * 2u);
        u16 maximum;
        uv_index++;
        wm_86798_sw(WM_86798_UV_INDEX, (u32)uv_index);
        if (wm_86798_project_quad(
                vertex,
#if defined(WM_86798_MUTANT_FAR_STRICT_FLAG)
                WM_86798_FLAG_STRICT,
#else
                WM_86798_FLAG_SIGN,
#endif
                xy, depth) &&
            wm_86798_screen_overlap(xy)) {
            maximum = wm_86798_max_depth(depth);
            wm_86798_sw(WM_86798_MAX_DEPTH, maximum);
            if (maximum >= 3329u)
                break;
            wm_86798_publish(packet, uv, 0x3Fu, xy, maximum);
            packet += WM_86798_PACKET_STRIDE;
            (*packet_count)++;
            wm_86798_sw(WM_86798_PACKET_COUNT, *packet_count);
            if (maximum >= 2817u)
                break;
        }
        vertex += 0x20u;
        (*attempt_count)++;
        wm_86798_sw(WM_86798_ATTEMPT_COUNT, *attempt_count);
    }
    return packet;
}

static u32 wm_86798_render_mid(u32 packet, u32 *packet_count,
                                u32 *attempt_count, s32 uv_index)
{
    u32 group;
    u32 vertex = WM_86798_MID_VERTICES;
    for (group = 0u; group < 3u; group++) {
        u32 tile;
        u16 base = wm_86798_lhu(WM_86798_UV_TABLE +
                                (u32)(uv_index + (s32)group) * 2u);
        for (tile = 0u; tile < 4u; tile++) {
            u32 xy[4] = {0u, 0u, 0u, 0u};
            u16 depth[4] = {0u, 0u, 0u, 0u};
            u16 uv = (u16)(base |
#if defined(WM_86798_MUTANT_MID_UV_SHIFTS)
                           ((tile & 2u) << 11u) |
                           ((tile & 1u) << 4u));
#else
                           ((tile & 2u) << 12u) |
                           ((tile & 1u) << 5u));
#endif
            if (wm_86798_project_quad(vertex, WM_86798_FLAG_SIGN,
                                      xy, depth) &&
                wm_86798_screen_overlap(xy)) {
                u16 maximum = wm_86798_max_depth(depth);
                wm_86798_sw(WM_86798_MAX_DEPTH, maximum);
                wm_86798_publish(packet, uv, 0x1Fu, xy, maximum);
                packet += WM_86798_PACKET_STRIDE;
                (*packet_count)++;
                wm_86798_sw(WM_86798_PACKET_COUNT, *packet_count);
            }
            vertex += 0x20u;
            (*attempt_count)++;
            wm_86798_sw(WM_86798_ATTEMPT_COUNT, *attempt_count);
        }
    }
    return packet;
}

static void wm_86798_set_quad(s16 x0, s16 y, s16 z0, s16 x1, s16 z1)
{
    wm_86798_sh(WM_86798_CELL_V0 + 0u, (u16)x0);
    wm_86798_sh(WM_86798_CELL_V0 + 2u, (u16)y);
    wm_86798_sh(WM_86798_CELL_V0 + 4u, (u16)z0);
    wm_86798_sh(WM_86798_CELL_V1 + 0u, (u16)x1);
    wm_86798_sh(WM_86798_CELL_V1 + 2u, (u16)y);
    wm_86798_sh(WM_86798_CELL_V1 + 4u, (u16)z0);
    wm_86798_sh(WM_86798_CELL_V2 + 0u, (u16)x0);
    wm_86798_sh(WM_86798_CELL_V2 + 2u, (u16)y);
    wm_86798_sh(WM_86798_CELL_V2 + 4u, (u16)z1);
    wm_86798_sh(WM_86798_CELL_V3 + 0u, (u16)x1);
    wm_86798_sh(WM_86798_CELL_V3 + 2u, (u16)y);
    wm_86798_sh(WM_86798_CELL_V3 + 4u, (u16)z1);
}

static u32 wm_86798_render_near(u32 packet, u32 *packet_count,
                                 u32 *attempt_count, s32 uv_index)
{
    s16 source_x = wm_86798_lh(WM_86798_FAR_VERTICES + 0u);
    s16 source_y = wm_86798_lh(WM_86798_FAR_VERTICES + 2u);
    s16 source_z = wm_86798_lh(WM_86798_FAR_VERTICES + 4u);
    u32 group;
    for (group = 0u; group < 3u; group++) {
        s16 y = (s16)(u16)((u32)(u16)source_y - group * 8u);
        u32 row;
        wm_86798_set_quad(source_x, y, source_z,
                          (s16)(u16)((u32)(u16)source_x + 0x180u),
                          (s16)(u16)((u32)(u16)source_z - 0x180u));
        if (!wm_86798_project_bounds(WM_86798_FLAG_STRICT))
            continue;
        for (row = 0u; row < 4u; row++) {
            u32 column;
            for (column = 0u; column < 4u; column++) {
#if defined(WM_86798_MUTANT_NEAR_GRID_STEP_80)
                const u32 cell_step = 0x80u;
#else
                const u32 cell_step = 0x60u;
#endif
                s16 x0 = (s16)(u16)((u32)(u16)source_x +
                                    column * cell_step);
                s16 z0 = (s16)(u16)((u32)(u16)source_z -
                                    row * cell_step);
                u32 xy[4] = {0u, 0u, 0u, 0u};
                u16 depth[4] = {0u, 0u, 0u, 0u};
                u16 base = wm_86798_lhu(WM_86798_UV_TABLE +
                    (u32)(uv_index + (s32)group) * 2u);
                u16 uv = (u16)(base | (row << 12u) | (column << 4u));
                wm_86798_set_quad(
                    x0, y, z0,
                    (s16)(u16)((u32)(u16)x0 + cell_step),
                    (s16)(u16)((u32)(u16)z0 - cell_step));
                if (wm_86798_project_quad(WM_86798_CELL_V0,
                                          WM_86798_FLAG_STRICT,
                                          xy, depth) &&
                    wm_86798_screen_overlap(xy)) {
                    u16 maximum = wm_86798_max_depth(depth);
                    wm_86798_sw(WM_86798_MAX_DEPTH, maximum);
                    wm_86798_publish(packet, uv, 0x0Fu, xy, maximum);
                    packet += WM_86798_PACKET_STRIDE;
                    (*packet_count)++;
                    wm_86798_sw(WM_86798_PACKET_COUNT, *packet_count);
                }
                (*attempt_count)++;
                wm_86798_sw(WM_86798_ATTEMPT_COUNT, *attempt_count);
            }
        }
    }
    return packet;
}

void wm_80086798(void)
{
    u32 records;
    u32 auxiliary;
    u32 packet;
    u32 packet_count = 0u;
    u32 attempt_count = 0u;
    u32 record_index;

    wm_86798_dispatch_callback();
    wm_86798_prepare_shared_state();
    records = wm_86798_lwu(WM_86798_RECORD_ROOT);
    auxiliary = wm_86798_lwu(WM_86798_AUX_ROOT);
    packet = wm_86798_lwu(WM_86798_PACKET_ROOTS +
                          wm_86798_lwu(WM_86798_BUFFER_INDEX) * 4u);
    wm_86798_sw(WM_86798_PACKET_COUNT, 0u);
    wm_86798_sw(WM_86798_ATTEMPT_COUNT, 0u);

    for (record_index = 0u; record_index < WM_86798_RECORD_COUNT;
         record_index++) {
        s32 relative_x;
        s32 relative_z;
        u32 origin_flag;
        u16 origin_depth;
        s32 uv_index;
#if !defined(WM_86798_MUTANT_NO_PACKET_CEILING)
        if (packet_count >= WM_86798_PACKET_CEILING)
            break;
#endif
        relative_x = wm_86798_relative(
            wm_86798_lw(records + record_index * WM_86798_RECORD_STRIDE),
            wm_86798_lw(WM_86798_CAMERA_X));
        relative_z = wm_86798_neg(wm_86798_relative(
            wm_86798_lw(records + record_index * WM_86798_RECORD_STRIDE + 8u),
            wm_86798_lw(WM_86798_CAMERA_Z)));
        if (!wm_86798_inside_wedge(relative_x, relative_z))
            continue;

        wm_86798_sw(WM_86798_OBJECT_MATRIX + 0x14u,
                    wm_86798_as_u32(relative_x));
        wm_86798_sw(WM_86798_OBJECT_MATRIX + 0x18u,
                    wm_86798_as_u32(wm_86798_sra(
                        wm_86798_lw(records +
                                    record_index * WM_86798_RECORD_STRIDE + 4u),
                        12u)));
        wm_86798_sw(WM_86798_OBJECT_MATRIX + 0x1Cu,
                    wm_86798_as_u32(relative_z));
#if defined(WM_86798_MUTANT_REVERSE_COMPOSITION)
        WM_86798_COMP(WM_86798_OBJECT_MATRIX, WM_86798_CAMERA_MATRIX,
                      WM_86798_Y_NEG_MATRIX);
#else
        WM_86798_COMP(WM_86798_CAMERA_MATRIX, WM_86798_OBJECT_MATRIX,
                      WM_86798_Y_NEG_MATRIX);
#endif
        WM_86798_SET_ROT(WM_86798_Y_NEG_MATRIX);
        WM_86798_SET_TRANS(WM_86798_Y_NEG_MATRIX);

        wm_86798_sh(WM_86798_CELL_V0 + 0u, 0u);
        wm_86798_sh(WM_86798_CELL_V0 + 2u, 0u);
        wm_86798_sh(WM_86798_CELL_V0 + 4u, 0u);
        origin_depth = wm_86798_project_origin(&origin_flag);
        wm_86798_sw(WM_86798_FLAG, origin_flag);
        if ((origin_flag & WM_86798_FLAG_STRICT) != 0u)
            continue;
        wm_86798_sw(WM_86798_ORIGIN_DEPTH, origin_depth);

        uv_index = (s32)wm_86798_lh(
            auxiliary + record_index * WM_86798_AUX_STRIDE + 2u) * 4;
        wm_86798_sw(WM_86798_UV_INDEX, (u32)uv_index);
#if defined(WM_86798_MUTANT_SWAP_LOD)
        if (origin_depth >= 1409u)
            packet = wm_86798_render_near(packet, &packet_count,
                                          &attempt_count, uv_index);
#else
        if (origin_depth >= 1409u)
            packet = wm_86798_render_far(packet, &packet_count,
                                         &attempt_count, uv_index);
#endif
        else if (origin_depth >= 1025u)
            packet = wm_86798_render_mid(packet, &packet_count,
                                         &attempt_count, uv_index);
        else
            packet = wm_86798_render_near(packet, &packet_count,
                                          &attempt_count, uv_index);
    }
}
