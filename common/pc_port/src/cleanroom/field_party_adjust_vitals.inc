static volatile u16 *fieldPartyCurrentVital(volatile GameCharacter *character, int useMp)
{
    return useMp ? &character->mp : &character->hp;
}

static u16 fieldPartyMaximumVital(volatile GameCharacter *character, int useMp)
{
    return useMp ? character->maxMp : character->maxHp;
}

static void fieldPartyRestoreAmount(int slot, int amount, int useMp)
{
    const volatile int *roster = g_GamePartyMembers;
    int first = roster[slot];
    GameState *state = *(GameState *volatile *)&g_pGameState;
    volatile u16 *quantity = fieldPartyCurrentVital(&state->characters[first], useMp);
    *quantity = (u16)((u32)*quantity + (u32)amount);

    int next = roster[slot];
    volatile GameCharacter *member = &state->characters[next];
    u16 limit = fieldPartyMaximumVital(member, useMp);
    quantity = fieldPartyCurrentVital(member, useMp);
    if (*quantity > limit) {
        *quantity = limit;
    }
}

static void fieldPartySpendAmount(int slot, int amount, int useMp)
{
    const volatile int *roster = g_GamePartyMembers;
    int member = roster[slot];
    GameState *state = *(GameState *volatile *)&g_pGameState;
    volatile u16 *quantity = fieldPartyCurrentVital(&state->characters[member], useMp);
    u32 remaining = (u32)*quantity - (u32)amount;
    *quantity = remaining > 0 && remaining <= 0x7fffffffu ? (u16)remaining : 1;
}

void FieldPartyMemberIncreaseHp(int slot, int amount)
{
    fieldPartyRestoreAmount(slot, amount, 0);
}

void FieldPartyMemberDecreaseHp(int slot, int amount)
{
    fieldPartySpendAmount(slot, amount, 0);
}

void FieldPartyMemberIncreaseMp(int slot, int amount)
{
    fieldPartyRestoreAmount(slot, amount, 1);
}

void FieldPartyMemberDecreaseMp(int slot, int amount)
{
    fieldPartySpendAmount(slot, amount, 1);
}
