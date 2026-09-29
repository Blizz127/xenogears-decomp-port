/*
 * World-map helpers 0x80096130 and 0x800967E4 — queue synchronization.
 *
 * 0x80096130 [0x80096130, 0x8009623C): 67 insns. Queue drain loop.
 * Polls archive debug table, calls Vsync(0) + wm_800967E4 until
 * queue entries are consumed.
 *
 * 0x800967E4 [0x800967E4, 0x800968E0): 63 insns. Queue processor.
 * Checks archive status, calls file I/O (966CC) or CD (9699C)
 * depending on available data.
 *
 * 0x80096694 [0x80096694,0x800966CC): queue drain barrier. Calls Vsync(0),
 * processes one queue item through 0x800967E4, then repeats while the
 * circular head/tail distance from 0x80096668 is nonzero.
 */
#ifndef WORLD_MAP_HELPER_96130_H
#define WORLD_MAP_HELPER_96130_H

#include "common.h"

void wm_80096130(void);
void wm_80096694(void);
u32 wm_800967E4(void);
u32 wm_800968E0(void);

#endif
