int FieldScriptVMGetArgument(int offset);
void FieldScriptMemoryWriteU16(int address, int value);

static int fieldPartyInstructionMember(unsigned operand)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned position = actor->scriptInstructionPointer;
    unsigned slot = script[position + operand];
    const volatile int *members = g_GamePartyMembers;
    return members[slot];
}

static void fieldPartyFinishStatInstruction(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 4;
}

static void fieldPartyWriteStatToScript(int useMp)
{
    if (fieldPartyInstructionMember(3) != 255) {
        u16 destination = (u16)FieldScriptVMGetInstructionArgument(1);
        int member = fieldPartyInstructionMember(3);
        GameState *state = *(GameState *volatile *)&g_pGameState;
        volatile GameCharacter *character = &state->characters[member];
        u16 value = useMp ? character->mp : character->hp;
        FieldScriptMemoryWriteU16(destination, value);
    }
    fieldPartyFinishStatInstruction();
}

static void fieldPartySetStatFromScript(int useMp)
{
    if (fieldPartyInstructionMember(3) != 255) {
        int requested = FieldScriptVMGetArgument(2);
        int member = fieldPartyInstructionMember(1);
        GameState *state = *(GameState *volatile *)&g_pGameState;
        volatile GameCharacter *character = &state->characters[member];
        u16 maximum = useMp ? character->maxMp : character->maxHp;
        u16 value = (u16)(requested < maximum ? requested : maximum);
        if (useMp) {
            character->mp = value;
        } else {
            character->hp = value;
        }
    }
    fieldPartyFinishStatInstruction();
}

void FieldScriptVMHandlerWritePartyMemberHp(void)
{
    fieldPartyWriteStatToScript(0);
}

void FieldScriptVMHandlerWritePartyMemberMp(void)
{
    fieldPartyWriteStatToScript(1);
}

void FieldScriptVMHandlerSetPartyMemberHp(void)
{
    fieldPartySetStatFromScript(0);
}

void FieldScriptVMHandlerSetPartyMemberMp(void)
{
    fieldPartySetStatFromScript(1);
}
