void func_8009BE9C(void)
{
    ActorData* volatile* currentActor = &g_FieldScriptVMCurActor;
    void* volatile* currentBank = &g_FieldScriptVMCurScriptData;
    volatile ActorData* actor = *currentActor;
    const volatile u8* bank = *currentBank;
    u16 cursor = actor->scriptInstructionPointer;
    u8 mode = bank[cursor + 1u];

    if (mode != 0) {
        actor->dialogWidth = 0;
        volatile ActorData* heightActor = *currentActor;
        actor->dialogPixelWidth = 0;
        actor->dialogPixelHeight = 0;
        heightActor->dialogHeight = 0;
        actor = *currentActor;
        cursor = actor->scriptInstructionPointer;
        actor->dialogFlags = 0;
    } else {
        extern int FieldDismissLookup(int* outIndex) __asm__("func_8009CD18");
        int window;
        int result = FieldDismissLookup(&window);
        actor = *currentActor;
        if (result == 0) {
            volatile FieldTextBox* box = &g_FieldTextBoxes[window];
            box->status = 0;
        }
        cursor = actor->scriptInstructionPointer;
    }
    actor->scriptInstructionPointer = cursor + 2u;
    volatile int* stop = &D_800B00C0;
    *stop = 1;
}
