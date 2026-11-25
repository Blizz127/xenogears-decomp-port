#include <stdint.h>
#include <string.h>

void FieldScriptFadeOut(void)
{
    FieldFadeInitializePrimitives(0);
    FieldFadeToBlack(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptFadeIn(void)
{
    FieldFadeToWhite(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_800973A4(void)
{
    uint16_t argument = (uint16_t)FieldScriptVMGetInstructionArgument(2);
    ActorData *actor = g_FieldScriptVMCurActor;
    const unsigned char *script = g_FieldScriptVMCurScriptData;
    int command = script[actor->scriptInstructionPointer + 1];

    if (func_8001B484(argument, command) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

void func_80098C00(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    uintptr_t slot = (uintptr_t)actor + 0x90u + 8u * actor->curScriptIndex;
    uint16_t reset = UINT16_MAX;

    memcpy((void *)slot, &reset, sizeof(reset));
    func_80098CAC(0);
}
