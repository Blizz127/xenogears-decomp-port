void func_801C9908(void)
{
    u8 *roster = g_Menu->unk1E14;
    unsigned count = 0;

    for (unsigned id = 0; id < MAX_GAME_CHARACTERS; ++id) {
        if (!g_Menu->availableCharacters[id]) {
            continue;
        }
        const u8 *party = g_Menu->pManager->currentCharacterIDs;
        if (id != party[0] && id != party[1] && id != party[2]) {
            roster[count++] = id;
        }
    }
    while (count < MAX_GAME_CHARACTERS) {
        roster[count++] = 0xff;
    }

    for (s32 slot = 0; slot <= 10; slot += 2) {
        func_801C95A0(slot, slot);
    }
}

void MemberChangeMenuInitializeWindow(u_char slot, u_short x, u_short y,
                                      u_short width, u_short height, u_char animate,
                                      u8 option, int depth, u_char scroll)
{
    if (slot >= 2) {
        MenuWindow *window = HeapAlloc(sizeof *window, 0);
        g_Menu->windows[slot] = window;
        bzero((void *)window, sizeof *window);

        MenuWindowParameters *parameters = HeapAlloc(sizeof *parameters, 0);
        g_Menu->windowParameters[slot] = parameters;
        bzero((void *)parameters, sizeof *parameters);
        MemberChangeMenuInitializeWindowGraphics(slot);
    }

    if (animate) {
        MenuWindowParameters *parameters = g_Menu->windowParameters[slot];
        parameters->index = slot;
        parameters->unk11 = 0;
        parameters->x = x;
        parameters->y = y;
        parameters->width = width;
        parameters->height = height;
        parameters->unk8 = 0;
        parameters->unkA = 0;
        g_Menu->pManager->unk27[slot] = 1;
        parameters->unk12 = option;
        parameters->zIndex = depth;
    } else {
        MemberChangeMenuSetWindow(slot, x, y, width, height, option, depth, scroll);
    }
}

void func_801C7EC8(void)
{
    long projection;
    long flags;

    for (unsigned slot = 0; slot < 6; ++slot) {
        if (!g_Menu->pManager->unk14[slot]) {
            continue;
        }

        MenuString *entry = &g_Menu->unkAE0[slot];
        if (entry->unk7F) {
            POLY_FT4 *packet = &entry->polys[entry->renderContext];
            RotTransPers4(&entry->vertices[0], &entry->vertices[1],
                          &entry->vertices[2], &entry->vertices[3],
                          (long *)(void *)&packet->x0, (long *)(void *)&packet->x1,
                          (long *)(void *)&packet->x2, (long *)(void *)&packet->x3,
                          &projection, &flags);
            entry = &g_Menu->unkAE0[slot];
        }
        AddPrim(&g_Menu->pGfxEnv->ot[4], &entry->polys[entry->renderContext]);
    }
}
