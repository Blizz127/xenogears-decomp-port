void MemberChangeMenuInitializeWindowBorders(void)
{
    SystemMenu* menu = g_Menu;
    func_80026338(menu->unk2DC, 0xfe, &menu->unk46C, &menu->texPage0,
                  &menu->clutX0, &menu->clutY0, &menu->texPageX0, &menu->texPageY0);

    menu = g_Menu;
    func_80026338(menu->unk2DC, 0x103, &menu->unk484, &menu->texPage1,
                  &menu->clutX1, &menu->clutY1, &menu->texPageX1, &menu->texPageY1);

    menu = g_Menu;
    func_80026338(menu->unk2DC, 0x100, &menu->unk49C, &menu->texPage2,
                  &menu->clutX2, &menu->clutY2, &menu->texPageX2, &menu->texPageY2);

    menu = g_Menu;
    func_80026338(menu->unk2DC, 0x101, &menu->unk4B4, &menu->texPage3,
                  &menu->clutX3, &menu->clutY3, &menu->texPageX3, &menu->texPageY3);
}
