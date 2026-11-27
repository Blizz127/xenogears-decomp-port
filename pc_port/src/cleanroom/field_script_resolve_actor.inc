u32 func_8009CD7C(int bytecodeOffset)
{
    u32 actorIndex = FieldScriptVMGetActorIndex(bytecodeOffset);
    if (actorIndex == 0xffu) {
        const volatile s32 *firstPartyActor = &D_8005A444;
        return (u32)*firstPartyActor;
    }
    return actorIndex;
}
