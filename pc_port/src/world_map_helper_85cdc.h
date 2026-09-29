/*
 * World-map helper 0x80085CDC (object position updater).
 *
 * Retail boundary: [0x80085CDC, 0x80085F58), 159 instructions / 636 bytes.
 * Iterates 64 world objects (stride 0x80), updates native sprite transforms,
 * projects their anchors, submits visible sprites to the active guest OT,
 * turns them toward their requested heading, and advances animation scripts.
 */
#ifndef WORLD_MAP_HELPER_85CDC_H
#define WORLD_MAP_HELPER_85CDC_H

#include "common.h"

void wm_80085CDC(void);

#endif
