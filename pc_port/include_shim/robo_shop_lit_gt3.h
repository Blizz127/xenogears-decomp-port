#ifndef XENO_ROBO_SHOP_LIT_GT3_H
#define XENO_ROBO_SHOP_LIT_GT3_H

#include <stdint.h>

/* Submit the productive triangles of one relocated, dense GT3 row. Caller
 * owns the authenticated row, matching vertex and indexed-normal extents,
 * count*40 packet bytes, scratchpad and 1024 depth words. Restore these
 * transaction resources and GTE before resuming the interrupted renderer. */
int32_t PcPort_RoboShopDrawAuthoredLitGt3Group(uint8_t *model, uint8_t *packets,
                                              uint32_t *ot, int32_t mode);

#endif
