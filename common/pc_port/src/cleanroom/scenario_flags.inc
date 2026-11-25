/* Original host C written from an independent behavioral brief for the US
 * field overlay. The author used only the supplied behavior and API facts. */

static void FieldScriptFinishScenarioComparison(int condition)
{
    if (condition) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        unsigned short target = (unsigned short)FieldScriptVMGetInstructionArgument(3);
        g_FieldScriptVMCurActor->scriptInstructionPointer = target;
    }
}

void FieldScriptCheckScenarioFlagsLessThan(void)
{
    int threshold = FieldScriptVMGetArgument(1);
    int scenario = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);

    FieldScriptFinishScenarioComparison(scenario < threshold);
}

void FieldScriptCheckScenarioFlagsGreaterThan(void)
{
    int threshold = FieldScriptVMGetArgument(1);
    int scenario = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);

    FieldScriptFinishScenarioComparison(scenario > threshold);
}

void FieldScriptCheckScenarioFlagsEqual(void)
{
    int threshold = FieldScriptVMGetArgument(1);
    int scenario = FieldScriptVMGetVariableValue(SCRIPT_VAR_SCENARIO_FLAG);

    FieldScriptFinishScenarioComparison(scenario == threshold);
}
