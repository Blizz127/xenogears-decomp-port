#include <stddef.h>

typedef struct {
    size_t packets, page, pageX, pageY, clutX, clutY;
} MemberWindowBorder;

#define MEMBER_WINDOW_BORDER(side, n) { \
    __builtin_offsetof(MenuWindow, side), __builtin_offsetof(SystemMenu, texPage##n), \
    __builtin_offsetof(SystemMenu, texPageX##n), __builtin_offsetof(SystemMenu, texPageY##n), \
    __builtin_offsetof(SystemMenu, clutX##n), __builtin_offsetof(SystemMenu, clutY##n) }

static const MemberWindowBorder memberWindowBorders[] = {
    MEMBER_WINDOW_BORDER(polysWindowBorderTop, 0),
    MEMBER_WINDOW_BORDER(polysWindowBorderBottom, 1),
    MEMBER_WINDOW_BORDER(polysWindowBorderLeft, 2),
    MEMBER_WINDOW_BORDER(polysWindowBorderRight, 3)
};
#undef MEMBER_WINDOW_BORDER

static int memberWindowSetting(const SystemMenu *menu, size_t offset)
{
    return *(const s32 *)((const u8 *)menu + offset);
}

void MemberChangeMenuInitializeWindowGraphics(u_char index)
{
    MenuWindow *window = g_Menu->windows[index];
    g_Menu->pManager->shouldRenderWindow[index] = 0;
    g_Menu->pManager->unk27[index] = 0;
    RECT texture = {0, 0, 256, 256};
    for (unsigned n = 0; n < 2; ++n) {
        POLY_G4 *bg = &window->polysBackground[n];
        SetPolyG4(bg);
        volatile POLY_G4 *colors = bg;
        setRGB0(colors, 104, 104, 104);
        setRGB1(colors, 104, 104, 104);
        setRGB2(colors, 104, 104, 104);
        setRGB3(colors, 104, 104, 104);
        SetSemiTrans(bg, 1);
        int page = (u_short)GetTPage(0, 0, g_Menu->texPageX0, g_Menu->texPageY0);
        SetDrawMode(&window->drawModes[n], 0, 0, page, &texture);
    }

    for (unsigned indexInBorder = 0; indexInBorder < 4; ++indexInBorder) {
        for (unsigned side = 0; side < 4; ++side) {
            const MemberWindowBorder *border = &memberWindowBorders[side];
            POLY_FT4 *packets = (POLY_FT4 *)((u8 *)window + border->packets);
            POLY_FT4 *packet = &packets[indexInBorder];
            SetPolyFT4(packet);
            SetShadeTex(packet, 1);
            volatile POLY_FT4 *color = packet;
            setRGB0(color, 255, 255, 255);
            SystemMenu *menu = g_Menu;
            packet->tpage = GetTPage(memberWindowSetting(menu, border->page), 0,
                                     memberWindowSetting(menu, border->pageX),
                                     memberWindowSetting(menu, border->pageY));
            menu = g_Menu;
            packet->clut = GetClut(memberWindowSetting(menu, border->clutX),
                                   memberWindowSetting(menu, border->clutY));
        }
    }
}
