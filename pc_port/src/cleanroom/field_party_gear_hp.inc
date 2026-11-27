static int fieldPartyGearForSlot(int slot)
{
    const volatile int *roster = g_GamePartyMembers;
    return GameCharacterGetGearID(roster[slot]);
}

int FieldPartyMemberIncreaseGearHp(int slot, unsigned int amount)
{
    int gearId = fieldPartyGearForSlot(slot);
    if (gearId == 255) {
        return 255;
    }

    volatile GameState *state = *(GameState *volatile *)&g_pGameState;
    volatile GameGear *gear = &state->gears[gearId];
    unsigned int current = gear->hp;
    unsigned int maximum = gear->maxHp;
    unsigned int result = current + amount;
    gear->hp = result;
    if (result > maximum) {
        gear->hp = maximum;
        return 1;
    }
    return 0;
}

int FieldPartyMemberDecreaseGearHp(int slot, unsigned int amount)
{
    int gearId = fieldPartyGearForSlot(slot);
    if (gearId == 255) {
        return 255;
    }

    volatile GameState *state = *(GameState *volatile *)&g_pGameState;
    volatile GameGear *gear = &state->gears[gearId];
    unsigned int result = gear->hp - amount;
    if (result == 0 || result > 0x7fffffffu) {
        result = 1;
    }
    gear->hp = result;
    return (int)result;
}
