#include "common.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern u16 D_800AFC6C; // Held buttons state?
extern u16 D_800AFE9C;

void FieldScriptVMCheckControllerInput(u_short buttonState) {
    u_short nButtonMask;
    
    nButtonMask = FieldScriptVMGetInstructionArgument(1);
    if (nButtonMask & buttonState) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptCheckControllerInputExact(u_short buttonState) {
    u_short buttonMask;

    buttonMask = FieldScriptVMGetInstructionArgument(1);
    if (buttonMask == buttonState) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void func_80096150(void) {
    FieldScriptCheckControllerInputExact(D_800AFE9C);
}

void func_80096178(void) {
    FieldScriptCheckControllerInputExact(D_800AFC6C);
}

// Is button pressed handler
void func_800961A0(void) {
    FieldScriptVMCheckControllerInput(D_800AFE9C);
}

void func_800961C8(void) {
    FieldScriptVMCheckControllerInput(D_800AFC6C);
}

void func_800961F0(void) {
    D_800AFC6C = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}
