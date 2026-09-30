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
extern u8 func_801CE8D8(u8*, u8*, s32, s32);
extern void func_80033B34(u16*, u8*, s32);
extern s32 SystemRenderStringEntry(void*, void*, s32, s32);
extern void func_801C5A7C(MenuString*, s32, s32, u8);
extern void ShopMenuSetVertices(SVECTOR*, u16, u16, u16, u16);

/* Retail 801CE91C..801CEB3C: inventory lookup and the Stored count string.
 * Shop initialization supplies item types 0..2. Use native inventory/string
 * fields because their host pointer layout differs from retail. */
void func_801CE91C(u8 type, s32 id) {
    u8* ids;
    u8* quantities;
    s32 count;
    u16 quantity;
    u16 digits[2];
    u8 text[8];
    void* work;
    MenuString* str = &g_Menu->pShop->str45B0;
    RECT rect = {408, 180, 40, 13};

    switch (type) {
        case 0:
            ids = g_GameState.weaponIDs;
            quantities = g_GameState.weaponQuantities;
            count = MAX_INVENTORY_WEAPONS;
            break;
        case 1:
            ids = g_GameState.accessoryIDs;
            quantities = g_GameState.accessoryQuantities;
            count = MAX_INVENTORY_ACCESSORIES;
            break;
        case 2:
            ids = g_GameState.itemIDs;
            quantities = g_GameState.itemQuantities;
            count = MAX_INVENTORY_ITEMS;
            break;
    }
    quantity = func_801CE8D8(ids, quantities, count, (u8)id);
    D_801D2260 = quantity;
    work = HeapAlloc(0x3F6, 0);
    digits[0] = quantity / 10 ? (u8)(quantity / 10 + 0x10) : 0xC3;
    digits[1] = quantity % 10 + 0x10;
    func_80033B34(digits, text, 2);
    str->width = SystemRenderStringEntry(text, work, 0x24, 1);
    LoadImage(&rect, work);
    DrawSync(0);
    func_801C5A7C(str, 9, 0x80, 0x82);
    ShopMenuSetVertices(str->vertices, 248, 142, str->width, 13);
    str->renderContext = (u8)g_Menu->renderContext;
    g_Menu->pShop->unk4785 = 1;
    HeapFree(work);
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc8", func_801CE91C);
#endif
#ifdef XENO_PC_PORT
extern void* GetStringEntry(void*, s32);
extern u_short ShopMenuGetCharacterEquippedItemFlags(u_char, u_char);
extern u_short ShopMenuIsCharacterFlagSet(u_short, u_char);
extern void func_801C5040(POLY_FT4*, short, short, u_char, u_char, short, short);
extern s32 func_8002675C(void*, s32, POLY_FT4*, s32, s32, s32, s32);
extern void func_801CE480(s32*, u8*, u8, u8, u8);
extern void ShopMenuParseNumberToString(unsigned int);
extern void ShopMenuSetStatChangeColor(int, POLY_FT4*, u_char);

/* Retail 801CEB3C..801CF2A0. The third caller argument is unused in retail. */
s32 func_801CEB3C(s32 row, s32 scrollOffset, u8* affordable) {
    MenuShop* shop = g_Menu->pShop;
    MenuString* desc = &shop->strItemDesc;
    u8 id = g_Menu->shopItemIDs[scrollOffset + row];
    u8 type = g_Menu->shopItemTypes[scrollOffset + row];
    u16 equipFlags = 0;
    u16 equipped;
    s32 price;
    s32 character, portrait = 0;
    void* work = HeapAlloc(0x618, 0);
    RECT rect = {320, 78, 60, 13};
    (void)affordable;
    bzero(work, 0x618);
    switch (type) {
        case 0:
            desc->width = SystemRenderStringEntry(GetStringEntry(shop->pWeaponDescriptions, id), work, 0x39, 0);
            price = g_Menu->unk330->pWeaponsData[id].price;
            equipFlags = g_Menu->unk330->pWeaponsData[id].equipFlags;
            break;
        case 1:
            desc->width = SystemRenderStringEntry(GetStringEntry(shop->pAccessoryDescriptions, id), work, 0x39, 0);
            price = g_Menu->unk330->pAccessoriesData[id].price;
            equipFlags = g_Menu->unk330->pAccessoriesData[id].equipFlags;
            break;
        case 2:
            desc->width = SystemRenderStringEntry(GetStringEntry(shop->pItemDescriptions, id), work, 0x39, 0);
            price = g_Menu->unk330->pItemsData[id].price;
            break;
    }
    equipped = ShopMenuGetCharacterEquippedItemFlags(id, type);
    LoadImage(&rect, work);
    DrawSync(0);
    func_801C5A7C(desc, 0, 0, 0);
    func_801C5040(&desc->polys[g_Menu->renderContext], 44, 18, 0, 78, desc->width, 13);
    ShopMenuSetVertices(desc->vertices, 44, 18, desc->width, 13);
    desc->renderContext = (u8)g_Menu->renderContext;
    HeapFree(work);
    shop->unk46A7 = id != 0;
    shop->unk46A9 = 0;
    for (character = 0; character < 16; character++) {
        if (g_Menu->availableCharacters[character]) {
            shop->unk469C[portrait] = ShopMenuIsCharacterFlagSet(equipFlags, character) != 0;
            if (ShopMenuIsCharacterFlagSet(equipped, character)) {
                shop->unk46A9 += func_8002675C(g_Menu->unk2DC, 0xE,
                    &shop->polys2D0[shop->unk46A9 * 2], g_Menu->renderContext,
                    D_801D21CC[portrait] + 14, 180, 0x1000);
            }
            shop->unk46BC[portrait] = 0;
            shop->unk46C5[portrait] = 0;
            if (shop->unk469C[portrait]) {
                s32 changes[2] = {0, 0};
                u8 colors[2];
                s32 stat, digit;
                func_801CE480(changes, colors, id, type, character);
                for (stat = 0; stat < 2; stat++) {
                    POLY_FT4* polys = stat ? &shop->polys3020[portrait * 6] : &shop->polys27B0[portrait * 6];
                    u8* length = stat ? &shop->unk46C5[portrait] : &shop->unk46BC[portrait];
                    u8* context = stat ? &shop->unk46D7[portrait] : &shop->unk46CE[portrait];
                    if (changes[stat] != 0) {
                        ShopMenuParseNumberToString(changes[stat]);
                        for (digit = 0; digit < 3; digit++) {
                            /* Retail uses the last three of the nine digits. */
                            u8 code = g_Menu->digits[6 + digit];
                            if (code != 0xFF) {
                                *length += func_8002675C(g_Menu->unk2DC, code,
                                    &polys[*length * 2], g_Menu->renderContext,
                                    73 + portrait * 26 + digit * 8, stat ? 198 : 190, 0x1000);
                            }
                        }
                        ShopMenuSetStatChangeColor(*length, polys, colors[stat]);
                        *context = (u8)g_Menu->renderContext;
                    }
                }
            }
            portrait++;
        }
    }
    shop->unk46A8 = (u8)g_Menu->renderContext;
    func_801CE91C(type, id);
    return price;
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc8", func_801CEB3C);
#endif

void ShopMenuHandleBoughtItems(u_int newGoldnewWeaponAmount) {
    int i;
    int j;
    u_char itemNotInInventory;
    u_char newWeaponAmount;
    u_char newAccessorynewWeaponAmount;
    u_char newItemnewWeaponAmount;

    ShopMenuPlaySoundEffect(0xD1);
    
    g_GameState.gold = newGoldnewWeaponAmount;
    if (newGoldnewWeaponAmount > 9999999) {
        g_GameState.gold = 9999999;
    }
    
    for (i = 0; i < MAX_SHOP_ITEMS; i++) {
        // Did we buy any of this item?
        if (g_Menu->shopItemIDs[i] && g_Menu->pShop->curItemQuantities[i]) {
            switch (g_Menu->shopItemTypes[i]) {
                case ITEM_TYPE_WEAPON:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_WEAPONS; j++) {
                        if (g_GameState.weaponIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newWeaponAmount = g_GameState.weaponQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.weaponQuantities[j] = newWeaponAmount;
                            if (newWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.weaponQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_WEAPONS; j++) {
                            if (g_GameState.weaponIDs[j] == 0) {
                                g_GameState.weaponIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.weaponQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;

                case ITEM_TYPE_ACCESSORY:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_ACCESSORIES; j++) {
                        if (g_GameState.accessoryIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newAccessorynewWeaponAmount = g_GameState.accessoryQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.accessoryQuantities[j] = newAccessorynewWeaponAmount;
                            if (newAccessorynewWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.accessoryQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_ACCESSORIES; j++) {
                            if (g_GameState.accessoryIDs[j] == 0) {
                                g_GameState.accessoryIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.accessoryQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;

                case ITEM_TYPE_ITEM:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_ITEMS; j++) {
                        if (g_GameState.itemIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newItemnewWeaponAmount = g_GameState.itemQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.itemQuantities[j] = newItemnewWeaponAmount;
                            if (newItemnewWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.itemQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_ITEMS; j++) {
                            if (g_GameState.itemIDs[j] == 0) {
                                g_GameState.itemIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.itemQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;
            }
        }
    }
}

// Parse and set graphics for final price for confirmation window
void ShopMenuSetFinalPriceGraphics(unsigned int number) {
    int i;
    u_char digit;

    ShopMenuParseNumberToString(number);
    g_Menu->pShop->finalPriceStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->finalPriceStrLen += func_8002675C(
                g_Menu->unk2DC, digit, 
                &g_Menu->pShop->polysFinalPrice[g_Menu->pShop->finalPriceStrLen * 2],
                g_Menu->renderContext, 
                107 + i * 8, 84, 
                0x1000
            );
        }
    }
    g_Menu->pShop->finalPriceRenderCtx = g_Menu->renderContext;
    g_Menu->pManager->unk5B = 2;
}

u_char ShopMenuBuyMenu(void) {
#ifdef XENO_PC_PORT
    /* func_801CDD14 writes one affordability flag for each visible row. */
    u8 sp28[MAX_ITEMS_IN_VIEW];
#else
    u8 sp28[0x4]; // ???
#endif
    u_char isRunning;
    u_char shouldInitialize;
    u_char refreshGoldGraphics;
    int initialGold;
    int newGoldAmount;
    int totalPrice;
    int itemPrice;
    int curChoice;
    int prevChoice;
    int scrollOffset;
    int prevScrollOffset;
    int i;

    shouldInitialize = TRUE;
    isRunning = TRUE;
    refreshGoldGraphics = TRUE;
    
    curChoice = 0;
    prevChoice = 0xFF;
    scrollOffset = 0;
    prevScrollOffset = 0xFF;
    
    initialGold = g_GameState.gold;
    totalPrice = 0;
    newGoldAmount = g_GameState.gold;
    
    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        func_801CCE1C(g_Menu->unk330, i);
        g_Menu->pShop->unk46E0[i] = g_Menu->unk330->unkB8;
        g_Menu->pShop->unk4700[i] = g_Menu->unk330->unkBC;
    }
    
    bzero(g_Menu->pShop->curItemQuantities, MAX_SHOP_ITEMS);
    g_Menu->pSelectionMenu->unk1192 = TRUE;
    
    ShopMenuInitializeArrowCursor(0);
    
    while (isRunning) {
        ShopMenuUpdateAndRender();
        
        if ((scrollOffset != prevScrollOffset) || refreshGoldGraphics) {
            func_801CDD14(scrollOffset, newGoldAmount, &sp28);
            ShopMenuUpdateScrollBarHandle(12, 50, 60, D_801D1F50, scrollOffset);
        }

        // Did we change our currently selected item in any way?
        if ((curChoice != prevChoice) || (scrollOffset != prevScrollOffset)) {
            itemPrice = func_801CEB3C(curChoice, scrollOffset, &sp28);
            prevChoice = curChoice;
            prevScrollOffset = scrollOffset;
            g_Menu->pManager->unk5A = TRUE;
        }
        
        ShopMenuUpdateArrowCursor(curChoice, scrollOffset, 0, 0);
        
        if (shouldInitialize) {
            func_801CBC88(1, 2, g_Menu->unk6E0, &D_801D1FD4, g_Menu->pManager->unkC);
            func_801CBCF0(2, g_Menu->unk6E0, &D_801D1FD4, &D_801D1FE8, g_Menu->pManager->unkC, 0, 0, 1);
            g_Menu->pManager->unkC[0] = FALSE;
            ShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            ShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            ShopMenuInitializeWindow(5, 0xE0, 0x7A, 0x40, 0x24, 0, 1, 4, 0);
            ShopMenuStartOpenMenuTransition();
            ShopMenuUpdateCharacterPortraits();
            ShopMenuUpdateBuyMenuExplanationGraphics();
            shouldInitialize = FALSE;
            while (g_Menu->transitionEffectState != MENU_ANIMATION_DONE) {
                ShopMenuUpdateAndRender();
            }
            g_Menu->pManager->unkC[0] = TRUE;
        }
        
        if (refreshGoldGraphics) {
            ShopMenuUpdateGoldGraphics(initialGold, totalPrice, newGoldAmount);
            refreshGoldGraphics = FALSE;
        }
        
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (totalPrice != 0) {
                    ShopMenuPlaySoundEffect(2);
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    isRunning = FALSE;
                    g_Menu->pManager->unkC[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    ShopMenuSetFinalPriceGraphics(totalPrice);
                    if (ShopMenuConfirmationWindow(0x8F, 0xFF, 1) != MENU_CHOICE_NO) {
                        ShopMenuHandleBoughtItems(newGoldAmount);
                    } else {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                        prevScrollOffset = 0xFF;
                        prevChoice = 0xFF;
                        g_Menu->pManager->unkC[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                } else {
                    ShopMenuPlaySoundEffect(4);
                }
                break;
            
            case MENU_INPUT_BACK:
                isRunning = FALSE;
                g_Menu->pManager->unkC[0] = 0;
                if (totalPrice != 0) {
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    if (ShopMenuConfirmationWindow(0x8C, 0xFF, 1) == MENU_CHOICE_NO) {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                        prevScrollOffset = 0xFF;
                        prevChoice = 0xFF;
                        g_Menu->pManager->unkC[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                }
                ShopMenuUpdateAndRender();
                break;
            
            case MENU_INPUT_DOWN:
                curChoice++;

                // If we move beyond the items visible at a time, we need to scroll down
                if (curChoice >= MAX_ITEMS_IN_VIEW) {
                    curChoice = MAX_ITEMS_IN_VIEW - 1;
                    scrollOffset++;

                    // If we're at the bottom of the window, we undo our scrolling,
                    // since there's nothing mroe to scroll to
                    if ((D_801D1F50 - MAX_ITEMS_IN_VIEW) < scrollOffset) {
                        scrollOffset--;
                    }
                }
                break;
            
            case MENU_INPUT_UP:
                curChoice--;
                if (curChoice < 0) {
                    scrollOffset--;
                    curChoice = 0;
                    if (scrollOffset < 0) {
                        scrollOffset = 0;
                    }
                }
                break;

            // Increase item quantity
            case MENU_INPUT_RIGHT:
                if (sp28[curChoice] != 0) {
                    if (g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] + 1 + D_801D2260 < 100) {
                        g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] += 1;
                        totalPrice += itemPrice;
                        newGoldAmount -= itemPrice;
                        refreshGoldGraphics = TRUE;
                    }
                }
                break;

            // Decrease item quantity
            case MENU_INPUT_LEFT:
                if (g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] != 0) {
                    g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] -= 1;
                    totalPrice -= itemPrice;
                    newGoldAmount += itemPrice;
                    refreshGoldGraphics = TRUE;
                }
                break;
        }
    }
    
    g_Menu->pShop->unk4785 = 0;
    return TRUE;
}
