/*
 * World-map perpendicular-heading vector builder 0x800941C4.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x800941C4, 0x80094238).  See world_map_helper_941c4.h.
 *
 * Retail load order: B.X, B.Z, A.Z, A.X; ratan2($a0 = B.Z - A.Z,
 * $a1 = B.X - A.X); angle bias +0x400 then andi 0xFFF; the sh of the
 * masked angle happens in rcos's delay slot; rsin's argument is the
 * lh RELOAD of the stored halfword; out.Z receives negu(rsin).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_941c4.h"

extern long ratan2(long y, long x);
extern long rcos(long a);
extern long rsin(long a);

#if defined(WM_941C4_TEST_TRACE)
extern long wm_941c4_test_ratan2(long y, long x);
extern long wm_941c4_test_rcos(long a);
extern long wm_941c4_test_rsin(long a);
#define WM_941C4_RATAN2(y, x) wm_941c4_test_ratan2((y), (x))
#define WM_941C4_RCOS(a)      wm_941c4_test_rcos(a)
#define WM_941C4_RSIN(a)      wm_941c4_test_rsin(a)
#else
#define WM_941C4_RATAN2(y, x) ratan2((y), (x))
#define WM_941C4_RCOS(a)      rcos(a)
#define WM_941C4_RSIN(a)      rsin(a)
#endif

static u32 wm_941c4_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s16 wm_941c4_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_941c4_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static void wm_941c4_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static s32 wm_941c4_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_941c4_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

s32 wm_800941C4(u32 vec_a, u32 vec_b, u32 out_vec, u32 out_angle)
{
    u32 bx = wm_941c4_load_u32(vec_b + 0u);
    u32 bz = wm_941c4_load_u32(vec_b + 8u);
    u32 az = wm_941c4_load_u32(vec_a + 8u);
    u32 ax = wm_941c4_load_u32(vec_a + 0u);
    long t;
    u32 ang;
    long c;
    s32 reload;
    long s;
    u32 neg;

#if defined(WM_941C4_MUTANT_SWAPPED_RATAN2_ARGS)
    t = WM_941C4_RATAN2((long)wm_941c4_as_s32(bx - ax),
                        (long)wm_941c4_as_s32(bz - az));
#else
    t = WM_941C4_RATAN2((long)wm_941c4_as_s32(bz - az),
                        (long)wm_941c4_as_s32(bx - ax));
#endif
#if defined(WM_941C4_MUTANT_WRONG_ANGLE_BIAS)
    ang = ((u32)(s32)t + 0x800u) & 0xFFFu;
#elif defined(WM_941C4_MUTANT_WRONG_ANGLE_MASK)
    ang = ((u32)(s32)t + 0x400u) & 0x7FFu;
#else
    ang = ((u32)(s32)t + 0x400u) & 0xFFFu;
#endif
    wm_941c4_store_u16(out_angle, (u16)ang);

    /* Retail order: rcos, store out.X, lh reload, rsin (matters if the
     * angle slot aliases the out vector). */
#if defined(WM_941C4_MUTANT_SWAPPED_TRIG_ORDER)
    reload = (s32)wm_941c4_load_s16(out_angle);
    s = WM_941C4_RSIN((long)reload);
    c = WM_941C4_RCOS((long)wm_941c4_as_s32(ang));
    wm_941c4_store_u32(out_vec + 0u, wm_941c4_as_u32((s32)c));
#else
    c = WM_941C4_RCOS((long)wm_941c4_as_s32(ang));
    wm_941c4_store_u32(out_vec + 0u, wm_941c4_as_u32((s32)c));
    reload = (s32)wm_941c4_load_s16(out_angle);
    s = WM_941C4_RSIN((long)reload);
#endif
    /* negu: 32-bit two's-complement wrap. */
#if defined(WM_941C4_MUTANT_MISSING_SIN_NEGATE)
    neg = wm_941c4_as_u32((s32)s);
#else
    neg = 0u - wm_941c4_as_u32((s32)s);
#endif
    wm_941c4_store_u32(out_vec + 8u, neg);
    return wm_941c4_as_s32(neg);
}
