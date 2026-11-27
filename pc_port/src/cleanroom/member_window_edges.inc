static void MemberChangeDrawWindowEdge(MenuWindow* window, POLY_FT4* packets,
                                       SVECTOR* first, SVECTOR* second)
{
    SVECTOR* halves[2] = {first, second};
    long depthCue;
    long flags;

    for (int half = 0; half < 2; ++half) {
        SVECTOR* vertices = halves[half];
        POLY_FT4* pages = packets + 2 * half;
        POLY_FT4* projected = &pages[window->renderContext];

        RotTransPers4(&vertices[0], &vertices[1], &vertices[2], &vertices[3],
                      (long*)&projected->x0, (long*)&projected->x1,
                      (long*)&projected->x2, (long*)&projected->x3,
                      &depthCue, &flags);
        AddPrim(&g_Menu->pGfxEnv->ot[window->zIndex], &pages[window->renderContext]);
    }
}

void MemberChangeMenuRenderTopWindowBorder(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    MemberChangeDrawWindowEdge(window, window->polysWindowBorderTop,
                               window->vertsWindowBorderTop1, window->vertsWindowBorderTop2);
}

void MemberChangeMenuRenderBottomWindowBorder(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    MemberChangeDrawWindowEdge(window, window->polysWindowBorderBottom,
                               window->vertsWindowBorderBottom1, window->vertsWindowBorderBottom2);
}

void MemberChangeMenuRenderLeftWindowBorder(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    MemberChangeDrawWindowEdge(window, window->polysWindowBorderLeft,
                               window->vertsWindowBorderLeft1, window->vertsWindowBorderLeft2);
}

void MemberChangeMenuRenderRightWindowBorder(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    MemberChangeDrawWindowEdge(window, window->polysWindowBorderRight,
                               window->vertsWindowBorderRight1, window->vertsWindowBorderRight2);
}
