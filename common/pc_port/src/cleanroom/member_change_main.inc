void MemberChangeMenuMain(void)
{
    volatile SystemMenu *menu;

    func_801C5034(1);
    MemberChangeMenuSetManager(1);
    MemberChangeMenuSetSelectionMenu(1);
    func_801C5160(1);
    func_801C51C4(1);
    func_801C5228(1);
    MemberChangeMenuSetCharacters(1);

    menu = g_Menu;
    /* Each rectangle write resolves the selection-menu pointer again. */
    ((volatile MenuSelectionMenu *)menu->pSelectionMenu)->unk1180.x = 704;
    ((volatile MenuSelectionMenu *)menu->pSelectionMenu)->unk1180.y = 256;
    ((volatile MenuSelectionMenu *)menu->pSelectionMenu)->unk1180.w = 320;
    ((volatile MenuSelectionMenu *)menu->pSelectionMenu)->unk1180.h = 224;
    menu->unk348->unk15B = 64;

    MemberChangeMenuInitialize();
    MemberChangeMenuResetRenderContext();
    func_801C5B90();
    MemberChangeMenuInitializeBackground();
    MemberChangeMenuInitializeWindowBorders();

    g_Menu->shouldDrawMenu = 1;
    MemberChangeMenuMainLoop();
    MemberChangeMenuFree();
}
