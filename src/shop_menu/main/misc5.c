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
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc5", func_801CC720);
#endif

u_char ShopMenuShopModeMenuHandleSelectedOption() {
    u_char isRunning;
    u_char result;

    isRunning = TRUE;
    switch (g_Menu->menu1Choice) {
        case MENU_CHOICE_EXIT:
            isRunning = FALSE;
            break;

        case MENU_CHOICE_SELL:
            result = ShopMenuSellModeMenu();
            break;

        case MENU_CHOICE_BUY:
            result = ShopMenuBuyMenu();
            break;
    }
    
    if (result) {
        ShopMenuStartCloseMenuTransition();
        func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);
    }
    
    func_801D1F10();

    // Render the selection menu as active again
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = TRUE;

    g_Menu->pManager->unk4 = TRUE;
    g_Menu->pManager->unk3 = TRUE;
    g_Menu->unk337 = 0xFF;
    g_Menu->pManager->unkA = FALSE;
    
    return isRunning;
}

// Buy / Sell / Exit menu
void ShopMenuShopModeMenuMain(void) {
    u_char isRunning;

    isRunning = TRUE;
    g_Menu->menu1Choice = 2;

    ShopMenuInitializeShopModeSelectionMenu(4, &D_801D1F54);

    func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);
    
    while (isRunning) {
        ShopMenuUpdateAndRender();

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);

                // Render the selection menu as disabled
                g_Menu->pSelectionMenu->unk1192 = TRUE;

                func_801C6430();
                func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);
                g_Menu->unk348->unk15B = 0x4C;
                isRunning = ShopMenuShopModeMenuHandleSelectedOption();
                g_Menu->unk348->unk15B = 0x40;
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                break;

            case MENU_INPUT_DOWN:
                if (g_Menu->menu1Choice) {
                    g_Menu->menu1Choice--;
                } else {
                    g_Menu->menu1Choice = 2;
                }
                break;

            case MENU_INPUT_UP:
                if (++g_Menu->menu1Choice >= 3) {
                    g_Menu->menu1Choice = 0;
                }
                break;
        }

        // Update the selected option to the active one if we need to
        if (g_Menu->menu1Choice != g_Menu->unk337) {
            ShopMenuUpdateShopModeSelectionMenu(3, g_Menu->menu1Choice, &D_801D1F54);
            func_801CBCF0(4, g_Menu->unk6E0, &D_801D1FCC, &D_801D1FD8, 
                g_Menu->pManager->unkC, g_Menu->menu1Choice, 0, 0
            );
            g_Menu->unk337 = g_Menu->menu1Choice;
        }
    }
}

void ShopMenuMain(void) {
    ShopMenuUnk32CManager(MENU_DATA_INITIALIZE);
    ShopMenuSetManager(MENU_DATA_INITIALIZE);
    ShopMenuSelectionMenuManager(MENU_DATA_INITIALIZE);
    ShopMenuUnk354Manager(MENU_DATA_INITIALIZE);
    ShopMenuUnk330Manager(MENU_DATA_INITIALIZE);
    ShopMenuUnk348Manager(MENU_DATA_INITIALIZE);
    ShopMenuUnk1E20Manager(MENU_DATA_INITIALIZE);
    ShopMenuShopManager(MENU_DATA_INITIALIZE);
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    ShopMenuInitialize();
    ShopMenuResetRenderContext();
    func_801C5EE8();
    ShopMenuInitializeBackgrounds();
    ShopMenuInitializeWindowBorders();
    ShopMenuInitializeShopData();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->unk32A = TRUE;
    ShopMenuShopModeMenuMain();
    ShopMenuFree();
}

void func_801CCE1C(void* output, u8 characterId) {
    u8* out = output;
    u8* stats = (u8*)&g_GameState + 0x26C + characterId * 0xA4;

    if (stats[0x56] == 4) {
        *(u16*)(out + 0xB8) = stats[0x04] + stats[0x1C];
    } else {
        *(u16*)(out + 0xB8) = stats[0x58] + stats[0x28] + stats[0x04];
    }

    *(u16*)(out + 0xB8) = stats[0x58] + stats[0x28] + stats[0x04];
    *(u16*)(out + 0xBA) = stats[0x5E] + stats[0x2E];
    *(u16*)(out + 0xBC) = stats[0x59] + stats[0x29] + stats[0x2D];
    *(u16*)(out + 0xBE) = stats[0x5F] + stats[0x2F];
    *(u16*)(out + 0xC0) = stats[0x5B] + stats[0x2B];
    *(u16*)(out + 0xC2) = 99;
    *(u16*)(out + 0xC4) = stats[0x5C] + stats[0x2C];
    *(u16*)(out + 0xC6) = 10;
    *(u16*)(out + 0xC8) = stats[0x5A] + stats[0x2A];

    if (*(u16*)(out + 0xB8) >= 1000) *(u16*)(out + 0xB8) = 999;
    if (*(u16*)(out + 0xBA) >= 100) *(u16*)(out + 0xBA) = 99;
    if (*(u16*)(out + 0xBC) >= 1000) *(u16*)(out + 0xBC) = 999;
    if (*(u16*)(out + 0xBE) >= 100) *(u16*)(out + 0xBE) = 99;
    if (*(u16*)(out + 0xC0) >= 1000) *(u16*)(out + 0xC0) = 999;
    if (*(u16*)(out + 0xC2) >= 100) *(u16*)(out + 0xC2) = 99;
    if (*(u16*)(out + 0xC4) >= 1000) *(u16*)(out + 0xC4) = 999;
    if (*(u16*)(out + 0xC6) >= 100) *(u16*)(out + 0xC6) = 99;
    if (*(u16*)(out + 0xC8) >= 100) *(u16*)(out + 0xC8) = 99;
}

// Render Shop Stuff
