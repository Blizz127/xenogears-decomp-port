/* Each argument has one flag selecting a signed literal or a variable reference. */
static int fieldScriptReadArgument(int index, int flags, int literalFlag)
{
    if (flags & literalFlag) {
        return FieldScriptVMGetInstructionArgumentS16(index);
    }

    int operand = FieldScriptVMGetInstructionArgument(index);
    return FieldScriptVMGetVariableValue(operand & 0xffff);
}

int FieldScriptArgument1(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x80);
}

int FieldScriptArgument2(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x40);
}

int FieldScriptArgument3(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x20);
}

int FieldScriptArgument4(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x10);
}

int FieldScriptArgument5(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x08);
}

int FieldScriptArgument6(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x04);
}

int FieldScriptArgument7(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x02);
}

int FieldScriptArgument8(int index, int flags)
{
    return fieldScriptReadArgument(index, flags, 0x01);
}
