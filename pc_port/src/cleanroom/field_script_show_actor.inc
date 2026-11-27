void FieldScriptVMHandlerShowActor(void)
{
    extern s32 D_800AFD1C;
    s32 index = *(const volatile s32 *)&D_800AFD1C;
    volatile FieldActor *record = &g_FieldActors[index];
    short status = record->status;
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;

    record->status = status & ~ACTOR_STATUS_INVISIBLE;
    unsigned short cursor = actor->scriptInstructionPointer;
    actor->curAnimationId = 0xff;
    actor->flags &= ~(1u << 25);
    actor->scriptInstructionPointer = (unsigned short)(cursor + 1);
}
