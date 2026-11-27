#include <stdint.h>

extern int FieldScriptVMGetArgument(int);
extern int FieldScriptVMGetInstructionArgument(int);
extern int FieldScriptVMGetVariableValue(int);
extern void fieldScenarioStore(int, int) __asm__("FieldScriptMemoryWriteU16");

static void fieldScenarioAdvance(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (uint16_t)(actor->scriptInstructionPointer + 3u);
}

void FieldScriptSetScenarioFlags(void)
{
    volatile unsigned int *budget = (volatile unsigned int *)&g_FieldScriptMaxInstructionCount;
    *budget += 32u;
    int flags = FieldScriptVMGetArgument(1);
    fieldScenarioStore(0, flags);
    fieldScenarioAdvance();
}

void FieldScriptGetScenarioFlags(void)
{
    int destination = (uint16_t)FieldScriptVMGetInstructionArgument(1);
    int flags = FieldScriptVMGetVariableValue(0);
    fieldScenarioStore(destination, flags);
    fieldScenarioAdvance();
}
