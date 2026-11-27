/* Host implementation from the independent field VM behavior contract. */
void FieldScriptVMRun(int budget)
{
    D_800B00C0 = 0;
    g_FieldScriptMaxInstructionCount = budget;
    for (int completed = 0; completed < g_FieldScriptMaxInstructionCount; ++completed) {
        if (completed == 1025) {
            if (g_FieldSystemMode == 0) {
                func_800379C8(&D_8006FD84, D_800AFD1C);
            }
            return;
        }

        unsigned short cursor = g_FieldScriptVMCurActor->scriptInstructionPointer;
        unsigned char opcode = ((const unsigned char *)g_FieldScriptVMCurScriptData)[cursor];
        PcPort_FieldVmTrace(cursor, opcode);
        g_FieldScriptVMHandlers[opcode]();

        if (D_800AFFEC == 0) {
            g_FieldScriptMaxInstructionCount = 65535;
        }
        if ((D_800ADB1C && (!D_800ADBE0 || !D_800ADBE4 || !D_800ADBEC)) ||
            (D_800B00C0 == 1 && D_800AFFEC == 1)) {
            return;
        }
    }
}
