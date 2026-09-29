/*
 * Host-only fixture for run_recording_audio_test.py.
 * The recorder is compiled from the selected production .inl, while this
 * fixture supplies a deterministic 64x64 completed backbuffer and drives the
 * same pre-swap capture seam at a requested presentation rate.
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

using u_char = unsigned char;

static int g_windowWidth = 64;
static int g_windowHeight = 64;

#define RENDERER_OGL 1
#define GL_BGRA 1
#define GL_UNSIGNED_BYTE 2

#define eprintwarn(...) fprintf(stderr, __VA_ARGS__)
#define eprintinfo(...) fprintf(stderr, __VA_ARGS__)

static void glReadPixels(int, int, int width, int height, int, int, void* pixels)
{
	memset(pixels, 128, (size_t)width * (size_t)height * 4);
}

#include "RECORDING_SOURCE_PLACEHOLDER"

int main(int argc, char** argv)
{
	int presentationRate;
	int frameCount;
	int i;
	if (argc != 2)
		return 64;
	presentationRate = atoi(argv[1]);
	if (presentationRate <= 0)
		return 65;
	frameCount = presentationRate * 4;
	PsyX_StartRecording();
	if (!PsyX_IsRecording())
		return 2;
	fprintf(stderr, "XENO_RECORDING_FFMPEG_PID=%ld\n",
		(long)g_xenoRecordingPid);
	fflush(stderr);
	for (i = 0; i < frameCount; i++)
	{
		PsyX_RecordPresentedFrame();
		std::this_thread::sleep_for(std::chrono::nanoseconds(
			1000000000LL / presentationRate));
	}
	PsyX_StopRecording();
	return 0;
}
