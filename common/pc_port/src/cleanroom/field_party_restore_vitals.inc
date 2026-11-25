static void fieldPartyRestoreStat(int useMp)
{
    GameState *state = *(GameState *volatile *)&g_pGameState;
    for (int id = 0; id < MAX_GAME_CHARACTERS; id++) {
        volatile GameCharacter *character = &state->characters[id];
        if (useMp) {
            character->mp = character->maxMp;
        } else {
            character->hp = character->maxHp;
        }
    }
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerRestoreHp(void)
{
    fieldPartyRestoreStat(0);
}

void FieldScriptVMHandlerRestoreMp(void)
{
    fieldPartyRestoreStat(1);
}

void FieldScriptVMHandlerRestoreCharacterHpAndMp(void)
{
    int id = FieldScriptVMGetArgument(1);
    GameState *state = *(GameState *volatile *)&g_pGameState;
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    volatile GameCharacter *character = &state->characters[id];
    u16 hp = character->maxHp;
    u16 mp = character->maxMp;
    character->hp = hp;
    character->mp = mp;
    actor->scriptInstructionPointer += 3;
}
