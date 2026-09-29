/*
 * Retail world-map paired allocation teardown leaves.
 *
 * 0x80086124 frees D7EC then D7E8.
 * 0x800866C8 frees D7FC then D7F8.
 * 0x80089128 frees BE1C then BE20.
 */
#ifndef WORLD_MAP_HELPER_86124_H
#define WORLD_MAP_HELPER_86124_H

void wm_80086124(void);
void wm_800866C8(void);
void wm_80089128(void);

#endif
