typedef u32 FieldDialogueScriptWord __attribute__((__may_alias__));

int func_8009CD18(int *);
void func_800A1B70(void);
void FieldScriptMemoryWriteU16(int, int);

void func_8009BB0C(void)
{
    int window;
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    if (func_8009CD18(&window) == -1) {
        volatile int *budget = &g_FieldScriptMaxInstructionCount;
        unsigned int before = (unsigned int)*budget;
        const volatile ActorData *source = *current;
        *budget = (int)(before + 8u);
        FieldScriptMemoryWriteU16(20, source->unk81);
        volatile ActorData *resume = *current;
        resume->scriptInstructionPointer = (u16)(resume->scriptInstructionPointer + 1);
        return;
    }

    int speakerId = ((const volatile FieldTextBox *)g_FieldTextBoxes)[window].talkingActorID;
    const volatile FieldActor *speaker = &g_FieldActors[speakerId];
    const volatile ActorData *dialogueActor = (const void *)(uintptr_t)speaker->pActorData;
    if (dialogueActor->flags & 0x200u) {
        unsigned int flags = ((const volatile ActorData *)*current)->dialogFlags;
        unsigned int policy = flags >> 16;
        if (policy == 0)
            policy = (u16)flags;
        if (!(policy & 1u)) {
            const volatile ActorData *script = *current;
            unsigned int index = script->curScriptIndex;
            const volatile u8 *slot = (const volatile u8 *)script +
                __builtin_offsetof(ActorData, scripts) + index * sizeof(ActorScriptSlot);
            unsigned int state = *(const volatile FieldDialogueScriptWord *)(slot + 4);
            if (((state >> 18) & 15u) != 7)
                func_800A1B70();
            ((volatile FieldTextBox *)g_FieldTextBoxes)[window].status = 0;
        }
    }
    D_800B00C0 = 1;
}
