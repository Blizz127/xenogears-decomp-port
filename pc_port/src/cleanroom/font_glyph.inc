typedef uint32_t FontGlyphWord __attribute__((may_alias));
typedef uint16_t FontGlyphHalf __attribute__((may_alias));

static void fontGlyphNextRow(volatile Font *font)
{
    int y = font->curY;
    int height = font->rowHeight;
    short x = font->nextRowX;
    font->curX = x;
    font->curY = (short)(uint16_t)(y + height);
}

void FontAddLetterPrimitive(int letter)
{
    volatile Font *font = *(Font *volatile *)&g_Font;
    if (!font)
        return;

    int y = font->curY;
    int height = font->rowHeight;
    int top = font->startY;
    int spanY = font->areaHeight;
    if (y + height > top + spanY)
        return;
    int count = font->curNumLetters;
    int capacity = font->maxNumLetters;
    if (count > capacity)
        return;

    if (letter < 32) {
        if (letter == 10)
            fontGlyphNextRow(font);
        return;
    }

    unsigned int flags = (uint16_t)font->letterFlags;
    unsigned int ordinal = (unsigned int)letter - 32u;
    if ((flags & FONT_LETTER_ALL_UPPERCASE) && letter >= 96)
        ordinal -= 32u;

    int width;
    if ((uint16_t)font->letterFlags & 8u) {
        if (ordinal >= sizeof(font->letterWidths))
            return;
        width = (uint8_t)font->letterWidths[ordinal];
    } else {
        width = font->letterWidth;
    }

    int x = font->curX;
    int left = font->startX;
    int spanX = font->areaWidth;
    if (x + width >= left + spanX) {
        if (font->flags & 8)
            return;
        fontGlyphNextRow(font);
    }

    if (ordinal) {
        volatile SPRT_16 *packet = font->pLetterRenderPacket;
        FontGlyphWord color = *(volatile FontGlyphWord *)&font->r;
        *(volatile FontGlyphWord *)&packet->r0 = color;
        int row = font->curY;
        int column = font->curX;
        packet = font->pLetterRenderPacket;
        *(volatile FontGlyphWord *)&packet->x0 = (uint32_t)column | ((uint32_t)row << 16);

        unsigned int palette, u, v;
        if (font->letterFlags & FONT_LETTER_16x16) {
            palette = (ordinal >> 3) & 3u;
            u = (ordinal & 7u) * 16u;
            v = (ordinal & 96u) / 2u;
        } else {
            palette = (ordinal >> 4) & 3u;
            u = (ordinal & 15u) * 8u;
            v = (ordinal & 192u) / 8u;
        }
        packet = font->pLetterRenderPacket;
        packet->clut = (uint16_t)font->letterClutIds[palette];
        v += (uint8_t)font->vramLetterOffsetX;
        packet = font->pLetterRenderPacket;
        *(volatile FontGlyphHalf *)&packet->u0 = (uint16_t)(u | (v << 8));

        count = font->curNumLetters;
        SPRT_16 *current = font->pLetterRenderPacket;
        font->pLetterRenderPacket = current + 1;
        font->curNumLetters = (short)(uint16_t)(count + 1);
    }
    x = font->curX;
    font->curX = (short)(uint16_t)(x + width);
}
