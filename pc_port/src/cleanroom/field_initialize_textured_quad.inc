void func_8007A7F4(Quad* quad, int horizontal, int vertical, int texture)
{
    POLY_FT4* first = &quad->polys[0];
    const SVec4* x = &D_800ADCE0[horizontal];
    const SVec4* z = &D_800ADD28[vertical];
    const SVec4* u = &D_800ADD70[horizontal];
    const SVec4* v = &D_800ADDB8[vertical];
    /* Each label names one halfword of the first six-halfword preset. */
    int preset = 6 * texture;

    SetPolyFT4(first);
    const short vx[4] = { x->x, x->y, x->z, x->w };
    const short vz[4] = { z->x, z->y, z->z, z->w };
    for (int i = 0; i < 4; ++i) {
        quad->vertices[i].vx = vx[i];
        quad->vertices[i].vy = 0;
        quad->vertices[i].vz = vz[i];
    }
    first->r0 = first->g0 = first->b0 = 128;
    first->tpage = GetTPage(D_800ADE00[preset], D_800ADE02[preset],
                            D_800ADE04[preset], D_800ADE06[preset]);
    first->clut = GetClut(D_800ADE08[preset], D_800ADE0A[preset]);
    FieldClampPolyFT4UVs(first, u->x, v->x, u->y, v->y,
                         u->z, v->z, u->w, v->w);
    quad->polys[1] = *first;
}
