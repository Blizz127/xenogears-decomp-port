#include "common.h"
#include "field/actor.h"
#include "field/script_vm.h"

void FieldScriptCheckScenarioFlagsLessThan(void) {
    int scenarioFlags;
    int targetFlags;

    targetFlags = FieldScriptVMGetArgument(1);
    scenarioFlags = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);
    if (scenarioFlags < targetFlags) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptCheckScenarioFlagsGreaterThan(void) {
    int scenarioFlags;
    int targetFlags;

    targetFlags = FieldScriptVMGetArgument(1);
    scenarioFlags = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);
    if (targetFlags < scenarioFlags) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptCheckScenarioFlagsEqual(void) {
    int targetFlags;
    int scenarioFlags;

    targetFlags = FieldScriptVMGetArgument(1);
    scenarioFlags = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);
    if (scenarioFlags == targetFlags) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptSetScenarioFlags(void) {
    g_FieldScriptMaxInstructionCount += 0x20;
    FieldScriptMemoryWriteU16(
        SCRIPT_VAR_SCENARIO_FLAG, 
        FieldScriptVMGetArgument(1)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptGetScenarioFlags(void) {
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

