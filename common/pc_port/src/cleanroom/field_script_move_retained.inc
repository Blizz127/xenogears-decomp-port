void func_80098C3C(void)
{
    typedef u16 SlotHalfword __attribute__((__may_alias__));
    ActorData* actor = g_FieldScriptVMCurActor;

#ifdef _WIN32
    if (*(const volatile SlotHalfword*)((const u8*)&actor->scripts[actor->curScriptIndex] + 4) == 0xffffu) {
#else
    if (actor->scripts[actor->curScriptIndex].flags_0 == 0xffffu) {
#endif
        u16 value = (u16)FieldScriptVMGetArgument(11);
        actor = g_FieldScriptVMCurActor;
        ActorScriptSlot* slot = &actor->scripts[actor->curScriptIndex];

        /* Update only the low halfword following the slot metadata. */
        *(volatile SlotHalfword*)((u8*)slot + 4) = value;
    }
    func_80098CAC(1);
}
