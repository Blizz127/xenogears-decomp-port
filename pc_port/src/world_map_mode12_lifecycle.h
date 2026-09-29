#ifndef WORLD_MAP_MODE12_LIFECYCLE_H
#define WORLD_MAP_MODE12_LIFECYCLE_H

/* Retail mode-12 mode-table lifecycle callbacks. */
int wm_8007BF50(void);
void wm_8007C260(void);

/* Slot 0 submits the shared archive wave; slot 1 polls it and applies the
 * retail 0x80076954 relocation layout used by mode 12. */
int wm_mode12_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE12_LIFECYCLE_H */
