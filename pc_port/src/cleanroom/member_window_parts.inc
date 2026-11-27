static void MemberChangeDrawTexturedWindowPart(MenuWindow* window, SVECTOR* vertices,
                                              POLY_FT4* pages, long* depthCue, long* flags)
{
    POLY_FT4* projected = &pages[window->renderContext];
    RotTransPers4(&vertices[0], &vertices[1], &vertices[2], &vertices[3],
                  (long*)&projected->x0, (long*)&projected->x1,
                  (long*)&projected->x2, (long*)&projected->x3, depthCue, flags);
    AddPrim(&g_Menu->pGfxEnv->ot[window->zIndex], &pages[window->renderContext]);
}

void MemberChangeMenuRenderWindowBackground(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    SVECTOR* vertices = window->vertsBackground;
    POLY_G4* projected = &window->polysBackground[window->renderContext];
    long depthCue;
    long flags;

    RotTransPers4(&vertices[0], &vertices[1], &vertices[2], &vertices[3],
                  (long*)&projected->x0, (long*)&projected->x1,
                  (long*)&projected->x2, (long*)&projected->x3, &depthCue, &flags);
    AddPrim(&g_Menu->pGfxEnv->ot[window->zIndex],
            &window->polysBackground[window->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[window->zIndex], &window->drawModes[window->renderContext]);
}

void MemberChangeMenuRenderWindowBorderCorners(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    long depthCue;
    long flags;

    for (int corner = 0; corner < 4; ++corner) {
        MemberChangeDrawTexturedWindowPart(window, &window->vertsWindowBorderCorners[4 * corner],
                                          &window->polysWindowBorderCorners[2 * corner],
                                          &depthCue, &flags);
    }
}

void MemberChangeMenuRenderScrollBar(int slot)
{
    MenuWindow* window = g_Menu->windows[slot];
    long depthCue;
    long flags;

    MemberChangeDrawTexturedWindowPart(window, &window->vertsScrollBarEnds[0],
                                      &window->polysScrollBarEnds[0], &depthCue, &flags);
    MemberChangeDrawTexturedWindowPart(window, &window->vertsScrollBarEnds[4],
                                      &window->polysScrollBarEnds[2], &depthCue, &flags);
    MemberChangeDrawTexturedWindowPart(window, window->vertsScrollBarEmpty,
                                      window->polysScrollBarEmpty, &depthCue, &flags);
}
