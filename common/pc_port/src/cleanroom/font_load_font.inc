#include <string.h>

extern void *LZSSHeapDecompress(void *, int);

void *FontLoadFont(int x, int y, int width, int height, int capacity,
                   unsigned int flags, int pageX, int pageY, int clutX,
                   int clutY, void *compressed)
{
    Font *font = D_800593A0;
    uint32_t bankBytes = (uint32_t)capacity * (uint32_t)sizeof(SPRT_16);
    if (font == NULL) {
        HeapSetCurrentContentType(50);
        uint32_t bytes = (uint32_t)sizeof(Font) + bankBytes * ((flags & 1u) ? 1u : 2u);
        font = HeapAlloc(bytes, (flags & 4u) ? 0u : 1u);
    }

    uintptr_t packets = (uintptr_t)font + sizeof(Font);
    font->pPrimBuffers[0] = (void *)packets;
    if (!(flags & 1u))
        packets += (uintptr_t)(intptr_t)(int32_t)bankBytes;
    font->pPrimBuffers[1] = (void *)packets;

    unsigned char *file = LZSSHeapDecompress(compressed ? compressed : D_80050240, 0);
    unsigned int mode = file[0];
    unsigned int large = mode & 1u;
    unsigned int tall = (mode & 2u) && !(flags & 2u);
    unsigned int variable = file[2] == 0;
    font->startX = font->curX = (short)(uint16_t)x;
    font->startY = font->curY = (short)(uint16_t)y;
    font->areaWidth = (short)(uint16_t)width;
    font->areaHeight = (short)(uint16_t)height;
    font->maxNumLetters = (short)(uint16_t)capacity;
    font->flags = (short)(uint16_t)flags;
    font->curNumLetters = 0;
    font->r = font->g = font->b = (char)255;
    font->primitiveCode = (char)(large ? 0x7d : 0x75);
    font->letterFlags = (short)((large ? 2 : 0) | (tall ? 0 : 4) | (variable ? 8 : 0));
    font->letterWidth = file[2];
    font->rowHeight = file[3];
    font->vramLetterOffsetX = (char)(uint8_t)pageY;

    unsigned char *image = file + 4;
    if (variable) {
        memmove(font->letterWidths, image, sizeof(font->letterWidths));
        image += sizeof(font->letterWidths);
    }
    RECT area;
    area.x = (short)(uint16_t)pageX;
    area.y = (short)(uint16_t)pageY;
    area.w = 32;
    area.h = (short)(large ? (tall ? 48 : 32) : (tall ? 16 : 8));
    LoadImage(&area, (u_long *)(void *)image);
    font->texpage = (short)GetTPage(0, 0, pageX, pageY);
    for (unsigned int i = 0; i < 4; ++i)
        font->letterClutIds[i] = (short)GetClut((int)((uint32_t)clutX + i * 16u), clutY);

    g_FontClutDest.x = (short)(uint16_t)clutX;
    g_FontClutDest.y = (short)(uint16_t)clutY;
    g_FontClutDest.w = 64;
    g_FontClutDest.h = 1;
    FontLoadClut(0x7fff, 0);
    for (unsigned int i = 0; i < 2; ++i)
        SetDrawTPage(&font->texpageSettings[i], 0, 0, font->texpage);

    TILE *tile = &font->bgTiles[0];
    setlen(tile, 3);
    tile->r0 = 0;
    tile->g0 = 0;
    tile->b0 = 0;
    tile->code = 0x62;
    uint32_t xy = (uint32_t)x | ((uint32_t)y << 16);
    uint32_t size = (uint32_t)width | ((uint32_t)height << 16);
    tile->x0 = (short)(uint16_t)xy;
    tile->y0 = (short)(uint16_t)(xy >> 16);
    tile->w = (short)(uint16_t)size;
    tile->h = (short)(uint16_t)(size >> 16);
    font->bgTiles[1] = *tile;

    g_Font = font;
    FontResetLetterRendering();
    HeapFree(file);
    return font;
}
