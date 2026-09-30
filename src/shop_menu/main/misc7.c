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
extern void* GetWeaponName(s32);
extern void* GetAccessoryName(s32);
extern void* GetItemName(s32);
extern s32 SystemRenderStringEntry(void*, void*, s32, s32);
extern void func_80033B34(u16*, u8*, s32);
extern void func_801C5A7C(MenuString*, s32, s32, u8);
extern void ShopMenuSetVertices(SVECTOR*, u16, u16, u16, u16);
extern s32 func_8002675C(void*, s32, POLY_FT4*, s32, s32, s32, s32);

/* Retail 801CDD14..801CE480. ShopMenuInitializeShopData produces types0..2.
 * Keep the two uploads and duplicate first-record context store: both are
 * present in retail. No inventory or funds are changed by this builder. */
void func_801CDD14(s32 firstItem, s32 gold, u8* affordable) {
    void* work = HeapAlloc(0x3F6, 0);
    s32 row;
    for (row = 0; row < 8; ++row) {
        s32 item = firstItem + row;
        MenuShop* shop = g_Menu->pShop;
        MenuString* name = &shop->strings3C30[row];
        MenuString* cost = &shop->strings4030[row];
        u8 id = g_Menu->shopItemIDs[item];
        u16 digits[7] = {0};
        u8 priceString[16];
        s32 price;
        s32 remaining, divisor, digit;
        s32 started = 0;
        RECT rect;
        affordable[row] = 0;
        shop->unk468C[row] = 0;
        if (!id) {
            shop->unk4684[row] = 0;
            continue;
        }
        switch (g_Menu->shopItemTypes[item]) {
        case 0:
            name->width = SystemRenderStringEntry(GetWeaponName(id), work, 0x24, 0);
            price = g_Menu->unk330->pWeaponsData[id].price;
            break;
        case 1:
            name->width = SystemRenderStringEntry(GetAccessoryName(id), work, 0x24, 0);
            price = g_Menu->unk330->pAccessoriesData[id].price;
            break;
        case 2:
            name->width = SystemRenderStringEntry(GetItemName(id), work, 0x24, 0);
            price = g_Menu->unk330->pItemsData[id].price;
            break;
        }
        affordable[row] = gold >= price ? 0x80 : 0;
        remaining = price;
        divisor = 10000;
        for (digit = 0; digit < 4; ++digit) {
            s32 value = remaining / divisor;
            if (value || started) {
                digits[digit] = value + 0x10;
                remaining -= value * divisor;
                started = 1;
            } else {
                digits[digit] = 0xC3;
            }
            divisor /= 10;
        }
        digits[4] = remaining % 10 + 0x10;
        func_80033B34(digits, priceString, 5);
        cost->width = SystemRenderStringEntry(priceString, work, 0x24, 1);
        rect.x = 384 + (row & 1) * 24;
        rect.y = 128 + (row / 2) * 13;
        rect.w = 40;
        rect.h = 13;
        LoadImage(&rect, work);
        func_801C5A7C(name, row, 128, affordable[row] + 1);
        ShopMenuSetVertices(name->vertices, 36, 50 + row * 13, name->width, 13);
        LoadImage(&rect, work);
        DrawSync(0);
        func_801C5A7C(cost, row, 128, affordable[row] + 2);
        ShopMenuSetVertices(cost->vertices, 140, 50 + row * 13, cost->width, 13);
        name->renderContext = (u8)g_Menu->renderContext;
        name->renderContext = (u8)g_Menu->renderContext;
        shop->unk4684[row] = 1;
        if (shop->curItemQuantities[item]) {
            POLY_FT4* polys = (POLY_FT4*)(shop->unk1DB0 + row * 0x140);
            u8 quantity = shop->curItemQuantities[item];
            shop->unk468C[row] += func_8002675C(g_Menu->unk2DC, 0xF1, polys,
                g_Menu->renderContext, 180, 54 + row * 13, 0x1000);
            if (quantity / 10)
                shop->unk468C[row] += func_8002675C(g_Menu->unk2DC, quantity / 10,
                    &polys[shop->unk468C[row] * 2], g_Menu->renderContext,
                    188, 54 + row * 13, 0x1000);
            shop->unk468C[row] += func_8002675C(g_Menu->unk2DC, quantity % 10,
                &polys[shop->unk468C[row] * 2], g_Menu->renderContext,
                196, 54 + row * 13, 0x1000);
            shop->unk4694[row] = (u8)g_Menu->renderContext;
        }
    }
    HeapFree(work);
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc7", func_801CDD14);
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc7", func_801CE480);
#endif

/* Retail 801CE8D8..801CE91C: nonpositive counts return before touching either
 * array. Keep the signed end-address comparison and native pointer width. */
u8 func_801CE8D8(u8* pKeys, u8* pValues, s32 count, s32 target) {
    s32 result = 0;
    intptr_t end;
    if (count > 0) {
        target &= 0xFF;
        end = count + (intptr_t)pValues;
        while (1) {
            if (*pKeys == target) {
                result = *pValues;
                break;
            }
            pValues++;
            pKeys++;
            if ((intptr_t)pValues >= end) {
                break;
            }
        }
    }
    return (u8)result;
}
