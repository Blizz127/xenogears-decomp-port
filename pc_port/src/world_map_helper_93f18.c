/*
 * World-map terrain selector helper 0x80093F18.
 *
 * Retail: [0x80093F18, 0x80093FE4) 204 bytes / 51 instructions.
 * Only callee: 0x80093E8C.
 *
 * Disassembly-faithful transcription:
 *
 *   addiu sp,-0x18
 *   sw s0,0x10(sp)
 *   sw ra,0x14(sp)
 *   jal 0x80093E8C
 *   move s0,a0
 *   move a2,v0
 *   lw v1,(s0)               ; x
 *   bgez v1,1f
 *   andi a1,a2,0xF            ; delay
 *   addiu v1,7
 * 1: sra v0,v1,3
 *   sll a1,a1,4
 *   andi v0,0xFFFF
 *   lw v1,-0x4B9C(at) with at=0x800A0000+a1 -> 0x8009B464+a1
 *   lw a0,8(s0)               ; z
 *   bgez a0,2f
 *   subu v1,v0,v1             ; delay: diff_x = coarse_x - T_A
 *   addiu a0,7
 * 2: lw v0,-0x4C9C(at) -> 0x8009B364+a1   ; T_B
 *   mult v1,v0
 *   sra v0,a0,3
 *   andi v0,0xFFFF
 *   lw v1,-0x4B94(at) -> 0x8009B46C+a1   ; T_C
 *   mflo a3
 *   lw a0,-0x4C94(at) -> 0x8009B36C+a1   ; T_D
 *   subu v0,v0,v1             ; diff_z = coarse_z - T_C
 *   mult v0,a0
 *   sra v0,a3,12
 *   mflo v1
 *   sra v1,v1,12
 *   addu v0,v0,v1
 *   bgtz v0,3f
 *   srl v0,a2,4               ; delay: (a2>>4)&7 path
 *   srl v0,a2,7
 * 3: andi v0,7
 *   jr ra
 *
 * If sum>0 return (a2>>4)&7 else (a2>>7)&7.
 */

#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93e8c.h"
#include "world_map_helper_93f18.h"

/* Table bases: 0x800A0000 - disp */
#define WM_93F18_TBL_A_BASE 0x8009B464u
#define WM_93F18_TBL_B_BASE 0x8009B364u
#define WM_93F18_TBL_C_BASE 0x8009B46Cu
#define WM_93F18_TBL_D_BASE 0x8009B36Cu

#if defined(WM_93F18_MUTANT_WRONG_GLOBAL_BASE) || defined(WM_93F18_MUTANT_WRONG_STRIDE)
#define WM_93F18_TBL_A_ADDR 0x8009B468u
#define WM_93F18_TBL_B_ADDR 0x8009B368u
#define WM_93F18_TBL_C_ADDR 0x8009B470u
#define WM_93F18_TBL_D_ADDR 0x8009B370u
#else
#define WM_93F18_TBL_A_ADDR WM_93F18_TBL_A_BASE
#define WM_93F18_TBL_B_ADDR WM_93F18_TBL_B_BASE
#define WM_93F18_TBL_C_ADDR WM_93F18_TBL_C_BASE
#define WM_93F18_TBL_D_ADDR WM_93F18_TBL_D_BASE
#endif

#if defined(WM_93F18_TEST_TRACE)
extern void wm_93f18_test_load(u32 address, u32 width, u32 value);
#define WM_93F18_TRACE_LOAD(addr, w, v) wm_93f18_test_load((addr),(w),(v))
#else
#define WM_93F18_TRACE_LOAD(addr, w, v) ((void)0)
#endif

static u32 wm_93f18_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93F18_TRACE_LOAD(address, 4u, value);
    return value;
}

static u32 wm_93f18_load_u16(u32 address) __attribute__((unused));
static u32 wm_93f18_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_93F18_TRACE_LOAD(address, 2u, (u32)value);
    return (u32)value;
}

static s32 wm_93f18_bits_to_s32(u32 bits)
{
    s32 v;
    memcpy(&v, &bits, sizeof(v));
    return v;
}

static u32 wm_93f18_s32_to_bits(s32 v)
{
    u32 b;
    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA without relying on C signed >>. */
static u32 wm_93f18_sra(u32 bits, u32 amount) __attribute__((unused));
static u32 wm_93f18_sra(u32 bits, u32 amount)
{
#if defined(WM_93F18_MUTANT_MISSING_SIGN_EXT) || defined(WM_93F18_MUTANT_WRONG_SIGNEDNESS)
    return bits >> amount;
#else
    u32 value = bits >> amount;
    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
#endif
}

static u32 wm_93f18_srl(u32 bits, u32 amount) __attribute__((unused));
static u32 wm_93f18_srl(u32 bits, u32 amount)
{
    return bits >> amount;
}

static s32 wm_93f18_coarse_x_from_raw(s32 raw)
{
#if defined(WM_93F18_MUTANT_WRONG_SIGNEDNESS)
    /* Treat as unsigned >=0 always: skip negative fixup. */
    (void)raw;
    /* Use unsigned shift path: will be caught by negative tests. */
    u32 bits = wm_93f18_s32_to_bits(raw);
    bits = wm_93f18_srl(bits, 3u);
    return wm_93f18_bits_to_s32(bits);
#else
#if defined(WM_93F18_MUTANT_SKIP_NEG_ADJUST)
    /* Skip +7 for negatives */
#else
    if (raw < 0)
        raw += 7;
#endif
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
    return wm_93f18_bits_to_s32(wm_93f18_sra(wm_93f18_s32_to_bits(raw), 2u));
#else
    return wm_93f18_bits_to_s32(wm_93f18_sra(wm_93f18_s32_to_bits(raw), 3u));
#endif
#endif
}

static s32 wm_93f18_coarse_z_from_raw(s32 raw)
{
#if defined(WM_93F18_MUTANT_WRONG_SIGNEDNESS)
    u32 bits = wm_93f18_s32_to_bits(raw);
    bits = wm_93f18_srl(bits, 3u);
    return wm_93f18_bits_to_s32(bits);
#else
#if defined(WM_93F18_MUTANT_SKIP_NEG_ADJUST)
#else
    if (raw < 0)
        raw += 7;
#endif
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
    return wm_93f18_bits_to_s32(wm_93f18_sra(wm_93f18_s32_to_bits(raw), 2u));
#else
    return wm_93f18_bits_to_s32(wm_93f18_sra(wm_93f18_s32_to_bits(raw), 3u));
#endif
#endif
}

s32 wm_80093F18(u32 vec_addr)
{
    /* Argument handling mutants */
#if defined(WM_93F18_MUTANT_WRONG_SITE_OFFSET)
    u32 x_addr = vec_addr + 4u;
    u32 z_addr = vec_addr + 4u;
#elif defined(WM_93F18_MUTANT_WRONG_93E8C_ARG)
    /* For the loads we still use correct offsets, but callee arg wrong */
    u32 x_addr = vec_addr;
    u32 z_addr = vec_addr + 8u;
#else
    u32 x_addr = vec_addr;
    u32 z_addr = vec_addr + 8u;
#endif

#if defined(WM_93F18_MUTANT_WRONG_WIDTH)
    u32 x_bits = wm_93f18_load_u16(x_addr);
    u32 z_bits = wm_93f18_load_u16(z_addr);
#else
    u32 x_bits = wm_93f18_load_u32(x_addr);
    u32 z_bits = wm_93f18_load_u32(z_addr);
#endif

    s32 x_raw = wm_93f18_bits_to_s32(x_bits);
    s32 z_raw = wm_93f18_bits_to_s32(z_bits);

    /* Coarse with exact retail bias */
    s32 coarse_x_s32 = wm_93f18_coarse_x_from_raw(x_raw);
    s32 coarse_z_s32 = wm_93f18_coarse_z_from_raw(z_raw);

    u32 coarse_x_bits = wm_93f18_s32_to_bits(coarse_x_s32);
#if defined(WM_93F18_MUTANT_WRONG_MASK)
    coarse_x_bits &= 0xFFu;
#else
    coarse_x_bits &= 0xFFFFu;
#endif

    u32 coarse_z_bits = wm_93f18_s32_to_bits(coarse_z_s32);
#if defined(WM_93F18_MUTANT_WRONG_MASK)
    coarse_z_bits &= 0xFFu;
#else
    coarse_z_bits &= 0xFFFFu;
#endif

    /* Call 0x80093E8C */
    s32 packed_s32;
#if defined(WM_93F18_MUTANT_WRONG_93E8C_ARG)
    packed_s32 = wm_80093E8C(vec_addr + 4u);
#elif defined(WM_93F18_MUTANT_WRONG_93E8C_ORDER)
    /* Same single-arg function; use offset to simulate wrong order */
    packed_s32 = wm_80093E8C(z_addr);
#else
    packed_s32 = wm_80093E8C(vec_addr);
#endif
    u32 packed_bits = wm_93f18_s32_to_bits(packed_s32);

    u32 low_nibble;
#if defined(WM_93F18_MUTANT_WRONG_MASK)
    low_nibble = packed_bits & 0x7u;
#else
    low_nibble = packed_bits & 0xFu;
#endif

    u32 a1;
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
    a1 = low_nibble << 3u;
#elif defined(WM_93F18_MUTANT_WRONG_SITE_OFFSET)
    /* Wrong stride for table indexing */
    a1 = low_nibble << 2u;
#else
    a1 = low_nibble << 4u;
#endif

    u32 tbl_a = wm_93f18_load_u32(WM_93F18_TBL_A_ADDR + a1);
    u32 tbl_b = wm_93f18_load_u32(WM_93F18_TBL_B_ADDR + a1);
    u32 tbl_c = wm_93f18_load_u32(WM_93F18_TBL_C_ADDR + a1);
    u32 tbl_d = wm_93f18_load_u32(WM_93F18_TBL_D_ADDR + a1);

#if defined(WM_93F18_MUTANT_SKIPPED_TABLE) || defined(WM_93F18_MUTANT_SKIPPED_SITE)
    tbl_a = 0u;
    tbl_b = 0u;
    tbl_c = 0u;
    tbl_d = 0u;
#endif

    /* diff_x = coarse_x - tbl_a  (SUBU) */
    u32 diff_x_bits = coarse_x_bits - tbl_a;
    u32 diff_z_bits = coarse_z_bits - tbl_c;

#if defined(WM_93F18_MUTANT_SWAPPED_INDEX)
    /* Swap x/z diffs */
    u32 tmp = diff_x_bits;
    diff_x_bits = diff_z_bits;
    diff_z_bits = tmp;
#endif

    s32 diff_x = wm_93f18_bits_to_s32(diff_x_bits);
    s32 diff_z = wm_93f18_bits_to_s32(diff_z_bits);
    s32 b = wm_93f18_bits_to_s32(tbl_b);
    s32 d = wm_93f18_bits_to_s32(tbl_d);

#if defined(WM_93F18_MUTANT_WRONG_SIGNEDNESS)
    /* Unsigned multiply */
    u64 prod_x_u = (u64)diff_x_bits * (u64)tbl_b;
    u64 prod_z_u = (u64)diff_z_bits * (u64)tbl_d;
    u32 lo_x = (u32)prod_x_u;
    u32 lo_z = (u32)prod_z_u;
    (void)diff_x;
    (void)diff_z;
    (void)b;
    (void)d;
#else
    s64 prod_x = (s64)diff_x * (s64)b;
    s64 prod_z = (s64)diff_z * (s64)d;
    u32 lo_x = (u32)(prod_x & 0xFFFFFFFFLL);
    u32 lo_z = (u32)(prod_z & 0xFFFFFFFFLL);
#endif

    u32 sra_x_bits;
    u32 sra_z_bits;
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
    sra_x_bits = wm_93f18_sra(lo_x, 11u);
    sra_z_bits = wm_93f18_sra(lo_z, 11u);
#else
    sra_x_bits = wm_93f18_sra(lo_x, 12u);
    sra_z_bits = wm_93f18_sra(lo_z, 12u);
#endif

    u32 sum_bits = sra_x_bits + sra_z_bits;
    s32 sum = wm_93f18_bits_to_s32(sum_bits);

#if defined(WM_93F18_MUTANT_WRONG_BRANCH)
    int take_gt = sum <= 0;
#else
    int take_gt = sum > 0;
#endif

    u32 result_bits;
#if defined(WM_93F18_MUTANT_BOOLEAN_RETURN)
    result_bits = take_gt ? 1u : 0u;
#else
    if (take_gt) {
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
        result_bits = wm_93f18_srl(packed_bits, 3u) & 0x7u;
#elif defined(WM_93F18_MUTANT_WRONG_MASK)
        result_bits = wm_93f18_srl(packed_bits, 4u) & 0x3u;
#else
        result_bits = wm_93f18_srl(packed_bits, 4u) & 0x7u;
#endif
    } else {
#if defined(WM_93F18_MUTANT_WRONG_SHIFT)
        result_bits = wm_93f18_srl(packed_bits, 6u) & 0x7u;
#elif defined(WM_93F18_MUTANT_WRONG_MASK)
        result_bits = wm_93f18_srl(packed_bits, 7u) & 0x3u;
#else
        result_bits = wm_93f18_srl(packed_bits, 7u) & 0x7u;
#endif
    }
#endif

#if defined(WM_93F18_MUTANT_CLAMP)
    if (result_bits > 6u)
        result_bits = 6u;
#endif

#if defined(WM_93F18_MUTANT_PLUS1_BOUNDARY)
    result_bits = (result_bits + 1u) & 0x7u;
#endif

#if defined(WM_93F18_MUTANT_MINUS1_BOUNDARY)
    result_bits = (result_bits - 1u) & 0x7u;
#endif

    return wm_93f18_bits_to_s32(result_bits);
}
