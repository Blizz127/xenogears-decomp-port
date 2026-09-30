#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"


void FieldScriptCheckGoldAmount(void) {
    if (g_pGameState->gold >= (
        SCRIPT_READ_U8_REL(1) + 
        (SCRIPT_READ_U8_REL(2) << 8) + 
        (SCRIPT_READ_U8_REL(3) << 0x10) + 
        (SCRIPT_READ_U8_REL(4) << 0x18)
    )) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(5);
}

void FieldScriptIncreaseGold(void) {
    int amount;
    int newGoldAmount;

    amount = FieldScriptVMGetArgument(1);
    newGoldAmount = g_pGameState->gold + amount;
    if (newGoldAmount > MAX_GOLD_AMOUNT) {
        newGoldAmount = MAX_GOLD_AMOUNT;
    }
    g_pGameState->gold = newGoldAmount;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptDecreaseGold(void) {
    int newGoldAmount;
    int amount;

    amount = FieldScriptVMGetArgument(1);
    newGoldAmount = g_pGameState->gold;
    newGoldAmount -= amount;
    if (newGoldAmount < 0) {
        newGoldAmount = 0;
    }
    g_pGameState->gold = newGoldAmount;
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}