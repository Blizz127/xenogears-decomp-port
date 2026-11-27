/* Field access goes through the selected font. The color update resamples
 * this selection at each of its four service boundaries. */
static volatile Font *fontCurrentRecord(void)
{
    return *(Font *volatile *)&g_Font;
}

int FontGetCurLetterCount(void)
{
    return fontCurrentRecord()->curNumLetters;
}

int FontGetCurLetterX(void)
{
    return fontCurrentRecord()->curX;
}

int FontGetCurLetterY(void)
{
    return fontCurrentRecord()->curY;
}

int FontGetDefaultLetterHeight(void)
{
    return fontCurrentRecord()->rowHeight;
}

int FontGetDefaultLetterWidth(void)
{
    return fontCurrentRecord()->letterWidth;
}

int FontGetFlags(void)
{
    return fontCurrentRecord()->flags;
}

int FontGetLetterFlags(void)
{
    return fontCurrentRecord()->letterFlags;
}

void FontSetFlag(u_short mask)
{
    volatile Font *font = fontCurrentRecord();
    font->flags = (short)((u_short)font->flags | mask);
}

void FontSetLetterFlag(u_short mask)
{
    volatile Font *font = fontCurrentRecord();
    font->letterFlags = (short)((u_short)font->letterFlags | mask);
}

void FontRemoveFlag(u_short mask)
{
    volatile Font *font = fontCurrentRecord();
    font->flags = (short)((u_short)font->flags & ~mask);
}

void FontRemoveLetterFlag(u_short mask)
{
    volatile Font *font = fontCurrentRecord();
    font->letterFlags = (short)((u_short)font->letterFlags & ~mask);
}

void FontSetCurLetterX(short value)
{
    fontCurrentRecord()->curX = value;
}

void FontSetCurLetterY(short value)
{
    fontCurrentRecord()->curY = value;
}

void FontSetLetterWidth(short value)
{
    fontCurrentRecord()->letterWidth = value;
}

void FontSetRowHeight(short value)
{
    fontCurrentRecord()->rowHeight = value;
}

void FontSetColor(int red, int green, int blue)
{
    fontCurrentRecord()->r = (char)red;
    fontCurrentRecord()->g = (char)green;
    fontCurrentRecord()->b = (char)blue;
    volatile Font *font = fontCurrentRecord();
    unsigned int op = (unsigned char)font->primitiveCode;
    unsigned int bright = red >= 128 && green >= 128 && blue >= 128;
    font->primitiveCode = (char)((op & ~1u) | bright);
}

void FontStoreCurrentPositions(void)
{
    volatile Font *font = fontCurrentRecord();
    short x = font->curX;
    short y = font->curY;
    short row = font->nextRowX;
    font->storedPosX = x;
    font->storedPosY = y;
    font->storedRowX = row;
}

void FontRestoreCurrentPositions(void)
{
    volatile Font *font = fontCurrentRecord();
    short x = font->storedPosX;
    short y = font->storedPosY;
    short row = font->storedRowX;
    font->curX = x;
    font->curY = y;
    font->nextRowX = row;
}

void FontSetPositionsByOffset(short x, short y)
{
    volatile Font *font = fontCurrentRecord();
    if (font) {
        short originX = font->startX;
        short originY = font->startY;
        font->curX = (short)(originX + x);
        font->curY = (short)(originY + y);
    }
}

void func_8003708C(int x, int y)
{
    volatile Font *font = fontCurrentRecord();
    if (font) {
        short originX = font->startX;
        short originY = font->startY;
        unsigned int dx = x < 0 ? 0u : (unsigned int)x;
        unsigned int dy = y < 0 ? 0u : (unsigned int)y;
        short nextX = (short)((unsigned int)originX + dx);
        font->curX = nextX;
        font->nextRowX = nextX;
        font->curY = (short)((unsigned int)originY + dy);
    }
}

void FontResetLetterRendering(void)
{
    volatile Font *font = fontCurrentRecord();
    unsigned int buffer = (unsigned short)font->letterFlags & 1u;
    short y = font->startY;
    unsigned int op = (unsigned char)font->primitiveCode;
    void *packet = font->pPrimBuffers[buffer];
    short x = font->startX;
    font->curNumLetters = 0;
    font->curY = y;
    font->storedPosY = y;
    font->primitiveCode = (char)(op & ~1u);
    font->curX = x;
    font->storedPosX = x;
    font->nextRowX = x;
    font->storedRowX = x;
    font->pLetterRenderPacket = packet;
}
