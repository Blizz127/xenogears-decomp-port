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
void func_801C5040(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, short width, short height) {
    setXY4(
        pPoly,
        x,         y,
        x + width, y,
        x,         y + height,
        x + width, y + height
    );

    setUV4(
        pPoly,
        u,         v,
        u + width, v,
        u,         v + height,
        u + width, v + height
    );
}

u_short ShopMenuIsCharacterFlagSet(u_short value, u_char maskIndex) {
    return D_801D21F0[maskIndex] & value;
}

u_short ShopMenuGetCharacterBitMask(int maskIndex) {
    return D_801D21F0[maskIndex & 0xFF];
}

void ShopMenuParseNumberToString(u_int number) {
    int i;
    unsigned int curValue;

    // 10 ** 8
    curValue = 100000000;
    
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        g_Menu->digits[i] = number / curValue;
        number %= curValue;
        curValue /= 10;
    }
    
    for (i = 1; i < MENU_MAX_DIGITS; i++) {
        if (g_Menu->digits[i]) {
            if (g_Menu->digits[i - 1] == 0) {
                g_Menu->digits[i - 1] = 0xFF;
            }
            break;
        }
        g_Menu->digits[i - 1] = 0xFF;
    }
}

void ShopMenuUnk32CManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk32C = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->unk32C, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->unk32C);
}

void ShopMenuSetManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

void ShopMenuSelectionMenuManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

void ShopMenuUnk354Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk354 = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->unk354, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->unk354);
}

void ShopMenuUnk330Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk330 = HeapAlloc(sizeof(MenuUnk6), 0);
        bzero(g_Menu->unk330, sizeof(MenuUnk6));
        return;
    }
    HeapFree(g_Menu->unk330);
}

void ShopMenuUnk348Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk348 = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->unk348, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->unk348);
}

void ShopMenuUnk1E20Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk1E20 = HeapAlloc(sizeof(MenuUnk7), 0);
        bzero(g_Menu->unk1E20, sizeof(MenuUnk7));
        return;
    }
    HeapFree(g_Menu->unk1E20);
}

void ShopMenuShopManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pShop = HeapAlloc(sizeof(MenuShop), 0);
        bzero(g_Menu->pShop, sizeof(MenuShop));
        return;
    }
    HeapFree(g_Menu->pShop);
}

const char D_801C5000[16] = { 'B', 'I', 'S', 'L', 'P', 'S', '-', '0', '0', '8', '0', '0', 0, 0, 3, 0 };

void ShopMenuLoadResources(void) {
    TIM_IMAGE tim;
    union { s32 word[3][6]; u16 half[3][12]; } pos;
    s32 i;
    s32 slot;
    u8 characterID;
    void* pResourceEntry;
    u32* pResources;

    pResources = D_8005945C;
    ResolveArchiveEntryPointers(pResources);
    pResourceEntry = LZSSHeapDecompress((void*)(uintptr_t)pResources[1], 1);
    OpenTIM(pResourceEntry);
    ReadTIM(&g_Menu->unk32C->tim);
    memcpy(&g_Menu->unk32C->unk4F80[0x4E], D_801C5000, 13);
    g_Menu->unk32C->unk4B94 = 0x53;
    g_Menu->unk32C->unk4B95 = 0x43;
    g_Menu->unk32C->unk4B96 = 0x11;
    g_Menu->unk32C->unk4B97 = 1;
    bzero(g_Menu->unk32C->unk4B98, 0x5C);
    memmove(g_Menu->unk32C->unk4BF4, g_Menu->unk32C->tim.caddr, 0x20);
    memmove(g_Menu->unk32C->unk4C14, g_Menu->unk32C->tim.paddr, 0x80);
    HeapFree(pResourceEntry);
    pResourceEntry = LZSSHeapDecompress((void*)(uintptr_t)pResources[2], 1);
    func_8002DD20(pResourceEntry);
    HeapFree(pResourceEntry);
    g_Menu->unk2DC = LZSSHeapDecompress((void*)(uintptr_t)pResources[3], 0);
    g_Menu->unk2E0 = LZSSHeapDecompress((void*)(uintptr_t)pResources[4], 0);
    func_80026338(g_Menu->unk2DC, 0x14B, &pos.word[0][0], &pos.word[0][1], &pos.word[0][2], &pos.word[0][3], &pos.word[0][4], &pos.word[0][5]);
    func_80026338(g_Menu->unk2DC, 0x14C, &pos.word[1][0], &pos.word[1][1], &pos.word[1][2], &pos.word[1][3], &pos.word[1][4], &pos.word[1][5]);
    func_80026338(g_Menu->unk2DC, 0x14D, &pos.word[2][0], &pos.word[2][1], &pos.word[2][2], &pos.word[2][3], &pos.word[2][4], &pos.word[2][5]);
    pos.word[1][4] = (u32)pos.word[1][4] + 12;
    pResourceEntry = LZSSHeapDecompress((void*)(uintptr_t)pResources[5], 1);
    for (i = 0, slot = 0; slot < 3; i++) {
        characterID = g_Menu->pManager->currentCharacterIDs[slot++];
        if (characterID != 0xFF) {
            OpenTIM((u_long*)((u8*)pResourceEntry + characterID * 0xB20));
            ReadTIM(&tim);
            tim.crect->x = *(u16*)((u8*)&pos + 8 + i * 24);
            tim.crect->y = *(u16*)((u8*)&pos + 12 + i * 24);
            tim.prect->x = *(u16*)((u8*)&pos + 16 + i * 24);
            tim.prect->y = *(u16*)((u8*)&pos + 20 + i * 24);
            LoadImage(tim.crect, tim.caddr);
            LoadImage(tim.prect, tim.paddr);
        }
    }
    DrawSync(0);
    HeapFree(pResourceEntry);
    if (g_MenuDebugEnabled) {
        ArchiveSetIndex(0x10, 2);
        D_8006259C = HeapAlloc(ArchiveDecodeAlignedSize(5), 0);
        ArchiveReadFileToBuffer(5, D_8006259C, 0, 0x80);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(0x10, 0);
        SoundAddSedsEntry(D_8006259C);
    }
    g_Menu->unk2E4 = D_8006259C;
    g_Menu->pShopEntries = LZSSHeapDecompress((void*)(uintptr_t)pResources[7], 0);
    HeapFree(pResources);
}


void ShopMenuInitialize(void) {
    int flags;
    int characterID;
    int i;

    g_Menu->menu1Choice = 4;
    g_Menu->unk337 = 0xFF;
    g_Menu->unk326 = 0x3C;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;
    
    flags = (g_GameState.unk1D30 & g_GameState.FrMask) & 0x7FF;
    for (i = 0; i < 0x10; i++) {
        if (ShopMenuIsCharacterFlagSet(flags, i)) {
            g_Menu->availableCharacters[i] = TRUE;
        } else {
            g_Menu->availableCharacters[i] = FALSE;
        }
    }
    
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        characterID = g_GameState.partyMembers[i];
        if (characterID != 0xFF && g_Menu->availableCharacters[characterID]) {
            g_Menu->pManager->currentCharacterIDs[i] = characterID;
        } else {
            g_Menu->pManager->currentCharacterIDs[i] = 0xFF;
        }
    }
    
    ShopMenuLoadResources();
}

void ShopMenuResetRenderContext(void) {
    g_Menu->renderContext = 0;
}

extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

static inline void MenuSetSystemPalette(void* pPrim, u8 flag) {
    POLY_FT4* pPoly_ = pPrim;
    pPoly_->clut = flag ? g_SystemPalette2 : g_SystemPalette1;
}

/* Rebuilds both render-context primitives of a menu string.
 *
 * flags == 0 selects the 320-wide staging page, where the row is derived from
 * (index + offset) / 4.  Otherwise the 384/128 font page is used, the low 7
 * bits of flags pick the palette, and clearing bit 7 enables the half-bright
 * semi-transparent draw.
 *
 * The repeated (half & 1) * 128 terms and the mid-block `u` assignment are
 * load-bearing for the retail instruction stream: `u` supplies the u2 slot
 * through a register copy while the remaining slots read the hoisted
 * loop-invariant directly.
 */
void func_801C5A7C(MenuString* pString, s32 index, s32 offset, u8 flags) {
    s32 i;
    u16 blend;
    POLY_FT4* pPoly;
    s32 odd;
    s32 half;
    u8 u;

    i = 0;
    odd = index & 1;
    half = index / 2;

    for (; i < 2; i++) {
        pPoly = &pString->polys[i];
        blend = 0;
        SetPolyFT4(pPoly);
        SetSemiTrans(pPoly, 0);
        SetShadeTex(pPoly, 0);
        pPoly->r0 = 128;
        pPoly->g0 = 128;
        pPoly->b0 = 128;

        if (flags == 0) {
            pString->unk7C = odd;
            pPoly->tpage = GetTPage(0, 0, 320, 0);
            pPoly->u0 = (half & 1) * 128;
            pPoly->v0 = ((index + offset) / 4) * FONT_LETTER_HEIGHT;
            u = (half & 1) * 128;
            pPoly->u1 = u + pString->width;
            pPoly->v1 = ((index + offset) / 4) * FONT_LETTER_HEIGHT;
            pPoly->u2 = u;
            pPoly->v2 = ((index + offset) / 4) * FONT_LETTER_HEIGHT + FONT_LETTER_HEIGHT;
            pPoly->u3 = u + pString->width;
            pPoly->v3 = ((index + offset) / 4) * FONT_LETTER_HEIGHT + FONT_LETTER_HEIGHT;
        } else {
            if (!(flags & 0x80)) {
                blend = 0x20;
                SetSemiTrans(pPoly, 1);
                pPoly->r0 = blend;
                pPoly->g0 = blend;
                pPoly->b0 = blend;
            }
            pString->unk7C = (flags & 0x7F) + 0xFF;
            pPoly->tpage = blend | GetTPage(0, 0, 384, 128);
            setUV4(
                pPoly,
                odd * 96,                  half * FONT_LETTER_HEIGHT + offset,
                odd * 96 + pString->width, half * FONT_LETTER_HEIGHT + offset,
                odd * 96,                  half * FONT_LETTER_HEIGHT + offset + FONT_LETTER_HEIGHT,
                odd * 96 + pString->width, half * FONT_LETTER_HEIGHT + offset + FONT_LETTER_HEIGHT
            );
        }
        MenuSetSystemPalette(pPoly, pString->unk7C);
    }
    pString->unk7F = 0;
}
