/* Port-only translation of the field party gear-presence refresh. */
#include "common.h"

extern void* g_pGameState;
extern s32 g_GamePartyMembers[3];
extern s16 D_8006BE2C[3];
extern s32 GameCharacterGetGearID(s32 characterId);
extern void func_800AD978(s32 mode);

/* Retail 0x800ACE90: clear the transient party flags, then mark each active
 * party slot whose character has a non-FF gear id. The alternate branch uses
 * the gear-present state byte (1) as its expected party-state value. */
void func_800ACE90(void)
{
    u8* gameState = (u8*)g_pGameState;
    s32 i;
    s32 expectedState;
    s32 gearSentinel;

    D_8006BE2C[0] = 0;
    D_8006BE2C[1] = 0;
    D_8006BE2C[2] = 0;

    if (gameState[0x22B1] == 0) {
        expectedState = 0;
        gearSentinel = 0xFF;
    } else {
        expectedState = 1;
        gearSentinel = 0xFF;
    }

    for (i = 0; i < 3; i++) {
        /* Retail reloads the global after each external gear lookup. */
        if (((u8*)g_pGameState)[0x22B1 + i] == expectedState &&
            GameCharacterGetGearID(g_GamePartyMembers[i]) != gearSentinel) {
            D_8006BE2C[i] = 1;
        }
    }

    func_800AD978(1);
}
