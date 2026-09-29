/*
 * World-map signed X/Z wrap helper 0x80093354.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80093354, 0x800933EC).  Shared leaf: one guest VECTOR pointer in
 * $a0, no callees, no callback registration.  X then Z are wrapped
 * independently against 0x8009D160 / 0x8009D2B4 << 23.  High wrap is
 * signed slt versus that period followed by subu; low wrap is bgez
 * versus zero followed by a reloaded-extent addu.  Y is never accessed.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"

#define WM_93354_WRAP_X  0x8009D160u
#define WM_93354_WRAP_Z  0x8009D2B4u

#if defined(WM_93354_MUTANT_WRONG_SHIFT)
#define WM_93354_SHIFT  11u
#else
#define WM_93354_SHIFT  23u
#endif

#if defined(WM_93354_TEST_TRACE)
extern void wm_93354_test_load(u32 address, u32 value);
extern void wm_93354_test_store(u32 address, u32 value);
#define WM_93354_TRACE_LOAD(address, value) \
    wm_93354_test_load((address), (value))
#define WM_93354_TRACE_STORE(address, value) \
    wm_93354_test_store((address), (value))
#else
#define WM_93354_TRACE_LOAD(address, value) ((void)0)
#define WM_93354_TRACE_STORE(address, value) ((void)0)
#endif

static u32 s_wm_80093354_exec_count;

u32 wm_80093354_get_exec_count(void)
{
    return s_wm_80093354_exec_count;
}

void wm_80093354_reset_exec_count(void)
{
    s_wm_80093354_exec_count = 0u;
}

static u32 wm_93354_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93354_TRACE_LOAD(address, value);
    return value;
}

static void wm_93354_store_u32(u32 address, u32 value)
{
    WM_93354_TRACE_STORE(address, value);
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 wm_93354_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int wm_93354_signed_lt(u32 left_bits, u32 right_bits)
{
    s32 left = wm_93354_bits_to_s32(left_bits);
    s32 right = wm_93354_bits_to_s32(right_bits);
#if defined(WM_93354_MUTANT_UNSIGNED_COMPARE)
    (void)left;
    (void)right;
    return left_bits < right_bits;
#else
    return left < right;
#endif
}

static u32 wm_93354_period(u32 extent)
{
    return extent << WM_93354_SHIFT;
}

static void wm_93354_wrap_word(u32 word_addr, u32 global_addr)
{
    u32 extent = wm_93354_load_u32(global_addr);
    u32 period = wm_93354_period(extent);
    u32 bits = wm_93354_load_u32(word_addr);
    u32 result;
    int high_wrap;
    int low_wrap;

#if defined(WM_93354_MUTANT_COMPARE_CONSTANT_16385)
    high_wrap = !wm_93354_signed_lt(bits, 16385u);
#elif defined(WM_93354_MUTANT_SKIP_HIGH)
    high_wrap = 0;
#else
    high_wrap = !wm_93354_signed_lt(bits, period);
#endif

#if defined(WM_93354_MUTANT_HIGH_LOW_SWAPPED)
    {
        u32 first = bits;
#if defined(WM_93354_MUTANT_LOW_SLTI_16384)
        low_wrap = wm_93354_signed_lt(first, 0xFFFFC000u);
#else
        low_wrap = wm_93354_signed_lt(first, 0u);
#endif
        if (low_wrap) {
            extent = wm_93354_load_u32(global_addr);
            period = wm_93354_period(extent);
#if defined(WM_93354_MUTANT_ADD_SUB_SWAPPED)
            result = first - period;
#else
            result = first + period;
#endif
            wm_93354_store_u32(word_addr, result);
        }
        bits = wm_93354_load_u32(word_addr);
#if defined(WM_93354_MUTANT_COMPARE_CONSTANT_16385)
        high_wrap = !wm_93354_signed_lt(bits, 16385u);
#elif defined(WM_93354_MUTANT_SKIP_HIGH)
        high_wrap = 0;
#else
        high_wrap = !wm_93354_signed_lt(bits, period);
#endif
        if (high_wrap) {
#if defined(WM_93354_MUTANT_ADD_SUB_SWAPPED)
            result = bits + period;
#else
            result = bits - period;
#endif
            wm_93354_store_u32(word_addr, result);
        }
#if defined(WM_93354_MUTANT_ALWAYS_STORE)
        else
            wm_93354_store_u32(word_addr, bits);
#endif
        return;
    }
#endif

    if (high_wrap) {
#if defined(WM_93354_MUTANT_ADD_SUB_SWAPPED)
        result = bits + period;
#else
        result = bits - period;
#endif
        wm_93354_store_u32(word_addr, result);
#if defined(WM_93354_MUTANT_ALWAYS_STORE)
    } else {
        wm_93354_store_u32(word_addr, bits);
#endif
    }

    bits = wm_93354_load_u32(word_addr);
#if defined(WM_93354_MUTANT_LOW_SLTI_16384)
    low_wrap = wm_93354_signed_lt(bits, 0xFFFFC000u);
#else
    low_wrap = wm_93354_signed_lt(bits, 0u);
#endif
    if (low_wrap) {
        extent = wm_93354_load_u32(global_addr);
        period = wm_93354_period(extent);
#if defined(WM_93354_MUTANT_ADD_SUB_SWAPPED)
        result = bits - period;
#else
        result = bits + period;
#endif
        wm_93354_store_u32(word_addr, result);
#if defined(WM_93354_MUTANT_ALWAYS_STORE)
    } else {
        wm_93354_store_u32(word_addr, bits);
#endif
    }
}

void wm_80093354(u32 vec_addr)
{
    s_wm_80093354_exec_count += 1u;

#if defined(WM_93354_MUTANT_WRONG_X_OFFSET)
    u32 x_addr = vec_addr + 4u;
#else
    u32 x_addr = vec_addr;
#endif
#if defined(WM_93354_MUTANT_WRONG_Z_OFFSET)
    u32 z_addr = vec_addr + 4u;
#else
    u32 z_addr = vec_addr + 8u;
#endif
#if defined(WM_93354_MUTANT_WRONG_X_GLOBAL)
    u32 x_global = WM_93354_WRAP_Z;
#else
    u32 x_global = WM_93354_WRAP_X;
#endif
#if defined(WM_93354_MUTANT_WRONG_Z_GLOBAL)
    u32 z_global = WM_93354_WRAP_X;
#else
    u32 z_global = WM_93354_WRAP_Z;
#endif

#if defined(WM_93354_MUTANT_SKIP_X)
    (void)x_addr;
    (void)x_global;
#endif
#if defined(WM_93354_MUTANT_SKIP_Z)
    (void)z_addr;
    (void)z_global;
#endif

#if defined(WM_93354_MUTANT_STORE_Z_FIRST)
#if !defined(WM_93354_MUTANT_SKIP_Z)
    wm_93354_wrap_word(z_addr, z_global);
#endif
#if !defined(WM_93354_MUTANT_SKIP_X)
    wm_93354_wrap_word(x_addr, x_global);
#endif
#else
#if !defined(WM_93354_MUTANT_SKIP_X)
    wm_93354_wrap_word(x_addr, x_global);
#endif
#if !defined(WM_93354_MUTANT_SKIP_Z)
    wm_93354_wrap_word(z_addr, z_global);
#endif
#endif

#if defined(WM_93354_MUTANT_TOUCH_Y)
    wm_93354_store_u32(vec_addr + 4u, 0u);
#endif
}
