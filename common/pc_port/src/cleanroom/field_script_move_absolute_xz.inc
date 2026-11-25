void func_80099980(void)
{
    typedef u32 FlagWord __attribute__((__may_alias__));
    typedef u16 FlagHalfword __attribute__((__may_alias__));
    volatile ActorData* actor = g_FieldScriptVMCurActor;
    volatile ActorScriptSlot* slot = &actor->scripts[actor->curScriptIndex];

    /* The flags follow four metadata bytes. Preserve both store widths and
     * reselect the slot between the movement-mode and distance updates. */
    *(volatile FlagWord*)((volatile u8*)slot + 4) &= ~(3u << 23);
    slot = &actor->scripts[actor->curScriptIndex];
    *(volatile FlagHalfword*)((volatile u8*)slot + 4) = 0xffff;

    if (func_80099AC0(65535) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    }
}
