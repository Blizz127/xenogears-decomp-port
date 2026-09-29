/*
 * World-map proximity/region query 0x8008C040.
 * Retail: [0x8008C040,0x8008C1DC) 412B/103 instrs, first-match.
 * Dependencies: wm_80093534 (wrap) and SquareRoot0 (0x80048C4C).
 * Role: iterate table at 0x8009BE6C count at 0x8009BD04, Y range check,
 * X/Z wrapped distance via 93534 + sqrt, tiered threshold.
 */

#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93534.h"
#include "world_map_helper_8c040.h"

#include <libgte.h> /* SquareRoot0 */

#define WM_8C040_COUNT_ADDR 0x8009BD04u
#define WM_8C040_BASE_ADDR  0x8009BE6Cu
#define WM_8C040_ENTRY_STRIDE 0x18u

#if defined(WM_8C040_MUTANT_WRONG_ITERATION_STRIDE)
#define WM_8C040_STRIDE 0x10u
#else
#define WM_8C040_STRIDE 0x18u
#endif

#if defined(WM_8C040_TEST_TRACE)
extern void wm_8c040_test_load(u32 addr, u32 width, u32 value);
extern void wm_8c040_test_store(u32 addr, u32 width, u32 value);
#define WM_8C040_TRACE_LOAD(a,w,v) wm_8c040_test_load((a),(w),(v))
#define WM_8C040_TRACE_STORE(a,w,v) wm_8c040_test_store((a),(w),(v))
#else
#define WM_8C040_TRACE_LOAD(a,w,v) ((void)0)
#define WM_8C040_TRACE_STORE(a,w,v) ((void)0)
#endif

static u32 wm_8c040_load_u32(u32 addr)
{
    u32 v;
    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_8C040_TRACE_LOAD(addr, 4u, v);
    return v;
}

static u16 wm_8c040_load_u16(u32 addr)
{
    u16 v;
    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_8C040_TRACE_LOAD(addr, 2u, v);
    return v;
}

static s16 wm_8c040_load_s16(u32 addr)
{
    s16 v;
    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_8C040_TRACE_LOAD(addr, 2u, (u32)(u16)v);
    return v;
}

static void wm_8c040_store_u8(u32 addr, u8 v)
{
    WM_8C040_TRACE_STORE(addr, 1u, v);
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static s32 wm_8c040_bits_to_s32(u32 b)
{
    s32 v;
    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_8c040_s32_to_bits(s32 v)
{
    u32 b;
    memcpy(&b, &v, sizeof(b));
    return b;
}

static u32 wm_8c040_sra(u32 bits, u32 amt) __attribute__((unused));
static u32 wm_8c040_sra(u32 bits, u32 amt)
{
    u32 v = bits >> amt;
    if (bits & 0x80000000u)
        v |= UINT32_MAX << (32u - amt);
    return v;
}

static u32 wm_8c040_sll(u32 bits, u32 amt)
{
    return bits << amt;
}

static int wm_8c040_slt(s32 a, s32 b)
{
#if defined(WM_8C040_MUTANT_UNSIGNED_DELTA)
    return (u32)a < (u32)b;
#else
    return a < b;
#endif
}

/* Temporary wrapped delta storage: use fixed guest addr like other helpers */
#define WM_8C040_TMP_ADDR 0x801C0300u

static void wm_8c040_store_s32(u32 addr, s32 v)
{
    u32 bits = wm_8c040_s32_to_bits(v);
    WM_8C040_TRACE_STORE(addr, 4u, bits);
    memcpy(PSX_ADDR(addr), &bits, sizeof(bits));
}

static s32 wm_8c040_load_s32(u32 addr)
{
    u32 bits = wm_8c040_load_u32(addr);
    return wm_8c040_bits_to_s32(bits);
}

void wm_8008C040(u32 vec_addr, s32 arg1, s32 arg2, u32 out1_addr, u32 out2_addr)
{
    /* arg1 = s7 (extra radius), arg2 = s5 (Y padding) */
    s32 s7 = arg1;
    s32 s5 = arg2;
    u32 s4 = out1_addr;
    u32 s6 = out2_addr;

    wm_8c040_store_u8(s4, 0u);
    wm_8c040_store_u8(s6, 0u);

#if defined(WM_8C040_MUTANT_WRONG_SENTINEL)
    s16 count = 1;
#else
    s16 count = wm_8c040_load_s16(WM_8C040_COUNT_ADDR);
#endif

#if defined(WM_8C040_MUTANT_OFF_BY_ONE_COUNT)
    count = count + 1;
#endif

#if defined(WM_8C040_MUTANT_WRONG_SENTINEL)
    /* Skip blez check */
#else
    if (count <= 0)
        return;
#endif

    u32 s1 = WM_8C040_BASE_ADDR;
    s32 s2 = 0;

    while (1) {
        u32 entry = s1;
        /* s0 = entry + 0x14u is the retail's second pointer, kept for documentation */
        (void)entry;

        /* Y range check */
        s32 y_in = wm_8c040_load_s32(vec_addr + 4u);
        s32 y_low = wm_8c040_load_s32(entry + 8u);
#if defined(WM_8C040_MUTANT_WRONG_DISTANCE_Z)
        /* Use X as Y */
        y_in = wm_8c040_load_s32(vec_addr + 0u);
#endif
        s32 y_high = wm_8c040_load_s32(entry + 16u);

        s32 v0, v1;
        int y_lt_low = wm_8c040_slt(y_in, y_low);
        if (y_lt_low) {
#if defined(WM_8C040_MUTANT_SKIP_WRAP)
            v0 = y_in - (s5 << 12);
#else
            v0 = y_in - wm_8c040_bits_to_s32(wm_8c040_sll(wm_8c040_s32_to_bits(s5), 12u));
#endif
            v1 = y_low;
        } else {
            s32 tmp = wm_8c040_bits_to_s32(wm_8c040_sll(wm_8c040_s32_to_bits(y_high), 12u));
            v0 = y_low - tmp;
            v1 = y_in;
        }

        /* v1 = v1 - v0 */
        v1 = v1 - v0;
        /* v0 = y_high reload */
        s32 y_high_reload = wm_8c040_load_s32(entry + 16u);
        /* sra v1, 12 */
        u32 v1_bits = wm_8c040_s32_to_bits(v1);
#if defined(WM_8C040_MUTANT_UNSIGNED_DELTA)
        v1_bits = v1_bits >> 12;
#else
        v1_bits = wm_8c040_sra(v1_bits, 12u);
#endif
        v1 = wm_8c040_bits_to_s32(v1_bits);

        s32 cmp = s5 + y_high_reload;
        if (wm_8c040_slt(v1, cmp)) {
            /* Y passes, continue to X/Z */
        } else {
            goto next_entry;
        }

        /* X/Z delta */
        s32 x_center = wm_8c040_load_s32(entry + 4u);
        s32 x_in = wm_8c040_load_s32(vec_addr + 0u);
#if defined(WM_8C040_MUTANT_WRONG_DISTANCE_X)
        x_in = wm_8c040_load_s32(vec_addr + 8u);
#endif
        s32 dx = x_center - x_in;
        s32 dx_sra = wm_8c040_bits_to_s32(wm_8c040_sra(wm_8c040_s32_to_bits(dx), 12u));
        wm_8c040_store_s32(WM_8C040_TMP_ADDR + 0u, dx_sra);
        /* Y slot unused for 93534, but keep */
        wm_8c040_store_s32(WM_8C040_TMP_ADDR + 4u, 0);

        s32 z_center = wm_8c040_load_s32(entry + 12u);
        s32 z_in = wm_8c040_load_s32(vec_addr + 8u);
#if defined(WM_8C040_MUTANT_WRONG_DISTANCE_Z)
        z_in = wm_8c040_load_s32(vec_addr + 0u);
#endif
        s32 dz = z_center - z_in;
        s32 dz_sra = wm_8c040_bits_to_s32(wm_8c040_sra(wm_8c040_s32_to_bits(dz), 12u));
        wm_8c040_store_s32(WM_8C040_TMP_ADDR + 8u, dz_sra);

#if defined(WM_8C040_MUTANT_SKIP_WRAP)
        /* Skip wrap */
#else
#if defined(WM_8C040_MUTANT_WRONG_WRAP_ARG)
        wm_80093534(vec_addr);
#else
        wm_80093534(WM_8C040_TMP_ADDR);
#endif
#endif

        s32 wrapped_x = wm_8c040_load_s32(WM_8C040_TMP_ADDR + 0u);
        s32 wrapped_z = wm_8c040_load_s32(WM_8C040_TMP_ADDR + 8u);

#if defined(WM_8C040_MUTANT_WRONG_DISTANCE_X)
        wrapped_x = wrapped_z;
#endif
#if defined(WM_8C040_MUTANT_WRONG_DISTANCE_Z)
        wrapped_z = wrapped_x;
#endif

        s64 sqx = (s64)wrapped_x * (s64)wrapped_x;
        s64 sqz = (s64)wrapped_z * (s64)wrapped_z;
        u32 lo_x = (u32)(sqx & 0xFFFFFFFFLL);
        u32 lo_z = (u32)(sqz & 0xFFFFFFFFLL);
        /* Actually we need low from mflo, which is low 32 of product */
        u32 sum = lo_x + lo_z;

#if defined(WM_8C040_MUTANT_SKIP_SQUARE_ROOT)
        s32 sqrt_res = wm_8c040_bits_to_s32(sum);
#else
#if defined(WM_8C040_MUTANT_WRONG_SQUARE_ROOT_INPUT)
        s32 sqrt_input = wrapped_x;
        s32 sqrt_res = SquareRoot0(sqrt_input);
        (void)sum;
#else
        s32 sqrt_res = SquareRoot0((int)sum);
#endif
#endif

        s16 thresh = wm_8c040_load_s16(entry + 20u);
        s32 v1_thresh = (s32)thresh + s7;

#if defined(WM_8C040_MUTANT_BOUNDARY_LT_VS_LE)
        int cond1 = sqrt_res <= v1_thresh;
        int cond2 = sqrt_res <= v1_thresh + 16;
#else
        int cond1 = wm_8c040_slt(sqrt_res, v1_thresh);
        int cond2 = wm_8c040_slt(sqrt_res, v1_thresh + 16);
#endif

        u8 out_val = 0;
        int is_match = 0;
        if (cond1) {
            out_val = 2;
            is_match = 1;
        } else if (cond2) {
            out_val = 1;
            is_match = 1;
        }

#if defined(WM_8C040_MUTANT_WRONG_RETURN)
        out_val = out_val ^ 0xFFu;
#endif

        if (is_match) {
#if defined(WM_8C040_MUTANT_FIRST_MATCH_VS_LAST_MATCH)
            /* Continue searching, last match wins */
            wm_8c040_store_u8(s4, out_val);
            u16 id = wm_8c040_load_u16(entry + 0u);
            wm_8c040_store_u8(s6, (u8)id);
            /* Do not return, go to next */
            goto next_entry;
#else
            wm_8c040_store_u8(s4, out_val);
            u16 id = wm_8c040_load_u16(entry + 0u);
            wm_8c040_store_u8(s6, (u8)id);
            return;
#endif
        }

next_entry:
        s2++;
#if defined(WM_8C040_MUTANT_OFF_BY_ONE_COUNT)
        /* Already handled */
#endif
        s1 += WM_8C040_STRIDE;
        /* s0 = s1+0x14 would be implicit */

        if (s2 < count)
            continue;
        else
            break;
    }

    /* No match, out already zero */
    return;
}
