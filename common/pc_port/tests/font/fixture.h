#ifndef FONT_FIXTURE_H
#define FONT_FIXTURE_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef unsigned short u_short;
typedef void FnPrintf_t(char *, ...);

enum {
    FONT_LETTER_RENDER_CONTEXT = 1,
    FONT_LETTER_16x16 = 2,
    FONT_LETTER_ALL_UPPERCASE = 4
};

/* Only the packed words written by the glyph encoder need a packet layout. */
typedef struct {
    uint32_t link;
    uint8_t r0, g0, b0, code;
    int16_t x0, y0;
    uint8_t u0, v0;
    uint16_t clut;
} SPRT_16;

/* A standalone test model. These members are deliberately grouped by use.
 * Packet, page and tile fixtures are ordinary host-owned storage. */
typedef struct {
    _Alignas(uint32_t) char r;
    char g, b, primitiveCode;
    short flags, letterFlags;
    short startX, startY, areaWidth, areaHeight;
    short curX, curY, nextRowX, storedPosX, storedPosY, storedRowX;
    short letterWidth, rowHeight, curNumLetters, maxNumLetters;
    char vramLetterOffsetX, letterWidths[96];
    short letterClutIds[4];
    void *pPrimBuffers[2], *pPrimBuffersStart[2];
    SPRT_16 *pLetterRenderPacket;
    uint32_t texpageSettings[2], bgTiles[2];
} Font;

_Static_assert(sizeof(short) == 2 && sizeof(int) == 4, "16-bit short and 32-bit int required");
_Static_assert(sizeof(SPRT_16) == 16 && offsetof(SPRT_16, r0) == 4 &&
               offsetof(SPRT_16, x0) == 8 && offsetof(SPRT_16, u0) == 12, "glyph packet words");
_Static_assert(offsetof(Font, primitiveCode) == offsetof(Font, r) + 3, "adjacent color and code bytes");

static Font *g_Font;
static void *D_800593A0;
static FnPrintf_t *g_HeapDebugPrintfFn;
static void HeapFree(void *);
static void func_80036718(int, char *, va_list);
static int DrawSync(int);
static void TermPrim(void *);
static void func_800317E0(void *, void *);
static void AddPrim(void *, void *);
static void func_80031804(void *, void *);
static void DrawOTag(void *);

#endif
