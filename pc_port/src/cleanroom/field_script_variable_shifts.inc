#include <stdint.h>

static int fieldScriptUnaryVariableIndex(void);
static int fieldScriptReadBinaryOperand(void);
static int fieldScriptReadArithmeticVariable(void);
static void fieldScriptStoreArithmeticResult(uint16_t value);
extern void fieldScriptUnaryWrite(int address, unsigned short value)
    __asm__("FieldScriptMemoryWriteU16");

static uint32_t fieldScriptVariableBitMask(void)
{
    unsigned int bit = (unsigned int)fieldScriptReadBinaryOperand() & 31u;
    return UINT32_C(1) << bit;
}

void FieldScriptVMHandlerVariableSetBit(void)
{
    uint32_t value = (uint32_t)fieldScriptReadArithmeticVariable();
    uint32_t mask = fieldScriptVariableBitMask();
    fieldScriptStoreArithmeticResult((uint16_t)(value | mask));
}

void FieldScriptVMHandlerVariableUnsetBit(void)
{
    uint32_t value = (uint32_t)fieldScriptReadArithmeticVariable();
    uint32_t mask = fieldScriptVariableBitMask();
    fieldScriptStoreArithmeticResult((uint16_t)(value & ~mask));
}

static unsigned int fieldScriptVariableShiftCount(void)
{
    extern int fieldScriptShiftOperand(int offset) __asm__("FieldScriptVMGetArgument");
    return (unsigned int)fieldScriptShiftOperand(3) & 31u;
}

static void fieldScriptStoreShiftedVariable(uint32_t value)
{
    int destination = fieldScriptUnaryVariableIndex();
    fieldScriptUnaryWrite(destination, (uint16_t)value);
    ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + 5u);
}

void FieldScriptVMHandlerLShiftVariable(void)
{
    uint32_t value = (uint32_t)fieldScriptReadArithmeticVariable();
    unsigned int count = fieldScriptVariableShiftCount();
    fieldScriptStoreShiftedVariable(value << count);
}

void FieldScriptVMHandlerRShiftVariable(void)
{
    int value = fieldScriptReadArithmeticVariable();
    unsigned int count = fieldScriptVariableShiftCount();
    uint32_t bits = (uint32_t)value;
    /* Complementing before and after the unsigned shift fills with sign bits. */
    uint32_t shifted = value < 0 ? ~(~bits >> count) : bits >> count;
    fieldScriptStoreShiftedVariable(shifted);
}
