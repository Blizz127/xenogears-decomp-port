extern int FieldScriptVMGetArgument(int index);

void FieldScriptCheckGoldAmount(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned int ip = actor->scriptInstructionPointer;
    u32 price = (u32)script[ip + 2] << 8;
    price |= script[ip + 1];
    price |= (u32)script[ip + 3] << 16;
    price |= (u32)script[ip + 4] << 24;
    volatile GameState *game = *(GameState *volatile *)&g_pGameState;
    if (game->gold >= price) {
        actor->scriptInstructionPointer = (u16)(ip + 7);
    } else {
        int destination = FieldScriptVMGetInstructionArgument(5);
        actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
        actor->scriptInstructionPointer = (u16)destination;
    }
}

void FieldScriptIncreaseGold(void)
{
    u32 amount = (u32)FieldScriptVMGetArgument(1);
    volatile GameState *game = *(GameState *volatile *)&g_pGameState;
    u32 balance = game->gold + amount;
    if (balance > MAX_GOLD_AMOUNT && balance <= 0x7fffffffu) {
        balance = MAX_GOLD_AMOUNT;
    }
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    game->gold = balance;
    actor->scriptInstructionPointer += 3;
}
