/*
 * XENO_DETERMINISTIC_VBLANK_BATTLE unit test (production-linked).
 *
 * Compiles the real pc_port/src/psyq_compat.c + controller_vblank_service.c
 * and drives Vsync / the guest pump / the battle entry and return hooks
 * through a simulated field -> battle -> field sequence with a fake host
 * vblank counter.  The serviced vblank count (Vsync(-1)'s value, the one
 * func_800BE790 turns into D_80059494 catch-up ticks) is checked against the
 * test-clock rule:
 *   on : in battle +K per blocking Vsync and +1 per Vsync(-1) spin, whatever
 *        the host does; host-paced stall guard after 600 idle host vblanks;
 *        return restores the field's load state (host-paced, continuous
 *        from the battle cursor) and the next drain goes live again.
 *   off: (switch unset) the hooks are inert, battle stays host-paced.
 * argv[1] = "on" | "off"; the run script sets the environment.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "system/controller_vblank.h"

extern int Vsync(int mode);
extern void PcPort_PadVblankPump(void);
extern void PcPort_DetVblankNoteDrain(void);
extern void PcPort_DetVblankBattleEnter(void);
extern void PcPort_DetVblankBattleReturn(void);
int32_t D_80059488, D_80010000 = -1, D_80059390;
uint8_t D_800501F8, D_80059370, D_80059418, D_80059420, D_80059484;
uint8_t D_8005A1BC[16];

/* ---- fakes for the Vsync closure (as vsync_present_regression_test.c) ---- */
int PsyX_IsSceneOpen(void) { return 0; }
void GR_SwapWindow(void) {}
void PsyX_EndScene(void) {}
int PsyX_PresentDisplayFromVRAM(void) { return 1; }
void DrawAllSplits(void) {}
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
int PsyX_GpuTraceOn(void) { return 0; }
int g_xeno_swap_index;
void* g_pSoundSpuRegisters;
unsigned char D_800594CC;
unsigned char D_800B21D0;
int g_GameSceneMapNum;
void func_8001B970(void) {}
void PcPort_FieldCaptureOnVsync(void) {}
void PcPort_CullCamLogOnVsync(void) {}
void PcPort_FieldPosDiag(void) {}
int PcPort_FieldStepPadApply(void) { return 0; }
int PcPort_QuickCheckpointGetUiState(void) { return 0; }
int PcPort_QuickCheckpointRequestLoad(void) { return 0; }
int PcPort_QuickCheckpointRequestSave(void) { return 0; }
int PcPort_FieldStepPadArmedStep(unsigned *step) { *step = 0; return 0; }
static int s_latch;                     /* field loop running (quick-checkpoint latch) */
int PcPort_QuickCheckpointFieldIsActive(void) { return s_latch; }

static uint32_t s_host;                 /* fake host vblank counter */
int PsyX_Sys_GetVBlankCount(void) { return (int)s_host; }
int VSync(int mode)
{
    if (mode == 0) ++s_host;            /* a blocking wait spans one host vblank */
    return (int)s_host;
}

int g_KernelMenuCurChoice;
int g_KernelMenuIsRunning;
unsigned short g_C1ButtonStateReleased;
int D_800ADB64;
int D_800ADB68;
unsigned char g_GameState[0x8000];
unsigned char g_C1Buffer[4];
unsigned char g_ControllerButtonMappings[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
unsigned short g_ControllerButtonMasks[8] = { 0x20, 0x40, 0x10, 0x80, 0x04, 0x01, 0x08, 0x02 };
int g_XenoMenuNavReaderTicks;
int g_XenoMenuN2Phase;
int g_XenoMenuN2c2Phase;
int g_XenoMenuN3aPhase;
unsigned char g_XenoMenuN3aRowFlags[32];
int g_XenoMenuN2c3SpecialHits;
unsigned int PcPort_N2c1ReadItemPair(void) { return 0; }

/* ---- harness ---- */
static int s_failures;
#define CHECK(tag, cond) do { if (!(cond)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, tag); s_failures++; } } while (0)

static unsigned s_ticks;
static void tick(void) { ++s_ticks; }

static int serviced(void) { return PcPort_GetServicedVblankCount(); }
static void pump(void) { PcPort_PadVblankPump(); }

/* One retail-style battle frame: the elapsed-vblank sample, then the
 * blocking frame wait.  Returns the vblanks this frame saw. */
static int battle_frame(int *last, uint32_t host_jump)
{
    int now, d;
    s_host += host_jump;                /* host speed varies freely */
    pump();
    now = Vsync(-1);
    d = now - *last;
    *last = now;
    Vsync(0);
    return d;
}

int main(int argc, char **argv)
{
    int on = argc > 1 && strcmp(argv[1], "on") == 0;
    int last, d, i, base;

    g_GameSceneMapNum = 490;            /* XENO_DETERMINISTIC_VBLANK_FROM_FIELD=490 */
    s_host = 1000;
    PcPort_ResetVblankService();
    func_8004B7D0(tick);

    /* Field: armed at map 490; live after the first field-step drain. */
    s_latch = 1;
    Vsync(0);
    PcPort_DetVblankNoteDrain();
    Vsync(0);
    base = serviced();
    for (i = 0; i < 4; i++) { s_host += 7; Vsync(0); }
    CHECK("field: +K=2 per blocking Vsync regardless of the host", serviced() == base + 8);

    /* Field teardown -> battle load: host-paced service. */
    s_latch = 0;
    s_host += 50;                        /* host passes the field's cursor */
    pump();
    base = serviced();
    s_host += 5;
    pump();
    CHECK("load: host-paced before the battle", serviced() == base + 5);

    /* Battle entry (func_80070F40). */
    PcPort_DetVblankBattleEnter();
    pump();
    last = Vsync(-1);
    if (on) {
        int ok = 1;
        for (i = 0; i < 20; i++) {
            d = battle_frame(&last, (uint32_t)(i * 37 % 11));   /* jittery host */
            if (i > 0 && d != 2) ok = 0;
        }
        CHECK("battle on: every frame sees exactly K=2 vblanks (host-independent)", ok);
        /* Vsync(-1) spin rule (cee78410): a busy-wait cannot hang. */
        base = Vsync(-1);
        d = Vsync(-1);
        CHECK("battle on: Vsync(-1) spin advances one vblank", d == base + 1);
        Vsync(1);
        d = Vsync(-1);
        CHECK("battle on: Vsync(1) ends the spin", d == base + 1);
        /* Host-only pumps below the stall threshold: frozen. */
        base = serviced();
        s_host += 599;
        pump();
        CHECK("battle on: no Vsync, 599 host vblanks: clock frozen", serviced() == base);
        /* Stall guard: past 600 idle host vblanks the service follows the host. */
        s_host += 1;
        pump();
        s_host += 3;
        pump();
        CHECK("battle on: stall guard follows the host after 600", serviced() == base + 3);
        Vsync(0);                         /* the game's Vsync ends the stall span */
        base = serviced();
        s_host += 50;
        pump();
        CHECK("battle on: a Vsync ends the stall span", serviced() == base);
        for (i = 0; i < 3; i++) battle_frame(&last, 9);
        CHECK("battle on: back on the K rule", serviced() == base + 6);
    } else {
        base = serviced();
        for (i = 0; i < 5; i++) battle_frame(&last, 10);
        CHECK("battle off: host-paced (10 host + 1 wait per frame)", serviced() == base + 55);
    }

    /* Battle return: restore the load state; the service stays continuous. */
    PcPort_DetVblankBattleReturn();
    base = serviced();
    s_host += 4;
    pump();
    CHECK("return: host-paced load, continuous (no stall, no burst)", serviced() == base + 4);
    /* Next field: live again at the first drain, K rule. */
    s_latch = 1;
    pump();
    PcPort_DetVblankNoteDrain();
    Vsync(0);
    base = serviced();
    for (i = 0; i < 3; i++) { s_host += 13; Vsync(0); }
    if (on)
        CHECK("next field: deterministic again, +2 per Vsync", serviced() == base + 6);
    CHECK("callback ran once per serviced vblank", s_ticks == (unsigned)serviced());

    if (s_failures) {
        fprintf(stderr, "DET VBLANK BATTLE %s: FAIL (%d)\n", argv[1], s_failures);
        return 1;
    }
    printf("DET VBLANK BATTLE %s: PASS\n", on ? "on" : "off");
    return 0;
}
