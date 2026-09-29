/*
 * World-map terrain-attribute halfword lookup 0x80093E8C.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80093E8C, 0x80093F18).  Leaf: one guest VECTOR pointer in $a0,
 * no callees, no stores.  Returns the sign-extended s16 at
 * cell_base + 0x510 + 2 * packed_subcell.  Callers mask the raw
 * halfword; they do not treat it as a boolean.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93e8c.h"

#define WM_93E8C_STRIDE  0x8009D160u
#define WM_93E8C_TABLE   0x8009C184u
#define WM_93E8C_MASK    0x007FF000u
#define WM_93E8C_RECORD  0x510u

#if defined(WM_93E8C_TEST_TRACE)
extern void wm_93e8c_test_load(u32 address, u32 width, u32 value);
#define WM_93E8C_TRACE_LOAD(address, width, value) \
    wm_93e8c_test_load((address), (width), (value))
#else
#define WM_93E8C_TRACE_LOAD(address, width, value) ((void)0)
#endif

static u32 wm_93e8c_load_u32(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93E8C_TRACE_LOAD(address, 4u, value);
    return value;
}

static s16 wm_93e8c_load_s16(u32 address) __attribute__((unused));
static s16 wm_93e8c_load_s16(u32 address)
{
    s16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93E8C_TRACE_LOAD(address, 2u, (u32)(u16)value);
    return value;
}

static s32 wm_93e8c_bits_to_s32(u32 bits)
{
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_93e8c_s32_to_bits(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* Exact MIPS SRA of a 32-bit register, without relying on C signed >>. */
static u32 wm_93e8c_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

#if !defined(WM_93E8C_MUTANT_UNSIGNED_SHIFT)
    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
#endif
    return value;
}

static s32 wm_93e8c_coarse(u32 bits, u32 first_shift)
{
    s32 coarse = wm_93e8c_bits_to_s32(wm_93e8c_sra(bits, first_shift));

#if !defined(WM_93E8C_MUTANT_SKIP_NEG_ADJUST)
    if (coarse < 0)
        coarse += 7;
#endif
    return wm_93e8c_bits_to_s32(wm_93e8c_sra(wm_93e8c_s32_to_bits(coarse), 3u));
}

s32 wm_80093E8C(u32 vec_addr)
{
#if defined(WM_93E8C_MUTANT_WRONG_Z_OFFSET)
    u32 z_addr = vec_addr;
#else
    u32 z_addr = vec_addr + 8u;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_X_OFFSET)
    u32 x_addr = vec_addr + 4u;
#else
    u32 x_addr = vec_addr;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_SHIFT_20)
    const u32 first_shift = 19u;
#else
    const u32 first_shift = 20u;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_STRIDE_GLOBAL)
    u32 stride_addr = 0x8009D2B4u;
#else
    u32 stride_addr = WM_93E8C_STRIDE;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_TABLE_BASE)
    u32 table_addr = 0x8009C180u;
#else
    u32 table_addr = WM_93E8C_TABLE;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_MASK)
    const u32 mask = 0x00FFF000u;
#else
    const u32 mask = WM_93E8C_MASK;
#endif
#if defined(WM_93E8C_MUTANT_WRONG_RECORD_OFFSET)
    const u32 record = 0x512u;
#else
    const u32 record = WM_93E8C_RECORD;
#endif
    u32 z_bits = wm_93e8c_load_u32(z_addr);
    s32 coarse_z = wm_93e8c_coarse(z_bits, first_shift);
    u32 x_bits = wm_93e8c_load_u32(x_addr);
    u32 stride = wm_93e8c_load_u32(stride_addr);
    s32 coarse_x = wm_93e8c_coarse(x_bits, first_shift);
    u32 product = (u32)coarse_z * stride;
    u32 tile = product + (u32)coarse_x;
#if defined(WM_93E8C_MUTANT_SKIP_S16_INDEX)
    u32 table_off = tile << 2;
#else
    s32 tile_s16 = (s32)(s16)(u16)tile;
    u32 table_off = (u32)tile_s16 << 2;
#endif
    u32 cell = wm_93e8c_load_u32(table_addr + table_off);
#if defined(WM_93E8C_MUTANT_WRONG_PACK)
    u32 packed = ((x_bits & mask) >> 19) << 4;
    packed |= (z_bits & mask) >> 19;
#elif defined(WM_93E8C_MUTANT_WRONG_PACK_SHIFT)
    u32 packed = ((z_bits & mask) >> 19) << 3;
    packed |= (x_bits & mask) >> 19;
#elif defined(WM_93E8C_MUTANT_ADD_NOT_OR)
    u32 packed = ((z_bits & mask) >> 19) << 4;
    packed += (x_bits & mask) >> 19;
#else
    u32 packed = ((z_bits & mask) >> 19) << 4;
    packed |= (x_bits & mask) >> 19;
#endif
    u32 half_addr = cell + record + (packed << 1);
#if defined(WM_93E8C_MUTANT_LOAD_U32)
    {
        u32 word = wm_93e8c_load_u32(half_addr);

        return wm_93e8c_bits_to_s32(word);
    }
#elif defined(WM_93E8C_MUTANT_LOAD_U16)
    {
        u16 half;
        u32 bits;

        memcpy(&half, PSX_ADDR(half_addr), sizeof(half));
        bits = half;
        WM_93E8C_TRACE_LOAD(half_addr, 2u, bits);
        return wm_93e8c_bits_to_s32(bits);
    }
#elif defined(WM_93E8C_MUTANT_BOOLEAN_RETURN)
    return wm_93e8c_load_s16(half_addr) != 0;
#else
    return (s32)wm_93e8c_load_s16(half_addr);
#endif
}
