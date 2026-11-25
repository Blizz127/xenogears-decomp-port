void MemberChangeMenuInitializeScrollBar(unsigned char slot, u_short x, u_short y,
                                        u_short width, u_short height)
{
    SystemMenu* menu = g_Menu;
    MenuWindow* window = menu->windows[slot];
    int bottom = (int)y + height;
    int center = (int)y + 8;
    (void)width;

    func_8002675C(menu->unk2DC, 261, &window->polysScrollBarEnds[0],
                  menu->renderContext, x, y, 4096);
    menu = g_Menu;
    func_800263E4(menu->unk2DC, 261, &window->polysScrollBarEnds[2],
                  menu->renderContext, x, bottom - 8, 4096, 0, 1);
    menu = g_Menu;
    func_8002675C(menu->unk2DC, 262, &window->polysScrollBarEmpty[0],
                  menu->renderContext, x, center, 4096);

    MemberChangeMenuSetVertices(&window->vertsScrollBarEnds[0], x, y, 8, 8);
    MemberChangeMenuSetVertices(&window->vertsScrollBarEnds[4], x, (u_short)bottom, 8,
                                (u_short)-8);
    MemberChangeMenuSetVertices(window->vertsScrollBarEmpty, x, (u_short)center, 8,
                                (u_short)(height - 8));
}
