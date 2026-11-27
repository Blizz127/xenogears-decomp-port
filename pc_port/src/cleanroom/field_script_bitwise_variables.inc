#include <stdint.h>

static int fieldScriptUnaryVariableIndex(void);
extern void fieldScriptUnaryWrite(int address, unsigned short value)
    __asm__("FieldScriptMemoryWriteU16");

static int fieldScriptReadBinaryOperand(void)
{
    extern int FieldScriptArgument2(int index, int flags);
    ActorData *actor = g_FieldScriptVMCurActor;
    const unsigned char *script = g_FieldScriptVMCurScriptData;
    unsigned int cursor = actor->scriptInstructionPointer;
    return FieldScriptArgument2(3, script[cursor + 5u]);
}

static void fieldScriptFinishBinaryVariable(int destination, int value)
{
    fieldScriptUnaryWrite(destination, (uint16_t)value);
    ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + 6u);
}

void FieldScriptVMHandlerVariableAssign(void)
{
    int destination = fieldScriptUnaryVariableIndex();
    int value = fieldScriptReadBinaryOperand();
    fieldScriptFinishBinaryVariable(destination, value);
}

void FieldScriptVMHandlerVariableAND(void)
{
    int left = FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
    int right = fieldScriptReadBinaryOperand();
    fieldScriptFinishBinaryVariable(fieldScriptUnaryVariableIndex(), left & right);
}

void FieldScriptVMHandlerVariableOR(void)
{
    int left = FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
    int right = fieldScriptReadBinaryOperand();
    fieldScriptFinishBinaryVariable(fieldScriptUnaryVariableIndex(), left | right);
}

void FieldScriptVMHandlerVariableXOR(void)
{
    int left = FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
    int right = fieldScriptReadBinaryOperand();
    fieldScriptFinishBinaryVariable(fieldScriptUnaryVariableIndex(), left ^ right);
}
