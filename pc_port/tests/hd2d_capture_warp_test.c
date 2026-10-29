#include <assert.h>
int g_GameSceneMapNum;
unsigned g_PcPortPresentedFrames;
static int safe,allow,requests;
int PcPort_QuickCheckpointFieldIsSafe(void) {return safe;}
int PcPort_QuickCheckpointRequestWarp(int map,int entrance) {
    assert(map==12 && entrance==0);requests++;return allow;
}
void PcPort_Hd2dCaptureWarpTick(void);
int main(void) {
    g_GameSceneMapNum=490;safe=1;PcPort_Hd2dCaptureWarpTick();assert(!requests);
    g_GameSceneMapNum=14;safe=0;PcPort_Hd2dCaptureWarpTick();assert(!requests);
    safe=1;PcPort_Hd2dCaptureWarpTick();assert(requests==0);
    g_PcPortPresentedFrames=120;PcPort_Hd2dCaptureWarpTick();assert(requests==1);
    allow=1;PcPort_Hd2dCaptureWarpTick();assert(requests==2);
    PcPort_Hd2dCaptureWarpTick();assert(requests==2);
    g_GameSceneMapNum=12;safe=0;PcPort_Hd2dCaptureWarpTick();
    safe=1;PcPort_Hd2dCaptureWarpTick();PcPort_Hd2dCaptureWarpTick();
    return 0;
}
