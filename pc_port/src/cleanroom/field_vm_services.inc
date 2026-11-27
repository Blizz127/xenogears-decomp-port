#include <stdint.h>

extern int g_FieldSystemMode;
extern char D_8006FD44;
extern int D_800AFD1C;
extern void func_800379C8(char *, ...);
extern void func_800A476C(int, int);
extern void fieldVmWritePartyId(int address, int member)
    __asm__("FieldScriptMemoryWriteU16");

/* This packed state word holds the call depth in bits 6 through 8. */
typedef uint32_t FieldVmStateWord __attribute__((__may_alias__));

static volatile FieldVmStateWord *fieldVmCallState(ActorData *actor)
{
    return (volatile FieldVmStateWord *)((unsigned char *)actor + 0x12c);
}

void func_800A1730(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    unsigned depth = (*fieldVmCallState(actor) >> 6) & 7u;
    if (depth == 4) {
        if (g_FieldSystemMode == 0) {
            func_800379C8(&D_8006FD44, D_800AFD1C);
        }
        D_800B00C0 = 1;
        return;
    }

    uint16_t cursor = ((const volatile ActorData *)actor)->scriptInstructionPointer;
    uint16_t resume = (uint16_t)(cursor + 5u);
    unsigned offset = __builtin_offsetof(ActorData, scriptPointersStack) + depth * sizeof resume;
    /* A damaged depth can select a halfword beyond the four named stack slots. */
    __builtin_memcpy((unsigned char *)actor + offset, &resume, sizeof resume);

    uint16_t destination = (uint16_t)FieldScriptVMGetInstructionArgument(1);
    actor = g_FieldScriptVMCurActor;
    ((volatile ActorData *)actor)->scriptInstructionPointer = destination;
    volatile FieldVmStateWord *state = fieldVmCallState(actor);
    uint32_t flags = *state;
    const uint32_t depthMask = 7u << 6;
    *state = (flags & ~depthMask) | ((flags + (1u << 6)) & depthMask);
}

void FieldScriptWritePartyMemberIDs(void)
{
    const volatile int *members = g_GamePartyMembers;
    for (unsigned slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        fieldVmWritePartyId((int)(62u + 2u * slot), members[slot]);
    }
}

void func_800A4748(void)
{
    func_800A476C(704, 256);
}
