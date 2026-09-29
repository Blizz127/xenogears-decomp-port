#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/controller.h"

u_short g_C1ButtonState, g_C1ButtonStatePressedOnce, g_C1ButtonStateReleased;
u_short g_C2ButtonState, g_C2ButtonStatePressedOnce, g_C2ButtonStateReleased;
u_short g_C1ButtonStatesPressed[16], g_C1ButtonStatesPressedOnce[16];
u_short g_C1ButtonStatesReleased[16], g_C2ButtonStatesPressed[16];
u_short g_C2ButtonStatesPressedOnce[16], g_C2ButtonStatesReleased[16];
int g_ControllerCurStateWriteIndex, g_ControllerCurStateReadIndex;
int g_ControllerIsStateStackFull, D_80050200;
u_int g_ControllerNumStates;
short D_800594EC, D_800594E8, D_800594E0, D_800594DC, D_800595CC, D_800595C8;
extern void ControllerPushState(void);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern int func_80036410(void);

static u_short* const states[6] = {
    &g_C1ButtonState, &g_C2ButtonState, &g_C1ButtonStateReleased,
    &g_C2ButtonStateReleased, &g_C1ButtonStatePressedOnce, &g_C2ButtonStatePressedOnce
};
static u_short* const rings[6] = {
    g_C1ButtonStatesPressed, g_C2ButtonStatesPressed, g_C1ButtonStatesReleased,
    g_C2ButtonStatesReleased, g_C1ButtonStatesPressedOnce, g_C2ButtonStatesPressedOnce
};
#ifdef TEST_VBLANK_QUEUE
int32_t D_80059488, D_80010000, D_80059390;
uint8_t D_800501F8, D_80059370, D_80059418, D_80059420, D_80059484;
uint8_t D_8005A1BC[16];
static unsigned polls;
#ifdef TEST_VBLANK_SERVICE
static uint32_t host_vblanks;
int PsyX_Sys_GetVBlankCount(void) { return (int)host_vblanks; }
#endif
void __wrap_ControllerPoll(void)
{
    for (unsigned lane = 0; lane < 6; ++lane)
        *states[lane] = (u_short)(0x1000 * (lane + 1) + polls);
    ++polls;
}
#endif
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "ASSERTION controller.queue line=%d %s\n", __LINE__, #x); \
    exit(1); \
} } while (0)

static void run_case(int start)
{
    u_short saved[6][16];
    ControllerResetState();
    g_ControllerCurStateReadIndex = g_ControllerCurStateWriteIndex = start;
    for (unsigned i = 0; i < 16; ++i) {
        for (unsigned lane = 0; lane < 6; ++lane)
            *states[lane] = (u_short)(0x1000 * (lane + 1) + i);
        ControllerPushState();
        CHECK(g_ControllerNumStates == i + 1);
        CHECK((uint32_t)g_ControllerCurStateWriteIndex == (uint32_t)start + i + 1);
        CHECK(func_80036410() == 0);
    }
    for (unsigned lane = 0; lane < 6; ++lane) {
        memcpy(saved[lane], rings[lane], sizeof(saved[lane]));
        *states[lane] = 0xFFFF;
    }
    ControllerPushState();
    CHECK(func_80036410() == 1);
    CHECK(g_ControllerNumStates == 16);
    CHECK((uint32_t)g_ControllerCurStateWriteIndex == (uint32_t)start + 16);
    CHECK(g_ControllerCurStateReadIndex == start);
    for (unsigned lane = 0; lane < 6; ++lane)
        CHECK(memcmp(saved[lane], rings[lane], sizeof(saved[lane])) == 0);
    for (unsigned i = 0; i < 16; ++i) {
        CHECK(ControllerPopState() == (int)(16 - i));
        for (unsigned lane = 0; lane < 6; ++lane)
            CHECK(*states[lane] == 0x1000 * (lane + 1) + i);
        CHECK(g_ControllerNumStates == 15 - i);
        CHECK((uint32_t)g_ControllerCurStateReadIndex == (uint32_t)start + i + 1);
        CHECK(func_80036410() == 1);
    }
    CHECK(ControllerPopState() == 0);
    CHECK((uint32_t)g_ControllerCurStateReadIndex == (uint32_t)start + 16);
    for (unsigned lane = 0; lane < 6; ++lane)
        CHECK(*states[lane] == 0x1000 * (lane + 1) + 15);
    ControllerResetState();
    CHECK(g_ControllerNumStates == 0 && func_80036410() == 0);
    CHECK(g_ControllerCurStateReadIndex == 0 && g_ControllerCurStateWriteIndex == 0);
    for (unsigned lane = 0; lane < 6; ++lane) {
        CHECK(*states[lane] == 0);
        CHECK(memcmp(saved[lane], rings[lane], sizeof(saved[lane])) == 0);
    }
}

int main(void)
{
    for (int start = 0; start < 16; ++start)
        run_case(start);
    run_case(INT_MAX - 7);
    run_case(INT_MIN);
    run_case(-7);
#ifdef TEST_VBLANK_QUEUE
    ControllerResetState();
    D_80010000 = -1;
#ifdef TEST_VBLANK_SERVICE
    func_8004B7D0(func_8003634C);
    host_vblanks = 17;
    PcPort_ServiceVblank();
    for (int repeat = 0; repeat < 50; ++repeat)
        PcPort_ServiceVblank();
#else
    for (int tick = 0; tick < 17; ++tick)
        func_8003634C();
#endif
    CHECK(polls == 17 && D_80059488 == 17 && D_80059370 == 17);
    CHECK(g_ControllerNumStates == 16 && func_80036410() == 1);
    for (unsigned i = 0; i < 16; ++i) {
        CHECK(ControllerPopState() == (int)(16 - i));
        for (unsigned lane = 0; lane < 6; ++lane)
            CHECK(*states[lane] == 0x1000 * (lane + 1) + i);
    }
    puts("CONTROLLER VBLANK/QUEUE INTEGRATION PASS: 17 ticks preserve first 16 states and report overflow");
#ifdef TEST_VBLANK_SERVICE
    func_8004B7D0(NULL);
    host_vblanks = 20;
    PcPort_ServiceVblank();
    CHECK(polls == 17 && g_ControllerNumStates == 0);
    puts("CONTROLLER VBLANK SERVICE/QUEUE PASS: duplicate service and unregister do not push");
#endif
#endif
    puts("CONTROLLER QUEUE PASS: six-lane FIFO, full-queue drop, sticky overflow, index wrap, reset");
    return 0;
}
