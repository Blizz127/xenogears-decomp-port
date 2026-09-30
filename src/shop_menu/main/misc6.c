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
extern void ShopMenuRenderString(int, POLY_FT4*, int);
extern void ShopMenuRenderPolygons(int, SVECTOR*, POLY_FT4*, int);

/* Retail 801CCFF4..801CD404. Keep draw order and independent price gate;
 * native members account for pointer expansion in embedded MenuStrings. */
void func_801CCFF4(void) {
    MenuShop* shop = g_Menu->pShop;
    s32 i;
    if (g_Menu->pManager->unk5A) {
        for (i = 0; i < 9; ++i) {
            if (shop->unk469C[i]) {
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        &shop->linesPortraitHighlight1[i * 2 + g_Menu->renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        &shop->linesPortraitHighlight2[i * 2 + g_Menu->renderContext]);
            }
        }
        ShopMenuRenderString(shop->explanationsLen, shop->polysExplanations, shop->explanationsRenderCtx);
        ShopMenuRenderString(shop->unk46A9, shop->polys2D0, shop->unk46A8);
        ShopMenuRenderString(shop->numPortraits, shop->polysCharacterPortraits, shop->portraitsRenderCtx);
        for (i = 0; i < 8; ++i) {
            if (shop->unk4684[i]) {
                MenuString* first = &shop->strings3C30[i];
                MenuString* second = &shop->strings4030[i];
                ShopMenuRenderPolygons(1, first->vertices, first->polys, first->renderContext);
                ShopMenuRenderPolygons(1, second->vertices, second->polys, second->renderContext);
            }
        }
        if (shop->unk46A7)
            ShopMenuRenderPolygons(1, shop->strItemDesc.vertices, shop->strItemDesc.polys, shop->strItemDesc.renderContext);
        if (shop->unk46B5)
            ShopMenuRenderPolygons(1, shop->str44B0.vertices, shop->str44B0.polys, shop->str44B0.renderContext);
        if (shop->unk4785)
            ShopMenuRenderPolygons(1, shop->str45B0.vertices, shop->str45B0.polys, shop->str45B0.renderContext);
        if (shop->unk46B2) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &shop->lines3BF0[g_Menu->renderContext]);
            ShopMenuRenderString(shop->goldBeforeStrLen, shop->polysGoldBefore, shop->goldBeforeRenderCtx);
            ShopMenuRenderString(shop->totalPriceStrLen, shop->polysTotalPrice, shop->totalPriceRenderCtx);
            ShopMenuRenderString(shop->goldAfterStrLen, shop->polysGoldAfter, shop->goldAfterRenderCtx);
        }
        for (i = 0; i < 8; ++i)
            ShopMenuRenderString(shop->unk468C[i], (POLY_FT4*)(shop->unk1DB0 + i * 0x140), shop->unk4694[i]);
        for (i = 0; i < 9; ++i) {
            ShopMenuRenderString(shop->unk46BC[i], &shop->polys27B0[i * 6], shop->unk46CE[i]);
            ShopMenuRenderString(shop->unk46C5[i], &shop->polys3020[i * 6], shop->unk46D7[i]);
        }
    }
    if (g_Menu->pManager->unk5B == 1)
        ShopMenuRenderString(shop->finalPriceStrLen, shop->polysFinalPrice, shop->finalPriceRenderCtx);
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc6", func_801CCFF4);
#endif

// Set the color for the stat change a piece of gear provides
void ShopMenuSetStatChangeColor(int numPolygons, POLY_FT4* pPolys, u_char mode) {
    int i;

    for (i = 0; i < numPolygons; i++) {
        SetShadeTex(&pPolys[i*2 + g_Menu->renderContext], 0);
        switch (mode) {
            case MENU_STAT_CHANGE_INCREASE:
                // Red
                setRGB0(&pPolys[i*2 + g_Menu->renderContext], 128, 64, 64);
                break;
            case MENU_STAT_CHANGE_DECREASE:
                // Blue
                setRGB0(&pPolys[i*2 + g_Menu->renderContext], 64, 64, 128);
                break;
        }
    }
}

void ShopMenuUpdateCharacterPortraits(void) {
    int i;
    int characterIndex;
    int numCharacters;

    g_Menu->pManager->unk5A = 1;
    
    // This loop seems horribly inefficient, because for each character index, all
    // portraits are recomputed and the entire menu redrawn...
    for (i = 1; i < MAX_GAME_CHARACTERS + 1; i++) {
        g_Menu->pShop->numPortraits = 0;
        
        numCharacters = 0;
        for (characterIndex = 0; characterIndex < i; characterIndex++) {
            if (g_Menu->availableCharacters[characterIndex]) {
                g_Menu->pShop->numPortraits += func_8002675C(
                    g_Menu->unk2DC, characterIndex + MENU_TEX_CHARACTER_PORTRAITS_SMALL, 
                    &g_Menu->pShop->polysCharacterPortraits[numCharacters * 2], 
                    g_Menu->renderContext, 
                    D_801D21CC[numCharacters], 166, 
                    0x1000
                );
                numCharacters++;
            }
        }
        
        g_Menu->pShop->portraitsRenderCtx = g_Menu->renderContext;
        ShopMenuUpdateAndRender();
    }
}

// Explanations are strings such as "Attack", "Defense", the "- <==> +" graphics.
void ShopMenuUpdateBuyMenuExplanationGraphics(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for (i = 0; i < 4; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->unk2DC, D_801D2210[i], 
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext, 
            D_801D2218[i], D_801D2230[i], 
            0x1000
        );
    }
    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void ShopMenuUpdateSellMenuExplanationGraphics(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for (i = 0; i < 2; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->unk2DC, D_801D2214[i], 
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext, 
            D_801D2228[i], D_801D2240[i], 
            0x1000
        );
    }
    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void ShopMenuUpdateGoldGraphics(u_int goldBefore, u_int totalPrice, u_int goldAfter) {
    int i;
    u_char digit;

    ShopMenuParseNumberToString(goldBefore);
    g_Menu->pShop->goldBeforeStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->goldBeforeStrLen += func_8002675C(
                g_Menu->unk2DC, digit, 
                &g_Menu->pShop->polysGoldBefore[g_Menu->pShop->goldBeforeStrLen * 2], 
                g_Menu->renderContext, 
                (i * 8) + D_801D2248, D_801D224C, 
                0x1000
            );
        }
    }
    g_Menu->pShop->goldBeforeRenderCtx = g_Menu->renderContext;
    
    ShopMenuParseNumberToString(totalPrice);
    g_Menu->pShop->totalPriceStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->totalPriceStrLen += func_8002675C(
                g_Menu->unk2DC, digit, 
                &g_Menu->pShop->polysTotalPrice[g_Menu->pShop->totalPriceStrLen * 2], 
                g_Menu->renderContext, 
                (i * 8) + D_801D2250, D_801D2254, 
                0x1000
            );
        }
    }
    g_Menu->pShop->totalPriceRenderCtx = g_Menu->renderContext;
    
    ShopMenuParseNumberToString(goldAfter);
    g_Menu->pShop->goldAfterStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->goldAfterStrLen += func_8002675C(
                g_Menu->unk2DC, digit, 
                &g_Menu->pShop->polysGoldAfter[g_Menu->pShop->goldAfterStrLen * 2], 
                g_Menu->renderContext, 
                (i * 8) + D_801D2258, D_801D225C, 
                0x1000
            );
        }
    }
    g_Menu->pShop->goldAfterRenderCtx = g_Menu->renderContext;
    
    g_Menu->pShop->unk46B2 = 1;
}

u_short ShopMenuGetCharacterEquippedItemFlags(u_char itemID, u_char itemType) {
    int i;
    int j;
    u_char isItemEquipped;
    u_short equippedFlags;
    
    equippedFlags = 0;
    
    if (itemID) {
        if (itemType != ITEM_TYPE_ITEM) {
            for (i = 0; i < 0x10; i++) {
                isItemEquipped = FALSE;

                if (g_Menu->availableCharacters[i]) {
                    switch (itemType) {
                        case ITEM_TYPE_WEAPON:
                            for (j = 0; j < 5; j++) {
                                if (itemID < 50) {
                                    if (g_GameState.characters[i].unk6A[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                } else {
                                    if (g_GameState.characters[i].unk6F[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                }
                            }
                            break;
                        
                        case ITEM_TYPE_ACCESSORY:
                            for (j = 0; j < 3; j++) {
                                if (g_GameState.characters[i].unk74[j] == itemID) {
                                    isItemEquipped = TRUE;
                                    break;
                                }
                            }
                            break;
                    }
                }
                
                if (isItemEquipped) {
                    equippedFlags |= ShopMenuGetCharacterBitMask(i);
                }
            }
        }   
    }

    return equippedFlags;
}
