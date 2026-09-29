/*
 * World-map reference-vector sign projector 0x800952B0.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x800952B0, 0x80095324).  See world_map_helper_952b0.h.
 *
 * Retail load order: ref[0], mov[0], ref[8], mov[8]; the sign paths
 * RELOAD ref[8] after storing out[0] (faithful ordering; matters only
 * under aliasing).  Store orders: sign paths +0, +8 then +4; zero path
 * +8, +0 then +4.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_952b0.h"

#if defined(WM_952B0_TEST_TRACE)
extern void wm_952b0_test_load(u32 address, u32 value);
extern void wm_952b0_test_store(u32 address, u32 value);
#define WM_952B0_TRACE_LOAD(a, v)  wm_952b0_test_load((a), (v))
#define WM_952B0_TRACE_STORE(a, v) wm_952b0_test_store((a), (v))
#else
#define WM_952B0_TRACE_LOAD(a, v)  ((void)0)
#define WM_952B0_TRACE_STORE(a, v) ((void)0)
#endif

static u32 wm_952b0_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_952B0_TRACE_LOAD(addr, v);
    return v;
}

static void wm_952b0_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_952B0_TRACE_STORE(addr, v);
}

static s32 wm_952b0_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

void wm_800952B0(u32 mov_vec, u32 out_vec, u32 ref_vec)
{
    u32 ref_x;
    u32 mov_x;
    u32 ref_z;
    u32 mov_z;
    u32 sum;

    ref_x = wm_952b0_load_u32(ref_vec + 0u);
#if defined(WM_952B0_MUTANT_SWAPPED_OPERANDS)
    mov_x = wm_952b0_load_u32(mov_vec + 8u);
#else
    mov_x = wm_952b0_load_u32(mov_vec + 0u);
#endif
    /* mult + mflo: low 32 bits of the signed product (u32 mult is the
     * identical low word without overflow UB). */
    sum = ref_x * mov_x;
    ref_z = wm_952b0_load_u32(ref_vec + 8u);
#if defined(WM_952B0_MUTANT_SWAPPED_OPERANDS)
    mov_z = wm_952b0_load_u32(mov_vec + 0u);
#else
    mov_z = wm_952b0_load_u32(mov_vec + 8u);
#endif
#if defined(WM_952B0_MUTANT_WRONG_WIDTH)
    {
        s64 wide = (s64)wm_952b0_as_s32(ref_x) * wm_952b0_as_s32(mov_x) +
                   (s64)wm_952b0_as_s32(ref_z) * wm_952b0_as_s32(mov_z);
        sum = (wide < 0) ? 0x80000000u : ((wide > 0) ? 1u : 0u);
    }
#else
    sum += ref_z * mov_z;
#endif

#if defined(WM_952B0_MUTANT_WRONG_BRANCH_POLARITY)
    if (wm_952b0_as_s32(sum) > 0) {
#else
    if (wm_952b0_as_s32(sum) < 0) {
#endif
        /* negu: 32-bit two's-complement wrap. */
        wm_952b0_store_u32(out_vec + 0u, 0u - ref_x);
        ref_z = wm_952b0_load_u32(ref_vec + 8u);
#if defined(WM_952B0_MUTANT_MISSING_NEGATE)
        wm_952b0_store_u32(out_vec + 8u, ref_z);
#else
        wm_952b0_store_u32(out_vec + 8u, 0u - ref_z);
#endif
#if defined(WM_952B0_MUTANT_WRONG_BRANCH_POLARITY)
    } else if (wm_952b0_as_s32(sum) < 0) {
#else
    } else if (wm_952b0_as_s32(sum) > 0) {
#endif
        wm_952b0_store_u32(out_vec + 0u, ref_x);
        ref_z = wm_952b0_load_u32(ref_vec + 8u);
        wm_952b0_store_u32(out_vec + 8u, ref_z);
    } else {
#if defined(WM_952B0_MUTANT_ZERO_PATH_COPY)
        wm_952b0_store_u32(out_vec + 8u, ref_z);
        wm_952b0_store_u32(out_vec + 0u, ref_x);
#else
        /* Retail store order: +8 then +0. */
        wm_952b0_store_u32(out_vec + 8u, 0u);
        wm_952b0_store_u32(out_vec + 0u, 0u);
#endif
    }
#if !defined(WM_952B0_MUTANT_SKIPPED_Y_STORE)
    /* jr delay slot: always executed. */
    wm_952b0_store_u32(out_vec + 4u, 0u);
#endif
}
