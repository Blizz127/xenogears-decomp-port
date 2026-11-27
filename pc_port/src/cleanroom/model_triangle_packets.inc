/* GPU packet formats use an eight-byte material and three signed normal indices.
 * The model control record can consume the material without emitting a triangle. */
s32 func_8002DA14(u8 *material, s16 *normalIndices)
{
    if (!func_8002CD64(material))
        return 0;

    POLY_GT3 *packet = (POLY_GT3 *)D_80059424;
    SVECTOR *normals = TEMP2_WORD_PTR(D_8005952C);
    setlen(packet, 9);
    NormalColor3(&normals[normalIndices[0]], &normals[normalIndices[1]],
                 &normals[normalIndices[2]], (CVECTOR *)&packet->r0,
                 (CVECTOR *)&packet->r1, (CVECTOR *)&packet->r2);
    packet->u0 = material[4];
    packet->v0 = material[5];
    packet->clut = D_8005930C;
    packet->u1 = material[6];
    packet->v1 = material[7];
    packet->tpage = D_80059308;
    packet->u2 = material[0];
    packet->v2 = material[1];
    packet->code = material[3];
    return 1;
}

s32 func_8002DAFC(void)
{
    POLY_FT3 *packet = (POLY_FT3 *)D_80059424;
    SetPolyFT3(packet);
    SetShadeTex(packet, 1);
    packet->tpage = D_80059310 | (GetTPage(1, 0, 640, 0) & ~31u);
    packet->clut = D_80059314 | (GetClut(0, 480) & 15u);
    return 1;
}
