/* Included by PsyX_main.cpp through psycross_video_recording.patch.
 * Host-only F9 recording of the completed OpenGL backbuffer. */
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static FILE* g_xenoRecordingPipe = NULL;
static pid_t g_xenoRecordingPid = -1;
static u_char* g_xenoRecordingPixels = NULL;
static size_t g_xenoRecordingPixelBytes = 0;
static int g_xenoRecordingWidth = 0;
static int g_xenoRecordingHeight = 0;
static int64_t g_xenoRecordingNextFrameNs = 0;
static char g_xenoRecordingFinalPath[512];
static char g_xenoRecordingPartialPath[512];
static int g_xenoRecordingAtexit = 0;

static void PsyX_StopRecording();
static int PsyX_IsRecording()
{
	return g_xenoRecordingPipe != NULL;
}

static int64_t PsyX_RecordingMonotonicNs()
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	return (int64_t)now.tv_sec * 1000000000LL + (int64_t)now.tv_nsec;
}

static int PsyX_ResolveRecordingAudioSource(char* audioSource, size_t audioSourceSize)
{
	const char* overrideSource = getenv("XENO_RECORDING_AUDIO_SOURCE");
	char defaultSink[256];
	char* newline;
	FILE* pactlOutput;
	pid_t pactlPid;
	int pipeFds[2];
	int status = -1;

	if (overrideSource != NULL && overrideSource[0] != '\0') {
		if (snprintf(audioSource, audioSourceSize, "%s", overrideSource) >=
			(int)audioSourceSize) {
			eprintwarn("Recording unavailable: audio source name is too long\n");
			return 0;
		}
		return 1;
	}

	if (pipe(pipeFds) != 0) {
		eprintwarn("Recording unavailable: cannot query the default audio sink (%s)\n",
			strerror(errno));
		return 0;
	}
	pactlPid = fork();
	if (pactlPid == 0) {
		dup2(pipeFds[1], STDOUT_FILENO);
		close(pipeFds[0]);
		close(pipeFds[1]);
		execlp("pactl", "pactl", "get-default-sink", (char*)NULL);
		_exit(127);
	}
	close(pipeFds[1]);
	if (pactlPid < 0) {
		close(pipeFds[0]);
		eprintwarn("Recording unavailable: cannot launch pactl (%s)\n",
			strerror(errno));
		return 0;
	}
	pactlOutput = fdopen(pipeFds[0], "r");
	if (pactlOutput == NULL) {
		close(pipeFds[0]);
		waitpid(pactlPid, NULL, 0);
		eprintwarn("Recording unavailable: cannot read pactl output (%s)\n",
			strerror(errno));
		return 0;
	}
	defaultSink[0] = '\0';
	fgets(defaultSink, sizeof(defaultSink), pactlOutput);
	fclose(pactlOutput);
	waitpid(pactlPid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || defaultSink[0] == '\0') {
		eprintwarn("Recording unavailable: pactl could not resolve the default audio sink\n");
		return 0;
	}
	newline = strpbrk(defaultSink, "\r\n");
	if (newline != NULL)
		*newline = '\0';
	if (defaultSink[0] == '\0' ||
		snprintf(audioSource, audioSourceSize, "%s.monitor", defaultSink) >=
		(int)audioSourceSize) {
		eprintwarn("Recording unavailable: default audio monitor name is invalid\n");
		return 0;
	}
	return 1;
}

static void PsyX_StartRecording()
{
	const char* directory = getenv("XENO_RECORDING_DIR");
	const char* ffmpeg = getenv("XENO_FFMPEG");
	char audioSource[320];
	char sizeArg[64];
	char stamp[32];
	time_t now;
	struct tm localNow;
	int pipeFds[2];

	if (g_xenoRecordingPipe != NULL)
		return;
	if (directory == NULL || directory[0] == '\0')
		directory = "recordings";
	if (ffmpeg == NULL || ffmpeg[0] == '\0')
		ffmpeg = "ffmpeg";
	if (!PsyX_ResolveRecordingAudioSource(audioSource, sizeof(audioSource)))
		return;
	if (mkdir(directory, 0755) != 0 && errno != EEXIST) {
		eprintwarn("Recording unavailable: cannot create '%s' (%s)\n",
			directory, strerror(errno));
		return;
	}

	now = time(NULL);
	localtime_r(&now, &localNow);
	strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &localNow);
	if (snprintf(g_xenoRecordingFinalPath, sizeof(g_xenoRecordingFinalPath),
		"%s/xenogears-%s.mp4", directory, stamp) >=
		(int)sizeof(g_xenoRecordingFinalPath) ||
		snprintf(g_xenoRecordingPartialPath, sizeof(g_xenoRecordingPartialPath),
		"%s/xenogears-%s.partial.mp4", directory, stamp) >=
		(int)sizeof(g_xenoRecordingPartialPath)) {
		eprintwarn("Recording unavailable: output path is too long\n");
		return;
	}

	g_xenoRecordingWidth = g_windowWidth;
	g_xenoRecordingHeight = g_windowHeight;
	g_xenoRecordingNextFrameNs = PsyX_RecordingMonotonicNs();
	g_xenoRecordingPixelBytes =
		(size_t)g_xenoRecordingWidth * (size_t)g_xenoRecordingHeight * 4;
	g_xenoRecordingPixels = (u_char*)malloc(g_xenoRecordingPixelBytes);
	if (g_xenoRecordingPixels == NULL) {
		eprintwarn("Recording unavailable: frame buffer allocation failed\n");
		return;
	}
	if (pipe(pipeFds) != 0) {
		eprintwarn("Recording unavailable: pipe failed (%s)\n", strerror(errno));
		free(g_xenoRecordingPixels);
		g_xenoRecordingPixels = NULL;
		return;
	}

	snprintf(sizeArg, sizeof(sizeArg), "%dx%d",
		g_xenoRecordingWidth, g_xenoRecordingHeight);
	g_xenoRecordingPid = fork();
	if (g_xenoRecordingPid == 0) {
		dup2(pipeFds[0], STDIN_FILENO);
		close(pipeFds[0]);
		close(pipeFds[1]);
#if defined(RENDERER_OGLES)
		const char* pixelFormat = "rgba";
#else
		const char* pixelFormat = "bgra";
#endif
		execlp(ffmpeg, ffmpeg, "-hide_banner", "-loglevel", "error", "-y",
			"-use_wallclock_as_timestamps", "1",
			"-f", "rawvideo", "-pixel_format", pixelFormat,
			"-video_size", sizeArg, "-framerate", "60", "-i", "-",
			/* Pulse timestamps describe the captured samples. Replacing them
			 * with read times collapses buffered audio into bursts and makes
			 * aresample fill the gaps with silence. */
			"-thread_queue_size", "1024",
			"-f", "pulse", "-i", audioSource,
			"-map", "0:v:0", "-map", "1:a:0",
			"-vf", "vflip,scale=trunc(iw/2)*2:trunc(ih/2)*2",
			"-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
			"-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "192k",
			"-ac", "2", "-af", "aresample=async=1000:first_pts=0",
			"-shortest",
			g_xenoRecordingPartialPath,
			(char*)NULL);
		_exit(127);
	}
	close(pipeFds[0]);
	if (g_xenoRecordingPid < 0) {
		eprintwarn("Recording unavailable: fork failed (%s)\n", strerror(errno));
		close(pipeFds[1]);
		free(g_xenoRecordingPixels);
		g_xenoRecordingPixels = NULL;
		return;
	}
	g_xenoRecordingPipe = fdopen(pipeFds[1], "wb");
	if (g_xenoRecordingPipe == NULL) {
		close(pipeFds[1]);
		waitpid(g_xenoRecordingPid, NULL, 0);
		g_xenoRecordingPid = -1;
		free(g_xenoRecordingPixels);
		g_xenoRecordingPixels = NULL;
		return;
	}
	setvbuf(g_xenoRecordingPipe, NULL, _IOFBF, 1024 * 1024);
	signal(SIGPIPE, SIG_IGN);
	if (!g_xenoRecordingAtexit) {
		atexit(PsyX_StopRecording);
		g_xenoRecordingAtexit = 1;
	}
	eprintinfo("Recording started: %s (audio=%s, F9 to stop)\n",
		g_xenoRecordingFinalPath, audioSource);
}

static void PsyX_StopRecording()
{
	int status = -1;
	if (g_xenoRecordingPipe == NULL)
		return;
	fclose(g_xenoRecordingPipe);
	g_xenoRecordingPipe = NULL;
	if (g_xenoRecordingPid > 0)
		waitpid(g_xenoRecordingPid, &status, 0);
	g_xenoRecordingPid = -1;
	free(g_xenoRecordingPixels);
	g_xenoRecordingPixels = NULL;
	g_xenoRecordingPixelBytes = 0;
	g_xenoRecordingNextFrameNs = 0;
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0 &&
		rename(g_xenoRecordingPartialPath, g_xenoRecordingFinalPath) == 0) {
		eprintinfo("Recording saved: %s\n", g_xenoRecordingFinalPath);
	} else {
		unlink(g_xenoRecordingPartialPath);
		eprintwarn("Recording failed; incomplete output removed\n");
	}
}

/* Test tooling: XENO_PRESENT_LOG=1 logs one line per presented frame with a
 * hash of the finished back buffer and its count of non-black pixels
 *   [present] n=<swap index> hash=<fnv64> nonblack=<pixels>/<total>
 * so a harness can assert what actually reached the window (not what the
 * VRAM mirror held).  XENO_PRESENT_DUMP_DIR=dir additionally writes each
 * frame that differs from the one before, 1/4 scale, as dir/present-<n>.ppm.  Off (one getenv) when unset. */
extern int g_xeno_swap_index;
static void PsyX_XenoPresentDiag()
{
	static int init = 0, on = 0;
	static const char* dumpDir = NULL;
	static u_char* px = NULL;
	static size_t cap = 0;
	int w = g_windowWidth, h = g_windowHeight;
	static unsigned long long lastHash = 0;
	unsigned long long hash = 1469598103934665603ULL;
	long nonBlack = 0;

	if (!init) {
		const char* e = getenv("XENO_PRESENT_LOG");
		init = 1;
		dumpDir = getenv("XENO_PRESENT_DUMP_DIR");
		if (dumpDir && !*dumpDir)
			dumpDir = NULL;
		on = (e && *e && *e != '0') || dumpDir != NULL;
	}
	if (!on || w <= 0 || h <= 0)
		return;
	if ((size_t)w * h * 4 > cap) {
		free(px);
		cap = (size_t)w * h * 4;
		px = (u_char*)malloc(cap);
		if (!px) { cap = 0; return; }
	}
#if defined(RENDERER_OGL) || defined(RENDERER_OGLES)
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px);
#endif
	for (size_t i = 0; i < (size_t)w * h; i++) {
		const u_char* p = &px[i * 4];
		if (p[0] > 8 || p[1] > 8 || p[2] > 8)
			nonBlack++;
		hash = (hash ^ p[0]) * 1099511628211ULL;
		hash = (hash ^ p[1]) * 1099511628211ULL;
		hash = (hash ^ p[2]) * 1099511628211ULL;
	}
	printf("[present] n=%d hash=%016llx nonblack=%ld/%ld\n", g_xeno_swap_index,
		hash, nonBlack, (long)w * h);
	fflush(stdout);
	/* Dump only frames that differ from the previous one. */
	if (dumpDir && hash != lastHash) {
		char path[600];
		FILE* f;
		int ow = w / 4, oh = h / 4;
		snprintf(path, sizeof(path), "%s/present-%06d.ppm", dumpDir, g_xeno_swap_index);
		f = fopen(path, "wb");
		if (f) {
			fprintf(f, "P6\n%d %d\n255\n", ow, oh);
			for (int j = oh - 1; j >= 0; j--)   /* GL rows are bottom-up */
				for (int i = 0; i < ow; i++)
					fwrite(&px[((size_t)(j * 4) * w + i * 4) * 4], 1, 3, f);
			fclose(f);
		}
	}
	lastHash = hash;
}

static void PsyX_RecordPresentedFrame()
{
	const int64_t frameIntervalNs = 1000000000LL / 60;
	int64_t nowNs;

	PsyX_XenoPresentDiag();
	if (g_xenoRecordingPipe == NULL)
		return;
	nowNs = PsyX_RecordingMonotonicNs();
	if (nowNs != 0 && nowNs < g_xenoRecordingNextFrameNs)
		return;
	if (nowNs != 0) {
		g_xenoRecordingNextFrameNs += frameIntervalNs;
		if (g_xenoRecordingNextFrameNs <= nowNs)
			g_xenoRecordingNextFrameNs = nowNs + frameIntervalNs;
	}
	if (g_windowWidth != g_xenoRecordingWidth ||
		g_windowHeight != g_xenoRecordingHeight) {
		eprintwarn("Recording stopped because the window size changed\n");
		PsyX_StopRecording();
		return;
	}
#if defined(RENDERER_OGL)
	glReadPixels(0, 0, g_xenoRecordingWidth, g_xenoRecordingHeight,
		GL_BGRA, GL_UNSIGNED_BYTE, g_xenoRecordingPixels);
#elif defined(RENDERER_OGLES)
	glReadPixels(0, 0, g_xenoRecordingWidth, g_xenoRecordingHeight,
		GL_RGBA, GL_UNSIGNED_BYTE, g_xenoRecordingPixels);
#endif
	if (fwrite(g_xenoRecordingPixels, 1, g_xenoRecordingPixelBytes,
		g_xenoRecordingPipe) != g_xenoRecordingPixelBytes) {
		eprintwarn("Recording stopped because ffmpeg closed its input\n");
		PsyX_StopRecording();
	}
}

static void PsyX_ToggleRecording()
{
	if (g_xenoRecordingPipe != NULL)
		PsyX_StopRecording();
	else
		PsyX_StartRecording();
}

#else

static void PsyX_StopRecording() {}
static void PsyX_RecordPresentedFrame() {}
static int PsyX_IsRecording() { return 0; }
static void PsyX_ToggleRecording()
{
	eprintwarn("Recording is unavailable on this platform\n");
}

#endif
