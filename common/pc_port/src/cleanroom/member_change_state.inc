u_short MemberChangeMenuIsCharacterFlagSet(u_short value, u_char maskIndex)
{
    return value & D_801CB57C[maskIndex];
}

void MemberChangeMenuSetManager(u_char isInitialization)
{
    if (isInitialization) {
        MenuManager* manager = HeapAlloc(sizeof(*manager), 0);
        g_Menu->pManager = manager;
        bzero((u8*)manager, sizeof(*manager));
    } else {
        HeapFree(g_Menu->pManager);
    }
}

void MemberChangeMenuSetSelectionMenu(u_char isInitialization)
{
    if (isInitialization) {
        MenuSelectionMenu* selection = HeapAlloc(sizeof(*selection), 0);
        g_Menu->pSelectionMenu = selection;
        bzero((u8*)selection, sizeof(*selection));
    } else {
        HeapFree(g_Menu->pSelectionMenu);
    }
}

void func_801C5160(u_char isInitialization)
{
    if (isInitialization) {
        MenuUnk5* item = HeapAlloc(sizeof(*item), 0);
        g_Menu->unk354 = item;
        bzero((u8*)item, sizeof(*item));
    } else {
        HeapFree(g_Menu->unk354);
    }
}

void func_801C5228(u_char isInitialization)
{
    if (isInitialization) {
        MenuUnk1* item = HeapAlloc(sizeof(*item), 0);
        g_Menu->unk348 = item;
        bzero((u8*)item, sizeof(*item));
    } else {
        HeapFree(g_Menu->unk348);
    }
}

void MemberChangeMenuSetCharacters(u_char isInitialization)
{
    int i;
    if (isInitialization) {
        for (i = 0; i < MAX_BENCHED_PARTY_MEMBERS; ++i) {
            MenuCharacter* character = HeapAlloc(sizeof(*character), 0);
            g_Menu->benchedCharacters[i] = character;
            bzero((u8*)character, sizeof(*character));
        }
        for (i = 0; i < MAX_PARTY_MEMBERS; ++i) {
            MenuCharacter* character = HeapAlloc(sizeof(*character), 0);
            g_Menu->currentCharacters[i] = character;
            bzero((u8*)character, sizeof(*character));
        }
    } else {
        for (i = 0; i < MAX_BENCHED_PARTY_MEMBERS; ++i) {
            HeapFree(g_Menu->benchedCharacters[i]);
        }
        for (i = 0; i < MAX_PARTY_MEMBERS; ++i) {
            HeapFree(g_Menu->currentCharacters[i]);
        }
    }
}

void MemberChangeMenuResetRenderContext(void)
{
    g_Menu->renderContext = 0;
}

void func_801C5724(void)
{
    RECT destination = {0, 448, 16, 1};
    u16* colors = HeapAlloc(16 * sizeof(*colors), 0);
    bzero((u8*)colors, 16 * sizeof(*colors));
    colors[1] = 32767;
    LoadImage(&destination, (u_long*)colors);
    DrawSync(0);
    HeapFree(colors);
}
