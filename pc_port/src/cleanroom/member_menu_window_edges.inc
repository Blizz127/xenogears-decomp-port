void MemberChangeMenuSetVertices(SVECTOR *, u_short, u_short, u_short, u_short);
void MemberChangeMenuSetWindowBorderPrimitive(POLY_FT4 *);

static volatile POLY_FT4 *MemberWindowEdgeFace(POLY_FT4 *pages, int offset)
{
    SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    return pages + ((volatile SystemMenu *)menu)->renderContext + offset;
}

static void MemberWindowPlaceEdge(POLY_FT4 *pages, SVECTOR *first, SVECTOR *second,
                                  int x, int y, int span, int horizontal,
                                  unsigned char u, unsigned char v)
{
    unsigned char right = u + (horizontal ? 7 : 16);
    unsigned char bottom = v + (horizontal ? 16 : 7);
    for (int half = 0; half < 2; ++half) {
        MemberWindowEdgeFace(pages, half * 2)->u0 = u;
        MemberWindowEdgeFace(pages, half * 2)->v0 = v;
        MemberWindowEdgeFace(pages, half * 2)->u1 = right;
        MemberWindowEdgeFace(pages, half * 2)->v1 = v;
        MemberWindowEdgeFace(pages, half * 2)->u2 = u;
        MemberWindowEdgeFace(pages, half * 2)->v2 = bottom;
        MemberWindowEdgeFace(pages, half * 2)->u3 = right;
        MemberWindowEdgeFace(pages, half * 2)->v3 = bottom;
    }

    int extent = (span - 16) / 2;
    u_short width = (u_short)(horizontal ? extent : 16);
    u_short height = (u_short)(horizontal ? 16 : extent);
    MemberChangeMenuSetVertices(first, (u_short)x, (u_short)y, width, height);
    if (horizontal)
        x += extent;
    else
        y += extent;
    MemberChangeMenuSetVertices(second, (u_short)x, (u_short)y, width, height);
    MemberChangeMenuSetWindowBorderPrimitive((POLY_FT4 *)MemberWindowEdgeFace(pages, 0));
    MemberChangeMenuSetWindowBorderPrimitive((POLY_FT4 *)MemberWindowEdgeFace(pages, 2));
}

void MemberChangeMenuSetWindowBorderTop(u_char index, u_short x, u_short y, u_short width)
{
    MenuWindow *window = g_Menu->windows[index];
    MemberWindowPlaceEdge(window->polysWindowBorderTop, window->vertsWindowBorderTop1,
                          window->vertsWindowBorderTop2, x + 8, y - 8, width, 1, 0, 132);
}

void MemberChangeMenuSetWindowBorderBottom(u_char index, u_short x, u_short y,
                                         u_short width, u_short height)
{
    MenuWindow *window = g_Menu->windows[index];
    MemberWindowPlaceEdge(window->polysWindowBorderBottom, window->vertsWindowBorderBottom1,
                          window->vertsWindowBorderBottom2, x + 8, y + height - 8, width, 1, 8, 132);
}

void MemberChangeMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height)
{
    MenuWindow *window = g_Menu->windows[index];
    MemberWindowPlaceEdge(window->polysWindowBorderLeft, window->vertsWindowBorderLeft1,
                          window->vertsWindowBorderLeft2, x - 8, y + 8, height, 0, 16, 132);
}

void MemberChangeMenuSetWindowBorderRight(u_char index, u_short x, u_short y,
                                        u_short width, u_short height)
{
    MenuWindow *window = g_Menu->windows[index];
    MemberWindowPlaceEdge(window->polysWindowBorderRight, window->vertsWindowBorderRight1,
                          window->vertsWindowBorderRight2, x + width - 8, y + 8, height, 0, 16, 140);
}
