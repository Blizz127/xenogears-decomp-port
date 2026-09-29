/*
 * World-map pose-ring publisher 0x80074794.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x80074794, 0x800747DC).  The leaf publishes X/tag/Z halfwords to the
 * current 8-byte ring entry, advances the 16-entry index, and returns no
 * meaningful value.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_74794.h"

#define WM_74794_RING_INDEX 0x8009BE38u
#define WM_74794_RING_BASE  0x8009D30Cu

#if defined(WM_74794_TEST_TRACE)
extern void wm_74794_test_trace(u32 address, u32 width, u32 value);
extern void wm_74794_test_shift(u32 input, u32 result);
#define WM_74794_TRACE(address, width, value) \
    wm_74794_test_trace((address), (width), (value))
#define WM_74794_TRACE_SHIFT(input, result) \
    wm_74794_test_shift((input), (result))
#else
#define WM_74794_TRACE(address, width, value) ((void)0)
#define WM_74794_TRACE_SHIFT(input, result) ((void)0)
#endif

static u32 wm_74794_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_74794_store_u16(u32 address, u16 value)
{
    WM_74794_TRACE(address, 2u, value);
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_74794_store_u32(u32 address, u32 value)
{
    WM_74794_TRACE(address, 4u, value);
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

/* Exact MIPS SRA-by-12 result expressed without relying on implementation-
 * defined signed right shift. */
static u32 wm_74794_sra12(u32 bits)
{
#if defined(WM_74794_MUTANT_WRONG_SHIFT)
    const u32 amount = 16u;
#else
    const u32 amount = 12u;
#endif
    u32 value = bits >> amount;

#if !defined(WM_74794_MUTANT_UNSIGNED_SHIFT)
    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
#endif
    WM_74794_TRACE_SHIFT(bits, value);
    return value;
}

void wm_80074794(s32 tag, u32 pose_addr)
{
    u32 index = wm_74794_load_u32(WM_74794_RING_INDEX);
    u32 ring_base = wm_74794_load_u32(WM_74794_RING_BASE);
    u32 entry;
    u32 x_bits;
    u32 z_bits;

#if defined(WM_74794_MUTANT_INDEX_BEFORE_WRITES)
    index = (index + 1u) & 0x0Fu;
#endif
#if defined(WM_74794_MUTANT_WRONG_STRIDE)
    entry = ring_base + (index << 2);
#else
    entry = ring_base + (index << 3);
#endif

#if defined(WM_74794_MUTANT_WRONG_X_OFFSET)
    x_bits = wm_74794_load_u32(pose_addr + 4u);
#else
    x_bits = wm_74794_load_u32(pose_addr);
#endif

#if defined(WM_74794_MUTANT_STORE_U32)
    wm_74794_store_u32(entry, wm_74794_sra12(x_bits));
#else
    wm_74794_store_u16(entry, (u16)wm_74794_sra12(x_bits));
#endif

#if defined(WM_74794_MUTANT_WRONG_Z_OFFSET)
    z_bits = wm_74794_load_u32(pose_addr + 4u);
#else
    z_bits = wm_74794_load_u32(pose_addr + 8u);
#endif

#if defined(WM_74794_MUTANT_WRONG_TAG_STORE)
    wm_74794_store_u16(entry + 4u, (u16)tag);
#else
    wm_74794_store_u16(entry + 2u, (u16)tag);
#endif

#if !defined(WM_74794_MUTANT_INDEX_BEFORE_WRITES)
    index += 1u;
#endif
#if defined(WM_74794_MUTANT_WRONG_MASK)
    index &= 0x1Fu;
#else
    index &= 0x0Fu;
#endif
    wm_74794_store_u32(WM_74794_RING_INDEX, index);
    wm_74794_store_u16(entry + 4u, (u16)wm_74794_sra12(z_bits));
}
