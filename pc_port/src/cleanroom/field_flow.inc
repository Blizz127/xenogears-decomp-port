#include <stddef.h>
#include <stdint.h>

/* The field VM uses overlapping views of guest memory. Keep its aligned
 * word accesses intact even when a cell also serves as an actor field. */
typedef uint32_t FieldFlowWord __attribute__((__may_alias__));

extern int D_800AFFEC;

void FieldScriptVMHandlerJmp(void)
{
    int target = FieldScriptVMGetInstructionArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer = (uint16_t)target;
}

void FieldScriptVMHandlerNop(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + 1u);
}

void func_800A1E9C(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    volatile FieldFlowWord *budget = (volatile FieldFlowWord *)&g_FieldScriptMaxInstructionCount;
    volatile uint16_t *cursor = &actor->scriptInstructionPointer;
    uint32_t old_budget = *budget;
    uint16_t old_cursor = *cursor;

    *budget = old_budget + UINT32_C(32);
    *cursor = (uint16_t)(old_cursor + 1u);
}

void func_800A1B70(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    volatile const unsigned char *selector = &actor->curScriptIndex;
    unsigned char *actor_bytes = (unsigned char *)actor;
    const size_t scripts = __builtin_offsetof(ActorData, scripts);
    const size_t id = __builtin_offsetof(ActorScriptSlot, scriptId);
    const size_t flags = id + sizeof(actor->scripts[0].scriptId);
    size_t slot = scripts + sizeof(ActorScriptSlot) * *selector;
    /* Scheduled slots always fit. A malformed selector may describe a guest
     * address outside this host actor. Only touch destinations it backs. */
    if (slot + flags + sizeof(FieldFlowWord) <= sizeof(*actor)) {
        volatile FieldFlowWord *state = (volatile FieldFlowWord *)(actor_bytes + slot + flags);
        *state |= UINT32_C(0x003c0000);
    }
    slot = scripts + sizeof(ActorScriptSlot) * *selector;
    if (slot + id < sizeof(*actor)) {
        ((volatile unsigned char *)actor_bytes)[slot + id] = UINT8_MAX;
    }
    *(volatile int *)&D_800AFFEC = 1;
    *(volatile int *)&D_800B00C0 = 1;
}
