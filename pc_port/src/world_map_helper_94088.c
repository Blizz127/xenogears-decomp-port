/*
 * World-map slide-vector selector 0x80094088.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80094088, 0x80094154).  See world_map_helper_94088.h.
 *
 * Table addressing (retail):
 *   n   = wm_80093FE4($a0)                  ; low nibble, [0, 15]
 *   idx = ((n << 16) >> 12)                 ; sll 16 / sra 12 = n * 16
 *   at  = 0x800A0000 + idx
 *   X   = lw -0x4D9C($at)                   ; 0x8009B264 + idx
 *   Z   = lw -0x4D94($at)                   ; 0x8009B26C + idx
 *
 * The multiply is `mult` + `mflo` only: the low 32 bits of the signed
 * product.  The sum wraps in 32 bits and its wrapped sign selects the
 * branch.  The dot == 0 path RELOADS src+8 after storing dst+0
 * (faithful to the retail load ordering); both nonzero paths reload
 * the table Z word before storing it.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93fe4.h"
#include "world_map_helper_94088.h"

#if defined(WM_94088_TEST_TRACE)
extern s32 wm_94088_test_93fe4(u32 vec_addr);
extern void wm_94088_test_load(u32 address, u32 value);
extern void wm_94088_test_store(u32 address, u32 value);
#define WM_94088_CALL_93FE4(a)      wm_94088_test_93fe4(a)
#define WM_94088_TRACE_LOAD(a, v)   wm_94088_test_load((a), (v))
#define WM_94088_TRACE_STORE(a, v)  wm_94088_test_store((a), (v))
#else
#define WM_94088_CALL_93FE4(a)      wm_80093FE4(a)
#define WM_94088_TRACE_LOAD(a, v)   ((void)0)
#define WM_94088_TRACE_STORE(a, v)  ((void)0)
#endif

/* Retail table cell (lui 0x800A base minus displacement). */
#if defined(WM_94088_MUTANT_WRONG_TABLE_BASE)
#define WM_94088_TABLE_X  (0x800A0000u - 0x4D94u)
#define WM_94088_TABLE_Z  (0x800A0000u - 0x4D8Cu)
#else
#define WM_94088_TABLE_X  (0x800A0000u - 0x4D9Cu) /* 0x8009B264 */
#define WM_94088_TABLE_Z  (0x800A0000u - 0x4D94u) /* 0x8009B26C */
#endif

static u32 wm_94088_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    WM_94088_TRACE_LOAD(addr, v);
    return v;
}

static void wm_94088_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_94088_TRACE_STORE(addr, v);
}

static s32 wm_94088_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_94088_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA of a 32-bit register. */
static u32 wm_94088_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

/* mult + mflo: low 32 bits of the signed 32x32 product.  Unsigned
 * multiply produces the identical low word without overflow UB. */
static u32 wm_94088_mult_lo(u32 a, u32 b)
{
    return a * b;
}

s32 wm_80094088(u32 attr_vec, u32 src_vec, u32 dst_vec)
{
    u32 idx;
    u32 src_x;
    u32 tab_x;
    u32 src_z;
    u32 tab_z;
    u32 sum;

    {
        s32 n = WM_94088_CALL_93FE4(attr_vec);
        u32 bits = wm_94088_as_u32(n);

        /* sll $v0, 16 / sra $a1, $v0, 12 */
#if defined(WM_94088_MUTANT_WRONG_SHIFT)
        idx = wm_94088_sra(bits << 16, 13);
#else
        idx = wm_94088_sra(bits << 16, 12);
#endif
    }

#if defined(WM_94088_MUTANT_SWAPPED_AXES)
    src_x = wm_94088_load_u32(src_vec + 8u);
#else
    src_x = wm_94088_load_u32(src_vec + 0u);
#endif
    tab_x = wm_94088_load_u32(WM_94088_TABLE_X + idx);
    sum = wm_94088_mult_lo(src_x, tab_x);
    src_z = wm_94088_load_u32(src_vec + 8u);
    tab_z = wm_94088_load_u32(WM_94088_TABLE_Z + idx);
    sum += wm_94088_mult_lo(src_z, tab_z);

    if (sum == 0u) {
        /* dst.X = src.X; reload src.Z; dst.Z = src.Z; return 0. */
        wm_94088_store_u32(dst_vec + 0u, src_x);
        src_z = wm_94088_load_u32(src_vec + 8u);
        wm_94088_store_u32(dst_vec + 8u, src_z);
#if defined(WM_94088_MUTANT_ZERO_PATH_RETURN)
        return 1;
#else
        return 0;
#endif
    }

#if defined(WM_94088_MUTANT_WRONG_BRANCH_POLARITY)
    if (wm_94088_as_s32(sum) > 0) {
#else
    if (wm_94088_as_s32(sum) < 0) {
#endif
        /* negu: 32-bit two's-complement wrap. */
        wm_94088_store_u32(dst_vec + 0u, 0u - tab_x);
        tab_z = wm_94088_load_u32(WM_94088_TABLE_Z + idx);
#if defined(WM_94088_MUTANT_MISSING_Z_NEGATE)
        wm_94088_store_u32(dst_vec + 8u, tab_z);
#else
        wm_94088_store_u32(dst_vec + 8u, 0u - tab_z);
#endif
        return 1;
    }

    wm_94088_store_u32(dst_vec + 0u, tab_x);
    tab_z = wm_94088_load_u32(WM_94088_TABLE_Z + idx);
    wm_94088_store_u32(dst_vec + 8u, tab_z);
    return 1;
}
