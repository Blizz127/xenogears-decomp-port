#ifndef WORLD_MAP_MODE14_LIFECYCLE_H
#define WORLD_MAP_MODE14_LIFECYCLE_H

/* Retail mode-14 mode-table lifecycle callbacks. */
int wm_8007A5DC(void);
void wm_8007A8AC(void);

/* Slot 0 submits the shared archive wave; slot 1 polls it and applies the
 * retail 0x80076954 relocation layout used by modes 9, 10, and 14. */
int wm_mode14_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE14_LIFECYCLE_H */
