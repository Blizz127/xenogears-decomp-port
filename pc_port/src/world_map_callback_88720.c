/*
 * World-map scheduler callback 0x80088720 -- STOPGAP hand body.
 *
 * Retail 0x80088720 is not byte-exact C yet (open in
 * docs/port/RETAIL_DIVERGENCES.md); this is a transcription of its 264
 * instructions so the scheduler stream can run.  Replace it with the matched
 * C when it lands.
 *
 * Per frame it advances the pool slot's two 12-bit angles (+0x50 += +0x54,
 * +0x58 += +0x5C, both & 0xFFF), builds two Y-rotation matrices from them in
 * the scratchpad (SVECTOR (0, a50, 0) at 0x1F8000A0 -> MATRIX 0x1F8000F0,
 * SVECTOR (0, a58, 0) at 0x1F8000A8 -> MATRIX 0x1F800110) and copies them
 * into the C620 record
 * selected by D_8009B688[D_8009C610] (stride 0x54): matrix 0 to +0x1C4,
 * matrix 1 to +0x11C, then cascades +0x11C -> +0xC8 -> +0x74 -> +0x20.  It
 * then takes the wrapped offset of the slot position from the reference
 * point g_GameState+0x182C..0x1830; if that is within 0x300 horizontally and
 * its Y is below -0x240 the slot's move flag (+0x4A) is cleared, otherwise
 * set to 1.  The slot moves by flag * (+0x38, +0x40) in X/Z, is wrapped
 * (0x80093354), its integer position is published to the record's +0x3F8
 * vector and pushed to the 0x8008BFD4 ring buffer, and the position is
 * written back to g_GameState+0x184C..0x1852.  Returns 1.
 *
 * g_GameState goes through world_map_gamestate.h (host copy authoritative,
 * guest copy kept equal).  The scratchpad follows the world-map convention
 * PSX_ADDR(0x1F80xxxx).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_gamestate.h"
#include "world_map_helper_8bfd4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93534.h"

#include <libgte.h> /* RotMatrixYXZ, SquareRoot0 */

#define WM_88720_POOL_PTR    0x8009BE24u
#define WM_88720_REC_SEL     0x8009C610u /* D_8009C610: index into B688 */
#define WM_88720_REC_TABLE   0x8009B688u /* D_8009B688: u16 record numbers */
#define WM_88720_CONTEXT_PTR 0x8009C620u /* D_8009C620: record array base */
#define WM_88720_REC_STRIDE  0x54u
#define WM_88720_GS(off)     (WM_GS_BASE + (off))
#define WM_88720_SPAD(off)   (0x1F800000u + (off))

static u32 wm_88720_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static s16 wm_88720_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 wm_88720_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void wm_88720_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void wm_88720_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

/* Word copy in retail order (sources and destinations never overlap). */
static void wm_88720_copy_words(u32 dst, u32 src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++)
        wm_88720_sw(dst + i * 4u, wm_88720_lw(src + i * 4u));
}

s32 wm_80088720(s32 slot_index)
{
    u32 slot;
    u32 rec_no;
    u32 rec;
    u32 v;
    u32 x;
    u32 z;
    u32 dx;
    u32 dz;
    s32 dist;
    s16 flag;
    u32 w0, w1, w2, w3;

    slot = wm_88720_lw(WM_88720_POOL_PTR) + ((u32)slot_index << 7);
    rec_no = wm_88720_lhu(WM_88720_REC_TABLE +
                          (wm_88720_lw(WM_88720_REC_SEL) << 1));

    wm_88720_sw(slot + 0x50u,
                (wm_88720_lw(slot + 0x50u) + wm_88720_lw(slot + 0x54u)) & 0xFFFu);
    wm_88720_sw(slot + 0x58u,
                (wm_88720_lw(slot + 0x58u) + wm_88720_lw(slot + 0x5Cu)) & 0xFFFu);

    wm_88720_sh(WM_88720_SPAD(0xA4u), 0u);
    wm_88720_sh(WM_88720_SPAD(0xA0u), 0u);
    v = wm_88720_lw(slot + 0x50u);
    wm_88720_sh(WM_88720_SPAD(0xACu), 0u);
    wm_88720_sh(WM_88720_SPAD(0xA8u), 0u);
    wm_88720_sh(WM_88720_SPAD(0xA2u), (u16)v);
    wm_88720_sh(WM_88720_SPAD(0xAAu), (u16)wm_88720_lw(slot + 0x58u));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_88720_SPAD(0xA0u)),
                       (MATRIX*)PSX_ADDR(WM_88720_SPAD(0xF0u)));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_88720_SPAD(0xA8u)),
                       (MATRIX*)PSX_ADDR(WM_88720_SPAD(0x110u)));

    /* Matrix 0 -> record +0x1C4 (8 words). */
    rec = rec_no * WM_88720_REC_STRIDE + wm_88720_lw(WM_88720_CONTEXT_PTR);
    wm_88720_copy_words(rec + 0x1C4u, WM_88720_SPAD(0xF0u), 8u);

    /* Matrix 1 -> +0x11C, then cascade +0x11C -> +0xC8 -> +0x74 -> +0x20
     * (C620 reloaded after the first copy, as retail does). */
    rec = rec_no * WM_88720_REC_STRIDE + wm_88720_lw(WM_88720_CONTEXT_PTR);
    wm_88720_copy_words(rec + 0x11Cu, WM_88720_SPAD(0x110u), 8u);
    wm_88720_copy_words(rec + 0xC8u, rec + 0x11Cu, 8u);
    wm_88720_copy_words(rec + 0x74u, rec + 0xC8u, 8u);
    wm_88720_copy_words(rec + 0x20u, rec + 0x74u, 8u);

    /* Offset of the slot from the reference point, wrapped. */
    x = wm_88720_lw(slot + 0x28u);
    wm_88720_sw(WM_88720_SPAD(0x0u),
                (u32)wm_gs_lhu(WM_88720_GS(0x182Cu)) - (u32)((s32)x >> 12));
    z = wm_88720_lw(slot + 0x30u);
    wm_88720_sw(WM_88720_SPAD(0x4u),
                (u32)(s32)(s16)wm_gs_lhu(WM_88720_GS(0x182Eu)));
    wm_88720_sw(WM_88720_SPAD(0x8u),
                (u32)wm_gs_lhu(WM_88720_GS(0x1830u)) - (u32)((s32)z >> 12));
    wm_80093534(WM_88720_SPAD(0x0u));

    dx = wm_88720_lw(WM_88720_SPAD(0x0u));
    dz = wm_88720_lw(WM_88720_SPAD(0x8u));
    dist = (s32)SquareRoot0((int)(dx * dx + dz * dz));
    if (dist < 0x300 && (s32)wm_88720_lw(WM_88720_SPAD(0x4u)) < -0x240)
        wm_88720_sh(slot + 0x4Au, 0u);
    else
        wm_88720_sh(slot + 0x4Au, 1u);

    /* Move by flag * velocity, then wrap. */
    flag = wm_88720_lh(slot + 0x4Au);
    dx = wm_88720_lw(slot + 0x38u) * (u32)(s32)flag;
    flag = wm_88720_lh(slot + 0x4Au);
    dz = wm_88720_lw(slot + 0x40u) * (u32)(s32)flag;
    x = wm_88720_lw(slot + 0x28u);
    z = wm_88720_lw(slot + 0x30u);
    wm_88720_sw(slot + 0x28u, x + dx);
    wm_88720_sw(slot + 0x30u, z + dz);
    wm_80093354(slot + 0x28u);

    /* Publish the integer position into the record's +0x3F8 vector. */
    wm_88720_sw(WM_88720_SPAD(0x0u), (u32)((s32)wm_88720_lw(slot + 0x28u) >> 12));
    wm_88720_sw(WM_88720_SPAD(0x4u), (u32)((s32)wm_88720_lw(slot + 0x2Cu) >> 12));
    v = wm_88720_lw(slot + 0x30u);
    rec = wm_88720_lw(WM_88720_CONTEXT_PTR);
    wm_88720_sw(WM_88720_SPAD(0x8u), (u32)((s32)v >> 12));
    rec = rec_no * WM_88720_REC_STRIDE + rec;
    w0 = wm_88720_lw(WM_88720_SPAD(0x0u));
    w1 = wm_88720_lw(WM_88720_SPAD(0x4u));
    w2 = wm_88720_lw(WM_88720_SPAD(0x8u));
    w3 = wm_88720_lw(WM_88720_SPAD(0xCu));
    wm_88720_sw(rec + 0x3F8u, w0);
    wm_88720_sw(rec + 0x3FCu, w1);
    wm_88720_sw(rec + 0x400u, w2);
    wm_88720_sw(rec + 0x404u, w3);

    wm_8008BFD4((u16)slot_index, slot + 0x28u, 0x180u, 0xC0);

    wm_gs_sh(WM_88720_GS(0x184Cu), (u16)wm_88720_lw(slot + 0x28u));
    wm_gs_sh(WM_88720_GS(0x184Eu), (u16)((s32)wm_88720_lw(slot + 0x28u) >> 12));
    wm_gs_sh(WM_88720_GS(0x1850u), (u16)wm_88720_lw(slot + 0x30u));
    wm_gs_sh(WM_88720_GS(0x1852u), (u16)((s32)wm_88720_lw(slot + 0x30u) >> 12));
    return 1;
}
