/* Probe the released scheduler boundary using synthetic state and no game media. */
#include "world_map_scheduler.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char expected[2 * 1024 * 1024];
unsigned char g_PsxRam[sizeof expected];

static void check_state(void)
{
    assert(wm_sched_get_entry() == 1);
    assert(wm_sched_get_outcome() == WM_SCHED_STOP_UNAVAILABLE);
    assert(wm_sched_get_frontier_pc() == WM_SCHED_RETAIL_ENTRY);
    assert(!memcmp(g_PsxRam, expected, sizeof expected));
    puts("World scheduler stopped at entry with guest memory intact");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const int reset = strcmp(argv[1], "cold") != 0;
    assert(!reset || !strcmp(argv[1], "reset"));
    memset(g_PsxRam, reset ? 0x3c : 0xa5, sizeof g_PsxRam);
    memcpy(expected, g_PsxRam, sizeof expected);
    if (reset) wm_sched_reset();
    assert(wm_sched_get_entry() == 0);
    assert(wm_sched_get_outcome() == WM_SCHED_NOT_STARTED);
    assert(wm_sched_get_frontier_pc() == 0);
    assert(atexit(check_state) == 0);
    wm_80097800();
    fputs("Scheduler returned unexpectedly\n", stderr);
    return 2;
}
