void MemberChangeMenuUpdateCharacters(u_char offset)
{
    volatile SystemMenu *menu;
    volatile MenuManager *manager;
    MenuCharacter *character;
    unsigned int slot;
    u_char id;

    for (slot = 0; slot < 3; ++slot) {
        menu = *(SystemMenu *volatile *)&g_Menu;
        manager = menu->pManager;
        id = manager->currentCharacterIDs[slot];
        character = menu->currentCharacters[slot];
        if (id == 255)
            ((volatile MenuCharacter *)character)->unkBE7 = 0;
        else
            MemberChangeMenuUpdateCharacter(character, id, slot, g_MemberChangeMenuCurCharPositionsX,
                                            g_MemberChangeMenuCurCharPositionsY, 56, 1);
    }
    menu = *(SystemMenu *volatile *)&g_Menu;
    manager = menu->pManager;
    manager->unk46 = 1;

    for (slot = 0; slot < 6 && offset + slot < 11; ++slot) {
        unsigned int index = offset + slot;

        menu = *(SystemMenu *volatile *)&g_Menu;
        character = menu->benchedCharacters[slot];
        id = menu->unk1E14[index];
        if (id == 255) {
            ((volatile MenuCharacter *)character)->unkBE7 = 0;
        } else {
            MemberChangeMenuUpdateAndRender();
            menu = *(SystemMenu *volatile *)&g_Menu;
            id = menu->unk1E14[index];
            MemberChangeMenuUpdateCharacter(character, id, slot,
                                            g_MemberChangeMenuBenchedCharPositionsX,
                                            g_MemberChangeMenuBenchedCharPositionsY, 32, 0);
        }
    }
}

void MemberChangeMenuSetCursorToCharacter(u_char isBenchedCharWindowActive, int characterIndex,
                                          u_char cursorIndex)
{
    int64_t position = (int64_t)characterIndex + (isBenchedCharWindowActive ? 3 : 0);
    volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    MenuPointerCursors *cursors = menu->pCursors;
    POLY_FT4 *cursor = &cursors->polysCursor[cursorIndex ? 2 : 0];
    void *font = menu->unk2DC;
    int page = menu->renderContext;
    int x = ((const volatile int *)g_MemberChangeMenuCurserPositionsX)[position];
    int y = ((const volatile int *)g_MemberChangeMenuCurserPositionsY)[position];
    volatile MenuPointerCursors *state;
    volatile MenuManager *manager;

    func_8002675C(font, MENU_TEX_POINTER_CURSOR, cursor, page, x, y, 2048);
    menu = *(SystemMenu *volatile *)&g_Menu;
    state = menu->pCursors;
    state->shouldRender[cursorIndex] = 1;
    menu = *(SystemMenu *volatile *)&g_Menu;
    state = menu->pCursors;
    state->renderContexts[cursorIndex] = (u_char)menu->renderContext;
    menu = *(SystemMenu *volatile *)&g_Menu;
    manager = menu->pManager;
    manager->shouldRenderPointerCursors = 1;
}
