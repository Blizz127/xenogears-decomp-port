/* World-loop screenshot requests, fulfilled at PsyX_EndScene's sole effective
 * presentation boundary. */
#include <stdio.h>
#include <string.h>

#include "world_map_capture.h"

#define WM_CAPTURE_PATH_SIZE 512

extern void PsyX_TakeScreenshotPath(const char* path)
    __asm__("_Z23PsyX_TakeScreenshotPathPKc");

static int s_current_frame;
static int s_request_frame;
static int s_last_fulfilled_frame;
static int s_pending;
static int s_error;
static char s_request_path[WM_CAPTURE_PATH_SIZE];

void PcPort_WorldCaptureReset(void)
{
    s_current_frame = 0;
    s_request_frame = 0;
    s_last_fulfilled_frame = 0;
    s_pending = 0;
    s_error = 0;
    s_request_path[0] = '\0';
}

void PcPort_WorldCaptureSetFrame(int frame)
{
    s_current_frame = frame;
}

int PcPort_WorldCaptureRequest(int frame, const char* path)
{
    int length;

    if (s_pending != 0) {
        fprintf(stderr,
                "[worldmap-capture] ERROR request overwrite old_frame=%d "
                "new_frame=%d\n",
                s_request_frame, frame);
        s_error = 1;
        return -1;
    }
    if (path == NULL || frame != s_current_frame) {
        fprintf(stderr,
                "[worldmap-capture] ERROR invalid request frame=%d "
                "current=%d path=%s\n",
                frame, s_current_frame, path != NULL ? "set" : "null");
        s_error = 1;
        return -1;
    }
    length = snprintf(s_request_path, sizeof(s_request_path), "%s", path);
    if (length < 0 || (size_t)length >= sizeof(s_request_path)) {
        fprintf(stderr, "[worldmap-capture] ERROR path too long frame=%d\n",
                frame);
        s_error = 1;
        s_request_path[0] = '\0';
        return -1;
    }

    s_request_frame = frame;
    s_pending = 1;
    return 0;
}

void PcPort_WorldCaptureFulfillAtPresent(void)
{
    if (s_pending == 0)
        return;
    if (s_request_frame != s_current_frame) {
        fprintf(stderr,
                "[worldmap-capture] ERROR frame mismatch request=%d "
                "fulfillment=%d\n",
                s_request_frame, s_current_frame);
        s_error = 1;
        return;
    }

    PsyX_TakeScreenshotPath(s_request_path);
    s_last_fulfilled_frame = s_current_frame;
    s_pending = 0;
    fprintf(stderr,
            "[worldmap-capture] fulfilled request_frame=%d "
            "fulfillment_frame=%d path=%s\n",
            s_request_frame, s_last_fulfilled_frame, s_request_path);
}

int PcPort_WorldCaptureFrameComplete(int frame)
{
    if (s_pending != 0 && s_request_frame <= frame) {
        fprintf(stderr,
                "[worldmap-capture] ERROR unfulfilled request_frame=%d "
                "completed_frame=%d\n",
                s_request_frame, frame);
        s_error = 1;
    }
    return s_error != 0 ? -1 : 0;
}

int PcPort_WorldCaptureFinish(void)
{
    if (s_pending != 0) {
        fprintf(stderr,
                "[worldmap-capture] ERROR bounded exit with pending frame=%d\n",
                s_request_frame);
        s_error = 1;
    }
    return s_error != 0 ? -1 : 0;
}

int PcPort_WorldCapturePending(void)
{
    return s_pending;
}

int PcPort_WorldCaptureLastFulfilledFrame(void)
{
    return s_last_fulfilled_frame;
}

int PcPort_WorldCaptureCurFrame(void)
{
    return s_current_frame;
}
