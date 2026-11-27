void func_8009F4CC(void)
{
    extern int FieldIdleFacingRandom(void) __asm__("rand");
    ActorData* volatile* current = &g_FieldScriptVMCurActor;
    volatile ActorData* actor = *current;
    u16 count = (u16)actor->unk102;
    s16 facing = actor->rotation.vy;
    ++count;
    actor->unk102 = (s16)count;

    if ((count & 15u) == 0) {
        int choice = FieldIdleFacingRandom();
        actor = *current;
        if (choice & 0x30) {
            facing = (s16)((u16)actor->rotation.vy | 0x8000u);
            actor->rotation.vy = facing;
        } else {
            int turn = (choice & 1) ? -512 : 512;
            facing = (s16)((u32)(actor->rotation.vy + turn) & 0xfffu);
        }
    }

    actor = *current;
    u16 cursor = actor->scriptInstructionPointer;
    volatile int* stop = &D_800B00C0;
    *stop = 1;
    actor->rotation.vx = facing;
    actor->scriptInstructionPointer = cursor + 1u;
}
