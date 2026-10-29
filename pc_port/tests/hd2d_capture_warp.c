/* Test-only native warp request after loading a real source checkpoint. */
#include "quick_checkpoint.h"
#include <stdio.h>
#ifndef HD2D_CAPTURE_MAP
#define HD2D_CAPTURE_MAP 12
#endif
#ifndef HD2D_CAPTURE_ENTRANCE
#define HD2D_CAPTURE_ENTRANCE 0
#endif
extern int g_GameSceneMapNum;
extern unsigned g_PcPortPresentedFrames;
void PcPort_Hd2dCaptureWarpTick(void) {
    static int requested,ready;
    static unsigned source_safe_at;
    int map=g_GameSceneMapNum&0x3fff;
    if(!requested && map==14 && PcPort_QuickCheckpointFieldIsSafe()) {
        if(!source_safe_at) {
            source_safe_at=g_PcPortPresentedFrames+1;
            fprintf(stderr,"[capture-warp] source-safe map=14 present=%u settle=120\n",source_safe_at);
        }
        if(g_PcPortPresentedFrames+1-source_safe_at<120) return;
        if(PcPort_QuickCheckpointRequestWarp(HD2D_CAPTURE_MAP,HD2D_CAPTURE_ENTRANCE)) {
            requested=1;
            fprintf(stderr,"[capture-warp] requested source-map=14 destination-map=%d entrance=%d present=%u\n",HD2D_CAPTURE_MAP,HD2D_CAPTURE_ENTRANCE,g_PcPortPresentedFrames+1);
        }
    }
    if(requested && !ready && map==HD2D_CAPTURE_MAP && PcPort_QuickCheckpointFieldIsSafe()) {
        ready=1;
        fprintf(stderr,"[capture-warp] destination-active map=%d entrance=%d present=%u\n",map,HD2D_CAPTURE_ENTRANCE,g_PcPortPresentedFrames+1);
    }
}
