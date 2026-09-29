/* Focused production-linked certificate for retail 0x80071488. */
#include <stdio.h>

#include "world_map_frame_driver_712d0.h"

static int scheduler_calls;

void wm_80097800(void)
{
    scheduler_calls++;
}

int main(void)
{
    wm_712d0_run_second_scheduler();
    if (scheduler_calls != 1) {
        fprintf(stderr, "ASSERTION second.scheduler.exactly.once got=%d\n",
                scheduler_calls);
        return 1;
    }
    puts("W34B68 second scheduler certificate PASS");
    return 0;
}
