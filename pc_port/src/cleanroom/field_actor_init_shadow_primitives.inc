void func_8007AA44(Quad* quad)
{
    POLY_FT4* first = &quad->polys[0];
    SetPolyFT4(first);

    volatile SVECTOR* vertices = quad->vertices;
    vertices[3].vx = -24;
    vertices[3].vz = -24;
    vertices[2].vz = -24;
    vertices[1].vx = -24;
    vertices[3].vy = 0;
    vertices[2].vx = 24;
    vertices[2].vy = 0;
    vertices[1].vy = 0;
    vertices[1].vz = 24;
    vertices[0].vx = 24;
    vertices[0].vy = 0;
    vertices[0].vz = 24;

    volatile POLY_FT4* packet = first;
    packet->r0 = 128;
    packet->g0 = 128;
    packet->b0 = 128;
    packet->tpage = GetTPage(0, 2, 640, 480);
    packet->clut = GetClut(256, 243);
    SetSemiTrans(first, 1);

    packet->v0 = 224;
    packet->v1 = 224;
    packet->u0 = 0;
    packet->u1 = 15;
    packet->u2 = 0;
    packet->v2 = 239;
    packet->u3 = 15;
    packet->v3 = 239;

    /* Read each group of packed words before writing it to the other buffer. */
    typedef u32 PacketWord __attribute__((__may_alias__));
    const volatile PacketWord* src = (const volatile PacketWord*)first;
    volatile PacketWord* dst = (volatile PacketWord*)&quad->polys[1];
    for (int i = 0; i < 8; i += 4) {
        u32 a = src[i];
        u32 b = src[i + 1];
        u32 c = src[i + 2];
        u32 d = src[i + 3];
        dst[i] = a;
        dst[i + 1] = b;
        dst[i + 2] = c;
        dst[i + 3] = d;
    }
    u32 a = src[8];
    u32 b = src[9];
    dst[8] = a;
    dst[9] = b;
}
