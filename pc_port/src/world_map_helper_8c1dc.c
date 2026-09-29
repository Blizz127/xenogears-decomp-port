/*
 * World-map terrain-class dispatch helper 0x8008C1DC.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x8008C1DC, 0x8008C28C).  See world_map_helper_8c1dc.h.
 *
 * Retail store order in the class-3 arm: +0xA0, +0xA2, +0xAC, +0xA8,
 * +0xA4, then +0xAA in the jal delay slot.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_8c1dc.h"

extern void wm_80089160(u32 a0, u32 a1, u32 a2);
extern void wm_800894C8(u32 record_index);

#if defined(WM_8C1DC_TEST_TRACE)
extern s32 wm_8c1dc_test_93f18(u32 vec_addr);
extern void wm_8c1dc_test_89160(u32 a0, u32 a1, u32 a2);
extern void wm_8c1dc_test_894c8(u32 a0);
extern void wm_8c1dc_test_store(u32 address, u32 value);
#define WM_8C1DC_CALL_93F18(a)       wm_8c1dc_test_93f18(a)
#define WM_8C1DC_CALL_89160(a, b, c) wm_8c1dc_test_89160((a), (b), (c))
#define WM_8C1DC_CALL_894C8(a)       wm_8c1dc_test_894c8(a)
#define WM_8C1DC_TRACE_STORE(a, v)   wm_8c1dc_test_store((a), (v))
#else
#define WM_8C1DC_CALL_93F18(a)       wm_80093F18(a)
#define WM_8C1DC_CALL_89160(a, b, c) wm_80089160((a), (b), (c))
#define WM_8C1DC_CALL_894C8(a)       wm_800894C8(a)
#define WM_8C1DC_TRACE_STORE(a, v)   ((void)0)
#endif

static u32 wm_8c1dc_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_8c1dc_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_8C1DC_TRACE_STORE(addr, v);
}

/* Exact MIPS SRA. */
static u32 wm_8c1dc_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

void wm_8008C1DC(u32 ctx, u32 obj, u32 out)
{
    s32 r = WM_8C1DC_CALL_93F18(obj + 0x28u);

#if !defined(WM_8C1DC_MUTANT_MISSING_SIGN16)
    r = (s32)(s16)(u16)((u32)r & 0xFFFFu);
#endif
#if defined(WM_8C1DC_MUTANT_WRONG_BRANCH_CONST)
    if (r == 2) {
#else
    if (r == 3) {
#endif
        wm_8c1dc_store_u16(out + 0xA0u,
                           (u16)wm_8c1dc_sra(wm_8c1dc_load_u32(obj + 0x28u),
                                             12u));
        wm_8c1dc_store_u16(out + 0xA2u,
                           (u16)wm_8c1dc_sra(wm_8c1dc_load_u32(obj + 0x2Cu),
                                             12u));
        wm_8c1dc_store_u16(out + 0xACu, 0u);
        wm_8c1dc_store_u16(out + 0xA8u, 0u);
#if defined(WM_8C1DC_MUTANT_WRONG_OFFSET_A4)
        wm_8c1dc_store_u16(out + 0xA6u,
                           (u16)wm_8c1dc_sra(wm_8c1dc_load_u32(obj + 0x30u),
                                             12u));
#else
        wm_8c1dc_store_u16(out + 0xA4u,
                           (u16)wm_8c1dc_sra(wm_8c1dc_load_u32(obj + 0x30u),
                                             12u));
#endif
#if defined(WM_8C1DC_MUTANT_WRONG_5C_WIDTH)
        {
            u32 wide = wm_8c1dc_load_u32(obj + 0x5Cu);

            memcpy(PSX_ADDR(out + 0xAAu), &wide, sizeof(wide));
            WM_8C1DC_TRACE_STORE(out + 0xAAu, wide);
        }
#else
        /* sh of the low half of the word at +0x5C (jal delay slot). */
        wm_8c1dc_store_u16(out + 0xAAu,
                           (u16)wm_8c1dc_load_u32(obj + 0x5Cu));
#endif
#if defined(WM_8C1DC_MUTANT_SWAPPED_CALLEE)
        WM_8C1DC_CALL_894C8(ctx);
    } else {
        WM_8C1DC_CALL_89160(ctx, out + 0xA0u, out + 0xA8u);
    }
#else
        WM_8C1DC_CALL_89160(ctx, out + 0xA0u, out + 0xA8u);
    } else {
        WM_8C1DC_CALL_894C8(ctx);
    }
#endif
}
