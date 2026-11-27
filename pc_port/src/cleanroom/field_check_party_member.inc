#include <stdint.h>

extern int FieldScriptVMGetInstructionArgument(int);
extern int g_GamePartyMembers[];

void FieldScriptCheckPartyMember(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile uint8_t *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    const volatile int *party = g_GamePartyMembers;

    for (unsigned i = 0; i < 3; ++i) {
        unsigned cursor = actor->scriptInstructionPointer;
        unsigned wanted = script[cursor + 1u];
        int member = party[i];
        if (member == (int)wanted) {
            actor->scriptInstructionPointer = (uint16_t)(cursor + 4u);
            return;
        }
    }

    int target = FieldScriptVMGetInstructionArgument(2);
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)target;
}
