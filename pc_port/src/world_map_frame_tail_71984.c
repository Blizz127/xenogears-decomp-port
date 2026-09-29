/*
 * W34B36 — bounded frame tail through the first D554 backedge test.
 *
 * Retail decode:
 *   0x80071994 lw a1,D_8009BE0C
 *   0x8007199C SetGeomOffset(0xA0,a1)
 *   0x800719A8 lw v0,D_8009BE3C; 0x800719B0 lw a0,0x70(v0)
 *   0x800719B4 DrawOTag(a0 + 0xFFC)
 *   0x800719C0 lw v0,D_8009D554; 0x800719C8 bnez v0,0x8007130C
 *
 * The first backedge is deliberately LOG-AND-HOLD. A raw guest OT value is
 * never handed to DrawOTag: known PSX/KSEG1 addresses are mapped, unknown
 * environment/OT values are counted and the DrawOTag call is skipped.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_tail_71984.h"
#include "world_map_ot_adapter.h"

typedef unsigned long WmTailULong;

extern void SetGeomOffset(int ofx, int ofy);

#define WMTAIL_GEOM_OFFSET 0x8009BE0Cu
#define WMTAIL_ENV_PTR     0x8009BE3Cu
#define WMTAIL_D554        0x8009D554u

static int s_wmtail_unknowns;
static int s_wmtail_geom_calls;
static int s_wmtail_draw_calls;
static int s_wmtail_backedge_hits;
static u32 s_wmtail_last_ot;

void wm_71984_tail_reset(void)
{
    s_wmtail_unknowns = 0;
    s_wmtail_geom_calls = 0;
    s_wmtail_draw_calls = 0;
    s_wmtail_backedge_hits = 0;
    s_wmtail_last_ot = 0;
}

int wm_71984_tail_get_unknowns(void) { return s_wmtail_unknowns; }
int wm_71984_tail_get_geom_calls(void) { return s_wmtail_geom_calls; }
int wm_71984_tail_get_draw_calls(void) { return s_wmtail_draw_calls; }
int wm_71984_tail_get_backedge_hits(void) { return s_wmtail_backedge_hits; }
u32 wm_71984_tail_get_last_ot(void) { return s_wmtail_last_ot; }

static int wm_tail_guest_ptr_known(u32 value)
{
#if defined(WM_71984_MUTANT_UNKNOWN_GUARD)
    (void)value;
    return 1;
#else
    return ((value & 0xFFE00000u) == 0x80000000u) ||
           ((value & 0xFFE00000u) == 0xA0000000u);
#endif
}

static u32 wm_tail_load_u32(u32 address)
{
    return *(volatile u32 *)PSX_ADDR(address);
}

static void wm_tail_unknown(const char *kind, u32 value, u32 pc)
{
    s_wmtail_unknowns++;
    fprintf(stderr,
            "[worldmap-frame-tail] unknown %s=0x%08x at 0x%08x count=%d\n",
            kind, value, pc, s_wmtail_unknowns);
}

static int wm_tail_draw_otag(void)
{
    u32 env = wm_tail_load_u32(WMTAIL_ENV_PTR);
    u32 ot;
    u32 submit;

    if (!wm_tail_guest_ptr_known(env)) {
        wm_tail_unknown("env", env, 0x800719A8u);
        return 0;
    }
    ot = wm_tail_load_u32(env + 0x70u);
    submit = ot + 0xFFCu;
    if (!wm_tail_guest_ptr_known(ot) || !wm_tail_guest_ptr_known(submit)) {
        wm_tail_unknown("ot", ot, 0x800719B0u);
        return 0;
    }

    s_wmtail_last_ot = submit;
    /* W34B38: guest-native OT walk; host boundary is DrawPrim inside. */
    (void)wm_ot_draw_otag_guest(submit);
    s_wmtail_draw_calls++;
    return 1;
}

int wm_80071984_tail(void)
{
    s32 geom_offset = (s32)wm_tail_load_u32(WMTAIL_GEOM_OFFSET);
    u32 d554;

    /* 0x80071994..0x800719A0: immediate + derived scalar, no pointer map. */
    SetGeomOffset(0xA0, (int)geom_offset);
    s_wmtail_geom_calls++;

    /* 0x800719A4..0x800719B8: guest OT map-or-log wrapper. */
    (void)wm_tail_draw_otag();

    /* 0x800719BC..0x800719CC: first backedge decision. */
    d554 = wm_tail_load_u32(WMTAIL_D554);
    if (d554 != 0u) {
        s_wmtail_backedge_hits++;
        fprintf(stderr,
                "[worldmap-frame-tail] BACKEDGE 0x800719C8 d554=0x%08x "
                "held=1 hit=%d\n",
                d554, s_wmtail_backedge_hits);
        return 1;
    }
    return 0;
}
