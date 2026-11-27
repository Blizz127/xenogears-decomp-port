extern uint8_t fieldActorStateByte __asm__("D_80050622");

static volatile FieldActorParameterHalf *fieldActorQueryGameWord(unsigned offset)
{
    GameState *state = *(GameState *volatile *)&g_pGameState;
    return (volatile FieldActorParameterHalf *)((unsigned char *)state + offset);
}

void func_8008752C(void)
{
    fieldActorParameterAdvance(3);
}

void func_8008754C(void)
{
    volatile FieldActorParameterHalf *flagWord = fieldActorQueryGameWord(__builtin_offsetof(GameState, unk22B6));
    uint16_t flags = *flagWord;
    ActorData *actor = fieldActorParameterCurrent();
    *flagWord = (uint16_t)(flags | 0x4000u);
    volatile FieldActorParameterHalf *cursor = fieldActorParameterCursor(actor);
    *cursor = (uint16_t)(*cursor + 1u);
}

void func_80087800(void)
{
    int destination = (uint16_t)fieldActorParameterRaw(1);
    unsigned value = *(volatile uint8_t *)&fieldActorStateByte;
    fieldActorParameterSet(destination, value);
    fieldActorParameterAdvance(3);
}

void func_800879D0(void)
{
    int destination = (uint16_t)fieldActorParameterRaw(1);
    unsigned first = *fieldActorQueryGameWord(__builtin_offsetof(GameState, unk184E));
    fieldActorParameterSet(destination, first);
    destination = (uint16_t)fieldActorParameterRaw(3);
    unsigned second = *fieldActorQueryGameWord(__builtin_offsetof(GameState, unk1852));
    fieldActorParameterSet(destination, second);
    fieldActorParameterAdvance(5);
}
