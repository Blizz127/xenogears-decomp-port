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
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc9", func_801CFF58);
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc9", func_801D05BC);
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc9", ShopMenuHandleSoldItems);
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc9", ShopMenuSellMenu);
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc9", ShopMenuSellEquipmentMenu);
/* ShopMenuSellMenu's switch table lives in the overlay's .rodata block at
 * 0x801C5028 rather than next to its code, so it is not part of this TU's
 * compiled output; splat emits it as a rodata blob which has to be included
 * here (its entries point at labels inside the ShopMenuSellMenu body above). */
INCLUDE_RODATA("asm/shop_menu/nonmatchings/main/misc", jtbl_801C5028);
#else
void ShopMenuSellMenu(int max, void* ids, void* qty, int type, int a4, void* qty2, int a6);
#endif

void ShopMenuSellAccessoriesMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_ACCESSORIES, 
        g_GameState.accessoryIDs, 
        g_GameState.accessoryQuantities, 
        ITEM_TYPE_ACCESSORY, 
        1, 
        g_GameState.accessoryQuantities, 
        0
    );
}

void ShopMenuSellWeaponsMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_WEAPONS, 
        g_GameState.weaponIDs, 
        g_GameState.weaponQuantities, 
        ITEM_TYPE_WEAPON, 
        1, 
        g_GameState.weaponQuantities, 
        0
    );
}

void ShopMenuSellItemsMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_ITEMS, 
        g_GameState.itemIDs, 
        g_GameState.itemQuantities, 
        ITEM_TYPE_ITEM, 
        1, 
        g_GameState.itemQuantities, 
        0
    );
}

void func_801D1968(u_char arg0, u_char arg1) {
    int i;

    g_Menu->pManager->unk5A = 0;
    g_Menu->pShop->unk46A7 = 0;
    g_Menu->pShop->unk46B5 = 0;
    g_Menu->pShop->unk46B2 = 0;
    g_Menu->pShop->explanationsLen = 0;
    g_Menu->pShop->unk46A9 = 0;
    g_Menu->pShop->numPortraits = 0;

    for (i = 0; i < 9; i++) {
        g_Menu->pShop->unk469C[i] = 0;
        g_Menu->pShop->unk46BC[i] = 0;
        g_Menu->pShop->unk46C5[i] = 0;
    }

    for (i = 0; i < 8; i++) {
        g_Menu->pShop->unk4684[i] = 0;
        g_Menu->pShop->unk468C[i] = 0;
    }
    
    if (arg0) {
        ShopMenuFreeWindow(2);
        ShopMenuFreeWindow(3);
        if (arg1) {
            ShopMenuFreeWindow(5);
        }
        ShopMenuFreeScrollBarHandle();
        ShopMenuFreeArrowCursor(0);
    }
}

void ShopMenuSellModeMenuHandleSelectedOption(void) {
    u_char var_a0;

    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    g_Menu->pManager->unkA = 0;
    func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
    
    var_a0 = 1;
    switch (g_Menu->menu2Choice) {
        case MENU_CHOCIE_EQUIPMENT:
            var_a0 = ShopMenuSellEquipmentMenu();
            break;
        
        case MENU_CHOICE_ACCESSORIES:
            ShopMenuSellAccessoriesMenu();
            break;

        case MENU_CHOICE_WEAPONS:
            ShopMenuSellWeaponsMenu();
            break;

        case MENU_CHOICE_ITEMS:
            ShopMenuSellItemsMenu();
            break;
    }
    
    func_801D1968(var_a0, 0);
    g_Menu->pManager->unkA = 1;
    g_Menu->pManager->unk4 = 1;
    g_Menu->pManager->unk3 = 1;
    func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
}

// Items / Weapons / Accessories / Equipment
int ShopMenuSellModeMenu() {
    u_char shouldInitialize;
    u_char isRunning;

    isRunning = TRUE;
    shouldInitialize = TRUE;
    g_Menu->menu2Choice = MENU_CHOICE_ITEMS;
    g_Menu->unk339 = 0xFF;
    
    while (isRunning) {
        ShopMenuUpdateAndRender();
        
        if (shouldInitialize) {
            func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
            ShopMenuStartOpenMenuTransition();
            func_801CC278(0);
            shouldInitialize = FALSE;
        }
        
        if (g_Menu->menu2Choice != g_Menu->unk339) {
            func_801CBCF0(4, g_Menu->unk6E0, &D_801D1FD0, &D_801D1FE8, g_Menu->pManager->unkC, g_Menu->menu2Choice, 3, 0);
            func_801CC720(0);
            g_Menu->unk339 = g_Menu->menu2Choice;
        }
        
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                ShopMenuSellModeMenuHandleSelectedOption();
                g_Menu->unk339 = 0xFF;
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                break;
            
            case MENU_INPUT_DOWN:
                if (g_Menu->menu2Choice != 0) {
                    g_Menu->menu2Choice--;
                } else {
                    g_Menu->menu2Choice = g_Menu->unk33A - 1;
                }
                break;

            case MENU_INPUT_UP:
                if (++g_Menu->menu2Choice >= g_Menu->unk33A) {
                    g_Menu->menu2Choice = 0;
                }
                break;
        }
    }
    
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
    return TRUE;
}

void func_801D1F10(void) {
    if (g_Menu->menu1Choice == 1) {
    } else if (g_Menu->menu1Choice == 2) {
        func_801D1968(1, 1);
    }
}
