#include <stddef.h>
#include <stdint.h>

void func_8001ACA4(void)
{
    *(volatile s32 *)&D_8004F334 = -1;
    *(volatile s32 *)&D_8004F330 = -1;
    HeapChangeCurrentUser(8, NULL);
    ArchiveSetIndex(4, 0);
    GamePartyStreamLoadSkinData(1);
}

s32 GameCharacterGetGearID(int characterIndex)
{
    const GameState *state = *(GameState *volatile *)&g_pGameState;
    uint32_t offset = (uint32_t)__builtin_offsetof(GameState, characters[0].gearId)
                    + (uint32_t)characterIndex * (uint32_t)sizeof(state->characters[0]);
    uintptr_t address = (uintptr_t)state + (uintptr_t)(intptr_t)(int32_t)offset;
    return *(const volatile unsigned char *)address;
}
