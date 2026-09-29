/*
 * World-map helper 0x80089C78 (scaled object renderer).
 *
 * Retail function boundary: [0x80089C78, 0x8008A2C8).  Iterates the dynamic
 * 256-record object pool, projects each live record's four static vertices,
 * and compacts accepted POLY_FT4 packets into the active guest OT.
 */
#ifndef WORLD_MAP_HELPER_89C78_H
#define WORLD_MAP_HELPER_89C78_H

#include "common.h"

void wm_80089C78(u32 input_addr);

#endif
