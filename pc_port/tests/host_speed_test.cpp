/* Host timing feature, not a retail-instruction oracle. Execute the actual
 * timer/CD/audio functions with deterministic wall clocks and controlled AL
 * leaves. Never write or skip game state to obtain the requested rate. */
#include <SDL.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../src/psycross_host_toolbar_logic.h"

#define eprintinfo(...) ((void)0)
#define eprintwarn(...) ((void)0)
#include "../src/psycross_host_speed.inl"

static double wall, stopWall;
struct timerCtx_t { double start; };
static timerCtx_t g_vblTimer, g_cnt2Timer;
static volatile char g_stopIntrThread;
static int g_vmode;
static volatile int g_psxSysCounters[1];
static int callbackCount, soundCount;
static SDL_mutex* g_intrMutex;
enum { PsxCounter_VBLANK, MODE_NTSC = 0 };
#define FIXED_TIME_STEP_NTSC (1.0 / 60.0)
#define FIXED_TIME_STEP_PAL (1.0 / 50.0)
#define PSYX_SOUND_CNT2_PERIOD (1.0 / 240.0)
static void Util_InitHPCTimer(timerCtx_t* timer) { timer->start = wall; }
static double Util_GetHPCTime(timerCtx_t* timer, int reset)
{
    wall += 0.0000001;
    if (wall >= stopWall) g_stopIntrThread = 1;
    double elapsed = wall - timer->start;
    if (reset) timer->start = wall;
    return elapsed;
}
static void CountVblank() { callbackCount++; }
static void (*vsync_callback)() = CountVblank;
static void PsyX_Sys_DispatchCounter2() { soundCount++; }

static Uint64 cdNow, g_xeno_cdPaceNext;
static int g_xeno_cdMode;
#define CdlModeSpeed 0x80
static Uint64 TestCounter() { cdNow += 100; return cdNow; }
static Uint64 TestFrequency() { return 1000000000ULL; }
static void TestDelay(Uint32 ms) { cdNow += ms * 1000000ULL; }

static SDL_atomic_t g_spuInit;
static SDL_mutex* g_SpuMutex;
static int s_streamMode, s_xaAlReady;
static unsigned s_xaAlSource = 7;
static const int s_spuVoiceCount = 3;
struct SPUALVoice { unsigned alSource; struct { unsigned short pitch; } attr; };
static SPUALVoice g_SpuVoices[s_spuVoiceCount];
enum { AL_NONE = 0, AL_PITCH = 1 };
static float sourcePitch[8];
static int pitchWrites;
static void alSourcef(unsigned source, int property, float value)
{
    assert(source > 0 && source < 8 && property == AL_PITCH && value > 0);
    sourcePitch[source] = value;
    pitchWrites++;
}
#define SDL_GetPerformanceCounter TestCounter
#define SDL_GetPerformanceFrequency TestFrequency
#define SDL_Delay TestDelay
#include "production.inc"
#undef SDL_GetPerformanceCounter
#undef SDL_GetPerformanceFrequency
#undef SDL_Delay

static void testClocks(int speed, int pal)
{
    PsyX_SetSpeedMultiplier(speed);
    wall = 0; stopWall = 1; g_stopIntrThread = 0;
    g_vmode = pal; g_psxSysCounters[0] = 0;
    callbackCount = soundCount = 0;
    intrThreadMain(NULL);
    int rate = pal ? 50 : 60;
    assert(std::abs(callbackCount - rate * speed) <= 1);
    assert(std::abs(soundCount - 240 * speed) <= 1);
    assert(g_psxSysCounters[0] == callbackCount);
    for (int i = 0; i < 10000; i++)
        assert(PsyX_Sys_GetVBlankCount() == callbackCount);
    printf("speed=%d mode=%d vblank=%d callbacks=%d sound=%d\n",
           speed, pal, g_psxSysCounters[0], callbackCount, soundCount);
    for (int doubleSpeed = 0; doubleSpeed < 2; doubleSpeed++) {
        g_xeno_cdMode = doubleSpeed ? CdlModeSpeed : 0;
        cdNow = g_xeno_cdPaceNext = 0;
        int sectors = (doubleSpeed ? 150 : 75) * speed;
        for (int i = 0; i <= sectors; i++) _eCdSpoolerPace();
        assert(std::abs((double)cdNow - 1000000000.0) < 2000000.0);
    }
}

int main()
{
    g_intrMutex = SDL_CreateMutex();
    g_SpuMutex = SDL_CreateMutex();
    assert(g_intrMutex && g_SpuMutex);
    assert(PsyX_GetSpeedMultiplier() == 1);
    for (int speed = 1; speed <= 5; speed++)
        for (int pal = 0; pal < 2; pal++) testClocks(speed, pal);
    PsyX_SetSpeedMultiplier(3);
    PsyX_SetFastForwardHeld(1);
    assert(PsyX_GetSpeedMultiplier() == 5);
    int before = PsyX_Sys_GetVBlankCount();
    for (int i = 0; i < 10000; i++) assert(PsyX_Sys_GetVBlankCount() == before);
    PsyX_SetFastForwardHeld(0);
    assert(PsyX_GetSpeedMultiplier() == 3);
    PsyX_SetSpeedMultiplier(0); assert(PsyX_GetSpeedMultiplier() == 1);
    PsyX_SetSpeedMultiplier(99); assert(PsyX_GetSpeedMultiplier() == 5);
    setenv("XENO_SPEED", "4", 1); PsyX_InitialiseSpeed();
    assert(PsyX_GetSpeedMultiplier() == 4);
    setenv("XENO_SPEED", "5junk", 1); PsyX_InitialiseSpeed();
    assert(PsyX_GetSpeedMultiplier() == 1);
    unsetenv("XENO_SPEED"); PsyX_InitialiseSpeed();
    assert(PsyX_GetSpeedMultiplier() == 1);
    assert(PcPort_HostToolbarHitTest(288, 4) == PC_PORT_TOOLBAR_SPEED);
    assert(PcPort_HostToolbarHitTest(391, 29) == PC_PORT_TOOLBAR_SPEED);
    assert(PcPort_HostToolbarHitTest(392, 29) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(300, 34) == PC_PORT_TOOLBAR_NONE);

    /* Changing presentation pitch must retain the emulated voice registers. */
    for (int i = 0; i < 3; i++) {
        g_SpuVoices[i].alSource = i + 1;
        g_SpuVoices[i].attr.pitch = 2048 << i;
    }
    SPUALVoice original[3]; memcpy(original, g_SpuVoices, sizeof original);
    SDL_AtomicSet(&g_spuInit, 1); s_xaAlReady = 1;
    for (s_streamMode = 0; s_streamMode < 2; s_streamMode++) {
        for (int speed = 1; speed <= 5; speed++) {
            PsyX_SetSpeedMultiplier(speed);
            pitchWrites = 0; PsyX_SPUAL_RefreshHostSpeed();
            assert(pitchWrites == 4);
            for (int i = 0; i < 3; i++) {
                float base = s_streamMode ? 1.0f : original[i].attr.pitch / 4096.0f;
                assert(sourcePitch[i+1] == base * speed);
            }
            assert(sourcePitch[7] == speed);
            assert(memcmp(original, g_SpuVoices, sizeof original) == 0);
        }
    }
    SDL_AtomicSet(&g_spuInit, 0); pitchWrites = 0;
    PsyX_SetSpeedMultiplier(1); assert(pitchWrites == 0);
    SDL_DestroyMutex(g_intrMutex); SDL_DestroyMutex(g_SpuMutex);
    puts("HOST SPEED PASS: 1-5x NTSC/PAL, sound/CD clocks, read-only queries, controls and audio register preservation");
}
