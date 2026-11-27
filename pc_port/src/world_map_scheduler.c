#include "world_map_scheduler.h"
#include "world_map_unavailable.h"

static int attempted;

void wm_80097800(void)
{
    attempted = 1;
    PcPort_WorldMapUnavailable(__func__);
}

void wm_sched_reset(void)
{
    attempted = 0;
}

int wm_sched_get_entry(void)
{
    return attempted;
}

int wm_sched_get_outcome(void)
{
    return attempted ? WM_SCHED_STOP_UNAVAILABLE : WM_SCHED_NOT_STARTED;
}

uint32_t wm_sched_get_frontier_pc(void)
{
    return attempted ? WM_SCHED_RETAIL_ENTRY : 0;
}
