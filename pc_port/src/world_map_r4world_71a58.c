/*
 * World-map R4_WORLD callback 0x80071A58.
 * Orchestrates all Phase 3-5 helpers.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_r4world_71a58.h"
#include "world_map_helper_737ec.h"
#include "world_map_helper_740b8.h"
#include "world_map_helper_747dc.h"
#include "world_map_helper_848f4.h"
#include "world_map_helper_85cdc.h"
#include "world_map_helper_8615c.h"
#include "world_map_helper_86798.h"
#include "world_map_helper_89748.h"
#include "world_map_helper_89c78.h"
#include "world_map_helper_96130.h"
#include "world_map_helper_97244.h"
#include "world_map_helper_9623c.h"
#include "world_map_helper_980d4.h"
#include "world_map_helper_981c8.h"
#include "world_map_helper_983a0.h"
#include "world_map_helper_98cc0.h"
#include "world_map_helper_9932c.h"
#include "world_map_helper_73b04.h"

#define D_8009D144  0x8009D144u
#define D_8009BD40  0x8009BD40u
#define D_8009D558  0x8009D558u
#define D_8009C5BC  0x8009C5BCu
#define D_8009BE3C  0x8009BE3Cu
#define D_8009BBB4  0x8009BBB4u
#define D_8006EE76  0x8006EE76u

static s32 r_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void r_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static s16 r_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 r_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

s32 wm_80071A58(s32 slot_idx)
{
    s32 flag;

    /* Step 1: GTE matrix setup (conditional on D_8009D144) */
    flag = r_lw(D_8009D144);
    if (flag == 0) {
        wm_80097440(D_8009BD40);
    } else {
        wm_80097244(D_8009BD40);
    }

    /* Step 2: Particle spawning */
    wm_80089748();

    /* Step 3: Scaled object rendering */
    wm_80089C78(D_8009BBB4);

    /* Step 4: Object position update */
    wm_80085CDC();

    /* Step 5: Vertex setup */
    wm_8008615C();

    /* Step 6: GTE_OT renderer B */
    wm_800747DC();

    /* Step 7: Rendering structure init */
    wm_800848F4();

    /* Step 8: position wrap / paging gate (retail 0x80071AC8-D0: a0 = 0x8009BBB4) */
    wm_800980D4(D_8009BBB4);

    /* Step 9: Tile processing (conditional on D_8009D558) */
    if (r_lh(D_8009D558) != 0) {
        wm_800981C8(D_8009BBB4);
        wm_80096130();
        wm_80098CC0();
    }

    /* Step 10: Matrix-composed GTE processing */
    wm_800983A0(D_8009BBB4);

    /* Step 11: Rendering context setup */
    {
        u32 src = (u32)r_lw(D_8009BE3C);
        u32 pal_data = r_lw(src + 0x70);
        u32 pal_data2 = r_lw(src + 0x74);
        wm_8009932C(pal_data, pal_data2, D_8009BBB4);
    }

    /* Step 12: Advance palette index */
    {
        u32 idx = (u32)r_lw(D_8009C5BC);
        r_sw(D_8009C5BC, idx + 0x40);
    }

    /* Step 13: Geometry submission */
    wm_80073B04();

    /* Step 14: Sky dome */
    wm_800737EC();

    /* Step 15: OT dispatcher */
    wm_80086798();

    /* Step 16: Conditional GTE_OT renderer A */
    if (r_lhu(D_8006EE76) == 0) {
        wm_800740B8();
    }

    return 1;
}
