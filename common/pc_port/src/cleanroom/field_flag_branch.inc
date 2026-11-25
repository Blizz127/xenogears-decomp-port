void FieldScriptVMConditionalJump6(unsigned short mask)
{
    u16 scriptMask = (u16)FieldScriptVMGetInstructionArgument(1);

    if (scriptMask & mask) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        u16 target = (u16)FieldScriptVMGetInstructionArgument(4);
        g_FieldScriptVMCurActor->scriptInstructionPointer = target;
    }
}
