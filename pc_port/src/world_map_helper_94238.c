/*
 * World-map rectangle-region trigger lookup 0x80094238.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80094238, 0x80094364).  See world_map_helper_94238.h.
 *
 * Coordinates: srl 12 (LOGICAL shift) of the raw 20.12 position words,
 * then andi 0xFFFF; compared SIGNED against sign-extended s16 record
 * fields with xori-inverted slt (inclusive bounds).
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_94238.h"

#if defined(WM_94238_TEST_TRACE)
extern void wm_94238_test_store(u32 address, u32 value, u32 width);
#define WM_94238_TRACE_STORE(a, v, w) wm_94238_test_store((a), (v), (w))
#else
#define WM_94238_TRACE_STORE(a, v, w) ((void)0)
#endif

#define WM_94238_TABLE_PTR 0x8009BD00u /* 0x800A0000 - 0x4300 */
#define WM_94238_G_RECPTR  0x8009D7D8u /* 0x800A0000 - 0x2828 */
#define WM_94238_G_ID_A    0x8009BD24u /* 0x800A0000 - 0x42DC */
#define WM_94238_G_ID_B    0x8009CE68u /* 0x800A0000 - 0x3198 */

static u32 wm_94238_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s16 wm_94238_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static u16 wm_94238_load_u16(u32 addr)
{
    u16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_94238_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_94238_TRACE_STORE(addr, v, 4u);
}

static void wm_94238_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_94238_TRACE_STORE(addr, v, 2u);
}

static s32 wm_94238_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static void wm_94238_miss_stores(void)
{
    wm_94238_store_u32(WM_94238_G_RECPTR, 0xFFFFFFFFu);
    wm_94238_store_u16(WM_94238_G_ID_A, 0xFFFFu);
    wm_94238_store_u16(WM_94238_G_ID_B, 0xFFFFu);
}

s32 wm_80094238(u32 pos_vec, u32 list_index)
{
    /* srl 12: LOGICAL shift.  (An sra here would be behaviorally
     * equivalent: the andi 0xFFFF below keeps only shifted bits [15:0]
     * = original bits [27:12], identical under both shifts — recorded
     * honestly in MUTANTS.csv, so no sra mutant is claimed.) */
    u32 cx_raw = wm_94238_load_u32(pos_vec + 0u) >> 12;
    u32 tab = wm_94238_load_u32(WM_94238_TABLE_PTR);
    u32 list = wm_94238_load_u32(tab + (list_index << 2));
    u32 cz_raw = wm_94238_load_u32(pos_vec + 8u) >> 12;
    s32 t2;
    s32 t1;
    u32 rec = list;

    if ((s32)wm_94238_load_s16(list + 8u) == -1) {
        wm_94238_miss_stores();
        return 0;
    }
#if defined(WM_94238_MUTANT_MISSING_COORD_MASK)
    t2 = wm_94238_as_s32(cx_raw);
    t1 = wm_94238_as_s32(cz_raw);
#else
    t2 = wm_94238_as_s32(cx_raw & 0xFFFFu);
    t1 = wm_94238_as_s32(cz_raw & 0xFFFFu);
#endif

    for (;;) {
        s32 x0 = (s32)wm_94238_load_s16(rec + 0u);
        s32 w = (s32)wm_94238_load_s16(rec + 4u);
        s32 h = (s32)wm_94238_load_s16(rec + 6u);
        s32 z0 = (s32)wm_94238_load_s16(rec + 2u);
        int in;

#if defined(WM_94238_MUTANT_EXCLUSIVE_BOUND)
        in = (t2 > x0) && (x0 + w >= t2) && (t1 >= z0) && (z0 + h >= t1);
#else
        in = (t2 >= x0) && (x0 + w >= t2) && (t1 >= z0) && (z0 + h >= t1);
#endif
        if (in) {
            s32 type = (s32)wm_94238_load_s16(rec + 0xEu);
            u32 id = (u32)wm_94238_load_u16(rec + 0xCu);

#if defined(WM_94238_MUTANT_WRONG_TYPE_CONST)
            if (type == 3) {
#else
            if (type == 4) {
#endif
#if defined(WM_94238_MUTANT_SWAPPED_GLOBALS)
                wm_94238_store_u32(WM_94238_G_RECPTR, 0xFFFFFFFFu);
                wm_94238_store_u16(WM_94238_G_ID_B, 0xFFFFu);
                wm_94238_store_u16(WM_94238_G_ID_A, (u16)id);
#else
                /* Retail store order: recptr, id_a, id_b. */
                wm_94238_store_u32(WM_94238_G_RECPTR, 0xFFFFFFFFu);
                wm_94238_store_u16(WM_94238_G_ID_A, 0xFFFFu);
                wm_94238_store_u16(WM_94238_G_ID_B, (u16)id);
#endif
            } else {
                /* Retail store order: recptr, id_b, id_a. */
                wm_94238_store_u32(WM_94238_G_RECPTR, rec);
                wm_94238_store_u16(WM_94238_G_ID_B, 0xFFFFu);
                wm_94238_store_u16(WM_94238_G_ID_A, (u16)id);
            }
            return 1;
        }
#if defined(WM_94238_MUTANT_WRONG_STRIDE)
        rec += 0xCu;
#else
        rec += 0x10u;
#endif
        if ((s32)wm_94238_load_s16(rec + 8u) == -1)
            break;
    }
    wm_94238_miss_stores();
    return 0;
}
