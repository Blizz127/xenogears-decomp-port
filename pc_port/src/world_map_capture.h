#ifndef WORLD_MAP_CAPTURE_H
#define WORLD_MAP_CAPTURE_H

void PcPort_WorldCaptureReset(void);
void PcPort_WorldCaptureSetFrame(int frame);
int PcPort_WorldCaptureRequest(int frame, const char* path);
void PcPort_WorldCaptureFulfillAtPresent(void);
int PcPort_WorldCaptureFrameComplete(int frame);
int PcPort_WorldCaptureFinish(void);
int PcPort_WorldCapturePending(void);
int PcPort_WorldCaptureLastFulfilledFrame(void);
int PcPort_WorldCaptureCurFrame(void);

#endif
