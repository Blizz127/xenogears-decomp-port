int FieldScriptArgument1(int offset, int flags);

static unsigned fieldPartyAdjustmentFlags(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned ip = actor->scriptInstructionPointer;
    return script[ip + 3];
}

static void fieldPartyAdjustSelected(void (*apply)(int, int))
{
    int amount = FieldScriptArgument1(1, fieldPartyAdjustmentFlags());
    unsigned selection = fieldPartyAdjustmentFlags() & 3;
    const volatile s16 *masks = g_FieldNumPartyMembersMasks;
    unsigned mask = (u16)masks[selection];
    const volatile int *roster = g_GamePartyMembers;
    for (int slot = 0; slot < MAX_PARTY_MEMBERS; slot++) {
        int id = roster[slot];
        if (id != 255 && (mask & (1u << slot))) {
            apply(slot, amount);
        }
    }
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerDecreasePartyHp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberDecreaseHp);
}

void FieldScriptVMHandlerIncreasePartyMp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberIncreaseMp);
}

void FieldScriptVMHandlerDecreasePartyMp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberDecreaseMp);
}
