#ifndef WORLD_MAP_MODE10_LIFECYCLE_H
#define WORLD_MAP_MODE10_LIFECYCLE_H

/* Retail mode-10 mode-table lifecycle callbacks. */
int wm_80078A60(void);
void wm_80078D24(void);

/* Slot-0 submits the shared archive wave; slot 1 polls it and applies the
 * retail 0x80076954 relocation layout used by modes 9 and 10. */
int wm_mode10_stage_second_wave_finish(void);

#endif /* WORLD_MAP_MODE10_LIFECYCLE_H */
