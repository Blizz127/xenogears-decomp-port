#ifndef WORLD_MAP_MODE15_LIFECYCLE_H
#define WORLD_MAP_MODE15_LIFECYCLE_H

/* Retail mode-15 mode-table lifecycle callbacks. */
int wm_8007D918(void);
void wm_8007DCE0(void);

/* Slot 0 submits the shared archive wave; slot 1 polls it and applies the
 * retail 0x80076954 relocation layout. */
int wm_mode15_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE15_LIFECYCLE_H */
