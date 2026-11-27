void MemberChangeMenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height, u_char option, int zIndex, u_char hasScrollBar)
{
    SystemMenu *volatile *currentMenu = &g_Menu;
    volatile SystemMenu *menu = *currentMenu;
    MenuWindow *window = menu->windows[index];
    volatile MenuWindow *state = window;

    menu->pManager->shouldRenderWindow[index] = 0;
    MemberChangeMenuSetVertices(window->vertsBackground, x, y, width, height);
    MemberChangeMenuInitializeWindowBorderCorners(index, x, y, width, height);
    MemberChangeMenuSetWindowBorderTop(index, x, y, width);
    MemberChangeMenuSetWindowBorderBottom(index, x, y, width, height);
    MemberChangeMenuSetWindowBorderLeft(index, x, y, height);
    MemberChangeMenuSetWindowBorderRight(index, x, y, width, height);
    if (hasScrollBar) {
        MemberChangeMenuInitializeScrollBar(index, x, y, width, height);
    }

    state->hasScrollBar = hasScrollBar;
    menu = *currentMenu;
    state->unk714 = option;
    state->zIndex = zIndex;
    state->renderContext = *(const volatile u_char *)&menu->renderContext;
    (*currentMenu)->pManager->shouldRenderWindow[index] = 1;
}
