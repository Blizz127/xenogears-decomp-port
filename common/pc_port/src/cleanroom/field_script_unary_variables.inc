#include <stdint.h>

extern void fieldScriptUnaryWrite(int address, unsigned short value)
    __asm__("FieldScriptMemoryWriteU16");

static int fieldScriptUnaryVariableIndex(void)
{
    return FieldScriptVMGetInstructionArgument(1) & 0xffff;
}

static void fieldScriptStoreUnaryVariable(uint16_t value)
{
    int destination = fieldScriptUnaryVariableIndex();
    fieldScriptUnaryWrite(destination, value);

    /* The write can change the active actor or its script position. */
    ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + 3u);
}

void FieldScriptVMHandlerVariableSetFalse(void)
{
    fieldScriptStoreUnaryVariable(0);
}

void FieldScriptVMHandlerVariableSetTrue(void)
{
    fieldScriptStoreUnaryVariable(1);
}

void FieldScriptVMHandlerIncVariable(void)
{
    uint32_t value = (uint32_t)FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
    fieldScriptStoreUnaryVariable((uint16_t)(value + 1u));
}

void FieldScriptVMHandlerDecVariable(void)
{
    uint32_t value = (uint32_t)FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
    fieldScriptStoreUnaryVariable((uint16_t)(value - 1u));
}
