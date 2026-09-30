#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"

void FieldPartyMemberIncreaseHp(int i, int amount) {
    g_pGameState->characters[g_GamePartyMembers[i]].hp += amount;
    if (g_pGameState->characters[g_GamePartyMembers[i]].maxHp < g_pGameState->characters[g_GamePartyMembers[i]].hp) {
        g_pGameState->characters[g_GamePartyMembers[i]].hp = g_pGameState->characters[g_GamePartyMembers[i]].maxHp;
    }
}

void FieldPartyMemberDecreaseHp(int partyMemberIndex, int amount) {
    int newHp;
    int characterId = g_GamePartyMembers[partyMemberIndex];
    newHp = g_pGameState->characters[characterId].hp - amount;
    if (newHp <= 0) {
        newHp = 1;
    }
    g_pGameState->characters[characterId].hp = newHp;
}

void FieldPartyMemberIncreaseMp(int i, int amount) {
    g_pGameState->characters[g_GamePartyMembers[i]].mp += amount;
    if (g_pGameState->characters[g_GamePartyMembers[i]].maxMp < g_pGameState->characters[g_GamePartyMembers[i]].mp) {
        g_pGameState->characters[g_GamePartyMembers[i]].mp = g_pGameState->characters[g_GamePartyMembers[i]].maxMp;
    }
}

void FieldPartyMemberDecreaseMp(int partyMemberIndex, int amount) {
    int newMp;
    int characterId = g_GamePartyMembers[partyMemberIndex];
    newMp = g_pGameState->characters[characterId].mp - amount;
    if (newMp <= 0) {
        newMp = 1;
    }
    g_pGameState->characters[characterId].mp = newMp;
}

extern s16 g_FieldNumPartyMembersMasks[4];

void FieldScriptVMHandlerDecreasePartyHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(ARG(2)));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(ARG(2)) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberDecreaseHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}


extern s16 D_800B2340;
extern s16 D_800B2342;
extern s16 D_800B2344;
extern s16 D_800B2346;

/* FEAE — arm animation/dolly counters (not CLUT). */
void func_80096AF4(void) {
#ifdef XENO_PC_PORT
    D_800B2344 = FieldScriptVMGetArgument(1);
    D_800B2346 = FieldScriptVMGetArgument(3);
    D_800B2340 = FieldScriptVMGetArgument(5);
    D_800B2342 = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
#else
    D_800B2344 = FieldScriptVMGetArgument(1);
    D_800B2346 = FieldScriptVMGetArgument(3);
    D_800B2340 = FieldScriptVMGetArgument(5);
    D_800B2342 = 0;
    *(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) = (u16) (*(u16 *)((s8*)(g_FieldScriptVMCurActor) + 0xCC) + 7);
#endif
}

void FieldScriptVMHandlerWritePartyMemberHp(void) {
    if (g_GamePartyMembers[SCRIPT_READ_U8_REL(ARG(2))] != CHARACTER_ID_NONE) {
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(1), 
            g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(ARG(2))]].hp
        );
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerWritePartyMemberMp(void) {
    if (g_GamePartyMembers[SCRIPT_READ_U8_REL(ARG(2))] != CHARACTER_ID_NONE) {
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(1), 
            g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(ARG(2))]].mp
        );
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Sets the HP of a character to a target amount, but only if we have a full party
void FieldScriptVMHandlerSetPartyMemberHp(void) {
    int targetHp;
    int characterHp;

    if (g_GamePartyMembers[SCRIPT_READ_U8_REL(3)] != CHARACTER_ID_NONE) {
        targetHp = FieldScriptVMGetArgument(2);
        characterHp = g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(1)]].maxHp;
        if (characterHp < targetHp) {
            targetHp = characterHp;
        }
        g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(1)]].hp = targetHp;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Sets the MP of a character to a target amount, but only if we have a full party
void FieldScriptVMHandlerSetPartyMemberMp(void) {
    int targetMp;
    int characterMp;

    if (g_GamePartyMembers[SCRIPT_READ_U8_REL(3)] != CHARACTER_ID_NONE) {
        targetMp = FieldScriptVMGetArgument(2);
        characterMp = g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(1)]].maxMp;
        if (characterMp < targetMp) {
            targetMp = characterMp;
        }
        g_pGameState->characters[g_GamePartyMembers[SCRIPT_READ_U8_REL(1)]].mp = targetMp;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerIncreasePartyMp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberIncreaseMp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerDecreasePartyMp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberDecreaseMp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern s32 g_GamePartyMembers[];
extern s16 g_FieldNumPartyMembersMasks[];
extern void FieldPartyMemberIncreaseMp(s32, s32);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/party/stats", func_80097108);
#else
void func_80097108(void) {
    u8 mask1 = SCRIPT_READ_U8_REL(3);
    s32 mpAmount = FieldScriptArgument1(1, mask1);
    u8 partyMask = SCRIPT_READ_U8_REL(3) & 3;
    s16 memberMask = g_FieldNumPartyMembersMasks[partyMask];
    s32 i;
    for (i = 0; i < 3; i++) {
        if (g_GamePartyMembers[i] != 0xFF && (memberMask & 1)) {
            FieldPartyMemberIncreaseMp(i, mpAmount);
        }
        memberMask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}
#endif /* XENO_PC_PORT */

void FieldScriptVMHandlerRestoreCharacterHpAndMp(void) {
    int id = FieldScriptVMGetArgument(1);
    g_pGameState->characters[id].hp = g_pGameState->characters[id].maxHp;
    g_pGameState->characters[id].mp = g_pGameState->characters[id].maxMp;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRestoreHp(void) {
    int i;
    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        g_pGameState->characters[i].hp = g_pGameState->characters[i].maxHp;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerRestoreMp(void) {
    int i;
    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        g_pGameState->characters[i].mp = g_pGameState->characters[i].maxMp;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}
