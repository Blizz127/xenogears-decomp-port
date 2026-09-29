/*
 * World-map queued ground-object renderer 0x800747DC.
 *
 * Retail boundary: [0x800747DC, 0x80074E58), 0x67C bytes / 415
 * instructions.  D_8009BE38 counts compact 8-byte placement records rooted
 * at D_8009D30C.  Each record is terrain-aligned, transformed through the
 * live camera, projected as a four-vertex POLY_FT4, and conditionally linked
 * into the active guest OT.  Retail consumes the queue by clearing BE38.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_747dc.h"
#include "world_map_terrain_normal.h"
#include "world_map_terrain_sampler.h"

/* PsyCross implements these executable-resident PsyQ helpers in
 * pc_port/src/psyq_compat.c; its public shim does not declare them. */
extern void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output);
extern long VectorNormal(VECTOR *input, VECTOR *output);

#define WM_747DC_SCRATCH       0x1F800000u
#define WM_747DC_CAMERA        0x8009C808u
#define WM_747DC_BASE_MATRIX   0x8009A180u
#define WM_747DC_QUEUE_COUNT   0x8009BE38u
#define WM_747DC_QUEUE_ROOT    0x8009D30Cu
#define WM_747DC_PACKET_ROOTS  0x8009BE14u
#define WM_747DC_BUFFER_INDEX  0x8009D7F0u
#define WM_747DC_CAMERA_X      0x8009BE28u
#define WM_747DC_CAMERA_Z      0x8009BE30u
#define WM_747DC_DRAW_RECORD   0x8009BE3Cu
#define WM_747DC_MODE2_ANGLE   0x8006EE66u

#define WM_747DC_SC_NORMAL     (WM_747DC_SCRATCH + 0x030u)
#define WM_747DC_SC_AXIS       (WM_747DC_SCRATCH + 0x040u)
#define WM_747DC_SC_CROSS      (WM_747DC_SCRATCH + 0x050u)
#define WM_747DC_SC_TANGENT    (WM_747DC_SCRATCH + 0x060u)
#define WM_747DC_SC_SCALE      (WM_747DC_SCRATCH + 0x080u)
#define WM_747DC_SC_VERTEX0    (WM_747DC_SCRATCH + 0x0A0u)
#define WM_747DC_SC_VERTEX1    (WM_747DC_SCRATCH + 0x0A8u)
#define WM_747DC_SC_VERTEX2    (WM_747DC_SCRATCH + 0x0B0u)
#define WM_747DC_SC_VERTEX3    (WM_747DC_SCRATCH + 0x0D8u)
#define WM_747DC_SC_BASIS      (WM_747DC_SCRATCH + 0x0F0u)
#define WM_747DC_SC_POSITION   (WM_747DC_SCRATCH + 0x104u)
#define WM_747DC_SC_COMPOSED   (WM_747DC_SCRATCH + 0x110u)
#define WM_747DC_SC_TRANSLATED (WM_747DC_SCRATCH + 0x124u)
#define WM_747DC_SC_CAMERA     (WM_747DC_SCRATCH + 0x130u)
#define WM_747DC_SC_MODE2      (WM_747DC_SCRATCH + 0x150u)

#define WM_747DC_RECORD_STRIDE 0x08u
#define WM_747DC_PACKET_STRIDE 0x28u

static u16 wm_747dc_lhu(u32 address) __attribute__((unused));
static u16 wm_747dc_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_747dc_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_747dc_lwu(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm_747dc_lw(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_747dc_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_747dc_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_747dc_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_747dc_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 wm_747dc_sra(s32 value, u32 amount)
{
    u32 bits = wm_747dc_as_u32(value);

    amount &= 31u;
    if (amount == 0u)
        return value;
    bits >>= amount;
    if (value < 0)
        bits |= UINT32_MAX << (32u - amount);
    return wm_747dc_as_s32(bits);
}

static s32 wm_747dc_sub(s32 left, s32 right) __attribute__((unused));
static s32 wm_747dc_sub(s32 left, s32 right)
{
    return wm_747dc_as_s32(wm_747dc_as_u32(left) -
                           wm_747dc_as_u32(right));
}

static s32 wm_747dc_s16_shift12(s16 value)
{
    return wm_747dc_as_s32((u32)(s32)value << 12u);
}

static void wm_747dc_initialize_vertices(void)
{
    /* Retail writes x/y/z only; the four SVECTOR pad halfwords are not
     * touched.  The vertex order is (-,+), (+,+), (-,-), (+,-). */
    wm_747dc_sh(WM_747DC_SC_VERTEX0 + 0u, (u16)(s16)-16);
    wm_747dc_sh(WM_747DC_SC_VERTEX0 + 2u, 0u);
    wm_747dc_sh(WM_747DC_SC_VERTEX0 + 4u, 16u);
    wm_747dc_sh(WM_747DC_SC_VERTEX1 + 0u, 16u);
    wm_747dc_sh(WM_747DC_SC_VERTEX1 + 2u, 0u);
    wm_747dc_sh(WM_747DC_SC_VERTEX1 + 4u, 16u);
    wm_747dc_sh(WM_747DC_SC_VERTEX2 + 0u, (u16)(s16)-16);
    wm_747dc_sh(WM_747DC_SC_VERTEX2 + 2u, 0u);
    wm_747dc_sh(WM_747DC_SC_VERTEX2 + 4u, (u16)(s16)-16);
    wm_747dc_sh(WM_747DC_SC_VERTEX3 + 0u, 16u);
    wm_747dc_sh(WM_747DC_SC_VERTEX3 + 2u, 0u);
    wm_747dc_sh(WM_747DC_SC_VERTEX3 + 4u, (u16)(s16)-16);
}

static void wm_747dc_build_basis(void)
{
    VECTOR *axis = (VECTOR*)PSX_ADDR(WM_747DC_SC_AXIS);
    VECTOR *normal = (VECTOR*)PSX_ADDR(WM_747DC_SC_NORMAL);
    VECTOR *cross = (VECTOR*)PSX_ADDR(WM_747DC_SC_CROSS);
    VECTOR *tangent = (VECTOR*)PSX_ADDR(WM_747DC_SC_TANGENT);
    MATRIX *basis = (MATRIX*)PSX_ADDR(WM_747DC_SC_BASIS);

    OuterProduct12(axis, normal, cross);
    (void)VectorNormal(cross, tangent);
    OuterProduct12(
#if defined(WM_747DC_MUTANT_REVERSE_SECOND_CROSS)
        tangent, normal,
#else
        normal, tangent,
#endif
        (VECTOR*)PSX_ADDR(WM_747DC_SC_SCALE));
    (void)VectorNormal((VECTOR*)PSX_ADDR(WM_747DC_SC_SCALE), cross);

    /* Retail copies the low halfword of each 32-bit VECTOR component into
     * the 3x3 MATRIX rows in this exact order. */
    basis->m[0][0] = (s16)tangent->vx;
    basis->m[0][1] = (s16)tangent->vy;
    basis->m[0][2] = (s16)tangent->vz;
    basis->m[1][0] = (s16)normal->vx;
    basis->m[1][1] = (s16)normal->vy;
    basis->m[1][2] = (s16)normal->vz;
    basis->m[2][0] = (s16)cross->vx;
    basis->m[2][1] = (s16)cross->vy;
    basis->m[2][2] = (s16)cross->vz;
}

static void wm_747dc_apply_mode(s16 mode)
{
    MATRIX *basis = (MATRIX*)PSX_ADDR(WM_747DC_SC_BASIS);
    VECTOR *scale = (VECTOR*)PSX_ADDR(WM_747DC_SC_SCALE);

    if (mode == 1) {
        scale->vx = 0x1800;
        scale->vy = 0x1800;
        scale->vz = 0x1800;
        (void)ScaleMatrix(basis, scale);
    } else if (mode == 2) {
        MATRIX *rotation = (MATRIX*)PSX_ADDR(WM_747DC_SC_MODE2);

        memcpy(rotation, PSX_ADDR(WM_747DC_BASE_MATRIX), sizeof(*rotation));
#if !defined(WM_747DC_MUTANT_SKIP_MODE2_ROTATION)
        (void)RotMatrixY((int)wm_747dc_lhu(WM_747DC_MODE2_ANGLE), rotation);
        (void)MulMatrix(basis, rotation);
#endif
        scale->vx = 0x1800;
        scale->vy = 0x1000;
        scale->vz = 0x4800;
        (void)ScaleMatrix(basis, scale);
    }
}

static void wm_747dc_compose_record_transform(s32 world_x, s32 world_y,
                                               s32 world_z)
{
    MATRIX *camera = (MATRIX*)PSX_ADDR(WM_747DC_SC_CAMERA);
    MATRIX *basis = (MATRIX*)PSX_ADDR(WM_747DC_SC_BASIS);
    MATRIX *composed = (MATRIX*)PSX_ADDR(WM_747DC_SC_COMPOSED);
    SVECTOR *position = (SVECTOR*)PSX_ADDR(WM_747DC_SC_POSITION);
    VECTOR *translated = (VECTOR*)PSX_ADDR(WM_747DC_SC_TRANSLATED);
    long dead_flag = 0;
    s32 camera_x = wm_747dc_lw(WM_747DC_CAMERA_X);
    s32 camera_z = wm_747dc_lw(WM_747DC_CAMERA_Z);

#if defined(WM_747DC_MUTANT_NO_CAMERA_SUBTRACT)
    (void)camera_x;
    (void)camera_z;
    position->vx = (s16)wm_747dc_sra(world_x, 12u);
    position->vz = (s16)wm_747dc_sra(world_z, 12u);
#else
    position->vx = (s16)wm_747dc_sra(wm_747dc_sub(world_x, camera_x), 12u);
    position->vz = (s16)wm_747dc_sra(wm_747dc_sub(camera_z, world_z), 12u);
#endif
    position->vy = (s16)wm_747dc_sra(world_y, 12u);

    /* Retail multiplies each basis column by camera R via MVMVA. */
    (void)MulMatrix0(camera, basis, composed);

    /* The placement vector is transformed by camera R and camera T; those
     * MAC results become the composed matrix translation. */
    SetRotMatrix(camera);
    SetTransMatrix(camera);
    RotTrans(position, translated, &dead_flag);
    composed->t[0] = translated->vx;
    composed->t[1] = translated->vy;
    composed->t[2] = translated->vz;
    SetRotMatrix(composed);
    SetTransMatrix(composed);
}

static int wm_747dc_project_and_publish(u32 packet)
{
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long p = 0;
    long flag = 0;
    int xy3 = 0;
    u16 depth0;
    u16 depth1;
    u16 depth2;
    u16 depth3;
    u16 minimum;
    u32 draw_record;
    u32 ot_base;

    (void)RotTransPers3(
        (SVECTOR*)PSX_ADDR(WM_747DC_SC_VERTEX0),
        (SVECTOR*)PSX_ADDR(WM_747DC_SC_VERTEX1),
#if defined(WM_747DC_MUTANT_WRONG_VERTEX2)
        (SVECTOR*)PSX_ADDR(WM_747DC_SC_VERTEX3),
#else
        (SVECTOR*)PSX_ADDR(WM_747DC_SC_VERTEX2),
#endif
        &xy0, &xy1, &xy2, &p, &flag);

#if !defined(WM_747DC_MUTANT_SKIP_FLAG_GATE)
    if (((u32)(unsigned long)flag & UINT32_C(0x80000000)) != 0u)
        return 0;
#endif

    depth0 = (u16)C2_SZ1;
    depth1 = (u16)C2_SZ2;
    depth2 = (u16)C2_SZ3;
    wm_747dc_sw(packet + 0x08u, (u32)(unsigned long)xy0);
    wm_747dc_sw(packet + 0x10u, (u32)(unsigned long)xy1);
    wm_747dc_sw(packet + 0x18u, (u32)(unsigned long)xy2);

    (void)RotTransPers((SVECTOR*)PSX_ADDR(WM_747DC_SC_VERTEX3),
                       &xy3, &p, &flag);
    depth3 = (u16)C2_SZ3;
    wm_747dc_sw(packet + 0x20u, (u32)xy3);

#if defined(WM_747DC_MUTANT_MAXIMUM_DEPTH)
    minimum = depth0;
    if (depth1 > minimum)
        minimum = depth1;
    if (depth2 > minimum)
        minimum = depth2;
    if (depth3 > minimum)
        minimum = depth3;
#else
    minimum = depth0;
    if (depth1 < minimum)
        minimum = depth1;
    if (depth2 < minimum)
        minimum = depth2;
    if (depth3 < minimum)
        minimum = depth3;
#endif

#if defined(WM_747DC_MUTANT_DEPTH_LIMIT_0C00)
    if (minimum >= 0x0C00u)
#else
    if (minimum >= 0x1000u)
#endif
        return 0;

    draw_record = wm_747dc_lwu(WM_747DC_DRAW_RECORD);
    ot_base = wm_747dc_lwu(draw_record + 0x70u);
#if !defined(WM_747DC_MUTANT_NO_PUBLICATION)
    PcPort_AddPrimDomainAware(
        PSX_ADDR(ot_base + ((u32)minimum >> 4u) * 4u), PSX_ADDR(packet));
#else
    (void)ot_base;
#endif
    return 1;
}

void wm_800747DC(void)
{
    u32 count = wm_747dc_lwu(WM_747DC_QUEUE_COUNT);
    u32 record;
    u32 packet;

    if (count == 0u)
        return;

    wm_747dc_initialize_vertices();
    memcpy(PSX_ADDR(WM_747DC_SC_CAMERA), PSX_ADDR(WM_747DC_CAMERA),
           sizeof(MATRIX));
    wm_747dc_sw(WM_747DC_SC_AXIS + 0u, 0u);
    wm_747dc_sw(WM_747DC_SC_AXIS + 4u, 0u);
    wm_747dc_sw(WM_747DC_SC_AXIS + 8u, 0x1000u);

#if defined(WM_747DC_MUTANT_FIXED_QUEUE)
    record = 0x8009D30Cu;
#else
    record = wm_747dc_lwu(WM_747DC_QUEUE_ROOT);
#endif
    packet = wm_747dc_lwu(WM_747DC_PACKET_ROOTS +
                           wm_747dc_lwu(WM_747DC_BUFFER_INDEX) * 4u);

#if defined(WM_747DC_MUTANT_DROP_LAST_RECORD)
    count--;
#endif
    while (count != 0u) {
        s32 world_x = wm_747dc_s16_shift12(wm_747dc_lh(record + 0u));
        s32 world_z = wm_747dc_s16_shift12(wm_747dc_lh(record + 4u));
        s32 world_y = wm_80093978(world_x, world_z);
        s16 mode = wm_747dc_lh(record + 2u);

#if defined(WM_747DC_MUTANT_TERRAIN_Z_FROM_X)
        wm_80093740(WM_747DC_SC_NORMAL, world_x, world_x);
#else
        wm_80093740(WM_747DC_SC_NORMAL, world_x, world_z);
#endif
        wm_747dc_build_basis();

#if defined(WM_747DC_MUTANT_MODE1_SCALE_1000)
        if (mode == 1)
            mode = 0;
#endif
        wm_747dc_apply_mode(mode);
        wm_747dc_compose_record_transform(world_x, world_y, world_z);
        if (wm_747dc_project_and_publish(packet) != 0)
            packet +=
#if defined(WM_747DC_MUTANT_PACKET_STRIDE_20)
                0x20u;
#else
                WM_747DC_PACKET_STRIDE;
#endif

        record +=
#if defined(WM_747DC_MUTANT_RECORD_STRIDE_10)
            0x10u;
#else
            WM_747DC_RECORD_STRIDE;
#endif
        count--;
    }

#if !defined(WM_747DC_MUTANT_NO_QUEUE_CLEAR)
    wm_747dc_sw(WM_747DC_QUEUE_COUNT, 0u);
#endif
}
