#ifndef WORLD_MAP_HELPER_75E7C_H
#define WORLD_MAP_HELPER_75E7C_H

#include "common.h"

/* Retail [0x80094028,0x80094060): select bits 2..5 from byte 3 of
 * the terrain cell containing the supplied guest VECTOR. */
s32 wm_80094028(u32 vec_addr);

/* Retail [0x80075E7C,0x80076098): choose a transition record from the
 * current terrain/threshold tables, copy its 0x200-byte payload to the
 * resident transition buffer, and publish the selected weighted entry. */
s32 wm_80075E7C(u32 vec_addr, s32 threshold);

#endif /* WORLD_MAP_HELPER_75E7C_H */
