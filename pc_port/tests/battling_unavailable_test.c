/* battling_unavailable_test: drives a field -> state 4 (Battling) switch
 * through the retail ChangeGameState semantics and the port's MainLoop
 * stopgap guard (pc_port/src/battling_unavailable.c), and asserts that the
 * unported minigame is never entered: notice set, result cleared, field
 * state restored, bound/other states left alone. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battling_unavailable.h"

/* Retail main_loop.c ChangeGameState + the state table, reduced to what the
 * guard touches. */
unsigned int g_CurGameState;
unsigned int g_CurGameStateOverlayID = 1;
static int s_overlayFreed;
void ChangeGameState(unsigned int state)
{
    g_CurGameState = state;
    if (state != g_CurGameStateOverlayID) {
        s_overlayFreed++;
        g_CurGameStateOverlayID = (unsigned int)-1;
    }
}

typedef struct { void (*pFnMain)(void); } TestState;
static TestState s_states[7];
static int s_entered[7];
static void FieldMainStub(void) { s_entered[1]++; }
static void BattlingMainStub(void) { s_entered[4]++; }

unsigned char D_8005061C[1];
unsigned char D_80050622[32];

static char s_notice[128];
static int s_noticeCalls;
void PcPort_QuickCheckpointNoticeSet(const char *text)
{
    snprintf(s_notice, sizeof s_notice, "%s", text);
    s_noticeCalls++;
}

static int s_fail;
#define CHECK(cond, what) do { if (!(cond)) { \
    printf("BATTLING GUARD FAIL %s\n", what); s_fail = 1; } } while (0)

/* The MainLoop prologue: guard, then dispatch the (possibly new) state.  A
 * NULL main here is the crash the guard exists to prevent. */
static void main_loop_dispatch(void)
{
    PcPort_BattlingUnavailableGuard(g_CurGameState,
                                    s_states[g_CurGameState].pFnMain != NULL);
    if (s_states[g_CurGameState].pFnMain == NULL) {
        printf("BATTLING GUARD FAIL dispatched NULL main for state %u\n",
               g_CurGameState);
        exit(1);
    }
    s_states[g_CurGameState].pFnMain();
}

int main(void)
{
    s_states[1].pFnMain = FieldMainStub;

    /* 1. Field exit code 2 (func_8007954C): ChangeGameState(4), MainLoop. */
    g_CurGameState = 1;
    D_8005061C[0] = 0;   /* field opcode func_80087848 argument */
    D_80050622[0] = 0x85; /* stale result from an earlier visit */
    ChangeGameState(PC_PORT_GAME_STATE_BATTLING);
    main_loop_dispatch();
    CHECK(g_CurGameState == PC_PORT_GAME_STATE_FIELD, "state not restored to field");
    CHECK(s_entered[1] == 1 && s_entered[4] == 0, "field not re-entered");
    CHECK(s_noticeCalls == 1, "notice not set once");
    CHECK(strcmp(s_notice, "BATTLING ARENA NOT YET AVAILABLE IN THIS BUILD") == 0,
          "notice text");
    CHECK(D_80050622[0] == 0, "result not cleared to no-match");
    CHECK(D_8005061C[0] == 1, "battling exit flag not set");
    CHECK(PcPort_BattlingUnavailableCount == 1, "guard count");

    /* 2. Ordinary state: untouched. */
    s_noticeCalls = 0;
    g_CurGameState = 1;
    CHECK(PcPort_BattlingUnavailableGuard(1, 1) == 0, "field intercepted");
    CHECK(g_CurGameState == 1 && s_noticeCalls == 0, "field state disturbed");

    /* 3. Once a native Battling entry is bound, the stopgap steps aside. */
    s_states[4].pFnMain = BattlingMainStub;
    ChangeGameState(PC_PORT_GAME_STATE_BATTLING);
    main_loop_dispatch();
    CHECK(g_CurGameState == 4 && s_entered[4] == 1, "bound entry intercepted");
    CHECK(s_noticeCalls == 0, "notice on bound entry");

    if (s_fail)
        return 1;
    printf("BATTLING GUARD PASS (state 4 -> notice, result=0, field restored; "
           "bound entry untouched)\n");
    return 0;
}
