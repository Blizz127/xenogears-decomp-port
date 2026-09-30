#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern u8* func_8009501C(s32 itemId);
extern u8* func_800950A0(s32 itemId);
extern s32 func_80095124(s32 itemId);
extern s32 func_800951B8(s32 itemId);
void func_8009635C(s32 itemId);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_80096214);
#else
void func_80096214(void) {
    s32 itemId = FieldScriptVMGetArgument(1);
    s32 idx = func_80095124(itemId);
    u8* pTable = func_800950A0(itemId);
    u8* pData = func_8009501C(itemId);
    u16 addr;
    s32 value;

    /* Retail 8009625C/80096274: the instruction-argument read is duplicated
     * into both arms; pData is called but its result is unused. */
    if (idx != -1) {
        addr = (u16)FieldScriptVMGetInstructionArgument(3);
        value = pTable[idx];
    } else {
        addr = (u16)FieldScriptVMGetInstructionArgument(3);
        value = 0;
    }

    FieldScriptMemoryWriteU16(addr, value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
#endif /* XENO_PC_PORT */

void func_800962C0(void) {
    if (func_80095124(FieldScriptVMGetArgument(1)) != -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void func_8009631C(void) {
    func_8009635C(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_8009635C);
#else
void func_8009635C(s32 itemId) {
    s32 slot = func_80095124(itemId);
    u8* quantities = func_800950A0(itemId);
    u8* itemIds = func_8009501C(itemId);

    if (slot != -1) {
        if (quantities[slot] < MAX_ITEM_QUANTITY) {
            quantities[slot]++;
        }
    } else {
        slot = func_800951B8(itemId);
        if (slot != -1) {
            itemIds[slot] = (u8)itemId;
            quantities[slot] = 1;
        }
    }
}
#endif /* XENO_PC_PORT */

extern u8* func_8009501C(s32 itemId);
extern u8* func_800950A0(s32 itemId);
extern s32 func_80095124(s32 itemId);

void func_8009640C(void) {
    s32 itemId = FieldScriptVMGetArgument(1);
    s32 idx = func_80095124(itemId);
    if (idx != -1) {
        /* Retail 80096440: pData (func_8009501C) is the array cleared to
         * 0xFF; pTable (func_800950A0) is the counter that is decremented. */
        u8* pData = func_8009501C(itemId);
        u8* pTable = func_800950A0(itemId);
        u8* pEntry = pTable + idx;
        u8 count = pEntry[0] - 1;
        pEntry[0] = count;
        if (count == 0) {
            pData[idx] = 0xFF;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Check if certain member is in current party
void FieldScriptCheckPartyMember(void) {
    int i;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (SCRIPT_READ_U8_REL(1) == g_GamePartyMembers[i]) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            return;
        }        
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
}

void func_80096534(void) {
    u16 flags = *(u16*)((u8*)g_pGameState + 0x1D30);
    u8 bit = SCRIPT_READ_U8_REL(1);
    if ((flags >> bit) & 1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = (u16)FieldScriptVMGetInstructionArgument(2);
    }
}

void func_800965A8(void) {
    g_pGameState->unk1D30 |= 1 << SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_800965F4(void) {
    g_pGameState->unk1D30 &= ~(1 << SCRIPT_READ_U8_REL(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
