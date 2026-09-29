/*
 * World-map signed X/Z wrap helper 0x80093534.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80093534, 0x800935DC).  Shared leaf: one guest VECTOR pointer in
 * $a0, no callees, no callback registration.  X and Z are independently
 * wrapped against 0x8009D160 / 0x8009D2B4 << 11 using signed slti
 * thresholds -16384 and 16385.  Y is never accessed.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93534.h"

#define WM_93534_WRAP_X  0x8009D160u
#define WM_93534_WRAP_Z  0x8009D2B4u

#if defined(WM_93534_MUTANT_WRONG_SHIFT)
#define WM_93534_SHIFT  12u
#else
#define WM_93534_SHIFT  11u
#endif

#if defined(WM_93534_TEST_TRACE)
extern void wm_93534_test_load(u32 address, u32 value);
extern void wm_93534_test_store(u32 address, u32 value);
#define WM_93534_TRACE_LOAD(address, value) \
    wm_93534_test_load((address), (value))
#define WM_93534_TRACE_STORE(address, value) \
    wm_93534_test_store((address), (value))
#else
#define WM_93534_TRACE_LOAD(address, value) ((void)0)
#define WM_93534_TRACE_STORE(address, value) ((void)0)
#endif

static u32 wm_93534_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93534_TRACE_LOAD(address, value);
    return value;
}

static void wm_93534_store_u32(u32 address, u32 value)
{
    WM_93534_TRACE_STORE(address, value);
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_93534_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int wm_93534_signed_lt(u32 bits, s32 threshold)
{
    s32 value = wm_93534_bits_to_s32(bits);
#if defined(WM_93534_MUTANT_UNSIGNED_COMPARE)
    (void)value;
    return bits < (u32)threshold;
#else
    return value < threshold;
#endif
}

static u32 wm_93534_period(u32 global_addr)
{
    return wm_93534_load_u32(global_addr) << WM_93534_SHIFT;
}

static void wm_93534_wrap_word(u32 word_addr, u32 global_addr)
{
    u32 bits = wm_93534_load_u32(word_addr);
    u32 result = bits;
    int wrapped = 0;

#if defined(WM_93534_MUTANT_LOW_INCLUSIVE)
    if (wm_93534_signed_lt(bits, -16383)) {
#else
    if (wm_93534_signed_lt(bits, -16384)) {
#endif
        u32 period = wm_93534_period(global_addr);
#if defined(WM_93534_MUTANT_ADD_SUB_SWAPPED)
        result = bits - period;
#else
        result = bits + period;
#endif
        wrapped = 1;
#if defined(WM_93534_MUTANT_HIGH_16384)
    } else if (!wm_93534_signed_lt(bits, 16384)) {
#else
    } else if (!wm_93534_signed_lt(bits, 16385)) {
#endif
        u32 period = wm_93534_period(global_addr);
#if defined(WM_93534_MUTANT_ADD_SUB_SWAPPED)
        result = bits + period;
#else
        result = bits - period;
#endif
        wrapped = 1;
    }

#if defined(WM_93534_MUTANT_ALWAYS_STORE)
    (void)wrapped;
    wm_93534_store_u32(word_addr, result);
#else
    if (wrapped != 0)
        wm_93534_store_u32(word_addr, result);
#endif
}

void wm_80093534(u32 vec_addr)
{
#if defined(WM_93534_MUTANT_WRONG_X_OFFSET)
    u32 x_addr = vec_addr + 4u;
#else
    u32 x_addr = vec_addr;
#endif
#if defined(WM_93534_MUTANT_WRONG_Z_OFFSET)
    u32 z_addr = vec_addr + 4u;
#else
    u32 z_addr = vec_addr + 8u;
#endif
#if defined(WM_93534_MUTANT_WRONG_X_GLOBAL)
    u32 x_global = WM_93534_WRAP_Z;
#else
    u32 x_global = WM_93534_WRAP_X;
#endif
#if defined(WM_93534_MUTANT_WRONG_Z_GLOBAL)
    u32 z_global = WM_93534_WRAP_X;
#else
    u32 z_global = WM_93534_WRAP_Z;
#endif

#if defined(WM_93534_MUTANT_SKIP_X)
    (void)x_addr;
    (void)x_global;
#endif
#if defined(WM_93534_MUTANT_SKIP_Z)
    (void)z_addr;
    (void)z_global;
#endif

#if defined(WM_93534_MUTANT_STORE_Z_FIRST)
#if !defined(WM_93534_MUTANT_SKIP_Z)
    wm_93534_wrap_word(z_addr, z_global);
#endif
#if !defined(WM_93534_MUTANT_SKIP_X)
    wm_93534_wrap_word(x_addr, x_global);
#endif
#else
#if !defined(WM_93534_MUTANT_SKIP_X)
    wm_93534_wrap_word(x_addr, x_global);
#endif
#if !defined(WM_93534_MUTANT_SKIP_Z)
    wm_93534_wrap_word(z_addr, z_global);
#endif
#endif

#if defined(WM_93534_MUTANT_TOUCH_Y)
    wm_93534_store_u32(vec_addr + 4u, 0u);
#endif
}
