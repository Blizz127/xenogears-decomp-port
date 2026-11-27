void KernelMenuInitializeCursor(int index)
{
    KernelMenuRenderEnvironment *page = &g_KernelMenuRenderEnvironments[index];
    DRAWENV *draw = &page->drawEnv;
    POLY_F3 *cursor = &page->cursor;

    draw->isbg = 1;
    draw->dtd = 1;
    draw->r0 = 0;
    draw->g0 = 0;
    draw->b0 = 32;
    SetPolyF3(cursor);
    cursor->r0 = 255;
    cursor->g0 = 255;
    cursor->b0 = 255;
}
