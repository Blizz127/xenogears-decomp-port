#ifndef g_PlayerActorIndex
extern s32 g_PlayerActorIndex;
#endif

int FieldGetPlayerActorDirection(void) {
    const FieldActor *record = &g_FieldActors[g_PlayerActorIndex];
    const ActorData *actor = (const ActorData *)(uintptr_t)record->pActorData;
    u32 yaw = (u16)actor->rotation.vy;
    u32 sector = (yaw + 256u) / 512u;
    return (int)((sector + 2u) % 8u);
}
