#ifndef WORLD_MAP_MODE13_LIFECYCLE_H
#define WORLD_MAP_MODE13_LIFECYCLE_H

/* Retail mode-13 mode-table lifecycle callbacks. */
int wm_8007FF70(void);
void wm_80080218(void);

/* Slot 0 submits the shared archive wave; slot 1 polls it and applies the
 * retail 0x80076954 relocation layout used by modes 9, 10, 12, 13, and 14. */
int wm_mode13_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE13_LIFECYCLE_H */
