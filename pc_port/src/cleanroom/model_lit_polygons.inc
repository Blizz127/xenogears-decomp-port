/* Native model material callbacks for the compact GPU packet ABI.
 * Contracts come from the Disc 1 packet differential and the independent
 * battle model packet dispatcher. The caller owns the output cursor and
 * supplies vertex and normal array bases as 32-bit pointer words.
 */
static SVECTOR* ModelIndexedVector(u32 base, s16 index)
{
    return (SVECTOR*)TEMP2_WORD_PTR(base + (u32)((s32)index * (s32)sizeof(SVECTOR)));
}

static void ModelPacketUVWord(u8* output, const u8* uv, u16 texture)
{
    u16 coords;
    __builtin_memcpy(&coords, uv, sizeof(coords));
    u32 word = coords | ((u32)texture << 16);
    __builtin_memcpy(output, &word, sizeof(word));
}

s32 func_8002D180(u8* material, s16* indices, s32 shade)
{
    POLY_G4* quad = (POLY_G4*)D_80059424;
    u32 normals = D_8005952C;
    (void)shade;
    setlen(quad, sizeof(*quad) / 4 - 1);
    NormalColorCol3(ModelIndexedVector(normals, indices[0]),
                    ModelIndexedVector(normals, indices[1]),
                    ModelIndexedVector(normals, indices[2]), (CVECTOR*)material,
                    (CVECTOR*)&quad->r0, (CVECTOR*)&quad->r1, (CVECTOR*)&quad->r2);
    NormalLightCol(ModelIndexedVector(D_8005952C, indices[3]), material, &quad->r3);
    quad->code = material[3];
    return 1;
}

s32 func_8002D244(u8* material, s16* indices)
{
    if (!func_8002CD64(material))
        return 0;

    POLY_GT4* quad = (POLY_GT4*)D_80059424;
    u32 normals = D_8005952C;
    setlen(quad, sizeof(*quad) / 4 - 1);
    NormalColor3(ModelIndexedVector(normals, indices[0]),
                 ModelIndexedVector(normals, indices[1]),
                 ModelIndexedVector(normals, indices[2]),
                 (CVECTOR*)&quad->r0, (CVECTOR*)&quad->r1, (CVECTOR*)&quad->r2);
    NormalColor(ModelIndexedVector(D_8005952C, indices[3]), (CVECTOR*)&quad->r3);
    ModelPacketUVWord(&quad->u0, material + 4, D_8005930C);
    ModelPacketUVWord(&quad->u1, material + 6, D_80059308);
    __builtin_memcpy(&quad->u2, material + 8, 2);
    __builtin_memcpy(&quad->u3, material + 10, 2);
    quad->code = material[3];
    return 1;
}

s32 func_8002D420(u8* material, s16* indices)
{
    return func_8002D244(material, indices);
}

s32 func_8002D814(u8* material, s16* indices, s32 flags)
{
    if (!func_8002CD64(material))
        return 0;

    POLY_FT3* triangle = (POLY_FT3*)D_80059424;
    setlen(triangle, sizeof(*triangle) / 4 - 1);
    ModelPacketUVWord(&triangle->u0, material + 4, D_8005930C);
    ModelPacketUVWord(&triangle->u1, material + 6, D_80059308);
    __builtin_memcpy(&triangle->u2, material, 2);

    SVECTOR local = {0};
    SVECTOR* normal = NULL;
    if (flags & 1) {
        u32 vertices = D_8005953C;
        normal = (flags & 2) ? TEMP2_WORD_PTR(D_80059498) : &local;
        func_8002DB84(ModelIndexedVector(vertices, indices[0]),
                      ModelIndexedVector(vertices, indices[1]),
                      ModelIndexedVector(vertices, indices[2]), normal);
        if (flags & 2)
            normal = TEMP2_WORD_PTR(D_80059498);
    } else if (flags & 4) {
        normal = TEMP2_WORD_PTR(D_80059498);
    }
    if (flags & (1 | 4))
        NormalColor(normal, (CVECTOR*)&triangle->r0);
    D_80059498 += sizeof(SVECTOR);
    triangle->code = material[3];
    return 1;
}
