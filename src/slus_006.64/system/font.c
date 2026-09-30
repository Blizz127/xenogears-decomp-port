#include "common.h"

#include "psyq/stdarg.h"
#include "system/font.h"
#include "system/memory.h"

// screen printf
extern void func_80036718(char*, char*, ...);

extern void* g_FontClutData;
extern RECT g_FontClutDest;
extern void* D_800593A0; // Non-heap allocated Font?
extern u8 D_80050240[]; // Default embedded font file

void FontSetFlag(u_short flag) {
    g_Font->flags |= flag;
}

void FontRemoveFlag(u_short flag) {
    g_Font->flags &= ~flag;
}

int FontGetFlags(void) {
    return g_Font->flags;
}

void FontSetLetterFlag(u_short flag) {
    g_Font->letterFlags |= flag;
}

void FontRemoveLetterFlag(u_short flag) {
    g_Font->letterFlags &= ~flag;
}

int FontGetLetterFlags(void) {
    return g_Font->letterFlags;
}

void FontSetLetterWidth(short newLetterWidth) {
    g_Font->letterWidth = newLetterWidth;
}

void FontSetRowHeight(short newRowHeight) {
    g_Font->rowHeight = newRowHeight;
}

void FontSetCurLetterX(short newX) {
    g_Font->curX = newX;
}

void FontSetCurLetterY(short newY) {
    g_Font->curY = newY;
}

void FontSetColor(int red, int green, int blue) {
    g_Font->r = red;
    g_Font->g = green;
    g_Font->b = blue;
    
    // Shading
    if ((red < 0x80) || (green < 0x80) || (blue < 0x80))
        g_Font->primitiveCode &= 0xFE;
    else
        g_Font->primitiveCode |= 1;
}

void FontLoadClut(short arg0, short arg1) {
    short* pClutData;
    int i;
    int j;

    pClutData = &g_FontClutData;
    
    for (i = 0; i < 8; i++) {
        *pClutData++ = arg1;
        *pClutData++ = arg0;
    }
    
    if (i < 0xC) {
        while (i < 0xC) {
            *pClutData++ = arg1;
            *pClutData++ = arg1;
            *pClutData++ = arg0;
            *pClutData++ = arg0;
            i++;
        };
    }
    
    while (i < 0xE) {
        for (j = 3; j >= 0; j--)
            *pClutData++ = arg1;

        for (j = 3; j >= 0; j--)
            *pClutData++ = arg0;
        
        i++;
    }

    for (j = 7; j >= 0; j--)
        *pClutData++ = arg1;

    for (j = 8; j < 0x10; j++)
        *pClutData++ = arg0;

    LoadImage(&g_FontClutDest, &g_FontClutData);
}

int FontGetDefaultLetterWidth(void) {
    return g_Font->letterWidth;
}

int FontGetDefaultLetterHeight(void) {
    return g_Font->rowHeight;
}

int FontGetCurLetterX(void) {
    return g_Font->curX;
}

int FontGetCurLetterY(void) {
    return g_Font->curY;
}

int FontGetCurLetterCount(void) {
    return g_Font->curNumLetters;
}

void FontStoreCurrentPositions(void) {
    g_Font->storedPosX = g_Font->curX;
    g_Font->storedPosY = g_Font->curY;
    g_Font->storedRowX = g_Font->nextRowX;
}

void FontRestoreCurrentPositions(void) {
    g_Font->curX = g_Font->storedPosX;
    g_Font->curY = g_Font->storedPosY;
    g_Font->nextRowX = g_Font->storedRowX;
}

void FontPrintf(char* pFormat, ...) {
    va_list args;
    
    if (g_Font) {
        va_start(args, pFormat);
        func_80036718(0, pFormat, args);
        va_end(args);
    }
}

void FontSetPositionsByOffset(short xOffset, short yOffset) {
    if (g_Font) {
        g_Font->curX = g_Font->startX + xOffset;
        g_Font->curY = g_Font->startY + yOffset;
    }
}

void func_8003708C(int offsetX, int offsetY) {
    short nNewPosX;

    if (g_Font) {
        if (offsetX < 0)
            offsetX = 0;
        
        if (offsetY < 0)
            offsetY = 0;
        
        nNewPosX = g_Font->startX + offsetX;
        g_Font->curX = nNewPosX;
        g_Font->nextRowX = nNewPosX;
        g_Font->curY = g_Font->startY + offsetY;
    }
}

void FontAddLetterPrimitive(int letter) {
    Font* pFont;
    int nAsciiLetter;
    short nU0;
    short nV0;
    int nLetterWidth;

    pFont = g_Font;
    nAsciiLetter = letter;
    
    if (pFont && 
        (pFont->curY + pFont->rowHeight) <= (pFont->startY + pFont->areaHeight) && 
        pFont->curNumLetters <= pFont->maxNumLetters
    ) {
        
        if (nAsciiLetter < 0x20) {
            if (nAsciiLetter == '\n') {
                pFont->curX = pFont->nextRowX;
                pFont->curY = pFont->curY + pFont->rowHeight;
            }     
        } else {
            if ((pFont->letterFlags & FONT_LETTER_ALL_UPPERCASE) && (nAsciiLetter >= 0x60))
                nAsciiLetter -= 0x20;
            nAsciiLetter = nAsciiLetter - 0x20;
            
            if (pFont->letterFlags & 8)
                nLetterWidth = pFont->letterWidths[nAsciiLetter];
            else
                nLetterWidth = pFont->letterWidth;
            
            if ((pFont->curX + nLetterWidth) >= (pFont->startX + pFont->areaWidth)) {
                if (!(pFont->flags & 8)) {
                    pFont->curX = pFont->nextRowX;
                    pFont->curY = pFont->curY + pFont->rowHeight;
                } else {
                    return;
                }
            }
            
            if (nAsciiLetter) {
                // Color + primitive code
                *(int*)&pFont->pLetterRenderPacket->r0 = *(int*)&pFont->r;

                setXY0Fast(pFont->pLetterRenderPacket, pFont->curX, pFont->curY);

                if (pFont->letterFlags & FONT_LETTER_16x16) {
                    pFont->pLetterRenderPacket->clut = pFont->letterClutIds[(nAsciiLetter & 0x18) >> 3];

                    nU0 = (nAsciiLetter & 7) * 0x10;
                    nV0 = (nAsciiLetter & 0x60) >> 1;
                    setUV0Fast(pFont->pLetterRenderPacket, nU0, pFont->vramLetterOffsetX + nV0);
                } else {
                    pFont->pLetterRenderPacket->clut =  pFont->letterClutIds[(nAsciiLetter & 0x30) >> 4];

                    nU0 = (nAsciiLetter & 0xF) * 8;
                    nV0 = (nAsciiLetter & 0xC0) >> 3;
                    setUV0Fast(pFont->pLetterRenderPacket, nU0, pFont->vramLetterOffsetX + nV0);
                }
                
                pFont->pLetterRenderPacket++;
                pFont->curNumLetters++;
            }
            
            pFont->curX += nLetterWidth;
        }
    }
}

void FontResetLetterRendering(void) {
    short nStartX;
    short nStartY;
    int nContext;
    u_char nPrimitiveCode;
    
    nContext = g_Font->letterFlags & FONT_LETTER_RENDER_CONTEXT;
    nStartY = g_Font->startY;
    nPrimitiveCode = g_Font->primitiveCode & 0xFE;
    g_Font->pLetterRenderPacket = g_Font->pPrimBuffers[nContext];
    nStartX = g_Font->startX;
    g_Font->curNumLetters = 0;
    g_Font->curY = nStartY;
    g_Font->storedPosY = nStartY;
    g_Font->curX = nStartX;
    g_Font->storedPosX = nStartX;
    g_Font->nextRowX = nStartX;
    g_Font->storedRowX = nStartX;
    g_Font->primitiveCode = nPrimitiveCode;
}

void FontDrawLetters(void* ot) {
    Font* pFont;
    int nNumLetters;
    void* pPrim;
    void* pPrimBuffer;
    short nContext;
    short bShouldDrawPrimitives;
    void* pOtag;

    pFont = g_Font;
    pOtag = ot;
    
    if (pFont) {
        bShouldDrawPrimitives = 0;

        // Switch render context
        if (pFont->flags & FONT_LETTER_RENDER_CONTEXT) {
            nContext = 0;
            pFont->letterFlags &= 0xFFFE;
        } else {
            nContext = pFont->letterFlags & FONT_LETTER_RENDER_CONTEXT;
            if (nContext) {
                pFont->letterFlags &= 0xFFFE;
            } else {
                pFont->letterFlags |= FONT_LETTER_RENDER_CONTEXT;
            }
        }
        
        if ((pOtag == NULL) || (pOtag == -1)) {
            DrawSync(0);
            pOtag = &pFont->pPrimBuffersStart[nContext];
            bShouldDrawPrimitives = 1;
            TermPrim(pOtag);
        }
        
        nNumLetters = pFont->curNumLetters;
        pPrimBuffer = pFont->pPrimBuffers[nContext];
        while (nNumLetters) {
            pPrim = pPrimBuffer;
            pPrimBuffer += 0x10;
            func_800317E0(pOtag, pPrim);
            nNumLetters--;
        }

        AddPrim(pOtag, &pFont->texpageSettings[nContext]);

        if (pFont->flags & 0x10) {
            func_80031804(pOtag, &pFont->bgTiles[nContext]);
        }
        
        FontResetLetterRendering();
        if (bShouldDrawPrimitives) {
            DrawOTag(pOtag);
        }
    }
}

void func_8003747C(void* pFont) {
    D_800593A0 = pFont;
}

void FontFree(void) {
    if (g_Font) {
        g_HeapDebugPrintfFn = FontPrintf;
        if (D_800593A0 == NULL)
            HeapFree(g_Font);
        g_Font = NULL;
    }
    D_800593A0 = NULL;
}

void* FontLoadFont(int startX, int startY, int width, int height, int maxLetters, unsigned int flags,  int texpageX, int texpageY, int clutX, int clutY, void* pCompressedFontFile) {
    RECT rect;
    short nStartX;
    short nStartY;
    short nHeightFlag;
    short nHeight;
    short bUse16x16Sprite;
    int nAllocSize;
    u_char nMode;
    FontFile* pFontFile;
    void* pPacket;
    void* pImageData;
    Font* pFont;
    int nAllocMode;

    // Allocate Font
    if (D_800593A0 == NULL) {
        HeapSetCurrentContentType(HEAP_CONTENT_LS_FONT);

        if (!(flags & 1)) {
            nAllocSize = (maxLetters * 0x20) + sizeof(Font);
        } else {
            nAllocSize = (maxLetters * 0x10);
            nAllocSize += sizeof(Font);
        }
        
        nAllocMode = ((flags >> 2) ^ 1) & 1;
        pImageData = HeapAlloc(nAllocSize, nAllocMode);
    } else {
        pImageData = D_800593A0;
    }
    pFont = pImageData;
    // Set primitive buffer based on letter size?
    pPacket = pFont + 1;
    pFont->pPrimBuffers[0] = pPacket;
    if (flags & 1) {
        pFont->pPrimBuffers[1] = pPacket;
    } else {
        pFont->pPrimBuffers[1] = (void* ) (pPacket + (maxLetters * 0x10));
    }

    // Decompress font file
    if (pCompressedFontFile == NULL)
        pCompressedFontFile = D_80050240;
    pFontFile = LZSSHeapDecompress(pCompressedFontFile, 0);
    
    nMode = pFontFile->mode;
    bUse16x16Sprite = nMode & 1;
    nHeightFlag = nMode & 2;
    pFont->letterWidth = pFontFile->letterWidth;
    pFont->rowHeight = pFontFile->rowHeight;
    
    if (flags & 2) {
        nHeightFlag = 0;
    }

    nStartX = startX;
    nStartY = startY;
    pFont->startX = nStartX;
    pFont->curX = nStartX;
    pFont->flags = flags;
    pFont->startY = nStartY;
    pFont->curY = nStartY;
    pFont->areaWidth = width;
    pFont->areaHeight = height;
    
    // Default Color
    pFont->b = 0xFF;
    pFont->g = 0xFF;
    pFont->r = 0xFF;
    
    pFont->maxNumLetters = maxLetters;
    pFont->curNumLetters = 0;
    pFont->letterFlags = 0;
   
    
    if (bUse16x16Sprite) {
        pFont->primitiveCode = 0x7D; // SPRT_16, with shading
    } else {
        pFont->primitiveCode = 0x75; // SPRT_8, with shading
    }
    
    pFont->vramLetterOffsetX = texpageY;
    if (nHeightFlag == 0) {
        pFont->letterFlags |= 0x4;
    }
    
    if (bUse16x16Sprite) {
        nHeight = 0x20;
        pFont->letterFlags |= 0x2;
        if (nHeightFlag)
            nHeight = 0x30;
    } else {
        nHeight = 0x8;
        if (nHeightFlag)
            nHeight = 0x10;
    }
    
    rect.x = texpageX;
    rect.y = texpageY;
    rect.w = 0x20;
    rect.h = nHeight;
    
    pImageData = pFontFile + 1;
    if (!pFont->letterWidth) {
        pFont->letterFlags |= 0x8;
        memmove(&pFont->letterWidths, pImageData, 0x60);
        pImageData = (void*)pFontFile + 0x64;
    }
    
    LoadImage(&rect, pImageData);
    pFont->texpage = GetTPage(0, 0, texpageX, texpageY);
    rect.w = 0x40;
    rect.x = clutX;
    rect.y = clutY;
    rect.h = 0x1;
    pFont->letterClutIds[0] = GetClut(clutX, clutY);
    pFont->letterClutIds[1] = GetClut(clutX + 0x10, clutY);
    pFont->letterClutIds[2] = GetClut(clutX + 0x20, clutY);
    pFont->letterClutIds[3] = GetClut(clutX + 0x30, clutY);

    g_FontClutDest = rect;
    FontLoadClut(0x7FFF, 0);

    SetDrawTPage(&pFont->texpageSettings[0], 0, 0, pFont->texpage);
    SetDrawTPage(&pFont->texpageSettings[1], 0, 0, pFont->texpage);

    setTile(&pFont->bgTiles[0]);
    setRGB0(&pFont->bgTiles[0], 0x0, 0x0, 0x0);  
    setXY0Fast(&pFont->bgTiles[0], startX, startY);
    setWHFast(&pFont->bgTiles[0], width, height);
    setSemiTrans(&pFont->bgTiles[0], 1);
    pFont->bgTiles[1] = pFont->bgTiles[0];
    
    g_Font = pFont;
    FontResetLetterRendering();
    HeapFree(pFontFile);
    return pFont;
}
