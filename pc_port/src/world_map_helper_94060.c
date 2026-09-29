/*
 * World-map walkability-flag lookup 0x80094060.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80094060, 0x80094088).  Leaf: two s16 indices in $a0/$a1, no
 * callees, no stack frame, no saved registers, no stores.  The
 * sll16/sra12 pair on $a0 sign-extends the low half and scales by 16
 * (row byte stride); the sll16/sra15 pair on $a1 sign-extends the low
 * half and scales by 2 (halfword width).  Returns the sign-extended
 * s16 at 0x8009BAC8 + a0*16 + a1*2 (retail lui 0x800A, lh -17720).
 * The table is in-image (world_map.bin file offset 0x2BFD8), 128
 * bytes / 8 rows of 16-byte stride; observed retail values are 0/1.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_94060.h"

#if defined(WM_94060_MUTANT_WRONG_TABLE_BASE)
#define WM_94060_TABLE 0x8009BB48u
#else
#define WM_94060_TABLE 0x8009BAC8u
#endif

#if defined(WM_94060_TEST_TRACE)
extern void wm_94060_test_load(u32 address, u32 width, u32 value);
#define WM_94060_TRACE_LOAD(address, width, value) \
    wm_94060_test_load((address), (width), (value))
#else
#define WM_94060_TRACE_LOAD(address, width, value) ((void)0)
#endif

static u32 s_wm_80094060_exec_count;

u32 wm_80094060_get_exec_count(void)
{
    return s_wm_80094060_exec_count;
}

void wm_80094060_reset_exec_count(void)
{
    s_wm_80094060_exec_count = 0u;
}

static s16 wm_94060_load_s16(u32 address) __attribute__((unused));
static s16 wm_94060_load_s16(u32 address)
{
    s16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_94060_TRACE_LOAD(address, 2u, (u32)(u16)value);
    return value;
}

/* Exact MIPS SRA of a 32-bit register, without relying on C signed >>. */
static u32 wm_94060_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

s32 wm_80094060(s32 a0, s32 a1)
{
    u32 a0_bits = (u32)a0;
    u32 a1_bits = (u32)a1;
    u32 a0_part;
    u32 a1_part;
    u32 offset;

    s_wm_80094060_exec_count += 1u;

#if defined(WM_94060_MUTANT_CLAMP_ADDED)
    {
        s32 half0 = (s32)(s16)(u16)a0_bits;
        s32 half1 = (s32)(s16)(u16)a1_bits;

        if (half0 < 0)
            half0 = 0;
        if (half0 > 7)
            half0 = 7;
        if (half1 < 0)
            half1 = 0;
        if (half1 > 1)
            half1 = 1;
        a0_bits = (u32)half0;
        a1_bits = (u32)half1;
    }
#endif

    a0_bits <<= 16; /* sll $a0,$a0,16 */
    a1_bits <<= 16; /* sll $a1,$a1,16 */

#if defined(WM_94060_MUTANT_SWAPPED_INDICES)
    a0_part = wm_94060_sra(a1_bits, 12);
    a1_part = wm_94060_sra(a0_bits, 15);
#elif defined(WM_94060_MUTANT_WRONG_SHIFTS)
    a0_part = wm_94060_sra(a0_bits, 13);
    a1_part = wm_94060_sra(a1_bits, 15);
#elif defined(WM_94060_MUTANT_WRONG_WIDTH)
    a0_part = wm_94060_sra(a0_bits, 12);
    a1_part = wm_94060_sra(a1_bits, 14);
#else
    a0_part = wm_94060_sra(a0_bits, 12); /* sra $a0,$a0,12 */
    a1_part = wm_94060_sra(a1_bits, 15); /* sra $a1,$a1,15 */
#endif

    offset = a0_part + a1_part; /* addu $a0,$a0,$a1 */

    /* lui $at,0x800A; addu $at,$at,$a0; lh $v0,-17720($at) folds to
     * the table base above plus the packed byte offset. */
#if defined(WM_94060_MUTANT_UNSIGNED_RETURN)
    {
        u16 half;

        memcpy(&half, PSX_ADDR(WM_94060_TABLE + offset), sizeof(half));
        WM_94060_TRACE_LOAD(WM_94060_TABLE + offset, 2u, half);
        return (s32)half;
    }
#else
    return (s32)wm_94060_load_s16(WM_94060_TABLE + offset);
#endif
}
