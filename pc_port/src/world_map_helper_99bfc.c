/*
 * World-map wrapped-entry FT4 submitter 0x80099BFC.
 * Retail boundary: [0x80099BFC, 0x80099E88).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_99bfc.h"

#define WM_99BFC_SCRATCH       0x1F800000u
#define WM_99BFC_OUTPUT_COUNT  0x8009BE04u
#define WM_99BFC_CAMERA_X      0x8009BE28u
#define WM_99BFC_CAMERA_Z      0x8009BE30u
#define WM_99BFC_WRAP_X        0x8009D160u
#define WM_99BFC_WRAP_Z        0x8009D2B4u
#define WM_99BFC_PACKET_STRIDE 0x28u

static u16 wm_99bfc_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_99bfc_lwu(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 wm_99bfc_lw(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_99bfc_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_99bfc_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_99bfc_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_99bfc_bits_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_99bfc_s32_as_bits(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static s32 wm_99bfc_sra(s32 value, u32 amount)
{
    u32 bits = wm_99bfc_s32_as_bits(value);

    amount &= 31u;
    if (amount == 0u)
        return value;
    bits >>= amount;
    if (value < 0)
        bits |= UINT32_MAX << (32u - amount);
    return wm_99bfc_bits_as_s32(bits);
}

static s32 wm_99bfc_add(s32 left, s32 right)
{
    return wm_99bfc_bits_as_s32(wm_99bfc_s32_as_bits(left) +
                                wm_99bfc_s32_as_bits(right));
}

static s32 wm_99bfc_sub(s32 left, s32 right)
{
    return wm_99bfc_bits_as_s32(wm_99bfc_s32_as_bits(left) -
                                wm_99bfc_s32_as_bits(right));
}

static int wm_99bfc_screen_overlap(u32 xy0, u32 xy1, u32 xy2)
{
    u32 points[3] = { xy0, xy1, xy2 };
    u32 index;
    int x_visible = 0;
    int y_visible = 0;

    for (index = 0u; index < 3u; index++) {
        u32 x = points[index] & 0xFFFFu;
        u32 y = points[index] >> 16;
        if (x < 320u)
            x_visible = 1;
        if (y < 216u)
            y_visible = 1;
    }
    return x_visible != 0 && y_visible != 0;
}

void wm_80099BFC(u32 entries, s32 entry_count, u32 ot_base, u32 packet)
{
    u32 output_count = wm_99bfc_lwu(WM_99BFC_OUTPUT_COUNT);
    s32 camera_x = wm_99bfc_sra(wm_99bfc_lw(WM_99BFC_CAMERA_X), 12u);
    s32 camera_z = wm_99bfc_sra(wm_99bfc_lw(WM_99BFC_CAMERA_Z), 12u);
    s32 wrap_x = wm_99bfc_bits_as_s32(
        wm_99bfc_s32_as_bits(wm_99bfc_lw(WM_99BFC_WRAP_X)) << 11u);
    s32 wrap_z = wm_99bfc_bits_as_s32(
        wm_99bfc_s32_as_bits(wm_99bfc_lw(WM_99BFC_WRAP_Z)) << 11u);
    u32 remaining = (u32)entry_count;

#if defined(WM_99BFC_MUTANT_NO_PUBLICATION)
    (void)ot_base;
#endif

    while (remaining != 0u) {
        u32 packed;
        s32 x;
        s32 y;
        s32 z;
        SVECTOR position;
        VECTOR rotated;
        MATRIX *camera;
        MATRIX *model;
        long xy0 = 0;
        long xy1 = 0;
        long xy2 = 0;
        long p = 0;
        long flag = 0;
        int xy3 = 0;
        u16 depth;
        u32 shade;

#if defined(WM_99BFC_MUTANT_OUTPUT_LIMIT_256)
        if (output_count >= 256u)
#else
        if (output_count >= 512u)
#endif
            break;

        packed = wm_99bfc_lwu(entries);
        x = (s32)(s16)(u16)packed;
        y = (s32)(s16)(u16)(packed >> 16);
        z = (s32)wm_99bfc_lh(entries + 4u);
#if !defined(WM_99BFC_MUTANT_NO_CAMERA_SUBTRACT)
        x = wm_99bfc_sub(x, camera_x);
        z = wm_99bfc_sub(z, camera_z);
#else
        (void)camera_x;
        (void)camera_z;
#endif

        if (x < -0x4000)
            x = wm_99bfc_add(x, wrap_x);
        if (x >= 0x4000)
            x = wm_99bfc_sub(x, wrap_x);
        if (z < -0x4000)
            z = wm_99bfc_add(z, wrap_z);
        if (z >= 0x4000)
            z = wm_99bfc_sub(z, wrap_z);

        position.vx = (s16)(u16)x;
        position.vy = (s16)y;
#if defined(WM_99BFC_MUTANT_POSITIVE_Z)
        position.vz = (s16)(u16)z;
#else
        position.vz = (s16)(u16)(0u - (u32)z);
#endif
        position.pad = 0;

        camera = (MATRIX*)PSX_ADDR(WM_99BFC_SCRATCH + 0x28u);
        SetRotMatrix(camera);
        (void)ApplyRotMatrix(&position, &rotated);
        model = (MATRIX*)PSX_ADDR(WM_99BFC_SCRATCH + 0x48u);
        model->t[0] = wm_99bfc_add(rotated.vx, camera->t[0]);
        model->t[1] = wm_99bfc_add(rotated.vy, camera->t[1]);
        model->t[2] = wm_99bfc_add(rotated.vz, camera->t[2]);
        SetRotMatrix(model);
        SetTransMatrix(model);

        (void)RotTransPers3(
            (SVECTOR*)PSX_ADDR(WM_99BFC_SCRATCH + 0x00u),
            (SVECTOR*)PSX_ADDR(WM_99BFC_SCRATCH + 0x08u),
            (SVECTOR*)PSX_ADDR(WM_99BFC_SCRATCH + 0x10u),
            &xy0, &xy1, &xy2, &p, &flag);
#if !defined(WM_99BFC_MUTANT_SKIP_FLAG_GATE)
        if (((u32)(unsigned long)flag & UINT32_C(0x80000000)) != 0u)
            goto next_entry;
#endif
#if defined(WM_99BFC_MUTANT_SKIP_SCREEN_GATE)
        (void)wm_99bfc_screen_overlap;
#else
        if (!wm_99bfc_screen_overlap((u32)(unsigned long)xy0,
                                     (u32)(unsigned long)xy1,
                                     (u32)(unsigned long)xy2))
            goto next_entry;
#endif
        depth = (u16)C2_SZ3;
#if defined(WM_99BFC_MUTANT_DEPTH_0C00)
        if (depth >= 0x0C00u)
#else
        if (depth >= 0x0E00u)
#endif
            goto next_entry;

        (void)RotTransPers((SVECTOR*)PSX_ADDR(WM_99BFC_SCRATCH + 0x18u),
                           &xy3, &p, &flag);
        shade = (u32)(u16)C2_IR0;
#if !defined(WM_99BFC_MUTANT_NO_SHADE_CLAMP)
        if (shade >= 4096u)
            shade = 4095u;
#endif

        wm_99bfc_sw(packet + 0x08u, (u32)(unsigned long)xy0);
        wm_99bfc_sw(packet + 0x10u, (u32)(unsigned long)xy1);
        wm_99bfc_sw(packet + 0x18u, (u32)(unsigned long)xy2);
        wm_99bfc_sw(packet + 0x20u, (u32)xy3);
        wm_99bfc_sh(packet + 0x0Eu,
                    wm_99bfc_lhu(WM_99BFC_SCRATCH + 0x68u +
                                  (shade >> 8u) * 2u));
        wm_99bfc_sw(packet, UINT32_C(0x09000000));
#if !defined(WM_99BFC_MUTANT_NO_PUBLICATION)
        PcPort_AddPrimDomainAware(
            PSX_ADDR(ot_base + ((u32)depth >> 4u) * 4u),
            PSX_ADDR(packet));
#endif
        packet +=
#if defined(WM_99BFC_MUTANT_PACKET_STRIDE_24)
            0x24u;
#else
            WM_99BFC_PACKET_STRIDE;
#endif
        output_count++;

next_entry:
        entries += 8u;
        remaining--;
    }

    wm_99bfc_sw(WM_99BFC_OUTPUT_COUNT, output_count);
}
