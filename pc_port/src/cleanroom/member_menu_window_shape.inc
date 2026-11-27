void MemberChangeMenuSetVertices(SVECTOR *vertices, u_short x, u_short y,
                                 u_short width, u_short height)
{
    int left = (int)x - 160;
    int top = (int)y - 112;
    for (unsigned int corner = 0; corner < 4; ++corner) {
        volatile SVECTOR *point = &vertices[corner];
        point->vx = (s16)(u16)(left + ((corner & 1) ? width : 0));
        point->vy = (s16)(u16)(top + ((corner & 2) ? height : 0));
        point->vz = 0;
    }
}

void MemberChangeMenuSetWindowBorderPrimitive(POLY_FT4 *primitive)
{
    SetSemiTrans(primitive, 1);
    SetShadeTex(primitive, 0);
    volatile POLY_FT4 *paint = primitive;
    paint->r0 = 128;
    paint->g0 = 128;
    paint->b0 = 128;
}
