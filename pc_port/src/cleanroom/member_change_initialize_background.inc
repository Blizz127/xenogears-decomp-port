/* SPDX-License-Identifier: MIT */
static MenuUnk1 *memberChangeCurrentBackground(void)
{
    SystemMenu *volatile *active_menu = &g_Menu;
    MenuUnk1 *volatile *background = &(*active_menu)->unk348;
    return *background;
}

#define MEMBER_CHANGE_BG_FIELDS ((volatile MenuUnk1 *)memberChangeCurrentBackground())

void MemberChangeMenuInitializeBackground(void)
{
    RECT textureWindow = {0, 0, 256, 256};

    func_801C5CF4();
    for (int bank = 0; bank < 2; ++bank) {
        func_801C5D24(&memberChangeCurrentBackground()->polyG4s[bank], 128, 128, 0);
        SetSemiTrans(&memberChangeCurrentBackground()->polyG4s[bank], 1);

        SetLineF3(&memberChangeCurrentBackground()->lines1[bank]);
        MEMBER_CHANGE_BG_FIELDS->lines1[bank].r0 = 0;
        MEMBER_CHANGE_BG_FIELDS->lines1[bank].g0 = 64;
        MEMBER_CHANGE_BG_FIELDS->lines1[bank].b0 = 0;
        SetLineF3(&memberChangeCurrentBackground()->lines2[bank]);
        MEMBER_CHANGE_BG_FIELDS->lines2[bank].r0 = 0;
        MEMBER_CHANGE_BG_FIELDS->lines2[bank].g0 = 64;
        MEMBER_CHANGE_BG_FIELDS->lines2[bank].b0 = 0;

        SetPolyF4(&memberChangeCurrentBackground()->polysDimEffect[bank]);
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].x0 = 0;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].y0 = 0;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].x1 = 320;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].y1 = 0;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].x2 = 0;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].y2 = 224;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].x3 = 320;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].y3 = 224;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].r0 = 128;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].g0 = 128;
        MEMBER_CHANGE_BG_FIELDS->polysDimEffect[bank].b0 = 128;
        SetSemiTrans(&memberChangeCurrentBackground()->polysDimEffect[bank], 1);

        const u16 firstPage = (u16)GetTPage(0, 0, 320, 128);
        SetDrawMode(&memberChangeCurrentBackground()->drModes1[bank], 0, 0,
                    firstPage, &textureWindow);
        const u16 dimPage = (u16)GetTPage(0, 2, 384, 0);
        SetDrawMode(&memberChangeCurrentBackground()->drawModeDimEffect[bank], 0, 0,
                    dimPage, &textureWindow);
    }
}

#undef MEMBER_CHANGE_BG_FIELDS
