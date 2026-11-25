void FieldScriptCheckActorOnScreen(void)
{
    s32 x, y;
    FieldProjectActorOriginToScreen(&x, &y);

    s32 skipBounds = *(const volatile s32 *)&D_800ADC18;
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    /* Subtract in unsigned arithmetic to test the inclusive ranges
     * 1..223 and 1..319 with one read per coordinate. */
    if (skipBounds || ((u32)*(const volatile s32 *)&y - 1u < 223u &&
                       (u32)*(const volatile s32 *)&x - 1u < 319u)) {
        volatile ActorData *actor = *current;
        actor->scriptInstructionPointer += 4u;
    } else {
        u16 destination = (u16)FieldScriptVMGetInstructionArgument(2);
        volatile ActorData *actor = *current;
        actor->scriptInstructionPointer = destination;
    }
    if (!skipBounds)
        *(volatile s32 *)&D_800B00C0 = 1;
}
