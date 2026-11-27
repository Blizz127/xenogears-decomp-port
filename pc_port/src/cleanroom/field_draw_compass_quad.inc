void func_8007AC58(u_long *orderingTable, Quad *quad, MATRIX *transform, int bank) {
    intptr_t offset = (s32)((u32)bank * (u32)sizeof(POLY_FT4));
    POLY_FT4 *sprite = (POLY_FT4 *)((uintptr_t)quad->polys + offset);
    long points[4] = {0}, interpolation = 0, flags = 0;
    PushMatrix();
    SetRotMatrix(transform);
    SetTransMatrix(transform);
    RotAverage4(&quad->vertices[0], &quad->vertices[1], &quad->vertices[2],
                &quad->vertices[3], &points[0], &points[1], &points[2], &points[3],
                &interpolation, &flags);
    int x = ((s16)(u16)points[2] + (s16)(u16)points[3]) / 2;
    s16 bottom = (s16)(u16)((u32)points[3] >> 16);
    sprite->x0 = x - 8;
    sprite->x1 = x + 8;
    sprite->x2 = x - 8;
    sprite->x3 = x + 8;
    sprite->y2 = bottom;
    sprite->y3 = bottom;
    sprite->y0 = (s16)(bottom - 10);
    sprite->y1 = (s16)(bottom - 10);
    addPrim((u32 *)orderingTable + 1, sprite);
    PopMatrix();
}
