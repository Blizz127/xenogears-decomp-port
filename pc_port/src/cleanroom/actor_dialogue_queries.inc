int func_8009CD18(int *outIndex)
{
    int index;

    for (index = 0; index < 4; ++index) {
        const volatile FieldTextBox *box = &g_FieldTextBoxes[index];
        if (box->ownerActorID == D_800AFD1C && box->visibility == 0) {
            *outIndex = index;
            return 0;
        }
    }
    return -1;
}

unsigned int FieldScriptVMGetActorIndex(int displacement)
{
    const unsigned char *script = g_FieldScriptVMCurScriptData;
    long long offset = (long long)g_FieldScriptVMCurActor->scriptInstructionPointer + displacement;
    unsigned int selector = script[offset];

    switch (selector) {
    case 0xFB:
        return (unsigned int)D_800AFD1C;
    case 0xFD:
        return (unsigned int)D_8005A44C;
    case 0xFE:
        return (unsigned int)D_8005A448;
    case 0xFF:
        return (unsigned int)D_8005A444;
    default:
        return selector;
    }
}
