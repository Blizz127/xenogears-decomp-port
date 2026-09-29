/*
 * World-map framebuffer/GTE initializer (W34B4B).
 *
 * Retail slice: 0x80072BB0–0x80072DB0.
 * Called from mode_update_init at retail 0x80072244.
 *
 * Initializes two complementary 320×216 draw/display environments for
 * PSX double-buffering, sets GTE screen distance (256), back color,
 * far color, and fog parameters.
 *
 * Provenance-preserving name wm_80072BB0.
 */
#ifndef WORLD_MAP_FRAMEBUFFER_INIT_H
#define WORLD_MAP_FRAMEBUFFER_INIT_H

#include "common.h"

/* Production framebuffer/GTE initializer.
 * Exact transcription of retail 0x80072BB0–0x80072DB0. */
void wm_80072BB0(void);

/* Instrumentation counter accessors. */
int wm_fbi_get_calls(void);

/* Reset per-world-init counters. */
void wm_fbi_reset(void);

#endif /* WORLD_MAP_FRAMEBUFFER_INIT_H */
