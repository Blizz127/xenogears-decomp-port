/* Independently implemented from leaf-contract.md and its public actor ABI.
 * leaf-receipt.json identifies the behavioral reference as disc/field.bin,
 * SHA-256 38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc.
 */
extern s32 D_800AFD1C;
extern s32 D_8006F990[];

void FieldScriptVMHandlerDisableDialogActivation(void)
{
    g_FieldScriptVMCurActor->scriptFlags.flags |= 1u << 17;
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

void FieldScriptVMHandlerEnableDialogActivation(void)
{
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~(1u << 17);
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

void func_8009DA70(void)
{
    g_FieldScriptVMCurActor->scriptFlags.flags |= 1u << 23;
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

void func_8009DA98(void)
{
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~(1u << 23);
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

void func_8009E014(void)
{
    g_FieldScriptVMCurActor->flags |= 0x02000800u;
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

void FieldScriptVMHandlerHideActor(void)
{
    g_FieldActors[D_800AFD1C].status |= 1u << 5;
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
}

/* Interpret a 16-bit pattern as signed without an out-of-range signed cast. */
static short fieldActorSignedHalfword(unsigned int bits)
{
    return (int)(bits ^ 0x8000u) - 0x8000;
}

void func_8009E208(void)
{
    g_FieldScriptVMCurActor->unkEC = 0;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 1u << 22;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~(1u << 18);
    ++g_FieldScriptVMCurActor->scriptInstructionPointer;
    g_FieldScriptVMCurActor->curYPos = fieldActorSignedHalfword(
        (unsigned int)g_FieldScriptVMCurActor->position.vy >> 16);
}

short func_8009E330(int offset)
{
    const unsigned char *bytes = (const unsigned char *)g_FieldScriptVMCurScriptData + offset;

    return fieldActorSignedHalfword(bytes[0] | ((unsigned int)bytes[1] << 8));
}

void func_8009E83C(void)
{
    const unsigned char *instruction = (const unsigned char *)g_FieldScriptVMCurScriptData
        + g_FieldScriptVMCurActor->scriptInstructionPointer;
    unsigned int value = instruction[1];

    if (value != 0) {
        g_FieldScriptVMCurActor->width = value * 2;
    }
    value = instruction[2];
    if (value != 0) {
        g_FieldScriptVMCurActor->zWidth = value * 2;
    }
    value = instruction[3];
    if (value != 0) {
        g_FieldScriptVMCurActor->height = value * 2;
    }
    value = instruction[4];
    if (value != 0) {
        g_FieldScriptVMCurActor->solidRange = value * 2;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

int func_8009FC10(int actorIndex)
{
    int slot;

    for (slot = 0; slot < 3; ++slot) {
        if (D_8006F990[slot] == actorIndex) {
            return slot;
        }
    }
    return 255;
}
