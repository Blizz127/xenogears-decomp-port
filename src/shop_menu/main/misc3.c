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
extern void func_801CBC88(u8, u8, void*, void*, u8*);
/* Retail 801CBCF0..801CC024. Records expand with native pointers; the
 * geometry, byte argument narrowing and mode-1 base-record width do not. */
void func_801CBCF0(s32 count, MenuString* strings, void* ids, s32* xOffsets,
                  u8* active, u8 selected, u8 offset, u8 mode) {
    MenuString* string = &strings[selected];
    POLY_FT4* poly = &string->polys[g_Menu->renderContext];
    s32 x, y, width;
    if (mode == 0) {
        func_801CBC88(0, (u8)count, strings, ids, active);
        x = (u16)D_801D2194[selected + offset] + (u16)xOffsets[selected] + 22;
        y = (u16)D_801D21B0[selected + offset] - 34;
        width = string->width;
    } else if (mode == 1) {
        x = 236;
        y = 126;
        width = strings[0].width;
    } else {
        string->renderContext = (u8)g_Menu->renderContext;
        active[selected] = 1;
        return;
    }
    setXY4(poly, x, y, x + width, y, x, y + 13, x + width, y + 13);
    string->renderContext = (u8)g_Menu->renderContext;
    active[selected] = 1;
}
#else
INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc3", func_801CBCF0);
#endif

void ShopMenuInitializeShopModeSelectionMenu(int numTextures, int* pTextureIDs) {
    int i;
    int j;

    // Render the selection menu as active
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = FALSE;

    g_Menu->pManager->shouldRenderSelectionMenu = TRUE;
    
    // Another weird loop case similar to the one in ShopMenuUpdateCharacterPortraits.
    // We loop one and one cursor & text texture entry up to our current counter,
    // rebuild the cursor and text primitives and rerender the whole menu.
    for (i = 1; i <= numTextures; i++) {   
        
        // Cursors for each menu option
        if (i != numTextures) {
            g_Menu->pSelectionMenu->numCursors = 0;
            for (j = 0; j < i; j++) {
                g_Menu->pSelectionMenu->numCursors += func_8002675C(
                    g_Menu->unk2DC, pTextureIDs[j * 2], 
                    &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2], 
                    g_Menu->renderContext, 
                    160, 150, 
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
        }

        // Text / Content for each menu option
        g_Menu->pSelectionMenu->numTexts = 0;
        if (i != 1) {
            for (j = 0; j < i - 1; j++) {
                g_Menu->pSelectionMenu->numTexts += func_8002675C(
                    g_Menu->unk2DC, pTextureIDs[j * 2 + 1],
                    &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2], 
                    g_Menu->renderContext, 
                    160, 150, 
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
        }

        for (j = 0; j < 2; j++) {
            ShopMenuUpdateAndRender();
        }
    }
}
