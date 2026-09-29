/*
 * World-map terrain/position initializer (W34B4C).
 *
 * Retail slice: 0x80097BC0–0x80097CB4.
 * Called from mode_update_init at retail 0x8007250C (when C894==0).
 *
 * Copies identity-like matrix to terrain matrix (D534), clears 1024-byte
 * cell table (C580), sets terrain period (C618=1024), masks initial world
 * position (BBB4/BBBC), initializes cell indices (C838/C83C), and calls
 * terrain helpers 0x800981C8 and 0x80097DC0.
 *
 * Provenance-preserving name wm_80097BC0.
 */
#ifndef WORLD_MAP_TERRAIN_INIT_H
#define WORLD_MAP_TERRAIN_INIT_H

#include "common.h"

/* Production terrain/position initializer.
 * Exact transcription of retail 0x80097BC0–0x80097CB4.
 * pos_ptr: guest address of initial position vector (typically 0x8009C5AC). */
void wm_80097BC0(u32 pos_ptr);

/* Resume twin (retail 0x80097CB8..0x80097D60): same matrix copy, clear and
 * C5BC/C618 stores, but keeps the saved terrain origin and cell indices.
 * pos_ptr: guest address of the saved position (0x8009BE28 at its call site). */
void wm_80097CB8(u32 pos_ptr);

/* Instrumentation counter accessors. */
int wm_tpi_get_calls(void);

/* Reset per-world-init counters. */
void wm_tpi_reset(void);

#endif /* WORLD_MAP_TERRAIN_INIT_H */
