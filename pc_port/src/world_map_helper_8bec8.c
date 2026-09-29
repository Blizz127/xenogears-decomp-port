/*
 * World-map axis approach stepper 0x8008BEC8.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x8008BEC8, 0x8008BFD4).  See world_map_helper_8bec8.h.
 *
 * step/2 is the exact retail rounding-toward-zero sequence:
 * sign16(step); add (sign bit srl 31); sra 1.  The vel*half product is
 * mult/mflo (low word).  The close threshold is slti d, 5 on the
 * wrapped absolute difference.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_8bec8.h"

extern s32 wm_80093978(s32 x, s32 z);

#if defined(WM_8BEC8_TEST_TRACE)
extern void wm_8bec8_test_93354(u32 vec_addr);
extern s32 wm_8bec8_test_93978(s32 x, s32 z);
#define WM_8BEC8_CALL_93354(a)    wm_8bec8_test_93354(a)
#define WM_8BEC8_CALL_93978(x, z) wm_8bec8_test_93978((x), (z))
#else
#define WM_8BEC8_CALL_93354(a)    wm_80093354(a)
#define WM_8BEC8_CALL_93978(x, z) wm_80093978((x), (z))
#endif

static u32 wm_8bec8_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static u16 wm_8bec8_load_u16(u32 addr)
{
    u16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_8bec8_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static s32 wm_8bec8_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_8bec8_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA. */
static u32 wm_8bec8_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

/* One axis: returns 1 when close, else steps the position in place. */
static int wm_8bec8_axis(u32 obj, u32 pos_off, u32 tgt_off, u32 vel_off)
{
    u32 pos = wm_8bec8_load_u32(obj + pos_off);
    u32 tgt = wm_8bec8_load_u32(obj + tgt_off);
    u32 d = tgt - wm_8bec8_sra(pos, 12u);
    u32 half;
    u32 delta;

#if !defined(WM_8BEC8_MUTANT_MISSING_ABS)
    if (wm_8bec8_as_s32(d) < 0)
        d = 0u - d; /* negu wrap */
#endif
#if defined(WM_8BEC8_MUTANT_WRONG_CLOSE_THRESH)
    if (wm_8bec8_as_s32(d) < 6)
#else
    if (wm_8bec8_as_s32(d) < 5)
#endif
        return 1;

    {
        /* lhu +0x4A; sll16/sra16 sign; add sign bit (srl 31); sra 1. */
        u32 raw = (u32)wm_8bec8_load_u16(obj + 0x4Au) << 16;
        u32 signed16 = wm_8bec8_sra(raw, 16u);

#if defined(WM_8BEC8_MUTANT_WRONG_STEP_ROUND)
        half = wm_8bec8_sra(signed16, 1u);
#else
        half = wm_8bec8_sra(signed16 + (raw >> 31), 1u);
#endif
    }
    /* mult/mflo low word. */
    delta = wm_8bec8_load_u32(obj + vel_off) * half;
    wm_8bec8_store_u32(obj + pos_off, pos + delta);
    return 0;
}

s32 wm_8008BEC8(u32 obj)
{
    s32 mask = 0;

#if defined(WM_8BEC8_MUTANT_SWAPPED_AXES)
    if (wm_8bec8_axis(obj, 0x30u, 0x50u, 0x38u))
#else
    if (wm_8bec8_axis(obj, 0x28u, 0x50u, 0x38u))
#endif
#if defined(WM_8BEC8_MUTANT_MASK_POLARITY)
        mask = 2;
#else
        mask = 1;
#endif
#if defined(WM_8BEC8_MUTANT_SWAPPED_AXES)
    if (wm_8bec8_axis(obj, 0x28u, 0x54u, 0x40u))
#else
    if (wm_8bec8_axis(obj, 0x30u, 0x54u, 0x40u))
#endif
#if defined(WM_8BEC8_MUTANT_MASK_POLARITY)
        mask |= 1;
#else
        mask |= 2;
#endif

    WM_8BEC8_CALL_93354(obj + 0x28u);
    {
        s32 ang = WM_8BEC8_CALL_93978(
            wm_8bec8_as_s32(wm_8bec8_load_u32(obj + 0x28u)),
            wm_8bec8_as_s32(wm_8bec8_load_u32(obj + 0x30u)));

        wm_8bec8_store_u32(obj + 0x2Cu, wm_8bec8_as_u32(ang));
    }
    return mask;
}
