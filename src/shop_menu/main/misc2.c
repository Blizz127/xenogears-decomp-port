#include "common.h"
#include "psyq/libgpu.h"
#include "system/menu.h"
#include "system/archive.h"
extern void* D_8005945C;
extern void* D_8006259C;
extern const char D_801C5000[16];
extern unsigned int ResolveArchiveEntryPointers(u32*);
extern void* LZSSHeapDecompress(void*,int);
extern void func_8002DD20(u32*);
extern void func_80026338(u8*,s32,u32*,s32*,s32*,s32*,s32*,s32*);
extern void* HeapAlloc(u_int,u_int);
extern u_int HeapFree(void*);
extern void SoundAddSedsEntry(SoundFile*);
extern int ArchiveDecodeAlignedSize(unsigned int);
extern s32 ArchiveReadFileToBuffer(s32,void*,u32,u32);
extern void ArchiveCdDataSync(int);

#include "common.h"

#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/debug.h"
#include "system/math.h"
#include "system/menu.h"
#include "main/game.h"

// Confirmation window choices
#define MENU_CHOICE_NO 0
#define MENU_CHOICE_YES 1

// Shop mode choices
#define MENU_CHOICE_EXIT 0x0
#define MENU_CHOICE_SELL 0x1
#define MENU_CHOICE_BUY 0x2

// Sell mode menu choice
#define MENU_CHOCIE_EQUIPMENT 0x0
#define MENU_CHOICE_ACCESSORIES 0x1
#define MENU_CHOICE_WEAPONS 0x2
#define MENU_CHOICE_ITEMS 0x3

#define MENU_STAT_CHANGE_INCREASE 0x0
#define MENU_STAT_CHANGE_DECREASE 0x1

#define SHOP_DATA_INITIALIZE 0x0
#define SHOP_DATA_FREE 0x10

// Selection modes
#define MENU_AUTO_ADVANCE 0
#define MENU_MANUAL_CHOICE 0xFF

// Max items to display in the window at a time
#define MAX_ITEMS_IN_VIEW 8

// Debug-related?
extern s32* D_8005917C;

extern int D_801D1F50; // Number of items
extern s32 D_801D1FD0[];
extern s32 D_801D1FD4[];
extern s32 D_801D1FE8[];
extern u16 D_801D2260;


extern s32 D_801D1F54[];
// = {
//    MENU_TEX_BALL_CURSOR_1, MENU_TEX_STRING_BUY,
//    MENU_TEX_BALL_CURSOR_2, MENU_TEX_STRING_SELL,
//    MENU_TEX_BALL_CURSOR_3, MENU_TEX_STRING_EXIT
//    }

extern s32 D_801D1FCC[];
extern s32 D_801D1FD8[];

extern u_short D_801D21F0[];

extern s32 D_801D1FCC[];

extern s32 D_801D2248;
extern s32 D_801D224C;
extern s32 D_801D2250;
extern s32 D_801D2254;
extern s32 D_801D2258;
extern s32 D_801D225C;

// Character portrait X positions
extern int D_801D21CC[];

// Cursor stuff
extern int D_801D201C[];
extern int D_801D2094[];
extern int D_801D2114[];

extern int D_801D1FF8[];
extern int D_801D2008[];

// Argument passed to the menu by the outside.
// For the shop menu, this is expected to be the index
// of the shop to load and run.
extern u8 D_80059171;

extern s32 D_801D1F50;


extern int D_801D2194[];
extern int D_801D21B0[];

// Explanations stuff
extern u_char D_801D2210[]; // Buy menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION, 
//     MENU_TEX_STRING_ATTACK, 
//     MENU_TEX_STRING_DEFENSE, 
//     MENU_TEX_MINUS 
//    }

extern u_char D_801D2214[]; // Sell menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION, MENU_TEX_PLUS }

extern int D_801D2218[]; // Buy menu explanation graphics X positions
extern int D_801D2228[]; // Sell menu explanation graphics X positions
extern int D_801D2230[]; // Buy menu explanation graphics Y positions
extern int D_801D2240[]; // Sell menu explanation graphics Y positions

u_char ShopMenuBuyMenu(void);
/* Declared here rather than relying on the definition in misc5.c: the split
 * puts this call in a different TU, and without the u8 parameter GCC passes
 * the argument unmasked (retail: `andi $a1,$s0,0xFF`). */
void func_801CCE1C(void* output, u8 characterId);



/* Part of the split shop-menu TU. Retail interleaves unmatched (INCLUDE_ASM)
 * regions with C runs; cc1 emits every file-scope __asm__ block before every
 * compiled body, so each part below is (one retail asm run) + (the C run that
 * follows it) - the layout the compiler produces inside a single TU. */
#ifdef XENO_PC_PORT
extern void* GetStringEntry(void* bundle, s32 index);
extern s32 SystemRenderStringEntry(void* string, void* work, s32 height, s32 flag);
extern void func_801C5A7C(MenuString* pString, s32 index, s32 offset, u8 flags);

/* Retail uses 0x80-byte records. Native MenuString contains a wider pointer,
 * so both initialization and iteration must use the native structure layout. */
void func_801C5CBC(void* output, u8* stringIds, s32 firstIndex, s32 count) {
    MenuString* strings = output;
    s32 i;
    void* work = g_Menu->unk4E0[0].pVramBuffer;

    for (i = 0; i < count; i += 2) {
        MenuString* first = &strings[i];
        MenuString* second = &strings[i + 1];

        first->width = (u8)SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, stringIds[i]), work, 0x18, 0);
        second->width = (u8)SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, stringIds[i + 1]), work, 0x18, 1);

        /* Retail 801C5D6C..801C5DD8: both bitplanes share one upload. */
        first->vramDest.x = 0x140 + ((i * 16) & 0x20);
        first->vramDest.y = ((i + firstIndex) / 4) * 13;
        first->vramDest.w = 0x1C;
        first->vramDest.h = 13;
        second->vramDest = first->vramDest;
        func_801C5A7C(first, i, firstIndex, 0);
        func_801C5A7C(second, i + 1, firstIndex, 0);
        LoadImage(&first->vramDest, work);
        DrawSync(0);
    }
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc2", func_801C5CBC);
#endif

void func_801C5E6C(void) {
    RECT rect;
    u_short* pVramData;
    u8 _unused[0x8];

    pVramData = HeapAlloc(0x20, 0);
    bzero(pVramData, 0x20);
    pVramData[1] = 0x7FFF;
    rect.y = 448;
    rect.w = 16;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, pVramData);
    DrawSync(0);
    HeapFree(pVramData);
}

extern void func_801C5CBC(void* a0, u8* a1, s32 a2, s32 a3);
extern u8 D_801D2018[];

void func_801C5EE8(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
#ifdef XENO_PC_PORT
    g_Menu->unk4E0[0].pVramBuffer = HeapAlloc(0x38E, 0);
    func_801C5CBC(g_Menu->unk4E0, D_801D2018, 0, 4);
#else
    *(void**)((u8*)g_Menu + 0x558) = HeapAlloc(0x38E, 0);
    func_801C5CBC((u8*)g_Menu + 0x4E0, D_801D2018, 0, 4);
#endif
    func_801C5E6C();
}

void ShopMenuInitializeWindowBorders(void) {
    POLY_FT4 _unusued;
    
    func_80026338(
        g_Menu->unk2DC, MENU_TEX_WINDOW_BORDER_TOP, 
        &g_Menu->unk46C, 
        &g_Menu->texPage0, 
        &g_Menu->clutX0, &g_Menu->clutY0, 
        &g_Menu->texPageX0, &g_Menu->texPageY0
    );
    func_80026338(
        g_Menu->unk2DC, MENU_TEX_WINDOW_BORDER_BOTTOM, 
        &g_Menu->unk484, 
        &g_Menu->texPage1, 
        &g_Menu->clutX1, &g_Menu->clutY1, 
        &g_Menu->texPageX1, &g_Menu->texPageY1
    );
    func_80026338(
        g_Menu->unk2DC, MENU_TEX_WINDOW_BORDER_LEFT, 
        &g_Menu->unk49C, 
        &g_Menu->texPage2, 
        &g_Menu->clutX2, &g_Menu->clutY2, 
        &g_Menu->texPageX2, &g_Menu->texPageY2
    );
    func_80026338(
        g_Menu->unk2DC, MENU_TEX_WINDOW_BORDER_RIGHT, 
        &g_Menu->unk4B4, 
        &g_Menu->texPage3, 
        &g_Menu->clutX3, &g_Menu->clutY3, 
        &g_Menu->texPageX3, &g_Menu->texPageY3
    );
}

void ShopMenuMovePointerCursor(int index, u_char arg1) {
    func_8002675C(
        g_Menu->unk2DC, MENU_TEX_POINTER_CURSOR, 
        &g_Menu->unk348->polysPointerCursor, g_Menu->renderContext, 
        D_801D2194[index], D_801D21B0[index], 
        0x1000
    );
    g_Menu->unk348->cursorRenderContext = g_Menu->renderContext;
    
    if (arg1) {
        // A yellow -> black shaded rectangle of unk15B width, 
        // with green lines around it. Seems to be cursor-related,
        // as the pointer cursor is set to same position
        setXY4(
            &g_Menu->unk348->polyG4s[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 36,
            D_801D2194[index] + 20, D_801D21B0[index] - 20,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );

        setXY3(
            &g_Menu->unk348->lines1[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );

        setXY3(
            &g_Menu->unk348->lines2[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + 20, D_801D21B0[index] - 20,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );
        
        g_Menu->unk348->unk159 = g_Menu->renderContext;
        g_Menu->pManager->unk3 = 1;
    }
}

void func_801C6430(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
}

void func_801C6460(POLY_G4* pPoly, u_char red, u_char green, u_char blue) {
    SetPolyG4(pPoly);
    setRGB0(pPoly, red, green, blue);
    setRGB1(pPoly, red, green, blue);
    setRGB2(pPoly, 0, 0, 0);
    setRGB3(pPoly, 0, 0, 0);
}

void ShopMenuInitializeBackgrounds(void) {
    RECT rect;
    int i;

    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;
    
    func_801C6430();

    for (i = 0; i < 2; i++) {
        // Yellow -> Black gradient
        func_801C6460(&g_Menu->unk348->polyG4s[i], 128, 128, 0);
        SetSemiTrans(&g_Menu->unk348->polyG4s[i], 1);
        
        // Green lines
        SetLineF3(&g_Menu->unk348->lines1[i]);
        setRGB0(&g_Menu->unk348->lines1[i], 0, 64, 0);
        
        SetLineF3(&g_Menu->unk348->lines2[i]);
        setRGB0(&g_Menu->unk348->lines2[i], 0, 64, 0);
        
        // Backgrounds dimming screen
        SetPolyF4(&g_Menu->unk348->polysDimEffect[i]);
        setXY4(&g_Menu->unk348->polysDimEffect[i],
            0,   0,
            320, 0,
            0,   224,
            320, 224
        );
        setRGB0(&g_Menu->unk348->polysDimEffect[i], 128, 128, 128);
        SetSemiTrans(&g_Menu->unk348->polysDimEffect[i], 1);

        SetDrawMode(&g_Menu->unk348->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 128), &rect);
        SetDrawMode(&g_Menu->unk348->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
    }
}

void ShopMenuLoadShopItemsData(u_char mode) {
    u_int* pArchive;

    if (mode < SHOP_DATA_FREE) {
        pArchive = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
        ArchiveReadFileToBuffer(2, pArchive, 0, 0x80);
        ArchiveCdDataSync(0);
        ResolveArchiveEntryPointers(pArchive);
    }

    switch (mode) {
        case SHOP_DATA_INITIALIZE:
            // Shop item definitions
            g_Menu->unk330->pWeaponsData = LZSSHeapDecompress(pArchive[2], 0);
            g_Menu->unk330->pAccessoriesData = LZSSHeapDecompress(pArchive[3], 0);
            g_Menu->unk330->pItemsData = LZSSHeapDecompress(pArchive[1], 0);

            // Shop item descriptions
            g_Menu->pShop->pWeaponDescriptions = LZSSHeapDecompress(pArchive[40], 0);
            g_Menu->pShop->pAccessoryDescriptions = LZSSHeapDecompress(pArchive[41], 0);
            g_Menu->pShop->pItemDescriptions = LZSSHeapDecompress(pArchive[42], 0);       
            break;
        case SHOP_DATA_FREE:
            HeapFree(g_Menu->unk330->pWeaponsData);
            HeapFree(g_Menu->unk330->pAccessoriesData);
            HeapFree(g_Menu->unk330->pItemsData);

            HeapFree(g_Menu->pShop->pWeaponDescriptions);
            HeapFree(g_Menu->pShop->pAccessoryDescriptions);
            HeapFree(g_Menu->pShop->pItemDescriptions);
    }

    if (mode < SHOP_DATA_FREE) {
        HeapFree(pArchive);
    }
}

void ShopMenuInitializeShopData(void) {
    int i;
    int j;
    u_char* pShopItemData;
    
    j = 0;

    // Data for the items the shop we're loading is carrying
    pShopItemData = &g_Menu->pShopEntries[D_80059171 * 0x5C];
    
    for (i = 0; i < MAX_SHOP_ITEMS; i++) {
        g_Menu->shopItemIDs[i] = 0;
        g_Menu->shopItemTypes[i] = 0;
    }

    // 0x5A ??? Arrays being set are 0x30 sized
    // NOTE: Potentially danger if there's more than 0x30 valid items in the shop item data
    for (i = 0; i < 0x5A; i++) {
        if (pShopItemData[i]) {
            g_Menu->shopItemIDs[j] = pShopItemData[i];
            g_Menu->shopItemTypes[j] = i / 30;
            j++;
        }
    }
    D_801D1F50 = j;
    
    ShopMenuLoadShopItemsData(SHOP_DATA_INITIALIZE);

    // Red highlight around character portraits
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 2; j++) {
            SetLineF3(&g_Menu->pShop->linesPortraitHighlight1[i*2 + j]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight1[i*2 + j], 255, 0, 0);
            
            SetLineF3(&g_Menu->pShop->linesPortraitHighlight2[i*2 + j]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight2[i*2 + j], 255, 0, 0);

            setXY3(
                &g_Menu->pShop->linesPortraitHighlight1[i*2 + j],
                D_801D21CC[i], 0xA6,
                D_801D21CC[i] + 0x18, 0xA6,
                D_801D21CC[i] + 0x18, 0xBC
            );

            setXY3(
                &g_Menu->pShop->linesPortraitHighlight2[i*2 + j],
                D_801D21CC[i], 0xA6,
                D_801D21CC[i], 0xBC,
                D_801D21CC[i] + 0x18, 0xBC
            );
        }
        
        g_Menu->pShop->unk469C[i] = 0;
    }

    // White line between gold amounts?
    for (j = 0; j < 2; j++) {
        SetLineF2(&g_Menu->pShop->lines3BF0[j]);
        setRGB0(&g_Menu->pShop->lines3BF0[j], 255, 255, 255);
        setXY2(
            &g_Menu->pShop->lines3BF0[j],
            D_801D2250 - 8, D_801D2254 + 9,
            D_801D2250 + 0x4E, D_801D2254 + 9
        );
    }
}

// Set vertices relative to center of screen?
void ShopMenuSetVertices(SVECTOR* pVertices, u_short x, u_short y, u_short width, u_short height) {
    pVertices[0].vx = x - 160;
    pVertices[0].vy = y - 112;
    pVertices[0].vz = 0;
    
    pVertices[1].vx = x + width - 160;
    pVertices[1].vy = y - 112;
    pVertices[1].vz = 0;
    
    pVertices[2].vx = x - 160;
    pVertices[2].vy = y + height - 112;
    pVertices[2].vz = 0;
    
    pVertices[3].vx = x + width - 160;
    pVertices[3].vy = y + height - 112;
    pVertices[3].vz = 0;
}

void ShopMenuSetWindowBorderPrimitive(POLY_FT4* pPrim) {
    SetSemiTrans(pPrim, 1);
    SetShadeTex(pPrim, 0);
    setRGB0(pPrim, 128, 128, 128);
}

// scrollOffset refers to the index we're currently scrolled down at
void ShopMenuUpdateScrollBarHandle(int x, int y, int scrollHandleHeight, int numItems, int scrollOffset) {
    int yOffset = 0;
    
    if (!g_Menu->pManager->scrollHandleActive) {
        g_Menu->pScrollHandle = HeapAlloc(sizeof(MenuScrollBarHandle), 0x0);
        bzero(g_Menu->pScrollHandle, sizeof(MenuScrollBarHandle));
    }
    
    // If the total number of items in the window fits within what we can view,
    // there's no need for scrolling so we set the scroll handle height to fill
    // the entire scroll bar.
    if (numItems <= MAX_ITEMS_IN_VIEW) {
        scrollHandleHeight = 100;
    }
    // Compute the Y offset based on our current scroll offset
    else {
        yOffset = (scrollOffset * 100) / (numItems - MAX_ITEMS_IN_VIEW);
        yOffset = (yOffset * 4000) / 10000;
    }

    func_8002675C(g_Menu->unk2DC, MENU_TEX_SCROLL_BAR_HANDLE, 
        g_Menu->pScrollHandle->polys, g_Menu->renderContext, 
        x, y, 0x1000
    );
    
    ShopMenuSetVertices(
        g_Menu->pScrollHandle->vertices, 
        x, y + yOffset, 
        8, scrollHandleHeight
    );
    
    g_Menu->pScrollHandle->renderContext = g_Menu->renderContext;
    g_Menu->pManager->scrollHandleActive = TRUE;
}

void ShopMenuFreeScrollBarHandle(void) {
    g_Menu->pManager->scrollHandleActive = FALSE;
    HeapFree(g_Menu->pScrollHandle);
}

void ShopMenuInitializeArrowCursor(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(sizeof(MenuArrowCursor), 0x0);
    bzero(g_Menu->arrowCursors[index], sizeof(MenuArrowCursor));
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

void ShopMenuUpdateArrowCursor(int selectedIndex, int scrollOffset, u_char arg2, u_char cursorIndex) {
    int yOffset;
    MenuArrowCursor* pArrowCursor;
    POLY_FT4* pPoly;

    scrollOffset = 0;
    
    pArrowCursor = g_Menu->arrowCursors[cursorIndex];
    
    // Update animation
    if (++pArrowCursor->animFrameDuration >= 6) {
        pArrowCursor->curAnimFrame--;
        if (pArrowCursor->curAnimFrame < 0) {
            pArrowCursor->curAnimFrame = 4;
        }
        pArrowCursor->animFrameDuration = 0;
    }
    
    if (!arg2) {
        yOffset = (selectedIndex * FONT_LETTER_HEIGHT) + 50;
    }
    
    if (scrollOffset != 1) {
        func_8002675C(
            g_Menu->unk2DC, pArrowCursor->curAnimFrame + MENU_TEX_ARROW_CURSOR, 
            pArrowCursor->polys, g_Menu->renderContext,
            0, 0, 0x1000
        );
        
        pPoly = &pArrowCursor->polys[g_Menu->renderContext];

        ShopMenuSetVertices(
            pArrowCursor->vertices, 
            pPoly->x0 + 28, pPoly->y0 + yOffset, 
            pPoly->x1 - pPoly->x0, pPoly->y3 - pPoly->y0
        );
        pArrowCursor->renderContext = g_Menu->renderContext;
        g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = TRUE;
        return;
    }
    
    g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = FALSE;
}

void ShopMenuFreeArrowCursor(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = FALSE;
}

void ShopMenuInitializeWindowGraphics(u_char index) {
    RECT rect;
    MenuWindow* pWindow;
    u_char i;

    pWindow = g_Menu->windows[index];
    
    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;
    
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = FALSE;
    
    // Window background
    for (i = 0; i < 2; i++) {
        SetPolyG4(&pWindow->polysBackground[i]);
        setRGB0(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB1(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB2(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB3(&pWindow->polysBackground[i], 104, 104, 104);
        SetSemiTrans(&pWindow->polysBackground[i], 1);
        SetDrawMode(
            &pWindow->drawModes[i], 
            0, 0, 
            GetTPage(0, 0, g_Menu->texPageX0, g_Menu->texPageY0), 
            &rect
        );
    }
    
    // Window borders
    for (i = 0; i < 4; i++) {
        SetPolyFT4(&pWindow->polysWindowBorderTop[i]);
        SetShadeTex(&pWindow->polysWindowBorderTop[i], 1);
        setRGB0(&pWindow->polysWindowBorderTop[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderTop[i].tpage = GetTPage(g_Menu->texPage0, 0, g_Menu->texPageX0, g_Menu->texPageY0);
        pWindow->polysWindowBorderTop[i].clut = GetClut(g_Menu->clutX0, g_Menu->clutY0);
        
        SetPolyFT4(&pWindow->polysWindowBorderBottom[i]);
        SetShadeTex(&pWindow->polysWindowBorderBottom[i], 1);
        setRGB0(&pWindow->polysWindowBorderBottom[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderBottom[i].tpage = GetTPage(g_Menu->texPage1, 0, g_Menu->texPageX1, g_Menu->texPageY1);
        pWindow->polysWindowBorderBottom[i].clut = GetClut(g_Menu->clutX1, g_Menu->clutY1);
        
        SetPolyFT4(&pWindow->polysWindowBorderLeft[i]);
        SetShadeTex(&pWindow->polysWindowBorderLeft[i], 1);
        setRGB0(&pWindow->polysWindowBorderLeft[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderLeft[i].tpage = GetTPage(g_Menu->texPage2, 0, g_Menu->texPageX2, g_Menu->texPageY2);
        pWindow->polysWindowBorderLeft[i].clut = GetClut(g_Menu->clutX2, g_Menu->clutY2);
        
        SetPolyFT4(&pWindow->polysWindowBorderRight[i]);
        SetShadeTex(&pWindow->polysWindowBorderRight[i], 1);
        setRGB0(&pWindow->polysWindowBorderRight[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderRight[i].tpage = GetTPage(g_Menu->texPage3, 0, g_Menu->texPageX3, g_Menu->texPageY3);
        pWindow->polysWindowBorderRight[i].clut = GetClut(g_Menu->clutX3, g_Menu->clutY3);
    }
}

void ShopMenuInitializeScrollBar(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];
    
    // Top ornament
    func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_SCROLL_BAR_ORNAMENT, 
        pWindow->polysScrollBarEnds, 
        g_Menu->renderContext, 
        x, y, 0x1000
    );

    // Bottom ornament
    func_800263E4(
        g_Menu->unk2DC, 
        MENU_TEX_SCROLL_BAR_ORNAMENT, 
        &pWindow->polysScrollBarEnds[2], 
        g_Menu->renderContext, 
        x, 
        y + height - 8, 
        0x1000, 0, 1
    );

    func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_SCROLL_BAR_EMPTY, 
        pWindow->polysScrollBarEmpty, 
        g_Menu->renderContext, 
        x, 
        y + 8, 
        0x1000
    );
    
    ShopMenuSetVertices(pWindow->vertsScrollBarEnds, x, y, 8, 8);
    ShopMenuSetVertices(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
    ShopMenuSetVertices(pWindow->vertsScrollBarEmpty, x, y + 8, 8, height - 8);
}

void ShopMenuInitializeWindowBorderCorners(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;

    pWindow = g_Menu->windows[index];

    pWindow->unk710 = 0;
    pWindow->unk710 += func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_WINDOW_BORDER_TOP_LEFT, 
        &pWindow->polysWindowBorderCorners, 
        g_Menu->renderContext, 
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_WINDOW_BORDER_TOP_RIGHT, 
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710], 
        g_Menu->renderContext, 
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_WINDOW_BORDER_BOTTOM_LEFT, 
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710], 
        g_Menu->renderContext, 
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->unk2DC, 
        MENU_TEX_WINDOW_BORDER_BOTTOM_RIGHT, 
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710], 
        g_Menu->renderContext, 
        0, 0, 0x1000
    );
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderCorners, 
        x - 8, 
        y + 8, 
        16, -16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[4], 
        x + width + 8, 
        y + 8, 
        -16, -16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[8], 
        x - 8, 
        y + height - 8, 
        16, 16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[0xC], 
        x + width + 8,
        y + height - 8,
        -16, 16
    );

    for (i = 0; i < 4; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderCorners[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderTop(u_char index, u_short x, u_short y, u_short width) {
    MenuWindow* pWindow;
    int i;
    int innerWidth;
    u_short halfInnerWidth;

    pWindow = g_Menu->windows[index];

    setUV4(
        &pWindow->polysWindowBorderTop[g_Menu->renderContext],
        0, 132,
        7, 132,
        0, 148,
        7, 148
    );
    setUV4(
        &pWindow->polysWindowBorderTop[2 + g_Menu->renderContext],
        0, 132,
        7, 132,
        0, 148,
        7, 148
    );
    
    innerWidth = width - (MENU_WINDOW_BORDER_SIZE * 2);
    halfInnerWidth = innerWidth / 2;
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderTop1, 
        x + MENU_WINDOW_BORDER_SIZE,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderTop2, 
        x + MENU_WINDOW_BORDER_SIZE + halfInnerWidth, 
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth, 
        MENU_WINDOW_BORDER_SIZE * 2
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerWidth;

    pWindow = g_Menu->windows[index];

    setUV4(
        &pWindow->polysWindowBorderBottom[g_Menu->renderContext],
        8, 132,
        15, 132,
        8, 148,
        15, 148
    );

    setUV4(
        &pWindow->polysWindowBorderBottom[2 + g_Menu->renderContext],
        8, 132,
        15, 132,
        8, 148,
        15, 148
    );
    
    innerWidth = width - 16;
    width = innerWidth / 2;
    height = y + height - 8;
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom1, 
        x + 8, 
        height, 
        width, 16
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom2, 
        x + 8 + width, 
        height, 
        width, 16
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;
    
    pWindow = g_Menu->windows[index];

    setUV4(
        &pWindow->polysWindowBorderLeft[g_Menu->renderContext],
        16, 132,
        32, 132,
        16, 139,
        32, 139
    );

    setUV4(
        &pWindow->polysWindowBorderLeft[2 + g_Menu->renderContext],
        16, 132,
        32, 132,
        16, 139,
        32, 139
    );

    innerHeight = height - 16;
    halfInnerHeight = innerHeight / 2;
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft1, 
        x - 8, y + 8, 
        16, halfInnerHeight
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft2, 
        x - 8, y + 8 + halfInnerHeight, 
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderLeft[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;

    pWindow = g_Menu->windows[index];

    setUV4(
        &pWindow->polysWindowBorderRight[g_Menu->renderContext],
        16, 140,
        32, 140,
        16, 147,
        32, 147
    );

    setUV4(
        &pWindow->polysWindowBorderRight[2 + g_Menu->renderContext],
        16, 140,
        32, 140,
        16, 147,
        32, 147
    );
    
    width = x + width - 8;    
    innerHeight = height - 16;
    halfInnerHeight = innerHeight / 2;
    
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderRight1, 
        width, y + 8, 
        16, halfInnerHeight
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderRight2, 
        width, y + 8 + halfInnerHeight,
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderRight[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height, u_char arg5, int zIndex, u_char hasScrollBar) {
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    ShopMenuSetVertices(pWindow->vertsBackground, x, y, width, height);
    ShopMenuInitializeWindowBorderCorners(index, x, y, width, height);
    ShopMenuSetWindowBorderTop(index, x, y, width);
    ShopMenuSetWindowBorderBottom(index, x, y, width, height);
    ShopMenuSetWindowBorderLeft(index, x, y, height);
    ShopMenuSetWindowBorderRight(index, x, y, width, height);
    if (hasScrollBar) {
        ShopMenuInitializeScrollBar(index, x, y, width, height);
    }
    pWindow->hasScrollBar = hasScrollBar;
    pWindow->unk714 = arg5;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[index] = TRUE;
}

void ShopMenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = FALSE;
    HeapFree(g_Menu->windows[index]);
    HeapFree(g_Menu->windowParameters[index]);
}

void ShopMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollBar) {
    MenuWindowParameters* pWindowParams;

    if (windowIndex >= 2) {
        g_Menu->windows[windowIndex] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero(g_Menu->windows[windowIndex], sizeof(MenuWindow));
        g_Menu->windowParameters[windowIndex] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero(g_Menu->windowParameters[windowIndex] , sizeof(MenuWindowParameters));
        ShopMenuInitializeWindowGraphics(windowIndex);
    }
    
    pWindowParams = g_Menu->windowParameters[windowIndex];
    if (shouldInitializeHandle) {
        pWindowParams->index = windowIndex;
        pWindowParams->unk11 = 0;
        pWindowParams->x = x;
        pWindowParams->y = y;
        pWindowParams->width = width;
        pWindowParams->height = height;
        pWindowParams->unk8 = 0;
        pWindowParams->unkA = 0;
        g_Menu->pManager->unk27[windowIndex] = 1;
        pWindowParams->unk12 = arg6;
        pWindowParams->zIndex = zIndex;
        return;
    }
    
    ShopMenuSetWindow(windowIndex, x, y, width, height, arg6, zIndex, hasScrollBar);
}

void ShopMenuUpdateWindows(void) {
    u_char flag;
    int i;
    MenuWindowParameters* pWindowInfo;

    for (i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        pWindowInfo = g_Menu->windowParameters[i];
        if ((g_Menu->pManager->unk27[i]) && pWindowInfo->unk11 == 0) {
            
            flag = 0;
            
            if ((pWindowInfo->unk8 + 32) >= pWindowInfo->width) {
                pWindowInfo->unk8 = pWindowInfo->width;
                flag += 1;
            } else {
                pWindowInfo->unk8 += 32;
            }
            
            if ((pWindowInfo->unkA + 32) >= pWindowInfo->height) {
                pWindowInfo->unkA = pWindowInfo->height;
                flag += 1;
            } else {
                pWindowInfo->unkA += 32;
            }
            
            if (flag == 2) {
                pWindowInfo->unk11 = 1;
            }
            
            ShopMenuSetWindow(
                pWindowInfo->index, 
                (pWindowInfo->x + (pWindowInfo->width / 2)) - (pWindowInfo->unk8 / 2), 
                (pWindowInfo->y + (pWindowInfo->height / 2)) - (pWindowInfo->unkA / 2), 
                pWindowInfo->unk8,  
                pWindowInfo->unkA, 
                pWindowInfo->unk12, 
                pWindowInfo->zIndex, 
                pWindowInfo->hasScrollBar
            );
        }
    }
}

// Project and draw polygons
void ShopMenuRenderPolygons(int numPolygons, SVECTOR* pVertices, POLY_FT4* pPolys, int renderContext) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < numPolygons; i++) {
        RotTransPers4(
            &pVertices[i*4 + 0], 
            &pVertices[i*4 + 1], 
            &pVertices[i*4 + 2], 
            &pVertices[i*4 + 3], 
            &pPolys[i*2 + renderContext].x0, 
            &pPolys[i*2 + renderContext].x1,
            &pPolys[i*2 + renderContext].x2,
            &pPolys[i*2 + renderContext].x3,
            &interpolated, 
            &flag
        );

        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[i*2 + renderContext]);
    }
}

void ShopMenuRenderString(int stringLength, POLY_FT4* pPolys, int renderContext) {
    int i;

    for (i = 0; i < stringLength; i++) {
        AddPrim(
            &g_Menu->pGfxEnv->ot[4], 
            &pPolys[i * 2 + renderContext]
        );
    }
}

void ShopMenuRenderScrollBarHandle(void) {
    if (g_Menu->pManager->scrollHandleActive) {
        ShopMenuRenderPolygons(1, g_Menu->pScrollHandle->vertices, g_Menu->pScrollHandle->polys, g_Menu->pScrollHandle->renderContext);
    }
}

void func_801C8E28(void) {
    AddPrim(
        &g_Menu->pGfxEnv->ot[4], 
        &g_Menu->unk348->drModes1[g_Menu->unk348->unk159]
    );
    
    if (g_Menu->pManager->unk4) {
        AddPrim(
            &g_Menu->pGfxEnv->ot[4], 
            &g_Menu->unk348->polysPointerCursor[g_Menu->unk348->cursorRenderContext]
        );
    }
}

void ShopMenuRenderTopWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    
    RotTransPers4(
        &pWindow->vertsWindowBorderTop1[0], 
        &pWindow->vertsWindowBorderTop1[1], 
        &pWindow->vertsWindowBorderTop1[2], 
        &pWindow->vertsWindowBorderTop1[3], 
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderTop[pWindow->renderContext]
    );
    
    RotTransPers4(
        &pWindow->vertsWindowBorderTop2[0], 
        &pWindow->vertsWindowBorderTop2[1], 
        &pWindow->vertsWindowBorderTop2[2], 
        &pWindow->vertsWindowBorderTop2[3], 
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderTop[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderBottomWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    
    RotTransPers4(
        &pWindow->vertsWindowBorderBottom1[0], 
        &pWindow->vertsWindowBorderBottom1[1], 
        &pWindow->vertsWindowBorderBottom1[2], 
        &pWindow->vertsWindowBorderBottom1[3], 
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderBottom[pWindow->renderContext]
    );
    
    RotTransPers4(
        &pWindow->vertsWindowBorderBottom2[0], 
        &pWindow->vertsWindowBorderBottom2[1], 
        &pWindow->vertsWindowBorderBottom2[2], 
        &pWindow->vertsWindowBorderBottom2[3], 
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderLeftWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    
    RotTransPers4(
        &pWindow->vertsWindowBorderLeft1[0], 
        &pWindow->vertsWindowBorderLeft1[1], 
        &pWindow->vertsWindowBorderLeft1[2], 
        &pWindow->vertsWindowBorderLeft1[3], 
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderLeft[pWindow->renderContext]
    );
    
    RotTransPers4(
        &pWindow->vertsWindowBorderLeft2[0], 
        &pWindow->vertsWindowBorderLeft2[1], 
        &pWindow->vertsWindowBorderLeft2[2], 
        &pWindow->vertsWindowBorderLeft2[3], 
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderRightWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    
    RotTransPers4(
        &pWindow->vertsWindowBorderRight1[0], 
        &pWindow->vertsWindowBorderRight1[1], 
        &pWindow->vertsWindowBorderRight1[2], 
        &pWindow->vertsWindowBorderRight1[3], 
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderRight[pWindow->renderContext]
    );
    
    RotTransPers4(
        &pWindow->vertsWindowBorderRight2[0], 
        &pWindow->vertsWindowBorderRight2[1], 
        &pWindow->vertsWindowBorderRight2[2], 
        &pWindow->vertsWindowBorderRight2[3], 
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x0, 
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x1, 
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x2, 
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x3, 
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysWindowBorderRight[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderWindowBackground(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    
    RotTransPers4(
        &pWindow->vertsBackground[0], 
        &pWindow->vertsBackground[1],
        &pWindow->vertsBackground[2], 
        &pWindow->vertsBackground[3],
        (long*) &pWindow->polysBackground[pWindow->renderContext].x0,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x1,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x2,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x3,
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysBackground[pWindow->renderContext]
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->drawModes[pWindow->renderContext]
    );
}

void ShopMenuRenderWindowBorderCorners(int index) {
    long interpolated;
    long flag;
    int i;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    for (i = 0; i < 4; i++) {
        // Project vertices to x0/y0 coordinates of the polygon
        RotTransPers4(
            &pWindow->vertsWindowBorderCorners[i * 4], 
            &pWindow->vertsWindowBorderCorners[i * 4 + 1],
            &pWindow->vertsWindowBorderCorners[i * 4 + 2], 
            &pWindow->vertsWindowBorderCorners[i * 4 + 3],
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x0, 
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x1,
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x2,
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x3,
            &interpolated, 
            &flag
        );

        // Queue polygon for rendering
        AddPrim(
            &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
            &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext]
        );
    }
}

void ShopMenuRenderScrollBar(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;
    int i;

    pWindow = g_Menu->windows[index];
    
    for (i = 0; i < 2; i++) {
        RotTransPers4(
            &pWindow->vertsScrollBarEnds[i * 4], 
            &pWindow->vertsScrollBarEnds[i * 4 + 1],
            &pWindow->vertsScrollBarEnds[i * 4 + 2],
            &pWindow->vertsScrollBarEnds[i * 4 + 3],
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x0,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x1,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x2,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x3,
            &interpolated, 
            &flag
        );
        AddPrim(
            &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext]
        );
    }
    
    RotTransPers4(
        &pWindow->vertsScrollBarEmpty[0],
        &pWindow->vertsScrollBarEmpty[1],
        &pWindow->vertsScrollBarEmpty[2],
        &pWindow->vertsScrollBarEmpty[3],
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x0,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x1,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x2,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x3,
        &interpolated, 
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex], 
        &pWindow->polysScrollBarEmpty[pWindow->renderContext]
    );
}

void ShopMenuRenderWindows(void) {
    SVECTOR rotation;
    VECTOR translation;
    MATRIX matTransform;
    SVECTOR _unused;
    MenuWindow* pWindow;
    int i;

    // Project and render all active windows
    for (i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        if (g_Menu->pManager->shouldRenderWindow[i]) {
            pWindow = g_Menu->windows[i];
            if (pWindow->unk714 == 0) {
                PushMatrix();
                rotation.vz = 0;
                rotation.vy = 0;
                rotation.vx = 0;
                translation.vy = 0;
                translation.vx = 0;
                translation.vz = 512;
                RotMatrix(&rotation, &matTransform);
                TransMatrix(&matTransform, &translation);
                SetRotMatrix(&matTransform);
                SetTransMatrix(&matTransform);

                ShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) { 
                    ShopMenuRenderScrollBar(i); 
                }
                ShopMenuRenderTopWindowBorder(i);
                ShopMenuRenderBottomWindowBorder(i);
                ShopMenuRenderLeftWindowBorder(i);
                ShopMenuRenderRightWindowBorder(i);
                ShopMenuRenderWindowBackground(i);

                PopMatrix();
            } else {
                ShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) { 
                    ShopMenuRenderScrollBar(i); 
                }
                ShopMenuRenderTopWindowBorder(i);
                ShopMenuRenderBottomWindowBorder(i);
                ShopMenuRenderLeftWindowBorder(i);
                ShopMenuRenderRightWindowBorder(i);
                ShopMenuRenderWindowBackground(i);
            }
        }
    }
}

void ShopMenuRenderPointerCursors(void) {
    int i;

    if (g_Menu->pManager->shouldRenderPointerCursors) {
        for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
            if (g_Menu->pCursors->shouldRender[i]) {
                if (g_Menu->pCursors->unk144[i]) {
                    setXY4(
                        &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]],
                        D_801D2094[D_801D201C[g_Menu->unk32C->unk4F7C]] + 8,
                        D_801D2114[D_801D201C[g_Menu->unk32C->unk4F7C]] - 6,
                        D_801D2094[D_801D201C[g_Menu->unk32C->unk4F7C]] + 24,
                        D_801D2114[D_801D201C[g_Menu->unk32C->unk4F7C]] - 6,
                        D_801D2094[D_801D201C[g_Menu->unk32C->unk4F7C]] + 8,
                        D_801D2114[D_801D201C[g_Menu->unk32C->unk4F7C]] + 10,
                        D_801D2094[D_801D201C[g_Menu->unk32C->unk4F7C]] + 24,
                        D_801D2114[D_801D201C[g_Menu->unk32C->unk4F7C]] + 10  
                    );
                }
                
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4], 
                    &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]]
                );
            }
        }
    }
}

void func_801C9F7C(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk34[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->unk4E0[i].polys[g_Menu->unk4E0[i].renderContext]
            );
        }
    }
}

void func_801CA00C(void) {
    int i;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->unkC[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4], 
                &g_Menu->unk6E0[i].polys[g_Menu->unk6E0[i].renderContext]
            );
        }
    }
}

void func_801CA09C(void) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk14[i]) {
            if (g_Menu->unkAE0[i].unk7F) {
                RotTransPers4(
                    &g_Menu->unkAE0[i].vertices[0], 
                    &g_Menu->unkAE0[i].vertices[1],
                    &g_Menu->unkAE0[i].vertices[2],
                    &g_Menu->unkAE0[i].vertices[3],
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x0, 
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x1,
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x2,
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x3,
                    &interpolated, 
                    &flag
                );
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4], 
                    &g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext]
                );
            } else {
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4], 
                    &g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext]
                );
            }

        }
    }
}

int func_801CA214(void) {
    int i;
    for (i = 5; i >= 0; i--) {}
    return i;
}

// Render confirmation window text?
void func_801CA22C(void) {
    long interpolated;
    long flag;
    MenuString* pString;
    int i;

    if (g_Menu->pManager->unk2E) {
        for (i = 0 ; i < 3; i++) {
            pString = g_Menu->unk1DE0[i];
            if (pString->unk7F) {
                RotTransPers4(
                    &pString->vertices[0], 
                    &pString->vertices[1], 
                    &pString->vertices[2],
                    &pString->vertices[3], 
                    (long*) &pString->polys[pString->renderContext].x0, 
                    (long*) &pString->polys[pString->renderContext].x1, 
                    (long*) &pString->polys[pString->renderContext].x2, 
                    (long*) &pString->polys[pString->renderContext].x3, 
                    &interpolated, 
                    &flag
                );
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
            } else {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
            }
            
        }
    }
}

// Draw the black dimming effect
void ShopMenuRenderBackgroundDim(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->drawModeDimEffect[g_Menu->renderContext]);
}

void func_801CA404(void) {
    func_801C9F7C();
    func_801CA00C();
    func_801CA09C();
    func_801CA214();
    func_801CA22C();
}

void ShopMenuRenderSelectionMenu(void) {
    int i;

    if (g_Menu->pManager->shouldRenderSelectionMenu) {
        if (g_Menu->pSelectionMenu->unk1192 != g_Menu->pSelectionMenu->unk1193) {

            // Render the selection menu as disabled
            if (g_Menu->pSelectionMenu->unk1192) {
                for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 1);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 32, 32, 32);
                }

                for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 1);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 32, 32, 32);
                }

            // Render the seleciton menu as active
            } else {
                for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 128, 128, 128);
                }

                for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 128, 128, 128);
                }
            }
            
            g_Menu->pSelectionMenu->unk1193 = g_Menu->pSelectionMenu->unk1192;
        }

        ShopMenuRenderString(g_Menu->pSelectionMenu->numTexts, g_Menu->pSelectionMenu->polysTexts, g_Menu->pSelectionMenu->textsRenderCtx);
        ShopMenuRenderString(g_Menu->pSelectionMenu->numCursors, g_Menu->pSelectionMenu->polysCursors, g_Menu->pSelectionMenu->cursorsRenderCtx);
    }
}

void func_801CAB0C(void) {
    if (g_Menu->pManager->unkA) {
        ShopMenuRenderString(g_Menu->unk354->unk1404, g_Menu->unk354->polys500, g_Menu->unk354->unk1409);
        ShopMenuRenderString(g_Menu->unk354->unk1400, g_Menu->unk354->polys0, g_Menu->unk354->unk1408);
    }
}

void ShopMenuRenderArrowCursors(void) {
    int i;
    
    for (i = 0; i < MENU_MAX_NUM_ARROW_CURSORS; i++) {
        if (g_Menu->pManager->shouldRenderArrowCursor[i]) {
            ShopMenuRenderPolygons(1, 
                g_Menu->arrowCursors[i]->vertices, 
                g_Menu->arrowCursors[i]->polys, 
                g_Menu->arrowCursors[i]->renderContext
            );
        }
    }
}

void ShopMenuRender(void) {
    if (g_Menu->shouldDrawMenu) {
        ShopMenuUpdateWindows();
        ShopMenuRenderPointerCursors();
        func_801CA404();
        func_801C8E28();
        ShopMenuRenderArrowCursors();
        func_801CCFF4();
        ShopMenuRenderScrollBarHandle();
        ShopMenuRenderSelectionMenu();
        func_801CAB0C();
        ShopMenuRenderWindows();
    }
    ShopMenuRenderBackgroundDim();
}

void ShopMenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 16) | soundEffectId);
    }
}

void ShopMenuPollInput(void) {
    u_char wasControllerUnplugged;
    u_char isLooping;
    int savedValue;
    u_char input;

    input = MENU_INPUT_IDLE;
    isLooping = TRUE;
    wasControllerUnplugged = FALSE;
    while (isLooping) {
        // Is the main controller not plugged in?
        if (ControllerGetType(0) == CONTROLLER_TYPE_NONE) {
            if (wasControllerUnplugged == 0) {
                wasControllerUnplugged++;
                SoundMuteAllSpuChannels();
                savedValue = D_80059488;
            }
            continue;
        }
        
        isLooping--;
        if (wasControllerUnplugged) {
            SoundEnableAllSpuChannels();
            D_80059488 = savedValue;
        }
    }
    
    if (func_80036410()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
                input = MENU_INPUT_RIGHT;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = MENU_INPUT_DOWN;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = MENU_INPUT_LEFT;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = MENU_INPUT_UP;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = MENU_INPUT_CONFIRM;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = MENU_INPUT_BACK;
                ShopMenuPlaySoundEffect(3);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SQUARE) {
                input = 6;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_TRIANGLE) {
                input = 7;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_L1) {
                input = 10;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_R1) {
                input = 9;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_START) {
                input = 11;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SELECT) {
                g_Menu->unk1E94 = g_Menu->unk1E94 == 0;
                input = 12;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_L2) {
                g_Menu->unk1E95 += 1;
                break;
            }
        }
    }
    g_Menu->input = input;
}

// Updates the effect where the menu is rotated and zoomed in/out
void ShopMenuUpdateTransitionEffect(void) {
    switch (g_Menu->transitionEffectState) {
        case MENU_CLOSE_ANIMATION_START:
            g_Menu->translation.vz = 512;
            g_Menu->rotation.vz = 0;
            g_Menu->rotation.vy = 0;
            g_Menu->rotation.vx = 0;
            g_Menu->translation.vy = 0;
            g_Menu->translation.vx = 0;
            g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION;
            break;
        case MENU_OPEN_ANIMATION_START:
            // Set window to the far back
            g_Menu->translation.vz = 2048;
            g_Menu->rotation.vz = 0;
            g_Menu->rotation.vy = 0;
            g_Menu->rotation.vx = 0;
            g_Menu->translation.vy = 0;
            g_Menu->translation.vx = 0;
            g_Menu->transitionEffectState = MENU_OPEN_ANIMATION;
            break;
        case MENU_CLOSE_ANIMATION:
            g_Menu->rotation.vy -= 96;
            g_Menu->translation.vz += 64;
            if (g_Menu->translation.vz >= PSX_DEGREES(315)) {
                g_Menu->transitionEffectState = MENU_ANIMATION_DONE;
            }
            break;
        case MENU_OPEN_ANIMATION:
            // Flip the window around the X-axis while
            // bring it closer to the viewer, giving and
            // effect of zooming in.
            g_Menu->rotation.vx += 124;
            g_Menu->translation.vz -= 48;
            if (g_Menu->translation.vz < 512) {
                g_Menu->translation.vz = 512;
                g_Menu->rotation.vz = 0;
                g_Menu->rotation.vx = 0;
                g_Menu->rotation.vy = 0;
                g_Menu->transitionEffectState = MENU_ANIMATION_DONE;
            }
            break;
    }

    RotMatrix(&g_Menu->rotation, &g_Menu->matTransform);
    TransMatrix(&g_Menu->matTransform, &g_Menu->translation);
    SetRotMatrix(&g_Menu->matTransform);
    SetTransMatrix(&g_Menu->matTransform);
}

void ShopMenuUpdateAndRender(void) {
    GfxEnvironment* pNextGfxEnv;
    int renderContext;

    // Debugging
    if (*D_8005917C != -1) {
#ifndef XENO_PC_PORT
        asm("break 0x400");   /* MIPS trap; host assembler can't emit it (debug-only) */
#endif
    }
    
    ShopMenuPollInput();
    GameCheckAndHandleSoftReset();

    // Rotate graphics environment
    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;
    
    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    ShopMenuUpdateTransitionEffect();
    ShopMenuRender();
    renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->unk1180, 0, renderContext * 0xE0);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
}

void ShopMenuInitializePointerCursors(u_char mode) {
    int i;

    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));
    
    switch (mode) {
        case 0:
            g_Menu->pManager->shouldRenderPointerCursors = TRUE;
            g_Menu->pCursors->unk144[0] = TRUE;
            g_Menu->pCursors->unk144[1] = TRUE;
            /* fallthrough */
        case 2:
            for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
                func_8002675C(
                    g_Menu->unk2DC, MENU_TEX_POINTER_CURSOR, 
                    &g_Menu->pCursors->polysCursor[i * 2], g_Menu->renderContext, 
                    D_801D1FF8[i], D_801D2008[i],
                    0x800
                );
                g_Menu->pCursors->renderContexts[i] = g_Menu->renderContext;
            }
            return;
        case 3:
            func_8002675C(
                g_Menu->unk2DC, MENU_TEX_POINTER_CURSOR, 
                &g_Menu->pCursors->polysCursor[0], g_Menu->renderContext, 
                0, 0, 
                0x800
            );
            g_Menu->pCursors->renderContexts[0] = g_Menu->renderContext;
            g_Menu->pManager->shouldRenderPointerCursors = TRUE;
            return;
        case 1:
            return;
    }
}

void ShopMenuFreePointerCursors(void) {
    g_Menu->pManager->shouldRenderPointerCursors = FALSE;
    ShopMenuUpdateAndRender();
    HeapFree(g_Menu->pCursors);
}

void ShopMenuStartOpenMenuTransition(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    ShopMenuPlaySoundEffect(0x5B);
}

void ShopMenuStartCloseMenuTransition(void) {
    g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION_START;
}

void ShopMenuConfirmationWindowInitialize(u_char stringIndex) {
    MenuWindowParameters* pWindowParams;
    MenuString* pString;
    int i;
    int xPosition;
    
    xPosition = 0x50; 
    
    ShopMenuInitializeWindow(4, 0x42, 0x46, 0xBC, 0x40, 1, 1, 4, 0);
    pWindowParams = g_Menu->windowParameters[4];
    while (!pWindowParams->unk11) {
        ShopMenuUpdateAndRender();
    }

    for (i = 0; i < 4; i++) {
        g_Menu->unk1DE0[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero(g_Menu->unk1DE0[i], sizeof(MenuString));

        // Even indices
        if (!(i & 1)) {
            // Buffer size = FONT_LETTER_HEIGHT * 57 * 2
            g_Menu->unk1DE0[i]->pVramBuffer = HeapAlloc(0x5CA, 0);
            g_Menu->unk1DE0[i]->vramDest.x = 320;
            g_Menu->unk1DE0[i]->vramDest.y = ((i / 2) * FONT_LETTER_HEIGHT) + 78;
            g_Menu->unk1DE0[i]->vramDest.w = 58;
            g_Menu->unk1DE0[i]->vramDest.h = FONT_LETTER_HEIGHT;
            continue;
        }

        // Odd indices, share VRAM buffer w/ prev entry
        g_Menu->unk1DE0[i]->pVramBuffer = g_Menu->unk1DE0[i - 1]->pVramBuffer;
    }

    // Render strings to text areas
    for (i = 0; i < 3; i++) {
        pString = g_Menu->unk1DE0[i];
        pString->width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, stringIndex + i), 
            pString->pVramBuffer, 0x36, i % 2
        );
        func_801C5A7C(pString, i, 0, 0);
        ShopMenuSetVertices(pString->vertices, xPosition, (i * 16) + 0x50, pString->width, FONT_LETTER_HEIGHT);
        setUV4(
            &pString->polys[g_Menu->renderContext],
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x5B,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x5B
        );
        pString->renderContext = g_Menu->renderContext;
        pString->unk7F = 1;
    }

    // Transfer text areas to VRAM
    LoadImage(&g_Menu->unk1DE0[0]->vramDest, g_Menu->unk1DE0[0]->pVramBuffer);
    LoadImage(&g_Menu->unk1DE0[2]->vramDest, g_Menu->unk1DE0[2]->pVramBuffer);
    DrawSync(0);
    
    g_Menu->pManager->unk2E = 1;
    HeapFree(g_Menu->unk1DE0[0]->pVramBuffer);
    HeapFree(g_Menu->unk1DE0[2]->pVramBuffer);
    if (g_Menu->pManager->unk5B == 2) {
        g_Menu->pManager->unk5B = 1;
    }
    ShopMenuUpdateAndRender();
    ShopMenuUpdateAndRender();
}

void ShopMenuConfirmationWindowFree(void) {
    int i;

    if (g_Menu->pManager->shouldRenderWindow[4]) {
        ShopMenuFreeWindow(4);
        g_Menu->pManager->unk2E = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->unk1DE0[i]);
        }
    }
    g_Menu->pManager->unk5B = 0;
    ShopMenuUpdateAndRender();
}

u_char ShopMenuConfirmationWindowGetChoice(u_char mode) {
    u_char choice;
    u_char isRunning;
    u_char curTimer;
    
    isRunning = TRUE;
    choice = MENU_CHOICE_NO;
    curTimer = 60;

    while (isRunning) {
        if (mode == MENU_AUTO_ADVANCE) {
            g_Menu->pCursors->shouldRender[2] = FALSE;
            g_Menu->pCursors->shouldRender[3] = FALSE;

            // Was any button pressed?
            if (g_Menu->input != MENU_INPUT_IDLE) 
                break;
            
            curTimer--;
            if (curTimer == 0) 
                break;
        }

        ShopMenuUpdateAndRender();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                isRunning = FALSE;
                break;
            case MENU_INPUT_BACK:
                choice = MENU_CHOICE_NO;
                isRunning = FALSE;
                break;
            case MENU_INPUT_LEFT:
                g_Menu->pCursors->shouldRender[2] = TRUE;
                g_Menu->pCursors->shouldRender[3] = FALSE;
                choice = MENU_CHOICE_YES;
                break;
            case MENU_INPUT_RIGHT:
                g_Menu->pCursors->shouldRender[2] = FALSE;
                g_Menu->pCursors->shouldRender[3] = TRUE;
                choice = MENU_CHOICE_NO;
                break;
        }    
    }
    
    g_Menu->pCursors->shouldRender[2] = FALSE;
    g_Menu->pCursors->shouldRender[3] = FALSE;

    POSSIBLE_DEBUG_CODE;
    
    return choice;
}

int ShopMenuConfirmationWindow(u_char stringIndex, u_char stringIndex2, u_char mode) {
    u_char choice;

    ShopMenuConfirmationWindowInitialize(stringIndex);
    g_Menu->pCursors->shouldRender[3] = TRUE;
    choice = ShopMenuConfirmationWindowGetChoice(mode);
    ShopMenuConfirmationWindowFree();
    
    // If we chose yes and have another window, run that confirmation window as well
    if (stringIndex2 != 0xFF) {
        if (choice != MENU_CHOICE_NO) {
            ShopMenuConfirmationWindowInitialize(stringIndex2);
            g_Menu->pCursors->shouldRender[3] = TRUE;
            choice = ShopMenuConfirmationWindowGetChoice(mode);
            ShopMenuConfirmationWindowFree();
        }
    } 
    return choice;
}

void ShopMenuFree(void) {
    ShopMenuUpdateAndRender();
    ShopMenuUpdateAndRender();
    g_Menu->shouldDrawMenu = FALSE;
    ShopMenuUpdateAndRender();
    do {
        ShopMenuUpdateAndRender();
    } while (g_Menu->renderContext);
    ShopMenuUnk32CManager(MENU_DATA_FREE);
    ShopMenuSetManager(MENU_DATA_FREE);
    ShopMenuSelectionMenuManager(MENU_DATA_FREE);
    ShopMenuUnk354Manager(MENU_DATA_FREE);
    ShopMenuUnk330Manager(MENU_DATA_FREE);
    ShopMenuUnk348Manager(MENU_DATA_FREE);
    ShopMenuLoadShopItemsData(0x10);
    ShopMenuShopManager(MENU_DATA_FREE);
    HeapFree(g_Menu->unk2DC);
    HeapFree(g_Menu->unk2E0);
    HeapFree(g_Menu->unk4E0[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        func_8003A094(g_Menu->unk2E4);
        ShopMenuUpdateAndRender();
        func_8003852C(g_Menu->unk2E4);
        ShopMenuUpdateAndRender();
        HeapFree(g_Menu->unk2E4);
    }
    HeapFree(g_Menu->pShopEntries);
    ShopMenuUnk1E20Manager(MENU_DATA_FREE);
    HeapFree(g_Menu);
}

void func_801CBC88(u8 a0, u8 a1, void* a2, void* a3, u8* pClear) {
    s32 i;
    if (a0) {
        func_801C5CBC(a2, a3, 2, a1);
    } else {
        for (i = 0; i < a1; i++) {
            pClear[i] = 0;
        }
    }
}
