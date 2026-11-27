void MemberChangeMenuRender(void)
{
    if (g_Menu->shouldDrawMenu) {
        MemberChangeMenuUpdateWindows();
        MemberChangeMenuDrawCursors();
        func_801C846C();
        MemberChangeMenuRenderCharacters();
        MemberChangeMenuRenderWindows();
    }
    MemberChangeMenuRenderBackgroundDim();
}
