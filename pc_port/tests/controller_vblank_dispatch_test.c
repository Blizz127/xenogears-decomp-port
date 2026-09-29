#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system/controller_vblank.h"

int32_t D_80059488, D_80010000, D_80059390;
uint8_t D_800501F8, D_80059370, D_80059418, D_80059420, D_80059484;
_Alignas(8) uint8_t D_8005A1BC[16];
extern void (*D_800501FC)(void);
#ifdef VBLANK_TEST_TRUNCATE_CALLBACK
void __real_func_800363F0(void (*callback)(void));
void __wrap_func_800363F0(void (*callback)(void))
{
    __real_func_800363F0((void (*)(void))(uintptr_t)(uint32_t)(uintptr_t)callback);
}
#endif
static char order[16];
/* The helpers are in the same TU as the dispatcher (system/controller2.c),
 * so their calls cannot be intercepted; each is observed through its effect
 * instead: func_80035E44 advances the play clock, func_80036220 counts the
 * actuator record down.  Both must be unchanged at ControllerPushState and
 * done by the callback / end of the tick. */
static uint8_t clock0;
static uint16_t count0;
static int helpers_noted;
static unsigned index_in_order;
static uint32_t expected_tick;
static int callback_changes_debug;
static volatile sig_atomic_t traps;
static unsigned input_phase;

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "ASSERTION vblank.dispatch line=%d %s\n", __LINE__, #x); \
    exit(1); \
} } while (0)

void ControllerPoll(void)
{
    CHECK(input_phase == 1);
    input_phase = 2;
    CHECK((uint32_t)D_80059488 == expected_tick);
    order[index_in_order++] = 'P';
}
static uint16_t record0_count(void)
{
    return (uint16_t)(D_8005A1BC[4] | (D_8005A1BC[5] << 8));
}
static void note_helpers(void)
{
    if (helpers_noted)
        return;
    CHECK(D_80059370 != clock0);                            /* 'T' ran */
    CHECK(record0_count() == (uint16_t)(count0 - 1u));      /* 'R' ran */
    order[index_in_order++] = 'T';
    order[index_in_order++] = 'R';
    helpers_noted = 1;
}
void ControllerPushState(void)
{
    CHECK(input_phase == 3);
    CHECK(D_80059370 == clock0 && record0_count() == count0); /* helpers not yet */
    input_phase = 0;
    order[index_in_order++] = 'S';
}
void PcPort_PrepareControllerPoll(void)
{
    CHECK(input_phase == 0);
    CHECK((uint32_t)D_80059488 == expected_tick);
    input_phase = 1;
}
void PcPort_BeforeControllerPush(void)
{
    CHECK(input_phase == 2);
    input_phase = 3;
}
static void callback_b(void) { note_helpers(); order[index_in_order++] = 'B'; }
static void callback_a(void)
{
    note_helpers();
    order[index_in_order++] = 'A';
    CHECK(D_80059370 == 0 && D_80059418 == 1);
    CHECK(D_8005A1BC[4] == 0 && D_8005A1BC[6] == 1);
    func_800363F0(callback_b);
    if (callback_changes_debug)
        func_800363E0(1);
}
static void trap_handler(int sig) { if (sig == SIGTRAP) ++traps; }
static void run_tick(const char* expected_order)
{
    memset(order, 0, sizeof(order));
    index_in_order = 0;
    helpers_noted = 0;
    if (record0_count() == 0) {        /* keep the actuator step observable */
        D_8005A1BC[4] = 0xE8;
        D_8005A1BC[5] = 0x03;
    }
    clock0 = D_80059370;
    count0 = record0_count();
    expected_tick = (uint32_t)D_80059488 + 1u;
    func_8003634C();
    note_helpers();
    CHECK(strcmp(order, expected_order) == 0);
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = trap_handler;
    sigemptyset(&action.sa_mask);
    CHECK(sigaction(SIGTRAP, &action, NULL) == 0);
    /* The PIE test must exercise bits that the previous int owner lost. */
    CHECK((uintptr_t)callback_a > UINT32_MAX);
    CHECK(D_800501FC == NULL);
    D_80010000 = -1;
    D_80059488 = INT32_MAX;
    run_tick("PSTR");
    CHECK(D_80059488 == INT32_MIN);
    D_80059488 = -1;
    run_tick("PSTR");
    CHECK(D_80059488 == 0);

    D_80059370 = 59; D_80059418 = 0;
    D_8005A1BC[4] = 1;
    func_800363F0(callback_a);
    CHECK(D_800501FC == callback_a);
    run_tick("PSTRA");
    CHECK(D_800501FC == callback_b);
    run_tick("PSTRB");
    func_800363F0(NULL);
    run_tick("PSTR");

    for (int debug_image = 0; debug_image < 2; ++debug_image) {
        for (int enabled = 0; enabled < 2; ++enabled) {
            sig_atomic_t before = traps;
            D_80010000 = debug_image ? 0 : -1;
            func_800363E0(enabled ? -7 : 0);
            run_tick("PSTR");
            CHECK(traps - before == (debug_image && enabled));
        }
    }
    /* The debug flags are read after the callback, not cached on entry. */
    D_80059370 = 59; D_80059418 = 0;
    D_8005A1BC[4] = 1;
    D_80010000 = 0;
    callback_changes_debug = 1;
    func_800363E0(0);
    func_800363F0(callback_a);
    run_tick("PSTRA");
    CHECK(traps == 2);
    puts("CONTROLLER VBLANK DISPATCH PASS: order, real helpers, callback replacement, tick wrap, debug trap");
    return 0;
}
