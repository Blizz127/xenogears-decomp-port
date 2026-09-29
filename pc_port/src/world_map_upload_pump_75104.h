/*
 * World-map upload pump 0x80075104.
 *
 * Retail range: [0x80075104, 0x80075228), 73 instructions / 292 bytes.
 */
#ifndef WORLD_MAP_UPLOAD_PUMP_75104_H
#define WORLD_MAP_UPLOAD_PUMP_75104_H

#include "common.h"

int wm_80075104(void);
void wm_75104_reset(void);
int wm_75104_get_unknowns(void);
int wm_75104_get_transfers(void);

#endif /* WORLD_MAP_UPLOAD_PUMP_75104_H */
