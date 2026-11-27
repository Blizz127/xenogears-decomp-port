#ifndef WORLD_MAP_SCHEDULER_H
#define WORLD_MAP_SCHEDULER_H

#include <stdint.h>

#define WM_SCHED_RETAIL_ENTRY UINT32_C(0x80097800)

typedef enum wm_sched_outcome {
    WM_SCHED_NOT_STARTED = 0,
    WM_SCHED_STOP_UNAVAILABLE = 5
} wm_sched_outcome_t;

void wm_80097800(void);
void wm_sched_reset(void);
int wm_sched_get_entry(void);
int wm_sched_get_outcome(void);
uint32_t wm_sched_get_frontier_pc(void);

#endif
