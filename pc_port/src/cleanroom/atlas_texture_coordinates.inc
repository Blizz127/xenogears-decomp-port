/* The menu atlas directory holds little-endian byte offsets. Sprite entries
 * store the palette position and a texture origin within a VRAM page. */
static u16 MenuAtlasHalfword(const u8 *data)
{
    return (u16)(data[0] | ((u16)data[1] << 8));
}

void func_80026338(u8 *atlas, s32 index, u32 *count, s32 *depth,
                   s32 *paletteX, s32 *paletteY, s32 *textureX, s32 *textureY)
{
    const u8 *entry = atlas + MenuAtlasHalfword(atlas + 4 + 2 * index);
    s32 pixelDepth = (s16)MenuAtlasHalfword(entry + 20);
    s32 xPixels = (s16)MenuAtlasHalfword(entry + 4);
    s32 pageX = (s16)MenuAtlasHalfword(entry + 26);
    s32 pageY = (s16)MenuAtlasHalfword(entry + 28);
    unsigned pixelsPerWordShift = pixelDepth ? 2 : 4;

    *count = (u32)(s32)(s16)MenuAtlasHalfword(entry);
    *depth = pixelDepth;
    *paletteX = (s16)MenuAtlasHalfword(entry + 22);
    *paletteY = (s16)MenuAtlasHalfword(entry + 24);
    *textureX = (pageX & ~63) + (xPixels >> pixelsPerWordShift);
    *textureY = (pageY & ~255) + (s16)MenuAtlasHalfword(entry + 6);
}
