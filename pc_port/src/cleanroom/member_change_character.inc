void MemberChangeMenuUpdateCharacter(MenuCharacter *pCharacter, u_char characterIndex, u_char localIndex, int *pPositionsX, int *pPositionsY, int verticalSpacing, u_char isPartyCharacter)
{
    volatile MenuCharacter *character = pCharacter;
    SystemMenu *volatile *currentMenu = &g_Menu;

    (void)isPartyCharacter;
    func_801C9A08(pCharacter, characterIndex, localIndex,
                  pPositionsX, pPositionsY, verticalSpacing);
    MemberChangeMenuSetCharacterLevelStrings(pCharacter, characterIndex, localIndex,
                                            pPositionsX, pPositionsY, verticalSpacing);
    MemberChangeMenuSetCharacterHpAndMpStrings(pCharacter, characterIndex, localIndex,
                                              pPositionsX, pPositionsY, verticalSpacing);

    character->unkBE9 = 0;
    character->unkBE7 = 1;
    character->renderContext = *(const volatile u_char *)&(*currentMenu)->renderContext;
}
