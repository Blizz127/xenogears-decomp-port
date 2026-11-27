#ifndef XENO_ROBO_SHOP_LIT_FT3_H
#define XENO_ROBO_SHOP_LIT_FT3_H

#include <stdint.h>

/* Submit one relocated row-1, mode-1 group to the caller-owned 1024-word
 * depth OT with the NCS input timing observed on the map-292 opening actor.
 * The caller owns the validated model, command lookahead, packet storage and
 * OT. A zero return requires restoring the transaction, including any prior
 * writes to packet, OT or resident state. */
int32_t PcPort_RoboShopDrawLitFt3Group(uint8_t *model, uint8_t *packets,
                                      uint32_t *ot, int32_t mode);

/* Submit the authored faces of a validated single FT3 row. The caller owns
 * every indexed vertex, face normal, packet and depth bucket. The row ends
 * at the last triangle. Its packet result omits the terminal speculative
 * projection. Used for the unclipped shop draw (D_80050104 == 0) inside a
 * transaction that restores the GTE before the interrupted renderer resumes. */
int32_t PcPort_RoboShopDrawAuthoredLitFt3Group(uint8_t *model, uint8_t *packets,
                                              uint32_t *ot, int32_t mode);

/* Original mixed row order: lit FT3 then lit FT4. Same ownership and GTE
 * restoration contract as the single-row counted submission above. */
int32_t PcPort_RoboShopDrawAuthoredLitFt3Ft4Group(uint8_t *model, uint8_t *packets,
                                                 uint32_t *ot, int32_t mode);

/* Draw one to four authored FT3, GT3 or FT4 rows in model order. The caller
 * owns the counted tuples, indexed vertices/normals, flat-face lights, packet
 * bank, depth table and GTE/scratch transaction. */
int32_t PcPort_RoboShopDrawAuthoredGearGroup(uint8_t *model, uint8_t *packets,
                                            uint32_t *ot, int32_t mode);

#endif
