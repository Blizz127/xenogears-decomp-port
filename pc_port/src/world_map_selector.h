/*
 * World-map selector producer module.
 *
 * Extracted from world_map_init.c so that both the production game build
 * and the production-linked test link the same authoritative object.
 *
 * Retail slice: wm_80071B9C selector binning (0x80071B9C–0x80071C10).
 * Corrected to match retail lhu zero-extension and branch-delay increment.
 */
#ifndef WORLD_MAP_SELECTOR_H
#define WORLD_MAP_SELECTOR_H

#include "common.h"

/* Retail record table addresses (absolute PSX). */
#define WM_RECORD_TABLE_ABS      0x8009B57Cu
#define WM_RECORD_GE8_ABS        0x8009B58Cu
#define WM_SLOT_C610_ABS         0x8009C610u

/*
 * wm_selector_producer — corrected retail 0x80071B9C selector binning.
 *
 * For entrance < 8: reads the zero-extended u16 seed, bins it against nine
 * zero-extended u16 thresholds at 0x8009B566..0x8009B576, writes the
 * resulting selector (0–8) as a u32 to 0x8009C610.
 * Returns the incremented index (1–9) for record-table addressing.
 *
 * For entrance >= 8: no selector write; returns entrance for record-table
 * addressing. BSS clear remains authoritative.
 *
 * Seed 0xFFFF is a retail malformed edge (unbounded threshold scan);
 * the defensive backstop logs and preserves the existing C610 value.
 *
 * Caller uses the return value for record lookup:
 *   pRec = PSX_ADDR(WM_RECORD_TABLE_ABS) + (ret << 3)  [entrance < 8]
 *   pRec = PSX_ADDR(WM_RECORD_GE8_ABS) + (entrance << 3) [entrance >= 8]
 */
u32 wm_selector_producer(u32 entrance, u32 seed);

/* Selector instrumentation counter accessors. */
int wm_selector_get_entries(void);
int wm_selector_get_valid_writes(void);
int wm_selector_get_nowrite_ge8(void);
int wm_selector_get_malformed_seeds(void);
int wm_selector_get_last_selector(void);

/* Reset all per-world-init selector counters. */
void wm_selector_reset(void);

#endif /* WORLD_MAP_SELECTOR_H */
