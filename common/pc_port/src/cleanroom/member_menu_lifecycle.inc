void MemberChangeMenuFreeCursors(void) {
    g_Menu->pManager->shouldRenderPointerCursors = 0;
    MemberChangeMenuUpdateAndRender();
    HeapFree(g_Menu->pCursors);
}

void MemberChangeMenuFree(void) {
    const MenuManager *manager = g_Menu->pManager;
    int partySize = 0;

    for (int slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        const u8 character = manager->currentCharacterIDs[slot];
        if (character != 255) {
            g_GameState.partyMembers[partySize++] = character;
        }
    }
    while (partySize < MAX_PARTY_MEMBERS) {
        g_GameState.partyMembers[partySize++] = 255;
    }

    MemberChangeMenuUpdateAndRender();
    MemberChangeMenuUpdateAndRender();
    g_Menu->shouldDrawMenu = 0;
    MemberChangeMenuUpdateAndRender();
    do {
        MemberChangeMenuUpdateAndRender();
    } while (g_Menu->renderContext != 0);

    func_801C5034(0);
    MemberChangeMenuSetManager(0);
    MemberChangeMenuSetSelectionMenu(0);
    func_801C5160(0);
    func_801C51C4(0);
    func_801C5228(0);

    HeapFree(g_Menu->unk2DC);
    HeapFree(g_Menu->unk2E0);
    HeapFree(g_Menu->unk4E0[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        func_8003A094((void *)g_Menu->unk2E4);
        func_8003852C((void *)g_Menu->unk2E4);
        HeapFree(g_Menu->unk2E4);
    }
    MemberChangeMenuSetCharacters(0);
    HeapFree(g_Menu);
}

void MemberChangeMenuRenderWindows(void) {
    for (int slot = 0; slot < MENU_MAX_NUM_WINDOWS; ++slot) {
        if (!g_Menu->pManager->shouldRenderWindow[slot]) {
            continue;
        }

        const MenuWindow *window = g_Menu->windows[slot];
        const int useDefaultTransform = window->unk714 == 0;
        MATRIX transform;
        if (useDefaultTransform) {
            SVECTOR rotation = {0};
            VECTOR translation = {0, 0, 512, 0};
            PushMatrix();
            RotMatrix(&rotation, &transform);
            TransMatrix(&transform, &translation);
            SetRotMatrix(&transform);
            SetTransMatrix(&transform);
        }

        MemberChangeMenuRenderWindowBorderCorners(slot);
        if (window->hasScrollBar) {
            MemberChangeMenuRenderScrollBar(slot);
        }
        MemberChangeMenuRenderTopWindowBorder(slot);
        MemberChangeMenuRenderBottomWindowBorder(slot);
        MemberChangeMenuRenderLeftWindowBorder(slot);
        MemberChangeMenuRenderRightWindowBorder(slot);
        MemberChangeMenuRenderWindowBackground(slot);
        if (useDefaultTransform) {
            PopMatrix();
        }
    }
}
