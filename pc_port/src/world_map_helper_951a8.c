/*
 * World-map movement resolver 0x800951A8.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x800951A8, 0x800952B0).  See world_map_helper_951a8.h.
 *
 * Retail control flow:
 *   v = 0x80094A5C(base, dir, scale, sign16(mode))
 *   v == 0 -> return 1
 *   v == 1 -> r = 0x80094088(0x1F800000, dir, out)
 *             if (r == 0) out.Z = out.Y = out.X = 0   (store order 8,4,0)
 *             return 0
 *   v == 2 -> out.X = dir.X >= 0 ? 0x1000 : -0x1000   (store order 0,8,4)
 *             out.Z = out.Y = 0
 *             return 0
 *   v == 3 -> out.Y = out.X = 0                        (store order 4,0,8)
 *             out.Z = dir.Z >= 0 ? 0x1000 : -0x1000
 *             return 0
 *   other  -> returns the caller's saved $s2 (retail-unreachable;
 *             94A5C's result set is {0,1,2,3}) -- the port aborts.
 */
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_func_94a5c.h"
#include "world_map_helper_94088.h"
#include "world_map_helper_951a8.h"

#if defined(WM_951A8_TEST_TRACE)
extern s32 wm_951a8_test_94a5c(u32 base_vec, u32 dir_vec, s32 scale, s32 mode);
extern s32 wm_951a8_test_94088(u32 attr_vec, u32 src_vec, u32 dst_vec);
extern void wm_951a8_test_load(u32 address, u32 value);
extern void wm_951a8_test_store(u32 address, u32 value);
#define WM_951A8_CALL_94A5C(b, d, s, m) wm_951a8_test_94a5c((b), (d), (s), (m))
#define WM_951A8_CALL_94088(a, s, d)    wm_951a8_test_94088((a), (s), (d))
#define WM_951A8_TRACE_LOAD(a, v)       wm_951a8_test_load((a), (v))
#define WM_951A8_TRACE_STORE(a, v)      wm_951a8_test_store((a), (v))
#else
#define WM_951A8_CALL_94A5C(b, d, s, m) wm_80094A5C((b), (d), (s), (m))
#define WM_951A8_CALL_94088(a, s, d)    wm_80094088((a), (s), (d))
#define WM_951A8_TRACE_LOAD(a, v)       ((void)0)
#define WM_951A8_TRACE_STORE(a, v)      ((void)0)
#endif

/* Scratchpad workspace handed to the slide-vector selector. */
#if defined(WM_951A8_MUTANT_WRONG_WORKSPACE)
#define WM_951A8_WORKSPACE 0x1F800010u
#else
#define WM_951A8_WORKSPACE 0x1F800000u
#endif

static u32 wm_951a8_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_951A8_TRACE_LOAD(addr, v);
    return v;
}

static void wm_951a8_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_951A8_TRACE_STORE(addr, v);
}

static s32 wm_951a8_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

s32 wm_800951A8(u32 base_vec, u32 dir_vec, u32 out_vec, s32 scale, s32 mode)
{
    s32 v;

    /* lh 0x30($sp): only the sign-extended low halfword of the stacked
     * mode word reaches 0x80094A5C. */
#if defined(WM_951A8_MUTANT_MODE_ZERO_EXT)
    v = WM_951A8_CALL_94A5C(base_vec, dir_vec, scale, (s32)(u16)mode);
#else
    v = WM_951A8_CALL_94A5C(base_vec, dir_vec, scale, (s32)(s16)mode);
#endif

    if (v == 1) {
        s32 r = WM_951A8_CALL_94088(WM_951A8_WORKSPACE, dir_vec, out_vec);

#if defined(WM_951A8_MUTANT_WRONG_88_BRANCH)
        if (r != 0) {
#else
        if (r == 0) {
#endif
            /* Retail store order: +8, +4, +0. */
            wm_951a8_store_u32(out_vec + 8u, 0u);
            wm_951a8_store_u32(out_vec + 4u, 0u);
            wm_951a8_store_u32(out_vec + 0u, 0u);
        }
        return 0;
    }

    if (v == 0) {
#if defined(WM_951A8_MUTANT_SUCCESS_RETURN)
        return 0;
#else
        return 1;
#endif
    }

    if (v == 2) {
        u32 x = wm_951a8_load_u32(dir_vec + 0u);
        u32 d;

#if defined(WM_951A8_MUTANT_SIGN_POLARITY)
        d = (wm_951a8_as_s32(x) >= 0) ? 0xFFFFF000u : 0x1000u;
#else
        d = (wm_951a8_as_s32(x) >= 0) ? 0x1000u : 0xFFFFF000u;
#endif
        /* Retail store order: +0, +8, +4. */
        wm_951a8_store_u32(out_vec + 0u, d);
        wm_951a8_store_u32(out_vec + 8u, 0u);
        wm_951a8_store_u32(out_vec + 4u, 0u);
        return 0;
    }

    if (v == 3) {
        u32 z;
        u32 d;

        /* Retail store order: +4, +0, then the +8 result. */
        wm_951a8_store_u32(out_vec + 4u, 0u);
        wm_951a8_store_u32(out_vec + 0u, 0u);
#if defined(WM_951A8_MUTANT_CASE3_READS_X)
        z = wm_951a8_load_u32(dir_vec + 0u);
#else
        z = wm_951a8_load_u32(dir_vec + 8u);
#endif
        d = (wm_951a8_as_s32(z) >= 0) ? 0x1000u : 0xFFFFF000u;
        wm_951a8_store_u32(out_vec + 8u, d);
        return 0;
    }

    /* Retail would return the caller's saved $s2 here; 0x80094A5C only
     * returns {0,1,2,3}, so this is unreachable.  Fail closed rather
     * than invent a value. */
    abort();
}
