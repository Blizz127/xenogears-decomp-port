#define _POSIX_C_SOURCE 200809L
#include "hd2d_batch_stats.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int enabled;
int PcPort_Hd2dEnabled(void) { return enabled; }
int main(int argc, char **argv)
{
    PcPortHd2dBatchStats s={0}, before;
    int active=argc==2 && !strcmp(argv[1],"active");
    assert(!setenv("XENO_HD2D_BATCH_STATS",active?"1":"0",1));
    PcPort_Hd2dGpuBatchObserve(4095,4096,65535,1);
    assert(!PcPort_Hd2dGpuBatchStats((void *)1)); /* OFF touches no output */
    enabled=1;
    PcPort_Hd2dGpuBatchObserve(96,4096,1200,0);
    if (!active) { assert(!PcPort_Hd2dGpuBatchStats((void *)1)); return 0; }
    PcPort_Hd2dGpuBatchObserve(180,4096,1400,0);
    PcPort_Hd2dGpuBatchObserve(96,4096,1200,0);
    assert(PcPort_Hd2dGpuBatchStats(&s));
    assert(s.completed_batches==3 && s.peak_splits==180 && s.peak_vertices==1400 && s.array_limit==4096 && !s.capacity_rejections);
    before=s;
    PcPort_Hd2dGpuBatchObserve(4096,4096,9999,0); /* invalid index */
    PcPort_Hd2dGpuBatchObserve(3,100,9999,0); /* capacity changed */
    assert(PcPort_Hd2dGpuBatchStats(&s));assert(!memcmp(&s,&before,sizeof s));
    enabled=0;PcPort_Hd2dGpuBatchObserve(4095,4096,1500,1);
    enabled=1;assert(PcPort_Hd2dGpuBatchStats(&s));assert(!memcmp(&s,&before,sizeof s));
    PcPort_Hd2dGpuBatchObserve(4095,4096,1600,1);
    assert(PcPort_Hd2dGpuBatchStats(&s));
    assert(s.completed_batches==3 && s.capacity_rejections==1 && s.peak_splits==4095 && s.peak_vertices==1600);
    return 0;
}
