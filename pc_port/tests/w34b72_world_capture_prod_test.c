#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world_map_capture.h"

static int s_screenshot_calls;
static char s_screenshot_path[512];

void PsyX_TakeScreenshotPath(const char* path)
    __asm__("_Z23PsyX_TakeScreenshotPathPKc");

void PsyX_TakeScreenshotPath(const char* path)
{
    s_screenshot_calls++;
    (void)snprintf(s_screenshot_path, sizeof(s_screenshot_path), "%s", path);
}

static void require(int condition, const char* message)
{
    if (condition == 0) {
        fprintf(stderr, "W34B72 FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void reset_fixture(void)
{
    s_screenshot_calls = 0;
    s_screenshot_path[0] = '\0';
    PcPort_WorldCaptureReset();
}

static void test_exact_frame_fulfillment(void)
{
    reset_fixture();
    PcPort_WorldCaptureSetFrame(60);
    require(PcPort_WorldCaptureRequest(60, "/tmp/frame-60.bmp") == 0,
            "valid request rejected");
    require(PcPort_WorldCapturePending() == 1,
            "request was not marked pending");
    require(s_screenshot_calls == 0, "request captured before presentation");

    PcPort_WorldCaptureFulfillAtPresent();
    require(s_screenshot_calls == 1, "presentation did not capture exactly once");
    require(strcmp(s_screenshot_path, "/tmp/frame-60.bmp") == 0,
            "fulfillment used wrong path");
    require(PcPort_WorldCapturePending() == 0,
            "fulfilled request remained pending");
    require(PcPort_WorldCaptureLastFulfilledFrame() == 60,
            "fulfillment frame was not recorded");
    require(PcPort_WorldCaptureFrameComplete(60) == 0,
            "fulfilled frame failed completion");
    require(PcPort_WorldCaptureFinish() == 0,
            "fulfilled run failed finish");
}

static void test_no_request_is_noop(void)
{
    reset_fixture();
    PcPort_WorldCaptureSetFrame(12);
    PcPort_WorldCaptureFulfillAtPresent();
    require(s_screenshot_calls == 0, "empty presentation captured");
    require(PcPort_WorldCaptureFinish() == 0, "empty run failed finish");
}

static void test_overwrite_rejected(void)
{
    reset_fixture();
    PcPort_WorldCaptureSetFrame(20);
    require(PcPort_WorldCaptureRequest(20, "/tmp/original.bmp") == 0,
            "original request rejected");
    require(PcPort_WorldCaptureRequest(20, "/tmp/overwrite.bmp") == -1,
            "pending request was overwritten");
    PcPort_WorldCaptureFulfillAtPresent();
    require(s_screenshot_calls == 1, "original request did not fulfill");
    require(strcmp(s_screenshot_path, "/tmp/original.bmp") == 0,
            "overwrite changed pending path");
    require(PcPort_WorldCaptureFinish() == -1,
            "overwrite error was silently cleared");
}

static void test_frame_mismatch_rejected(void)
{
    reset_fixture();
    PcPort_WorldCaptureSetFrame(30);
    require(PcPort_WorldCaptureRequest(30, "/tmp/frame-30.bmp") == 0,
            "mismatch fixture request rejected");
    PcPort_WorldCaptureSetFrame(31);
    PcPort_WorldCaptureFulfillAtPresent();
    require(s_screenshot_calls == 0, "mismatched frame captured");
    require(PcPort_WorldCapturePending() == 1,
            "mismatched request was cleared");
    require(PcPort_WorldCaptureFrameComplete(31) == -1,
            "mismatched frame completion passed");
}

static void test_unfulfilled_exit_is_error(void)
{
    reset_fixture();
    PcPort_WorldCaptureSetFrame(40);
    require(PcPort_WorldCaptureRequest(40, "/tmp/frame-40.bmp") == 0,
            "pending-exit fixture request rejected");
    require(PcPort_WorldCaptureFinish() == -1,
            "bounded exit silently accepted pending request");
    require(PcPort_WorldCapturePending() == 1,
            "finish discarded pending request");
}

int main(void)
{
    test_exact_frame_fulfillment();
    test_no_request_is_noop();
    test_overwrite_rejected();
    test_frame_mismatch_rejected();
    test_unfulfilled_exit_is_error();
    puts("W34B72 WORLD CAPTURE CERTIFICATE PASS");
    return EXIT_SUCCESS;
}
