/* Test-only instrumentation; relink with --wrap=PcPort_Hd2dPostProcess.
 * Measures actual field presentation composition, not total game GPU time.
 * glFinish brackets each hook for completed work. Never part of production. */
#define _POSIX_C_SOURCE 200809L
#include "hd2d_integration.h"
#include "quick_checkpoint.h"
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern void __real_PcPort_Hd2dPostProcess(int,int,int);
extern int g_GameSceneMapNum;
static double samples[240], previous, intervals[240];
static unsigned seen, count;
static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (double)t.tv_sec+(double)t.tv_nsec*1e-9;
}
static int compare(const void *a,const void *b) {
    double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y);
}
static void report(void) {
    if(count<120) return;
    double sorted[240];
    for(unsigned i=0;i<count;i++) sorted[i]=samples[i];
    qsort(sorted,count,sizeof(*sorted),compare);
    double elapsed=0;for(unsigned i=0;i<count;i++) elapsed+=intervals[i];
    fprintf(stderr,"[hd2d-integrated-benchmark] map=%d enabled=%d samples=%u composition_median_ms=%.6f composition_p95_ms=%.6f present_interval_mean_ms=%.6f\n",
        g_GameSceneMapNum&0x3fff,PcPort_Hd2dEnabled(),count,sorted[count/2],sorted[(count*95)/100],elapsed/count);
}
void __wrap_PcPort_Hd2dPostProcess(int path,int width,int height) {
    const char *requested=getenv("XENO_HD2D_INTEGRATED_BENCHMARK");
    if(!requested || requested[0]!='1' || requested[1] || path!=0 ||
       !PcPort_QuickCheckpointFieldIsActive() || (g_GameSceneMapNum&0x3fff)!=14) {
        __real_PcPort_Hd2dPostProcess(path,width,height); return;
    }
    if(!seen++) {
        fprintf(stderr,"[hd2d-integrated-benchmark] GL_VENDOR=%s GL_RENDERER=%s GL_VERSION=%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
        atexit(report);
    }
    glFinish(); double start=now();
    __real_PcPort_Hd2dPostProcess(path,width,height);
    glFinish();double end=now();
    if(seen>480 && count<240) {
        samples[count]=(end-start)*1000;
        intervals[count]=(end-previous)*1000;count++;
        if(count==240) report();
    }
    previous=end;
}
