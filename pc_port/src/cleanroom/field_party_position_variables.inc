void func_800A0158(s32 slot, s32 *first, s32 *second, s32 *third)
{
    s32 firstVariable;

    if (slot < 0 || slot > 2) {
        return;
    }

    firstVariable = 42 + slot * 6;
    *first = FieldScriptVMGetVariableValue(firstVariable);
    *second = FieldScriptVMGetVariableValue(firstVariable + 2);
    *third = FieldScriptVMGetVariableValue(firstVariable + 4);
}
