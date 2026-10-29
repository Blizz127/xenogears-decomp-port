/* Read-only engine telemetry; no GL calls, GPU state or split-array writes. */
#include "hd2d_batch_stats.h"
#include "hd2d_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static PcPortHd2dBatchStats stats;
static int requested = -1;
static void PrintSummary(void)
{
    if (!PcPort_Hd2dEnabled() || requested != 1) return;
    fprintf(stderr,"[hd2d-gpu-batches] summary batches=%llu peak_splits=%u array_limit=%u usable_slots=%u peak_vertices=%u capacity_rejections=%llu\n",
        (unsigned long long)stats.completed_batches, stats.peak_splits,
        stats.array_limit, stats.array_limit ? stats.array_limit-1 : 0,
        stats.peak_vertices, (unsigned long long)stats.capacity_rejections);
}
static int Requested(void)
{
    if (requested < 0) {
        const char *value = getenv("XENO_HD2D_BATCH_STATS");
        requested = value && !strcmp(value,"1");
        if (requested && atexit(PrintSummary)) requested=0;
    }
    return requested;
}
void PcPort_Hd2dGpuBatchObserve(int splits, int limit, int vertices, int rejected)
{
    int new_peak;
    if (!PcPort_Hd2dEnabled()) return;
    if (!Requested() || limit < 2 || splits < 0 || splits >= limit || vertices < 0 ||
        (rejected != 0 && rejected != 1)) return;
    /* A single executable must use one immutable engine array capacity. */
    if (stats.array_limit && stats.array_limit != (unsigned)limit) return;
    stats.array_limit=(unsigned)limit;
    new_peak=(unsigned)splits > stats.peak_splits;
    if (new_peak) stats.peak_splits=(unsigned)splits;
    if ((unsigned)vertices > stats.peak_vertices) stats.peak_vertices=(unsigned)vertices;
    if (rejected) stats.capacity_rejections++;
    else stats.completed_batches++;
    if (new_peak || rejected)
        fprintf(stderr,"[hd2d-gpu-batches] batches=%llu peak_splits=%u array_limit=%u usable_slots=%u peak_vertices=%u capacity_rejections=%llu\n",
            (unsigned long long)stats.completed_batches,stats.peak_splits,stats.array_limit,
            stats.array_limit-1,stats.peak_vertices,(unsigned long long)stats.capacity_rejections);
}
int PcPort_Hd2dGpuBatchStats(PcPortHd2dBatchStats *out)
{
    if (!PcPort_Hd2dEnabled()) return 0;
    if (!Requested() || !out) return 0;
    *out=stats;
    return 1;
}
