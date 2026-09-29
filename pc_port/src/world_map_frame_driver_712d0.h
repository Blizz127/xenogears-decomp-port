/*
 * World-map frame driver 0x800712D0.
 *
 * Retail boundary: [0x800712D0, 0x80071A50), 643 instructions / 2572 bytes.
 * Session/frame render-update orchestrator. Manages controller input,
 * queue processing, CD sync, OT management, state-dependent rendering,
 * DrawOTag submission, and display environment setup.
 */
#ifndef WORLD_MAP_FRAME_DRIVER_712D0_H
#define WORLD_MAP_FRAME_DRIVER_712D0_H

#include "common.h"

typedef int (*Wm712D0FrameHook)(int frame, void* user);

typedef struct Wm712D0BoundedRun {
    /* Positive values request a bounded harness exit; zero is retail-style
     * unbounded recurrence until D554 becomes zero. */
    int frame_limit;
    int displayed_frames;
    Wm712D0FrameHook before_frame;
    Wm712D0FrameHook after_frame;
    void* user;
} Wm712D0BoundedRun;

typedef enum Wm712D0RunResult {
    WM_712D0_RUN_ERROR = -1,
    WM_712D0_RUN_NATURAL_EXIT = 0,
    WM_712D0_RUN_BOUNDED_EXIT = 1
} Wm712D0RunResult;

void wm_800712D0(void);
Wm712D0RunResult wm_800712D0_run_bounded(Wm712D0BoundedRun* run);
void wm_712d0_run_second_scheduler(void);
/* Retail 0x800713FC..0x8007142C recurring CD status/retry lane. */
void wm_712d0_run_cd_sync_lane(void);
/* Retail 0x800714C8..0x8007169C party-presence refresh lane. */
void wm_712d0_run_party_refresh_lane(void);
/* Retail 0x8007169C..0x80071774 modal pause/controller-loss lane. */
void wm_712d0_run_pause_lanes(void);
/* Retail 0x80071774..0x8007188C transition-selection and flag tail. */
void wm_712d0_run_transition_lane(void);
/* Retail 0x80071890..0x80071974 menu/field-transition dispatch lane. */
void wm_712d0_run_menu_mode_lane(void);

#endif
