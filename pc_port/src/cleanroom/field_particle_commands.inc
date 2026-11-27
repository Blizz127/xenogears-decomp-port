#include <stdint.h>

extern int particleArgument1(int, int) __asm__("FieldScriptArgument1");
extern int particleArgument2(int, int) __asm__("FieldScriptArgument2");
extern int particleArgument3(int, int) __asm__("FieldScriptArgument3");
extern int particleArgument4(int, int) __asm__("FieldScriptArgument4");
extern int particleArgument5(int, int) __asm__("FieldScriptArgument5");
extern int particleArgument6(int, int) __asm__("FieldScriptArgument6");
extern int particleArgument7(int, int) __asm__("FieldScriptArgument7");
extern int particleArgument8(int, int) __asm__("FieldScriptArgument8");
extern int particleInitializeBanks(int) __asm__("FieldInitializeParticleBanks");
extern void particleStopActor(int, int) __asm__("FieldParticleActorStop");
extern int D_800ADB8C, D_800AFD1C;

static unsigned fieldParticleScriptByte(unsigned displacement)
{
    const volatile ActorData *actor = g_FieldScriptVMCurActor;
    const volatile uint8_t *stream = g_FieldScriptVMCurScriptData;
    return stream[(unsigned)actor->scriptInstructionPointer + displacement];
}

static void fieldParticleAdvance(unsigned bytes)
{
    volatile ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + bytes);
}

static void fieldParticleChargeBudget(void)
{
    volatile int *budget = &g_FieldScriptMaxInstructionCount;
    *budget = (int)((uint32_t)*budget + 4u);
}

static int (*const fieldParticleArgumentSelectors[])(int, int) = {
    particleArgument1, particleArgument2, particleArgument3, particleArgument4,
    particleArgument5, particleArgument6, particleArgument7, particleArgument8
};

void FieldScriptSetParticleBankDirections(int first)
{
    for (unsigned step = 0; step < 8; ++step) {
        int value = fieldParticleArgumentSelectors[step]((int)(1u + step * 2u), (int)fieldParticleScriptByte(17));
        uint32_t bank = (uint32_t)D_800B2384.bankIndex;
        uint32_t offset = bank * (uint32_t)sizeof(ParticleBank) +
                          (uint32_t)__builtin_offsetof(ParticleBank, directions) +
                          (uint32_t)first * 4u + step * 2u;
        if (offset <= sizeof g_FieldDefaultParticleBanks - sizeof(uint16_t)) {
            uint16_t result = (uint16_t)value;
            __builtin_memcpy((unsigned char *)&g_FieldDefaultParticleBanks + offset, &result, sizeof result);
        }
    }
    volatile ActorData *actor = g_FieldScriptVMCurActor;
    volatile int *budget = &g_FieldScriptMaxInstructionCount;
    uint32_t remaining = (uint32_t)*budget;
    uint16_t cursor = actor->scriptInstructionPointer;
    *budget = (int)(remaining + 4u);
    actor->scriptInstructionPointer = (uint16_t)(cursor + 18u);
}

void func_80088CF8(void)
{
    FieldScriptSetParticleBankDirections(0);
}

void func_80088D18(void)
{
    FieldScriptSetParticleBankDirections(4);
}

void FieldScriptParticlesInitialize(void)
{
    int mode = *(volatile int *)&D_800ADB8C;
    fieldParticleChargeBudget();
    if (mode == 0) {
        particleInitializeBanks(D_800AFD1C);
    }
    fieldParticleAdvance(1);
}

void FieldScriptStopParticleActor(void)
{
    const volatile uint8_t *stream = g_FieldScriptVMCurScriptData;
    const volatile ActorData *actor = g_FieldScriptVMCurActor;
    unsigned cursor = actor->scriptInstructionPointer;
    fieldParticleChargeBudget();
    int actorId = *(volatile int *)&D_800AFD1C;
    int mode = stream[cursor + 1u];
    particleStopActor(actorId, mode);
    fieldParticleAdvance(2);
}
