#include <stdint.h>

static POLY_FT4* MemberChangeCornerPacket(MenuWindow* window, uint32_t index)
{
    uint32_t bytes = index * (uint32_t)sizeof(POLY_FT4);
    return (POLY_FT4*)((uintptr_t)&window->polysWindowBorderCorners[0] +
                       (uintptr_t)(intptr_t)(int32_t)bytes);
}

void MemberChangeMenuInitializeWindowBorderCorners(unsigned char slot, u_short x, u_short y,
                                                  u_short width, u_short height)
{
    static const u_short glyphs[4] = {0xfd, 0xff, 0x102, 0x104};
    SystemMenu* menu = g_Menu;
    MenuWindow* window = menu->windows[slot];
    volatile s32* parts = &window->unk710;
    uint32_t previous = 0;
    *parts = 0;

    for (int i = 0; i < 4; ++i) {
        s32 added = func_8002675C(menu->unk2DC, glyphs[i],
                                 MemberChangeCornerPacket(window, previous * 2u),
                                 menu->renderContext, 0, 0, 4096);
        if (i + 1 < 4)
            menu = *(SystemMenu* volatile*)&g_Menu;
        previous = (uint32_t)*parts + (uint32_t)added;
        *parts = (s32)previous;
    }

    u_short left = (u_short)(x - 8);
    u_short right = (u_short)(x + width + 8);
    u_short top = (u_short)(y + 8);
    u_short bottom = (u_short)(y + height - 8);
    SVECTOR* vertices = window->vertsWindowBorderCorners;
    MemberChangeMenuSetVertices(&vertices[0], left, top, 16, (u_short)-16);
    MemberChangeMenuSetVertices(&vertices[4], right, top, (u_short)-16, (u_short)-16);
    MemberChangeMenuSetVertices(&vertices[8], left, bottom, 16, 16);
    MemberChangeMenuSetVertices(&vertices[12], right, bottom, (u_short)-16, 16);

    for (unsigned corner = 0; corner < 4; ++corner) {
        unsigned page = (unsigned)g_Menu->renderContext;
        MemberChangeMenuSetWindowBorderPrimitive(MemberChangeCornerPacket(window, 2u * corner + page));
    }
}
