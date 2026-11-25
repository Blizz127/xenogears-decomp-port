#include <stdint.h>

static int fieldScriptUnaryVariableIndex(void);
static int fieldScriptReadBinaryOperand(void);
static void fieldScriptFinishBinaryVariable(int destination, int value);

static int fieldScriptReadArithmeticVariable(void)
{
    return FieldScriptVMGetVariableValue(fieldScriptUnaryVariableIndex());
}

static void fieldScriptStoreArithmeticResult(uint16_t value)
{
    fieldScriptFinishBinaryVariable(fieldScriptUnaryVariableIndex(), value);
}

void FieldScriptVMHandlerVariableAdd(void)
{
    uint32_t left = (uint32_t)fieldScriptReadArithmeticVariable();
    uint32_t right = (uint32_t)fieldScriptReadBinaryOperand();
    fieldScriptStoreArithmeticResult((uint16_t)(left + right));
}

void FieldScriptVMHandlerVariableSub(void)
{
    uint32_t left = (uint32_t)fieldScriptReadArithmeticVariable();
    uint32_t right = (uint32_t)fieldScriptReadBinaryOperand();
    fieldScriptStoreArithmeticResult((uint16_t)(left - right));
}

void FieldScriptVMHandlerVariableMul(void)
{
    uint32_t left = (uint32_t)fieldScriptReadArithmeticVariable();
    uint32_t right = (uint32_t)fieldScriptReadBinaryOperand();
    fieldScriptStoreArithmeticResult((uint16_t)(left * right));
}

void FieldScriptVMHandlerVariableDiv(void)
{
    /* The wider dividend also permits the quotient of INT32_MIN and -1. */
    int64_t dividend = fieldScriptReadArithmeticVariable();
    int divisor = fieldScriptReadBinaryOperand();
    int64_t quotient = dividend / (divisor == 0 ? 1 : divisor);
    fieldScriptStoreArithmeticResult((uint16_t)quotient);
}
