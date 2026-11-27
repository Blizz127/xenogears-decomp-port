#include <stdint.h>

typedef uint16_t FieldActorParameterHalf __attribute__((__may_alias__));
extern int fieldActorParameterGet(int offset) __asm__("FieldScriptVMGetArgument");
extern int fieldActorParameterRaw(int offset) __asm__("FieldScriptVMGetInstructionArgument");
extern void fieldActorParameterSet(int destination, int value) __asm__("FieldScriptMemoryWriteU16");
extern uint8_t fieldActorParameterByte __asm__("D_800B2358");

static volatile FieldActorParameterHalf *fieldActorParameterCursor(ActorData *actor)
{
    return (volatile FieldActorParameterHalf *)((unsigned char *)actor +
        __builtin_offsetof(ActorData, scriptInstructionPointer));
}

static ActorData *fieldActorParameterCurrent(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static void fieldActorParameterAdvance(unsigned bytes)
{
    volatile FieldActorParameterHalf *cursor = fieldActorParameterCursor(fieldActorParameterCurrent());
    *cursor = (uint16_t)(*cursor + bytes);
}

void func_80086DE0(void)
{
    ActorData *actor = fieldActorParameterCurrent();
    const volatile uint8_t *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    volatile FieldActorParameterHalf *cursor = fieldActorParameterCursor(actor);
    unsigned index = *cursor;
    *(volatile uint8_t *)&fieldActorParameterByte = script[index + 1u];
    *cursor = (uint16_t)(*cursor + 2u);
}

void func_80086F7C(void)
{
    uint32_t high = (uint32_t)fieldActorParameterGet(1);
    uint32_t low = (uint32_t)fieldActorParameterGet(3);
    ActorData *actor = fieldActorParameterCurrent();
    volatile FieldActorParameterHalf *cursor = fieldActorParameterCursor(actor);
    uint16_t oldCursor = *cursor;
    volatile FieldActorParameterHalf *animation = (volatile FieldActorParameterHalf *)(
        (unsigned char *)actor + __builtin_offsetof(ActorData, modelAnimation));
    *animation = (uint16_t)((high << 12) | low);
    *cursor = (uint16_t)(oldCursor + 5u);
}

static int fieldActorParameterScale(int multiplier, int divisor, int numerator)
{
    uint32_t dividendWord = (uint32_t)numerator << 16;
    int32_t dividend = (int32_t)dividendWord;
    int32_t quotient;
    if (divisor == 0)
        quotient = dividend < 0 ? 1 : -1;
    else if (dividend == INT32_MIN && divisor == -1)
        quotient = INT32_MIN;
    else
        quotient = dividend / divisor;

    uint32_t product = (uint32_t)quotient * (uint32_t)multiplier;
    unsigned upper = product >> 16;
    return upper >= 0x8000u ? (int)upper - 0x10000 : (int)upper;
}

void func_80087420(void)
{
    int xMultiplier = fieldActorParameterGet(1);
    int yMultiplier = fieldActorParameterGet(3);
    int xDivisor = fieldActorParameterGet(5);
    int yDivisor = fieldActorParameterGet(7);
    int xNumerator = fieldActorParameterGet(9);
    int yNumerator = fieldActorParameterGet(11);
    int x = fieldActorParameterScale(xMultiplier, xDivisor, xNumerator);
    int y = fieldActorParameterScale(yMultiplier, yDivisor, yNumerator);
    int destination = (uint16_t)fieldActorParameterRaw(13);
    fieldActorParameterSet(destination, x);
    destination = (uint16_t)fieldActorParameterRaw(15);
    fieldActorParameterSet(destination, y);
    fieldActorParameterAdvance(17);
}
