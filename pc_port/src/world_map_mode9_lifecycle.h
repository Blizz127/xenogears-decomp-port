#ifndef WORLD_MAP_MODE9_LIFECYCLE_H
#define WORLD_MAP_MODE9_LIFECYCLE_H

/* Retail mode-9 mode-table lifecycle callbacks. */
int wm_80077A64(void);
void wm_80077CC0(void);

/* Retail archive/SEDS loader [0x800721E4,0x80072238). */
int wm_800721E4(void);

/* Slot-0 submits the shared archive wave; slot 1 polls it and applies the
 * distinct mode-9 relocation layout at 0x80076954. */
int wm_mode9_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE9_LIFECYCLE_H */
