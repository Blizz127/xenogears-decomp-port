/* World-map queued ground-object renderer 0x800747DC. */
#ifndef WORLD_MAP_HELPER_747DC_H
#define WORLD_MAP_HELPER_747DC_H

/* Retail boundary: [0x800747DC, 0x80074E58), 0x67C bytes / 415
 * instructions.  Consumes the compact placement queue at D_8009D30C and
 * publishes accepted 0x28-byte FT4 packets into the active guest OT. */
void wm_800747DC(void);

#endif
