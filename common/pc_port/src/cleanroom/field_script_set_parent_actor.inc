extern unsigned int ParentActorIndexAt(int offset) __asm__("FieldScriptVMGetActorIndex");

void FieldScriptSetParentActor(void)
{
    unsigned int parent = ParentActorIndexAt(1);
    ActorData* volatile* current = &g_FieldScriptVMCurActor;

    if (parent != ACTOR_ID_INVALID)
        (*current)->parentActorId = (u8)parent;
    /* The parent byte store can change the VM's current actor pointer. */
    (*current)->scriptInstructionPointer += 2;
}
