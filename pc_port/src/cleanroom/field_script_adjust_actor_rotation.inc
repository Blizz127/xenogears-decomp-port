#include <stddef.h>

static void fieldScriptAdjustActorRotation(size_t componentOffset, int subtract)
{
    unsigned int amount = (unsigned int)FieldScriptVMGetArgument(1);
    int index = *(volatile s32 *)&D_800AFD1C;
    FieldActor *actors = *(FieldActor *volatile *)&g_FieldActors;
    volatile u16 *component =
        (volatile u16 *)((u8 *)&actors[index].rotation + componentOffset);
    unsigned int current = *component;
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    *component = (u16)(subtract ? current - amount : current + amount);
    actor->scriptInstructionPointer += 3;
    func_80072254(index);
}

void func_80094A5C(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, x), 0);
}

void func_80094ACC(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, x), 1);
}

void func_80094B3C(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, y), 0);
}

void func_80094BAC(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, y), 1);
}

void func_80094C1C(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, z), 0);
}

void func_80094C8C(void)
{
    fieldScriptAdjustActorRotation(__builtin_offsetof(SVEC, z), 1);
}
