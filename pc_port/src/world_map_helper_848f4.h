/*
 * World-map region-model renderer 0x800848F4.
 *
 * Retail boundary: [0x800848F4, 0x80084D00), 0x40C bytes / 259
 * instructions.  Walks the 0x54-byte C620 region records, composes active
 * records with their optional parent chain and the camera, projects the
 * origin, and dispatches visible model buffers through func_8002C700.
 */
#ifndef WORLD_MAP_HELPER_848F4_H
#define WORLD_MAP_HELPER_848F4_H

void wm_800848F4(void);

#endif
