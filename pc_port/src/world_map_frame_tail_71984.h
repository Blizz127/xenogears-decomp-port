/*
 * World-map frame tail [0x80071994, 0x800719CC): geometry offset, OT
 * submission, and the first D554 backedge test.
 */
#ifndef WORLD_MAP_FRAME_TAIL_71984_H
#define WORLD_MAP_FRAME_TAIL_71984_H

#include "common.h"

int wm_80071984_tail(void);
void wm_71984_tail_reset(void);
int wm_71984_tail_get_unknowns(void);
int wm_71984_tail_get_geom_calls(void);
int wm_71984_tail_get_draw_calls(void);
int wm_71984_tail_get_backedge_hits(void);
u32 wm_71984_tail_get_last_ot(void);

#endif /* WORLD_MAP_FRAME_TAIL_71984_H */
