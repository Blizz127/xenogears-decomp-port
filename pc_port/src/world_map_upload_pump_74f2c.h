/*
 * World-map upload pump 0x80074F2C.
 *
 * Retail range: [0x80074F2C, 0x8007502C), 64 instructions / 256 bytes.
 * The pump advances one 12-byte upload record per call and submits a
 * LoadImage only when that record's halfword timer reaches zero.
 */
#ifndef WORLD_MAP_UPLOAD_PUMP_74F2C_H
#define WORLD_MAP_UPLOAD_PUMP_74F2C_H

#include "common.h"

int wm_80074F2C(void);
void wm_74f2c_reset(void);
int wm_74f2c_get_unknowns(void);
int wm_74f2c_get_transfers(void);

#endif /* WORLD_MAP_UPLOAD_PUMP_74F2C_H */
