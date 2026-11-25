static void memberChangeSubmitCharacterPacket(MenuCharacter *character, POLY_FT4 *bank,
                                               unsigned int index)
{
    unsigned int page = ((volatile MenuCharacter *)character)->renderContext;
    volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    GfxEnvironment *environment = menu->pGfxEnv;
    uintptr_t address = (uintptr_t)bank + (2u * index + page) * sizeof(*bank);

    AddPrim(&environment->ot[4], (void *)address);
}

static void memberChangeSubmitCharacterBank(MenuCharacter *character, POLY_FT4 *bank,
                                             const volatile u_char *length)
{
    unsigned int index;

    for (index = 0; index < *length; ++index)
        memberChangeSubmitCharacterPacket(character, bank, index);
}

void MemberChangeMenuRenderCharacter(MenuCharacter *character, u_char flag)
{
    volatile MenuCharacter *state = character;

    if (!state->unkBE7)
        return;

    memberChangeSubmitCharacterPacket(character, character->polysPortraitSmall, 0);
    memberChangeSubmitCharacterPacket(character, character->polys4B0, 0);
    memberChangeSubmitCharacterBank(character, character->polysDescriptionStrings,
                                    &state->descriptionStringsLength);
    memberChangeSubmitCharacterBank(character, character->polysLevelString,
                                    &state->levelStringLength);
    memberChangeSubmitCharacterBank(character, character->polysHpString,
                                    &state->hpStringLength);
    memberChangeSubmitCharacterBank(character, character->polysMaxHpString,
                                    &state->maxHpStringLength);
    memberChangeSubmitCharacterBank(character, character->polysMpString,
                                    &state->mpStringLength);
    memberChangeSubmitCharacterBank(character, character->polysMaxMpString,
                                    &state->maxMpStringLength);
    if (flag)
        memberChangeSubmitCharacterBank(character, character->polys2D0, &state->unkBE9);
}

void MemberChangeMenuRenderCharacters(void)
{
    volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    volatile MenuManager *manager = menu->pManager;
    unsigned int slot;

    if (!manager->unk46)
        return;

    for (slot = 0; slot < 6; ++slot) {
        menu = *(SystemMenu *volatile *)&g_Menu;
        MemberChangeMenuRenderCharacter(menu->benchedCharacters[slot], 0);
    }
    for (slot = 0; slot < 3; ++slot) {
        menu = *(SystemMenu *volatile *)&g_Menu;
        MemberChangeMenuRenderCharacter(menu->currentCharacters[slot], 1);
    }
}
