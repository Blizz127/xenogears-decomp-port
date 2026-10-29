#ifndef PC_PORT_HD2D_BATCH_STATS_H
#define PC_PORT_HD2D_BATCH_STATS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Counts native PsyCross draw splits, including empty allocated slots because
 * those consume the fixed array. Slot zero is a sentinel, hence limit-1 usable. */
typedef struct {
    uint64_t completed_batches, capacity_rejections;
    unsigned peak_splits, peak_vertices, array_limit;
} PcPortHd2dBatchStats;
/* Optional via XENO_HD2D_BATCH_STATS=1, master ON. capacity_rejected is 0 for
 * completed GPU batch; 1 at the existing split allocation rejection. */
void PcPort_Hd2dGpuBatchObserve(int splits, int array_limit, int vertices,
    int capacity_rejected);
/* OFF or instrumentation disabled returns zero before touching out. */
int PcPort_Hd2dGpuBatchStats(PcPortHd2dBatchStats *out);
#ifdef __cplusplus
}
#endif
#endif
