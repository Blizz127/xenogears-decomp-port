#ifndef WORLD_MAP_MODE811_LIFECYCLE_H
#define WORLD_MAP_MODE811_LIFECYCLE_H

/* Retail mode-8/mode-11 mode-table lifecycle callbacks. */
int wm_80077214(void);
void wm_80077480(void);

/* Integration seams owned by world_map_init.c.  Retail slot 0 submits the
 * second archive wave; slot 1 later waits for it and performs its fixups. */
int wm_mode811_stage_second_wave_submit(void);
int wm_mode811_stage_second_wave_finish(void);

/* Retail teardown leaves shared with the base-world slot-2 callback. */
void wm_80086568(void);
void wm_80074F04(void);
void wm_800750DC(void);
void wm_80088FF4(void);
void wm_800976A0(void);

#endif /* WORLD_MAP_MODE811_LIFECYCLE_H */
