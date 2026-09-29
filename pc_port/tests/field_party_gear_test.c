#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void* g_pGameState;
s32 g_GamePartyMembers[3];
s16 D_8006BE2C[3];

static s32 s_gearIds[3];
static s32 s_refreshCount;
static s32 s_refreshMode;
static void* s_rebindState;
static unsigned s_calls;

s32 GameCharacterGetGearID(s32 characterId)
{
    s_calls++;
    if (s_rebindState != NULL) {
        g_pGameState = s_rebindState;
        s_rebindState = NULL;
    }
    return s_gearIds[characterId];
}

void func_800AD978(s32 mode)
{
    s_refreshCount++;
    s_refreshMode = mode;
}

void func_800ACE90(void);

static void run_case(const u8 states[3], const s32 expected[3])
{
    u8 gameState[0x2300];
    s32 i;

    memset(gameState, 0, sizeof(gameState));
    for (i = 0; i < 3; i++) {
        gameState[0x22B1 + i] = states[i];
        D_8006BE2C[i] = 0x1234;
        g_GamePartyMembers[i] = i;
    }
    g_pGameState = gameState;
    s_refreshCount = 0;
    s_refreshMode = -1;

    func_800ACE90();

    assert(s_refreshCount == 1);
    assert(s_refreshMode == 1);
    for (i = 0; i < 3; i++)
        assert(D_8006BE2C[i] == expected[i]);
}

/* Retail reloads g_pGameState at 800ACEF0/800ACF58 after helper calls.
 * Replace it at the helper seam to distinguish that from a cached pointer.
 * The selected branch stays fixed even when the replacement's slot 0 differs. */
static void run_rebind_case(u8 branch)
{
    u8 initial[0x2300] = {0};
    u8 replacement[0x2300] = {0};
    unsigned i;
    for (i = 0; i < 3; i++) {
        initial[0x22B1 + i] = branch;
        replacement[0x22B1 + i] = (u8)(1 - branch);
        g_GamePartyMembers[i] = (s32)i;
        s_gearIds[i] = 7;
        D_8006BE2C[i] = 123;
    }
    g_pGameState = initial;
    s_rebindState = replacement;
    s_calls = 0;
    s_refreshCount = 0;
    func_800ACE90();
    assert(s_calls == 1);
    assert(D_8006BE2C[0] == 1);
    assert(D_8006BE2C[1] == 0 && D_8006BE2C[2] == 0);
    assert(s_refreshCount == 1 && s_refreshMode == 1);
    g_pGameState = NULL;
}

int main(void)
{
    static const u8 state0[3] = {0, 0, 1};
    static const s32 expected0[3] = {1, 0, 0};
    static const u8 state1[3] = {1, 0, 1};
    static const s32 expected1[3] = {1, 0, 1};
    static const u8 state2[3] = {0, 1, 0};
    static const s32 expected2[3] = {1, 0, 1};
    static const u8 state3[3] = {1, 1, 0};
    static const s32 expected3[3] = {1, 1, 0};

    s_gearIds[0] = 3;
    s_gearIds[1] = 0xFF;
    s_gearIds[2] = 7;
    run_case(state0, expected0);

    s_gearIds[1] = 5;
    run_case(state1, expected1);
    run_case(state2, expected2);
    run_case(state3, expected3);

    run_rebind_case(0);
    run_rebind_case(1);
    puts("FIELD PARTY GEAR PASS cases=6");
    return 0;
}
