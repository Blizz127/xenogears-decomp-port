void func_800A19B0(void)
{
    typedef u32 FlagWord __attribute__((__may_alias__));
    typedef u16 FlagHalfword __attribute__((__may_alias__));
    volatile ActorData* actor = g_FieldScriptVMCurActor;

    for (int i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        volatile ActorScriptSlot* slot = &actor->scripts[i];
        volatile FlagWord* flags = (volatile FlagWord*)((volatile u8*)slot + 4);

        slot->waitTimer = 0;
        slot->scriptId = 0xff;
        slot->currentIP = 0xffff;
        *flags = (*flags & ~(3u << 16)) | (15u << 18);
        *flags &= ~(1u << 22);
        *(volatile FlagHalfword*)flags = 0xffff;
        *flags &= ~(3u << 23);
    }

    actor->curScriptIndex = 0;
    actor->unkCF = 0;
    *(volatile int*)&D_800B00C0 = 1;
    actor->dialogFlags = 0;
    actor->flags12C_0x6 = 0;
}
