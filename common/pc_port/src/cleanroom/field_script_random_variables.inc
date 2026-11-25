#include <stdint.h>

extern int rand(void);
extern int FieldScriptVMGetArgument(int index);
static int fieldScriptUnaryVariableIndex(void);
extern void fieldScriptUnaryWrite(int address, unsigned short value)
    __asm__("FieldScriptMemoryWriteU16");

/* Reserved field command 0xd1 leaves the caller's script position intact. */
void func_8009CF70(void)
{
}

static void fieldScriptStoreRandomVariable(int destination, int value, unsigned length)
{
    fieldScriptUnaryWrite(destination, (uint16_t)value);
    ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + length);
}

void FieldScriptVMHandlerRandVariable(void)
{
    int destination = fieldScriptUnaryVariableIndex();
    int draw = rand();
    fieldScriptStoreRandomVariable(destination, draw, 3);
}

void FieldScriptVMHandlerMulVariableWithRand(void)
{
    uint32_t draw = (uint32_t)rand();
    uint32_t limit = (uint32_t)FieldScriptVMGetArgument(3);
    uint32_t product = draw * (limit + 1u);

    /* The signed high 17 bits give the floor of the wrapped product / 32768. */
    int scaled = (int)(product >> 15);
    if (product & UINT32_C(0x80000000)) {
        scaled -= 131072;
    }
    int destination = fieldScriptUnaryVariableIndex();
    fieldScriptStoreRandomVariable(destination, scaled, 5);
}
