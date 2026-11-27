#include <stddef.h>
#include <stdint.h>

extern int FieldScriptVMGetInstructionArgument(int);
extern int FieldScriptVMGetArgument(int);
extern int FieldScriptVMGetVariableValue(int);
extern void fieldRecordStoreVariable(int, int) __asm__("FieldScriptMemoryWriteU16");

static volatile ActorData *fieldRecordActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static void fieldRecordAdvance(unsigned length)
{
    volatile ActorData *actor = fieldRecordActor();
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + length);
}

static void fieldRecordChangeBit(int set)
{
    unsigned slot = (uint16_t)FieldScriptVMGetInstructionArgument(1) >> 4;
    unsigned bit = (unsigned)FieldScriptVMGetInstructionArgument(1) & 15u;
    int value = FieldScriptVMGetVariableValue((int)slot);
    int mask = 1 << bit;
    fieldRecordStoreVariable((int)slot, set ? value | mask : value & ~mask);
    fieldRecordAdvance(3);
}

void func_8008D684(void)
{
    fieldRecordChangeBit(1);
}

void func_8008D700(void)
{
    fieldRecordChangeBit(0);
}

void FieldScriptVMConditionalJump5(unsigned short flag)
{
    int bits = FieldScriptVMGetInstructionArgument(1);
    if (((unsigned)bits & flag) != 0) {
        fieldRecordAdvance(5);
    } else {
        int cursor = FieldScriptVMGetInstructionArgument(3);
        fieldRecordActor()->scriptInstructionPointer = (uint16_t)cursor;
    }
}

void func_8008D2D8(void)
{
}

void FieldSetScreenDimensions(void)
{
    for (unsigned i = 0; i < 2; ++i) {
        volatile RenderContext *context = &g_FieldRenderContexts[i];
        context->dispEnv.screen.x = 0;
        context->dispEnv.screen.y = 10;
        context->dispEnv.screen.w = 256;
        context->dispEnv.screen.h = 216;
    }
}

typedef uint32_t FieldGearWord __attribute__((__may_alias__,aligned(1)));

void FieldScriptCopyGear(void)
{
    uint32_t source = (uint32_t)FieldScriptVMGetArgument(1);
    uint32_t target = (uint32_t)FieldScriptVMGetArgument(3);
    GameState *game = *(GameState *volatile *)&g_pGameState;
    uint8_t *bytes = (uint8_t *)game;
    uint32_t src = (uint32_t)__builtin_offsetof(GameState, gears) + source * (uint32_t)sizeof(GameGear);
    uint32_t dst = (uint32_t)__builtin_offsetof(GameState, gears) + target * (uint32_t)sizeof(GameGear);
    /* Observe each group of four source words before modifying any destination
       word in that group. A wrapped index may still designate real game data. */
    for (unsigned block = 0; block < sizeof(GameGear); block += 16u) {
        unsigned count = (sizeof(GameGear) - block < 16u) ? 1u : 4u;
        uint32_t words[4];
        unsigned eligible = 0;
        for (unsigned i = 0; i < count; ++i) {
            uint32_t s = src + block + i * 4u, t = dst + block + i * 4u;
            if (s <= sizeof(GameState) - 4u && t <= sizeof(GameState) - 4u) {
                words[i] = *(const volatile FieldGearWord *)(bytes + s);
                eligible |= 1u << i;
            }
        }
        for (unsigned i = 0; i < count; ++i) {
            if ((eligible & (1u << i)) != 0) {
                uint32_t t = dst + block + i * 4u;
                *(volatile FieldGearWord *)(bytes + t) = words[i];
            }
        }
    }
    fieldRecordAdvance(5);
}

void FieldScriptSetCharacterGear(void)
{
    uint32_t character = (uint32_t)FieldScriptVMGetArgument(1);
    uint8_t gear = (uint8_t)FieldScriptVMGetArgument(3);
    GameState *game = *(GameState *volatile *)&g_pGameState;
    uint32_t offset = (uint32_t)__builtin_offsetof(GameState, characters[0].gearId)
        + character * (uint32_t)sizeof(GameCharacter);
    if (offset < sizeof(GameState))
        *((volatile uint8_t *)game + offset) = gear;
    fieldRecordAdvance(5);
}
