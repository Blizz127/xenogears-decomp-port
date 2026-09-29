/*
 * World-map sky-quad submitter 0x800737EC.
 *
 * Retail boundary: [0x800737EC, 0x800739B8).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_737ec.h"
#include "world_map_helper_73b04.h"

#define WM_737EC_SCRATCH         0x1F800000u
#define WM_737EC_VERTEX_BASE     0x8009A280u
#define WM_737EC_HEADING         0x8009BD3Au
#define WM_737EC_CAMERA          0x8009C808u
#define WM_737EC_PACKET_BASE     0x8009D194u
#define WM_737EC_DRAW_RECORD     0x8009BE3Cu
#define WM_737EC_BUFFER_INDEX    0x8009D7F0u

#define WM_737EC_ROTATION        (WM_737EC_SCRATCH + 0x28u)
#define WM_737EC_COMPOSED        (WM_737EC_SCRATCH + 0x08u)
#define WM_737EC_P               (WM_737EC_SCRATCH + 0x48u)
#define WM_737EC_FLAG            (WM_737EC_SCRATCH + 0x4Cu)
#define WM_737EC_VERTEX_STRIDE   0x20u
#define WM_737EC_PACKET_STRIDE   0x48u
#define WM_737EC_BUFFER_STRIDE   0x24u
#define WM_737EC_QUAD_COUNT      4u

static u16 wm_737ec_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_737ec_lwu(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_737ec_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_737ec_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

#if !defined(WM_737EC_MUTANT_NO_DEPTH_SHIFT)
static s32 wm_737ec_bits_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 wm_737ec_s32_as_bits(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

/* Retail uses MIPS SRAV for the OT depth shift. */
static s32 wm_737ec_srav(s32 value, u32 shift)
{
    u32 amount = shift & 31u;
    u32 bits = wm_737ec_s32_as_bits(value);

    if (amount == 0u)
        return value;
    bits >>= amount;
    if (value < 0)
        bits |= UINT32_MAX << (32u - amount);
    return wm_737ec_bits_as_s32(bits);
}
#endif

void wm_800737EC(void)
{
    MATRIX *rotation = (MATRIX*)PSX_ADDR(WM_737EC_ROTATION);
    MATRIX *composed = (MATRIX*)PSX_ADDR(WM_737EC_COMPOSED);
    u32 packet_base;
    u32 quad_index;

    /* Retail builds a Y-axis heading matrix in scratchpad, then composes the
     * camera on the left.  The intermediate matrix has no translation. */
    wm_737ec_sh(WM_737EC_SCRATCH + 0u, 0u);
#if defined(WM_737EC_MUTANT_HEADING_Z)
    wm_737ec_sh(WM_737EC_SCRATCH + 2u, 0u);
    wm_737ec_sh(WM_737EC_SCRATCH + 4u, wm_737ec_lhu(WM_737EC_HEADING));
#else
    wm_737ec_sh(WM_737EC_SCRATCH + 2u, wm_737ec_lhu(WM_737EC_HEADING));
    wm_737ec_sh(WM_737EC_SCRATCH + 4u, 0u);
#endif
#if defined(WM_737EC_MUTANT_ROTATION_DEST_COMPOSED)
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_737EC_SCRATCH), composed);
#else
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_737EC_SCRATCH), rotation);
#endif
#if !defined(WM_737EC_MUTANT_KEEP_ROTATION_TRANSLATION)
    rotation->t[0] = 0;
    rotation->t[1] = 0;
    rotation->t[2] = 0;
#endif
#if defined(WM_737EC_MUTANT_REVERSE_COMPOSITION)
    (void)CompMatrix(rotation, (MATRIX*)PSX_ADDR(WM_737EC_CAMERA), composed);
#else
    (void)CompMatrix((MATRIX*)PSX_ADDR(WM_737EC_CAMERA), rotation, composed);
#endif
#if defined(WM_737EC_MUTANT_INSTALL_ROTATION)
    SetRotMatrix(rotation);
    SetTransMatrix(rotation);
#else
    SetRotMatrix(composed);
    SetTransMatrix(composed);
#endif

    packet_base = WM_737EC_PACKET_BASE +
#if defined(WM_737EC_MUTANT_PACKET_BUFFER_48)
                  wm_737ec_lwu(WM_737EC_BUFFER_INDEX) * 0x48u;
#else
                  wm_737ec_lwu(WM_737EC_BUFFER_INDEX) *
                      WM_737EC_BUFFER_STRIDE;
#endif

    for (quad_index = 0u;
#if defined(WM_737EC_MUTANT_THREE_QUADS)
         quad_index < 3u;
#else
         quad_index < WM_737EC_QUAD_COUNT;
#endif
         quad_index++) {
        u32 vertex = WM_737EC_VERTEX_BASE + quad_index *
#if defined(WM_737EC_MUTANT_VERTEX_STRIDE_28)
                     0x28u;
#else
                     WM_737EC_VERTEX_STRIDE;
#endif
        u32 packet = packet_base + quad_index *
#if defined(WM_737EC_MUTANT_PACKET_STRIDE_24)
                     0x24u;
#else
                     WM_737EC_PACKET_STRIDE;
#endif
        long xy0 = 0;
        long xy1 = 0;
        long xy2 = 0;
        long xy3 = 0;
        long p = 0;
        long flag = 0;
        s32 otz;

        otz = (s32)RotTransPers4(
            (SVECTOR*)PSX_ADDR(vertex + 0x00u),
            (SVECTOR*)PSX_ADDR(vertex + 0x08u),
            (SVECTOR*)PSX_ADDR(vertex + 0x10u),
            (SVECTOR*)PSX_ADDR(vertex + 0x18u),
            &xy0, &xy1, &xy2, &xy3, &p, &flag);

        /* The GTE writes XY before retail tests FLAG, so rejected quads also
         * receive their projected coordinates. */
        wm_737ec_sw(packet + 0x08u, (u32)(unsigned long)xy0);
        wm_737ec_sw(packet + 0x10u, (u32)(unsigned long)xy1);
        wm_737ec_sw(packet + 0x18u, (u32)(unsigned long)xy2);
        wm_737ec_sw(packet + 0x20u, (u32)(unsigned long)xy3);
        wm_737ec_sw(WM_737EC_P, (u32)(unsigned long)p);
        wm_737ec_sw(WM_737EC_FLAG, (u32)(unsigned long)flag);

#if defined(WM_737EC_MUTANT_ANY_FLAG_REJECTS)
        if (wm_737ec_lwu(WM_737EC_FLAG) != 0u)
            continue;
#elif !defined(WM_737EC_MUTANT_SKIP_FLAG_GATE)
        if ((wm_737ec_lwu(WM_737EC_FLAG) & UINT32_C(0x80000000)) != 0u)
            continue;
#endif
        {
            u32 draw_record = wm_737ec_lwu(WM_737EC_DRAW_RECORD);
            u32 ot_base = wm_737ec_lwu(draw_record + 0x70u);
            s32 bucket;
#if defined(WM_737EC_MUTANT_NO_DEPTH_SHIFT)
            bucket = otz;
#else
            bucket = wm_737ec_srav(otz, (u32)wm_73b04_ot_shift());
#endif
            PcPort_AddPrimDomainAware(
                PSX_ADDR(ot_base + (u32)bucket * 4u), PSX_ADDR(packet));
        }
    }
}
