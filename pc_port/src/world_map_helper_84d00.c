/*
 * World-map region scanner 0x80084D00.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80084D00, 0x80084DB8).  See world_map_helper_84d00.h.
 *
 * Retail flow:
 *   count = lh [0x8009D7E0]; if (count <= 0) return 0
 *   rec = lw [0x8009C620]; i = 0
 *   loop:
 *     if (lhu rec[4] & 1) {
 *         r = sign16(wm_80084DB8(pos_vec, sign16(i)))
 *         if (r != 0) { sh i -> *out_attr; return r; }
 *     }
 *     i++; rec += 0x54
 *     if (sign16(i) < lh [0x8009D7E0])   <- count RELOADED every pass
 *         goto loop
 *   return 0
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_84db8.h"
#include "world_map_helper_84d00.h"

#if defined(WM_84D00_TEST_TRACE)
extern s32 wm_84d00_test_84db8(u32 pos_vec, s32 region_idx);
#define WM_84D00_CALL_84DB8(p, i) wm_84d00_test_84db8((p), (i))
#else
#define WM_84D00_CALL_84DB8(p, i) wm_80084DB8((p), (i))
#endif

#define WM_84D00_COUNT_ADDR 0x8009D7E0u
#define WM_84D00_REC_PTR    0x8009C620u

static u32 wm_84d00_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static u16 wm_84d00_load_u16(u32 addr)
{
    u16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s16 wm_84d00_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_84d00_store_u16(u32 addr, u16 v) __attribute__((unused));
static void wm_84d00_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static s32 wm_84d00_sign16(s32 v)
{
    return (s32)(s16)(u16)((u32)v & 0xFFFFu);
}

s32 wm_80084D00(u32 pos_vec, u32 out_attr)
{
    s32 count = (s32)wm_84d00_load_s16(WM_84D00_COUNT_ADDR);
    u32 rec = wm_84d00_load_u32(WM_84D00_REC_PTR);
    s32 i = 0;

    if (count <= 0)
        return 0;
    for (;;) {
#if defined(WM_84D00_MUTANT_FLAG_BIT)
        if ((wm_84d00_load_u16(rec + 4u) & 2u) != 0u) {
#else
        if ((wm_84d00_load_u16(rec + 4u) & 1u) != 0u) {
#endif
            s32 r = WM_84D00_CALL_84DB8(pos_vec, wm_84d00_sign16(i));

#if !defined(WM_84D00_MUTANT_RET_UNTRUNC)
            r = wm_84d00_sign16(r);
#endif
            if (r != 0) {
#if defined(WM_84D00_MUTANT_STORE_WIDTH)
                u32 wide = (u32)i;

                memcpy(PSX_ADDR(out_attr), &wide, sizeof(wide));
#else
                wm_84d00_store_u16(out_attr, (u16)(u32)i);
#endif
                return r;
            }
        }
        i = i + 1;
        rec += 0x54u;
#if defined(WM_84D00_MUTANT_COUNT_NO_RELOAD)
        if (!(wm_84d00_sign16(i) < count))
            break;
#else
        /* Retail reloads the region count on every iteration. */
        if (!(wm_84d00_sign16(i) <
              (s32)wm_84d00_load_s16(WM_84D00_COUNT_ADDR)))
            break;
#endif
    }
    return 0;
}
