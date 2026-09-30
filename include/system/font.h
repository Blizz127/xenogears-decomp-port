#ifndef _XENO_FONT_H
#define _XENO_FONT_H

#include "psyq/libgpu.h"

// Should probably be moved to something like system/graphics.h at some point
#define setXY0Fast(p,_x0,_y0) *(int*)&(p)->x0 = (_x0) | ((_y0) << 0x10)
#define setXY1Fast(p,_x1,_y1) *(int*)&(p)->x1 = (_x1) | ((_y1) << 0x10)
#define setXY2Fast(p,_x2,_y2) *(int*)&(p)->x2 = (_x2) | ((_y2) << 0x10)
#define setWHFast(p,_w,_h) *(int*)&(p)->w = (_w) | ((_h) << 0x10)
#define setUV0Fast(p,_u0,_v0) *(short*)&(p)->u0 = (_u0) | ((_v0) << 0x8)

// Font Flags

// Font Letter Flags
#define FONT_LETTER_RENDER_CONTEXT 0x1
#define FONT_LETTER_16x16 0x2
#define FONT_LETTER_ALL_UPPERCASE 0x4

// This struct is just a header for the font,
// letter data follows directly after at the end
typedef struct {
    // Flags
    // 0x1: Indicates if letter size should be 0x10 or 0x20
    //
    // 0x4: Determines heap allocation mode
    short flags;

    short texpage;
    void* pPrimBuffers[2];
    short startX;
    short startY;
    short areaWidth;
    short areaHeight;
    short letterWidth; // Default letter width?
    short rowHeight;
    char r;
    char g;
    char b;
    char primitiveCode;
    DR_TPAGE texpageSettings[2];
    short maxNumLetters;
    short letterFlags;
    short curX;
    short curY;
    short curNumLetters;
    short nextRowX;
    SPRT_16* pLetterRenderPacket;
    short letterClutIds[4];
    TILE bgTiles[2];
    char letterWidths[0x60]; // Individual widths for each letter?
    void* pPrimBuffersStart[2];
    short storedPosX;
    short storedPosY;
    short storedRowX;
    char vramLetterOffsetX;
    char _pad;
} Font;

// Incomplete
typedef struct {
    short mode;
    char letterWidth;
    char rowHeight;
} FontFile;

extern Font* g_Font;

#endif