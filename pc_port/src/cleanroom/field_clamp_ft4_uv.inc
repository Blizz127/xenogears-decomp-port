static unsigned char FieldClampFT4UVByte(short value)
{
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return (unsigned char)value;
}

void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0,
                          short u1, short v1, short u2, short v2,
                          short u3, short v3)
{
    volatile POLY_FT4* packet = poly;

    packet->u0 = FieldClampFT4UVByte(u0);
    packet->v0 = FieldClampFT4UVByte(v0);
    packet->u1 = FieldClampFT4UVByte(u1);
    packet->v1 = FieldClampFT4UVByte(v1);
    packet->u2 = FieldClampFT4UVByte(u2);
    packet->v2 = FieldClampFT4UVByte(v2);
    packet->u3 = FieldClampFT4UVByte(u3);
    packet->v3 = FieldClampFT4UVByte(v3);
}
