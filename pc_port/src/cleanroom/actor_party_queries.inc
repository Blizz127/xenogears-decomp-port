int FieldActorGetScriptStatus(ActorData *actor, int scriptId)
{
    int slot;

    for (slot = 0; slot < ACTOR_MAX_NUM_SCRIPTS; ++slot) {
        if (actor->scripts[slot].scriptId == scriptId) {
            return ACTOR_SCRIPT_EXISTS;
        }
    }
    return 0;
}

int FieldCharacterIdToPartyId(int characterId)
{
    int slot;

    if (characterId == CHARACTER_ID_NONE) {
        return -1;
    }

    for (slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        int member = g_GamePartyMembers[slot];
        if (member == CHARACTER_ID_NONE) {
            break;
        }
        if (member == characterId) {
            return slot;
        }
    }
    return -1;
}
