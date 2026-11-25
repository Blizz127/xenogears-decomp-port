/* The two columns share cursor rendering and maintain separate valid row counts. */
void MemberChangeMenuMainLoop(void)
{
    int column = 0;
    int row = 0;
    int savedColumn = -1;
    int savedRow = 0;
    int refresh = 1;

    func_801C9908();
    MemberChangeMenuInitializeWindow(2, 120, 6, 184, 200, 1, 0, 4, 0);
    MemberChangeMenuInitializeCursors(1);
    while (g_Menu->windowParameters[2]->unk11 == 0)
        MemberChangeMenuUpdateAndRender();

    for (;;) {
        MemberChangeMenuUpdateAndRender();
        if (refresh) {
            MemberChangeMenuUpdateCharacters(0);
            refresh = 0;
        }
        MemberChangeMenuSetCursorToCharacter((u_char)column, row, 0);
        unsigned input = g_Menu->input;
        switch (input) {
        case 0:
        case 2: {
            int next = input == 0;
            if (savedColumn < 0 && column != next) {
                MemberChangeMenuPlaySoundEffect(1);
                column = next;
                row = 0;
            }
            break;
        }
        case 1:
            MemberChangeMenuPlaySoundEffect(1);
            if (column == 0)
                row = (row + 1) % 3;
            else if (row < 5)
                ++row;
            break;
        case 3:
            if (column == 0 || row > 0) {
                MemberChangeMenuPlaySoundEffect(1);
                row = row > 0 ? row - 1 : 2;
            }
            break;
        case 4:
            if (savedColumn < 0) {
                MemberChangeMenuSetCursorToCharacter((u_char)column, row, 1);
                savedRow = row;
                savedColumn = column;
                column ^= 1;
                row = 0;
                MemberChangeMenuPlaySoundEffect(2);
            } else if ((u_char)MemberChangeMenuSwapCharacters((u_char)column, row, 0,
                                                              (u_char)savedColumn, savedRow, 0)) {
                savedColumn = -1;
                refresh = 1;
                g_Menu->pCursors->shouldRender[1] = 0;
                MemberChangeMenuPlaySoundEffect(2);
            } else {
                MemberChangeMenuPlaySoundEffect(4);
            }
            break;
        case 5:
            MemberChangeMenuPlaySoundEffect(3);
            if (savedColumn < 0)
                goto finished;
            savedColumn = -1;
            g_Menu->pCursors->shouldRender[1] = 0;
            break;
        }
    }

finished:
    MemberChangeMenuFreeCursors();
    MemberChangeMenuFreeWindow(2);
    for (int slot = 0; slot < 3; ++slot) {
        SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
        u8 id = ((volatile MenuManager *)menu->pManager)->currentCharacterIDs[slot];
        ((volatile GameState *)&g_GameState)->partyMembers[slot] = id;
    }
}
