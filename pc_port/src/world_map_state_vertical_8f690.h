/* Retail vertical-transition states 8/12/16/20 of wm_8008E76C, and the
 * port body of the sound-bank switch 0x800767D4 they call. */
#ifndef WORLD_MAP_STATE_VERTICAL_8F690_H
#define WORLD_MAP_STATE_VERTICAL_8F690_H

#include "common.h"

s32 wm_8008E76C_state8(u32 slot);
s32 wm_8008E76C_state12(u32 slot);
s32 wm_8008E76C_state16(u32 slot);
s32 wm_8008E76C_state20(u32 slot);
void wm_800767D4(u32 song_data, u32 archive_entry);

#endif /* WORLD_MAP_STATE_VERTICAL_8F690_H */
