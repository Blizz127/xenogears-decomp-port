#ifndef WORLD_MAP_HELPER_72DB4_H
#define WORLD_MAP_HELPER_72DB4_H

#include "common.h"

/* Retail 0x80072DB4: loading-transition presentation used by the base-world
 * session setup callback.  Returns 0 after the requested number of frames,
 * or -1 if one of its three temporary primitive allocations fails. */
int wm_80072DB4(s32 frame_count, s32 initial_intensity,
                s32 intensity_step, s32 abr);

#endif /* WORLD_MAP_HELPER_72DB4_H */
