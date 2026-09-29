/*
 * World-map pool slot claim 0x80097770.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80097770, 0x800977A8).  See world_map_helper_97770.h.
 *
 *   80097770  lui   $v0, 0x800a
 *   80097774  lw    $v0, -0x41dc($v0)      ; pool base *(0x8009BE24)
 *   80097778  sll   $a0, $a0, 7            ; idx * 128
 *   8009777c  addu  $a0, $v0, $a0
 *   80097780  lh    $v0, 4($a0)            ; occupancy halfword
 *   80097788  bnez  $v0, ret0
 *   8009778c  move  $v0, $zero
 *   80097790  addiu $v0, $zero, 1
 *   80097794  addiu $v1, $zero, 1
 *   80097798  sh    $v1, ($a0)
 *   8009779c  sh    $a1, 4($a0)
 *   800977a0  jr    $ra
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_97770.h"

#if defined(WM_97770_TEST_TRACE)
extern void wm_97770_test_store(u32 address, u32 value, u32 width);
#define WM_97770_TRACE_STORE(a, v, w) wm_97770_test_store((a), (v), (w))
#else
#define WM_97770_TRACE_STORE(a, v, w) ((void)0)
#endif

#define WM_97770_POOL_PTR 0x8009BE24u

static u32 wm_97770_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s16 wm_97770_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_97770_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_97770_TRACE_STORE(addr, v, 2u);
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    u32 pool = wm_97770_load_u32(WM_97770_POOL_PTR);
#if defined(WM_97770_MUTANT_WRONG_STRIDE)
    u32 slot = pool + (slot_idx << 6);
#else
    u32 slot = pool + (slot_idx << 7);
#endif

#if defined(WM_97770_MUTANT_WRONG_OCC_OFFSET)
    if (wm_97770_load_s16(slot + 2u) != 0)
#elif defined(WM_97770_MUTANT_BUSY_POLARITY)
    if (wm_97770_load_s16(slot + 4u) == 0)
#else
    if (wm_97770_load_s16(slot + 4u) != 0)
#endif
        return 0;
#if !defined(WM_97770_MUTANT_MISSING_MARK)
    wm_97770_store_u16(slot + 0u, 1u);
#endif
#if defined(WM_97770_MUTANT_WRONG_VALUE_WIDTH)
    {
        u32 wide;

        memcpy(&wide, &value, sizeof(wide));
        memcpy(PSX_ADDR(slot + 4u), &wide, sizeof(wide));
        WM_97770_TRACE_STORE(slot + 4u, wide, 4u);
    }
#else
    wm_97770_store_u16(slot + 4u, (u16)(u32)value);
#endif
    return 1;
}
