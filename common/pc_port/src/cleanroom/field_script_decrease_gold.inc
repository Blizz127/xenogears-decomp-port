extern int FieldScriptVMGetArgument(int index);

void FieldScriptDecreaseGold(void)
{
    u32 charge = (u32)FieldScriptVMGetArgument(1);
    GameState* volatile* currentGame = &g_pGameState;
    volatile GameState* game = *currentGame;
    u32 remaining = game->gold - charge;

    ActorData* volatile* currentActor = &g_FieldScriptVMCurActor;
    volatile ActorData* actor = *currentActor;
    game->gold = (remaining & 0x80000000u) ? 0u : remaining;
    actor->scriptInstructionPointer += 3u;
}
