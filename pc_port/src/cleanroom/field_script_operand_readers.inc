#include <stdint.h>

extern int FieldScriptVMGetVariableValue(int);

/* A signed instruction-relative offset may cross either end of the 16-bit
   cursor range. Keep the address at host pointer width until both bytes load. */
static unsigned fieldScriptReadOperand(int offset)
{
    uintptr_t base = (uintptr_t)g_FieldScriptVMCurScriptData;
    unsigned cursor = g_FieldScriptVMCurActor->scriptInstructionPointer;
    uintptr_t address = base + cursor + (uintptr_t)(intptr_t)offset;
    const volatile uint8_t *operand = (const volatile uint8_t *)address;
    unsigned high = operand[1];
    unsigned low = operand[0];
    return (high << 8) | low;
}

short FieldScriptVMGetInstructionArgumentS16(int offset)
{
    int value = (int)fieldScriptReadOperand(offset);
    return (short)(value < 0x8000 ? value : value - 0x10000);
}

int FieldScriptVMGetInstructionArgument(int offset)
{
    return (int)fieldScriptReadOperand(offset);
}

int FieldScriptVMGetArgument(int offset)
{
    unsigned operand = (uint16_t)FieldScriptVMGetInstructionArgument(offset);
    if ((operand & 0x8000u) == 0)
        return FieldScriptVMGetVariableValue((int)operand);
    return (int)(operand & 0x7fffu);
}
