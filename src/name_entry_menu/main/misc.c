/* Menu mode 3, "Enter Name": archive directory 0x10 file 8 at 0x801C5000 (FE58).
 * Functions marked SIBLING are byte-identical (outside relocations) to the named
 * member_change_menu / shop_menu function: the same menu-library source compiled into
 * this overlay.  Their C is that sibling C with each relocated symbol renamed. */

#include "common.h"
#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/menu.h"
#include "main/game.h"
#include "system/archive.h"
#include "system/debug.h"
#include "system/math.h"

/* Engine routines the overlay calls; declarations as in shop_menu's TU
 * (pointer returns must be declared for the 64-bit port). */
extern void* LZSSHeapDecompress(void*, int);
extern unsigned int ResolveArchiveEntryPointers(u32*);
extern void func_8002DD20(u32*);
extern void SoundAddSedsEntry(SoundFile*);
extern void func_80026338(u8*, s32, u32*, s32*, s32*, s32*, s32*, s32*);

u_short NameEntryMenuIsCharacterFlagSet(u_short value, u_char maskIndex);
void NameEntryMenu_801C505C(u_char isInitialization);
void NameEntryMenuSetManager(u_char isInitialization);
void NameEntryMenuSetSelectionMenu(u_char bIsInitialization);
void NameEntryMenu_801C5188(u_char isInitialization);
void NameEntryMenu_801C51EC(u_char isInitialization);
void NameEntryMenu_801C5250(u_char isInitialization);
void NameEntryMenuUnk1E20Manager(u_char isInitialization);
void NameEntryMenu_801C5318(void);
void NameEntryMenuInitialize(void);
void NameEntryMenuResetRenderContext(void);
void NameEntryMenu_801C5A40(void);
void NameEntryMenu_801C5ABC(MenuString* pString, int index, s32 arg2, u32 attributes);
void NameEntryMenu_801C5CFC(MenuString* base, u8* strIndices, s32 arg2, s32 count);
void NameEntryMenu_801C5EAC(void);
void NameEntryMenuInitializeWindowBorders(void);
void NameEntryMenu_801C6010(void);
void NameEntryMenu_801C6040(POLY_G4* pPoly, u_char red, u_char green, u_char blue);
void NameEntryMenuInitializeBackground(void);
void NameEntryMenuSetVertices(SVECTOR* pVertices, u_short x, u_short y, u_short width, u_short height);
void NameEntryMenuSetWindowBorderPrimitive(POLY_FT4* pPrim);
void NameEntryMenuInitializeWindowGraphics(unsigned char index);
void NameEntryMenuInitializeScrollBar(unsigned char index, u_short x, u_short y, u_short width, u_short height);
void NameEntryMenuInitializeWindowBorderCorners(unsigned char index, u_short x, u_short y, u_short width, u_short height);
void NameEntryMenuSetWindowBorderTop(unsigned char index, u_short x, u_short y, u_short width);
void NameEntryMenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width, u_short height);
void NameEntryMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height);
void NameEntryMenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width, u_short height);
void NameEntryMenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height, u_char arg5, int zIndex, u_char hasScrollBar);
void NameEntryMenuFreeWindow(u_char index);
void NameEntryMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollBar);
void NameEntryMenuUpdateWindows(void);
void NameEntryMenuRenderTopWindowBorder(int index);
void NameEntryMenuRenderBottomWindowBorder(int index);
void NameEntryMenuRenderLeftWindowBorder(int index);
void NameEntryMenuRenderRightWindowBorder(int index);
void NameEntryMenuRenderWindowBackground(int index);
void NameEntryMenuRenderWindowBorderCorners(int index);
void NameEntryMenuRenderScrollBar(int index);
void NameEntryMenuRenderWindows(void);
void NameEntryMenuDrawCursors(void);
void NameEntryMenu_801C8E38(void);
void NameEntryMenu_801C8EC8(void);
void NameEntryMenu_801C8F58(void);
void NameEntryMenu_801C90D0(void);
void NameEntryMenu_801C9160(void);
void NameEntryMenuRenderBackgroundDim(void);
void NameEntryMenu_801C9338(void);
void NameEntryMenu_801C97FC(void);
void NameEntryMenuRender(void);
void NameEntryMenuPlaySoundEffect(u_char soundEffectId);
void NameEntryMenuPollInput(void);
void NameEntryMenuUpdateTransitionEffect(void);
void NameEntryMenuUpdateAndRender(void);
void NameEntryMenuInitializeCursors(unsigned char mode);
void NameEntryMenuFreeCursors(void);
void NameEntryMenuStartOpenMenuTransition(void);

extern u_short NameEntryMenu_D_801CC114[];
extern int NameEntryMenu_D_801CBEA0[];
extern int NameEntryMenu_D_801CBEB0[];
extern s32 NameEntryMenu_D_801CBF9C[];
extern s32 NameEntryMenu_D_801CC014[];
extern s32 NameEntryMenu_D_801CC094[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern void* GetStringEntry(void* table, s32 index);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern u8 NameEntryMenu_D_801CBF98[];
extern void func_80026338(u8*,s32,u32*,s32*,s32*,s32*,s32*,s32*);
extern void* HeapAlloc(u_int,u_int);
extern u_int HeapFree(void*);
#define MENU_CHOICE_NO 0
#define MENU_CHOICE_YES 1
#define MENU_CHOICE_EXIT 0x0
#define MENU_CHOICE_SELL 0x1
#define MENU_CHOICE_BUY 0x2
#define MENU_CHOCIE_EQUIPMENT 0x0
#define MENU_CHOICE_ACCESSORIES 0x1
#define MENU_CHOICE_WEAPONS 0x2
#define MENU_CHOICE_ITEMS 0x3
#define MENU_STAT_CHANGE_INCREASE 0x0
#define MENU_STAT_CHANGE_DECREASE 0x1
#define SHOP_DATA_INITIALIZE 0x0
#define SHOP_DATA_FREE 0x10
#define MENU_AUTO_ADVANCE 0
#define MENU_MANUAL_CHOICE 0xFF
#define MAX_ITEMS_IN_VIEW 8
extern s32* D_8005917C;
extern void* GetStringEntry(void* bundle, s32 index);
extern s32 SystemRenderStringEntry(void* string, void* work, s32 height, s32 flag);

static inline void MenuSetSystemPalette(void* pPrim, u8 flag) {
    POLY_FT4* pPoly_ = pPrim;
    pPoly_->clut = flag ? g_SystemPalette2 : g_SystemPalette1;
}

/* SIBLING member_change_menu:MemberChangeMenuIsCharacterFlagSet */
u_short NameEntryMenuIsCharacterFlagSet(u_short value, u_char maskIndex) {
    return NameEntryMenu_D_801CC114[maskIndex] & value;
}

/* SIBLING member_change_menu:func_801C5034 */
void NameEntryMenu_801C505C(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk32C = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->unk32C, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->unk32C);
}

/* SIBLING member_change_menu:MemberChangeMenuSetManager */
void NameEntryMenuSetManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

/* SIBLING member_change_menu:MemberChangeMenuSetSelectionMenu */
void NameEntryMenuSetSelectionMenu(u_char bIsInitialization) {
    if (bIsInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

/* SIBLING member_change_menu:func_801C5160 */
void NameEntryMenu_801C5188(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk354 = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->unk354, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->unk354);
}

/* SIBLING member_change_menu:func_801C51C4 */
void NameEntryMenu_801C51EC(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk330 = HeapAlloc(sizeof(MenuUnk6), 0);
        bzero(g_Menu->unk330, sizeof(MenuUnk6));
        return;
    }
    HeapFree(g_Menu->unk330);
}

/* SIBLING member_change_menu:func_801C5228 */
void NameEntryMenu_801C5250(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk348 = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->unk348, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->unk348);
}

/* SIBLING shop_menu:ShopMenuUnk1E20Manager */
void NameEntryMenuUnk1E20Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk1E20 = HeapAlloc(sizeof(MenuUnk7), 0);
        bzero(g_Menu->unk1E20, sizeof(MenuUnk7));
        return;
    }
    HeapFree(g_Menu->unk1E20);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/name_entry_menu/nonmatchings/main/misc", NameEntryMenu_801C5318);
#else
/* NON_MATCHING (port body): nearest retail-shaped C is 11 rows off (the table-pointer store / D_80059171 load order before the portrait branch); retail keeps the asm. */
extern void* D_8005945C; /* menu resources archive (loaded by MenuExecute / the field opener) */
extern void* D_8006259C; /* menu SEDS sound bank */
extern u8 D_80059171; /* FE58 argument: name-table id (characters 0-10, Gears 11-30) */
extern char NameEntryMenu_D_801C5000[16]; /* filled per entry (pc_port/src/data_name_entry_menu.c) */

/* Load the menu resources (archive D_8005945C), as shop_menu's
 * ShopMenuLoadResources does, plus the portrait selection for the name
 * being entered.  Characters 0-10 show their own portrait; a Gear (11-30)
 * shows the pilot from the resource table at pResources[6] (Gear 20 shows
 * character 10 once g_GameState.unk1D30 bit 0x400 is set).  The table also
 * gives three bytes per id, stored at g_Menu->unk1E24[4..6].  Retail parks
 * the table pointer in g_Menu+0x1E24; the port keeps it in a local, because
 * a native pointer there would overlap unk1E24[4..6]. */
void NameEntryMenu_801C5318(void) {
    TIM_IMAGE tim;
    union { s32 word[3][6]; u16 half[3][12]; } pos;
    s32 i;
    s32 slot;
    u8 characterID;
    void* pResourceEntry;
    u32* pResources;
    u8 fromTable = 1;
#ifdef XENO_PC_PORT
    u8* pNameTable;
#endif

    pResources = D_8005945C;
    ResolveArchiveEntryPointers(pResources);
    pResourceEntry = LZSSHeapDecompress((void*)(uintptr_t)pResources[1], 1);
    OpenTIM(pResourceEntry);
    ReadTIM(&g_Menu->unk32C->tim);
    memcpy(&g_Menu->unk32C->unk4F80[0x4E], NameEntryMenu_D_801C5000, 13);
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
#ifdef XENO_PC_PORT
    pNameTable = LZSSHeapDecompress((void*)(uintptr_t)pResources[6], 1);
#define NAME_TABLE pNameTable
#else
    *(u8**)g_Menu->unk1E24 = LZSSHeapDecompress((void*)(uintptr_t)pResources[6], 1);
#define NAME_TABLE (*(u8**)g_Menu->unk1E24)
#endif
    if (D_80059171 < 0xB) {
        g_Menu->unk1E20->unk0[0xDE9] = 9;
        g_Menu->pManager->currentCharacterIDs[0] = D_80059171;
    } else {
        g_Menu->unk1E20->unk0[0xDE9] = 0xA;
        if (D_80059171 - 0xB == 9) {
            if (g_GameState.unk1D30 & 0x400) {
                g_Menu->pManager->currentCharacterIDs[0] = 0xA;
                fromTable = 0;
            }
        }
        if (fromTable) {
            g_Menu->pManager->currentCharacterIDs[0] = NAME_TABLE[D_80059171 - 0xB];
        }
    }
    g_Menu->unk1E24[4] = NAME_TABLE[D_80059171 * 3 + 0x20];
    g_Menu->unk1E24[5] = NAME_TABLE[D_80059171 * 3 + 0x21];
    g_Menu->unk1E24[6] = NAME_TABLE[D_80059171 * 3 + 0x22];
    HeapFree(NAME_TABLE);
#undef NAME_TABLE
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
    HeapFree(pResources);
}
#endif

/* SIBLING member_change_menu:MemberChangeMenuInitialize */
void NameEntryMenuInitialize(void) {
    int flags;
    int i;
    int partyMemberID;

    g_Menu->menu1Choice = 4;
    g_Menu->unk337 = 0xFF;
    g_Menu->unk326 = 0x3C;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;
    
    // Check flags to see which characters are available and not
    flags = (g_GameState.unk1D30 & g_GameState.FrMask) & 0x7FF;
    for (i = 0; i < 0x10; i++) {
        if (NameEntryMenuIsCharacterFlagSet(flags, i)) {
            g_Menu->availableCharacters[i] = TRUE;
        } else {
            g_Menu->availableCharacters[i] = FALSE;
        }
    }

    // Set our current characters in the menu
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        partyMemberID = g_GameState.partyMembers[i];
        if (partyMemberID != 0xFF && g_Menu->availableCharacters[partyMemberID]) {
            g_Menu->pManager->currentCharacterIDs[i] = partyMemberID;
        } else {
            g_Menu->pManager->currentCharacterIDs[i] = 0xFF;
        }
    }

    NameEntryMenu_801C5318();
}

/* SIBLING member_change_menu:MemberChangeMenuResetRenderContext */
void NameEntryMenuResetRenderContext(void) {
    g_Menu->renderContext = 0;
}

/* SIBLING member_change_menu:func_801C5724 */
void NameEntryMenu_801C5A40(void) {
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

/* SIBLING member_change_menu:func_801C57A0 */
void NameEntryMenu_801C5ABC(MenuString* pString, int index, s32 arg2, u32 attributes) {
    s32 var_s1;
    int i;
    POLY_FT4* pPoly;
    u32 attr  = attributes;

    for (i = 0; i < 2; i++) {
        pPoly = &pString->polys[i];
        var_s1 = 0;

        SetPolyFT4(pPoly);
        SetSemiTrans(pPoly, 0);
        SetShadeTex(pPoly, 0);
        setRGB0(pPoly, 128, 128, 128);
        
        if (!(attr & 0xFF)) {
            pString->unk7C = (index & 1);
            setTPage(pPoly, 0, 0, 320, 0);
            
            #define texCoordV  (((index + arg2) / 4) * 13)
            #define texCoordU  (((index / 2) & 1) << 7)
            setUV4(
                pPoly,
                texCoordU,                   texCoordV,
                texCoordU + pString->width,  texCoordV,
                texCoordU,                   texCoordV + 13,
                texCoordU + pString->width,  texCoordV + 13
            );
            #undef texCoordV
            #undef texCoordU
        } else {
            if (!(attr & 0x80)) {
                var_s1 = 0x20;
                SetSemiTrans(pPoly, 1);
                setRGB0(pPoly, var_s1, var_s1, var_s1);
            }
            
            pString->unk7C = (attr & 0x7F) + 0xFF;
            setTPage(pPoly, 0, 0, 0x180, 0x80);
            pPoly->tpage = var_s1 | pPoly->tpage;
            
            #define texCoordU ((index & 1) * 0x60)
            #define texCoordV (((index / 2) * 13) + arg2)
            setUV4(
                pPoly,
                texCoordU,                   texCoordV,
                texCoordU + pString->width,  texCoordV,
                texCoordU,                   texCoordV + 13,
                texCoordU + pString->width,  texCoordV + 13
            );
            #undef texCoordV
            #undef texCoordU
        }

        MenuSetSystemPalette(pPoly, pString->unk7C);
    }

    pString->unk7F = 0;
}

/* SIBLING member_change_menu:func_801C59E0 */
void NameEntryMenu_801C5CFC(MenuString* base, u8* strIndices, s32 arg2, s32 count) {
    s32 i;
    RECT* rect;
    void* entry;
    for (i = 0; i < count; i += 2) {
        MenuString* item;
        u8* index = &strIndices[i];
        entry = GetStringEntry(g_Menu->unk2E0, *index);
        item = &base[i];
        item->width = SystemRenderStringEntry(
            entry,
            g_Menu->unk4E0[0].pVramBuffer, 0x18, 0);
        rect = &item->vramDest;
        entry = GetStringEntry(g_Menu->unk2E0, index[1]);
        base[i + 1].width = SystemRenderStringEntry(
            entry,
            g_Menu->unk4E0[0].pVramBuffer, 0x18, 1);
        item->vramDest.x = (i / 2 & 1) * 32 + 320;
        item->vramDest.y = (i + arg2) / 4 * 13;
        item->vramDest.w = 28;
        item->vramDest.h = 13;
        base[i + 1].vramDest = item->vramDest;
        NameEntryMenu_801C5ABC(item, i, arg2, 0);
        NameEntryMenu_801C5ABC(&base[i + 1], i + 1, arg2, 0);
        LoadImage(rect, (u_long*)g_Menu->unk4E0[0].pVramBuffer);
        DrawSync(0);
    }
}

/* SIBLING member_change_menu:func_801C5B90 */
void NameEntryMenu_801C5EAC(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
    g_Menu->unk4E0[0].pVramBuffer = HeapAlloc(0x38E, 0);
    NameEntryMenu_801C5CFC(g_Menu->unk4E0, NameEntryMenu_D_801CBF98, 0, 4);
    NameEntryMenu_801C5A40();
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeWindowBorders */
void NameEntryMenuInitializeWindowBorders(void) {
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

/* SIBLING member_change_menu:func_801C5CF4 */
void NameEntryMenu_801C6010(void) {
    g_Menu->pManager->unk4 = FALSE;
    g_Menu->pManager->unk3 = FALSE;
}

/* SIBLING member_change_menu:func_801C5D24 */
void NameEntryMenu_801C6040(POLY_G4* pPoly, u_char red, u_char green, u_char blue) {
    SetPolyG4(pPoly);
    setRGB0(pPoly, red, green, blue);
    setRGB1(pPoly, red, green, blue);
    setRGB2(pPoly, 0, 0, 0);
    setRGB3(pPoly, 0, 0, 0);
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeBackground */
void NameEntryMenuInitializeBackground(void) {
    RECT rect;
    int i;

    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;
    
    NameEntryMenu_801C6010();

    for (i = 0; i < 2; i++) {
        // Unknown effect, seems to be unused
        NameEntryMenu_801C6040(&g_Menu->unk348->polyG4s[i], 128, 128, 0);
        SetSemiTrans(&g_Menu->unk348->polyG4s[i], 1);
        
        SetLineF3(&g_Menu->unk348->lines1[i]);
        setRGB0(&g_Menu->unk348->lines1[i], 0, 64, 0);
        
        SetLineF3(&g_Menu->unk348->lines2[i]);
        setRGB0(&g_Menu->unk348->lines2[i], 0, 64, 0);
        
        // Background dimming screen
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

/* SIBLING member_change_menu:MemberChangeMenuSetVertices */
void NameEntryMenuSetVertices(SVECTOR* pVertices, u_short x, u_short y, u_short width, u_short height) {
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

/* SIBLING member_change_menu:MemberChangeMenuSetWindowBorderPrimitive */
void NameEntryMenuSetWindowBorderPrimitive(POLY_FT4* pPrim) {
    SetSemiTrans(pPrim, 1);
    SetShadeTex(pPrim, 0);
    setRGB0(pPrim, 128, 128, 128);
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeWindowGraphics */
void NameEntryMenuInitializeWindowGraphics(unsigned char index) {
    RECT rect;
    MenuWindow* pWindow;
    unsigned char i;

    pWindow = g_Menu->windows[index];
    
    rect.y = 0;
    rect.x = 0;
    rect.h = 0x100;
    rect.w = 0x100;
    
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = 0;
    
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

/* SIBLING member_change_menu:MemberChangeMenuInitializeScrollBar */
void NameEntryMenuInitializeScrollBar(unsigned char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    
    pWindow = g_Menu->windows[index];
    
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
    
    NameEntryMenuSetVertices(pWindow->vertsScrollBarEnds, x, y, 8, 8);
    NameEntryMenuSetVertices(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
    NameEntryMenuSetVertices(pWindow->vertsScrollBarEmpty, x, y + 8, 8, height - 8);
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeWindowBorderCorners */
void NameEntryMenuInitializeWindowBorderCorners(unsigned char index, u_short x, u_short y, u_short width, u_short height) {
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
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderCorners, 
        x - 8, 
        y + 8, 
        16, -16
    );
    NameEntryMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[4], 
        x + width + 8, 
        y + 8, 
        -16, -16
    );
    NameEntryMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[8], 
        x - 8, 
        y + height - 8, 
        16, 16
    );
    NameEntryMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[0xC], 
        x + width + 8,
        y + height - 8,
        -16, 16
    );

    for (i = 0; i < 4; i++) {
        NameEntryMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderCorners[i * 2 + g_Menu->renderContext]);
    }
}

/* SIBLING member_change_menu:MemberChangeMenuSetWindowBorderTop */
void NameEntryMenuSetWindowBorderTop(unsigned char index, u_short x, u_short y, u_short width) {
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
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderTop1, 
        x + MENU_WINDOW_BORDER_SIZE,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderTop2, 
        x + MENU_WINDOW_BORDER_SIZE + halfInnerWidth, 
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth, 
        MENU_WINDOW_BORDER_SIZE * 2
    );

    for (i = 0; i < 2; i++) {
        NameEntryMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]);
    }
}

/* SIBLING member_change_menu:MemberChangeMenuSetWindowBorderBottom */
void NameEntryMenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width, u_short height) {
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
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderBottom1, 
        x + 8, 
        height, 
        width, 16
    );
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderBottom2, 
        x + 8 + width, 
        height, 
        width, 16
    );

    for (i = 0; i < 2; i++) {
        NameEntryMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
    }
}

/* SIBLING member_change_menu:MemberChangeMenuSetWindowBorderLeft */
void NameEntryMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height) {
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
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderLeft1, 
        x - 8, y + 8, 
        16, halfInnerHeight
    );
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderLeft2, 
        x - 8, y + 8 + halfInnerHeight, 
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        NameEntryMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderLeft[i * 2 + g_Menu->renderContext]);
    }
}

/* SIBLING member_change_menu:MemberChangeMenuSetWindowBorderRight */
void NameEntryMenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width, u_short height) {
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
    
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderRight1, 
        width, y + 8, 
        16, halfInnerHeight
    );
    NameEntryMenuSetVertices(
        pWindow->vertsWindowBorderRight2, 
        width, y + 8 + halfInnerHeight,
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        NameEntryMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderRight[i * 2 + g_Menu->renderContext]);
    }
}

/* SIBLING member_change_menu:MemberChangeMenuSetWindow */
void NameEntryMenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height, u_char arg5, int zIndex, u_char hasScrollBar) {
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    NameEntryMenuSetVertices(pWindow->vertsBackground, x, y, width, height);
    NameEntryMenuInitializeWindowBorderCorners(index, x, y, width, height);
    NameEntryMenuSetWindowBorderTop(index, x, y, width);
    NameEntryMenuSetWindowBorderBottom(index, x, y, width, height);
    NameEntryMenuSetWindowBorderLeft(index, x, y, height);
    NameEntryMenuSetWindowBorderRight(index, x, y, width, height);
    if (hasScrollBar) {
        NameEntryMenuInitializeScrollBar(index, x, y, width, height);
    }
    pWindow->hasScrollBar = hasScrollBar;
    pWindow->unk714 = arg5;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[index] = TRUE;
}

/* SIBLING member_change_menu:MemberChangeMenuFreeWindow */
void NameEntryMenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = 0;
    HeapFree(g_Menu->windows[index]);
    HeapFree(g_Menu->windowParameters[index]);
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeWindow */
void NameEntryMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollBar) {
    MenuWindowParameters* pWindowParams;

    if (windowIndex >= 2) {
        g_Menu->windows[windowIndex] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero(g_Menu->windows[windowIndex], sizeof(MenuWindow));
        g_Menu->windowParameters[windowIndex] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero(g_Menu->windowParameters[windowIndex] , sizeof(MenuWindowParameters));
        NameEntryMenuInitializeWindowGraphics(windowIndex);
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
    
    NameEntryMenuSetWindow(windowIndex, x, y, width, height, arg6, zIndex, hasScrollBar);
}

/* SIBLING member_change_menu:MemberChangeMenuUpdateWindows */
void NameEntryMenuUpdateWindows(void) {
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
            
            NameEntryMenuSetWindow(
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

/* SIBLING member_change_menu:MemberChangeMenuRenderTopWindowBorder */
void NameEntryMenuRenderTopWindowBorder(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderBottomWindowBorder */
void NameEntryMenuRenderBottomWindowBorder(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderLeftWindowBorder */
void NameEntryMenuRenderLeftWindowBorder(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderRightWindowBorder */
void NameEntryMenuRenderRightWindowBorder(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderWindowBackground */
void NameEntryMenuRenderWindowBackground(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderWindowBorderCorners */
void NameEntryMenuRenderWindowBorderCorners(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderScrollBar */
void NameEntryMenuRenderScrollBar(int index) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderWindows */
void NameEntryMenuRenderWindows(void) {
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

                NameEntryMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) { 
                    NameEntryMenuRenderScrollBar(i); 
                }
                NameEntryMenuRenderTopWindowBorder(i);
                NameEntryMenuRenderBottomWindowBorder(i);
                NameEntryMenuRenderLeftWindowBorder(i);
                NameEntryMenuRenderRightWindowBorder(i);
                NameEntryMenuRenderWindowBackground(i);

                PopMatrix();
            } else {
                NameEntryMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) { 
                    NameEntryMenuRenderScrollBar(i); 
                }
                NameEntryMenuRenderTopWindowBorder(i);
                NameEntryMenuRenderBottomWindowBorder(i);
                NameEntryMenuRenderLeftWindowBorder(i);
                NameEntryMenuRenderRightWindowBorder(i);
                NameEntryMenuRenderWindowBackground(i);
            }
        }
    }
}

/* SIBLING member_change_menu:MemberChangeMenuDrawCursors */
void NameEntryMenuDrawCursors(void) {
    int i;

    if (g_Menu->pManager->shouldRenderPointerCursors) {
        for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
            if (g_Menu->pCursors->shouldRender[i]) {
                if (g_Menu->pCursors->unk144[i]) {
                    setXY4(
                        &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]],
                        NameEntryMenu_D_801CC014[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 8,
                        NameEntryMenu_D_801CC094[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] - 6,
                        NameEntryMenu_D_801CC014[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 24,
                        NameEntryMenu_D_801CC094[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] - 6,
                        NameEntryMenu_D_801CC014[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 8,
                        NameEntryMenu_D_801CC094[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 10,
                        NameEntryMenu_D_801CC014[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 24,
                        NameEntryMenu_D_801CC094[NameEntryMenu_D_801CBF9C[g_Menu->unk32C->unk4F7C]] + 10  
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

/* SIBLING member_change_menu:func_801C7DA8 */
void NameEntryMenu_801C8E38(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk34[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4],  &g_Menu->unk4E0[i].polys[g_Menu->unk4E0[i].renderContext]);
        }
    }
}

/* SIBLING member_change_menu:func_801C7E38 */
void NameEntryMenu_801C8EC8(void) {
    int i;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->unkC[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->unk6E0[i].polys[g_Menu->unk6E0[i].renderContext]);
        }
    }
}

/* SIBLING member_change_menu:func_801C7EC8 */
void NameEntryMenu_801C8F58(void) {
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

/* Queue the six label strings in g_Menu->unkDE0 (MenuStrings; 0x80 bytes each
 * on the PSX) whose manager flag is set, at OT slot 4.  The retail arm keeps
 * the PSX byte offsets: the original mixes a base-first and an offset-first
 * address in one call, which no field expression reproduces.  The port arm
 * is the same walk over the native array. */
void NameEntryMenu_801C90D0(void) {
    int i;
#ifdef XENO_PC_PORT
    MenuString* strings = (MenuString*)g_Menu->unkDE0;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk1A[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &strings[i].polys[strings[i].renderContext]);
        }
    }
#else
    int offset;

    i = 0;
    offset = 0xDE0;
    do {
        if (g_Menu->pManager->unk1A[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    offset + (u8*)g_Menu + ((u8*)g_Menu + (i << 7))[0xE5D] * sizeof(POLY_FT4));
        }
        i++;
        offset += 0x80;
    } while (i < 6);
#endif
}


/* SIBLING shop_menu:func_801CA22C */
void NameEntryMenu_801C9160(void) {
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

/* SIBLING member_change_menu:MemberChangeMenuRenderBackgroundDim */
void NameEntryMenuRenderBackgroundDim(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->drawModeDimEffect[g_Menu->renderContext]);
}

/* Name-entry work block (g_Menu->unk1E20, MenuUnk7: 0xDEC raw bytes) as
 * this overlay uses it.  Prim sizes are the retail ones in both builds. */
typedef struct {
    /* 0x000 */ POLY_FT4 polys0[2];
    /* 0x050 */ POLY_FT4 polys50[2];
    /* 0x0A0 */ POLY_FT4 polysA0[4];
    /* 0x140 */ POLY_FT4 polysPortrait[2];
    /* 0x190 */ union {
        POLY_FT4 all[0x4A]; /* initialised as one run */
        struct {
            /* 0x190 */ POLY_FT4 letters[0x48]; /* 36 letters x 2 contexts */
            /* 0xCD0 */ POLY_FT4 label[2];      /* name-kind label strip */
        } s;
    } grid;
    /* 0xD20 */ LINE_F3 linesD20[2];
    /* 0xD50 */ LINE_F3 linesD50[2];
    /* 0xD80 */ LINE_F2 linesCaret[2];
    /* 0xDA0 */ SVECTOR vertsPortrait[4];
    /* 0xDC0 */ SVECTOR vertsLabel[4];
    /* 0xDE0 */ u8 renderContext;
    /* 0xDE1 */ u8 portraitRenderContext;
    /* 0xDE2 */ u8 lettersRenderContext;
    /* 0xDE3 */ u8 labelRenderContext;
    /* 0xDE4 */ u8 linesRenderContext;
    /* 0xDE5 */ u8 drawPortrait;
    /* 0xDE6 */ u8 drawGrid;
    /* 0xDE7 */ u8 caretTimer;
    /* 0xDE8 */ u8 caretColumn;
    /* 0xDE9 */ u8 labelWidth; /* label strip width in 8-pixel units: 9 character, 10 Gear */
    /* 0xDEA */ u8 numPolysA0;
    /* 0xDEB */ u8 unkDEB;
} NameEntryMenuWork; // Size: 0xDEC

#ifdef XENO_PC_PORT
_Static_assert(sizeof(NameEntryMenuWork) == sizeof(MenuUnk7), "name entry work size");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, grid) == 0x190, "name entry letters");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, grid.s.label) == 0xCD0, "name entry label");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, linesD20) == 0xD20, "name entry lines");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, linesCaret) == 0xD80, "name entry caret");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, vertsPortrait) == 0xDA0, "name entry verts");
_Static_assert(__builtin_offsetof(NameEntryMenuWork, renderContext) == 0xDE0, "name entry bytes");
#endif

#define NAME_ENTRY_WORK ((NameEntryMenuWork*)g_Menu->unk1E20)

void NameEntryMenu_801C9338(void) {
    long interpolated;
    long flag;
    int i;

    if (g_Menu->pManager->unk47) {
        if (NAME_ENTRY_WORK->drawPortrait) {
            RotTransPers4(
                &NAME_ENTRY_WORK->vertsPortrait[0],
                &NAME_ENTRY_WORK->vertsPortrait[1],
                &NAME_ENTRY_WORK->vertsPortrait[2],
                &NAME_ENTRY_WORK->vertsPortrait[3],
                (long*)&NAME_ENTRY_WORK->polysPortrait[NAME_ENTRY_WORK->portraitRenderContext].x0,
                (long*)&NAME_ENTRY_WORK->polysPortrait[NAME_ENTRY_WORK->portraitRenderContext].x1,
                (long*)&NAME_ENTRY_WORK->polysPortrait[NAME_ENTRY_WORK->portraitRenderContext].x2,
                (long*)&NAME_ENTRY_WORK->polysPortrait[NAME_ENTRY_WORK->portraitRenderContext].x3,
                &interpolated,
                &flag
            );
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &NAME_ENTRY_WORK->polysPortrait[NAME_ENTRY_WORK->portraitRenderContext]
            );
            RotTransPers4(
                &NAME_ENTRY_WORK->vertsLabel[0],
                &NAME_ENTRY_WORK->vertsLabel[1],
                &NAME_ENTRY_WORK->vertsLabel[2],
                &NAME_ENTRY_WORK->vertsLabel[3],
                (long*)&NAME_ENTRY_WORK->grid.s.label[NAME_ENTRY_WORK->labelRenderContext].x0,
                (long*)&NAME_ENTRY_WORK->grid.s.label[NAME_ENTRY_WORK->labelRenderContext].x1,
                (long*)&NAME_ENTRY_WORK->grid.s.label[NAME_ENTRY_WORK->labelRenderContext].x2,
                (long*)&NAME_ENTRY_WORK->grid.s.label[NAME_ENTRY_WORK->labelRenderContext].x3,
                &interpolated,
                &flag
            );
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &NAME_ENTRY_WORK->grid.s.label[NAME_ENTRY_WORK->labelRenderContext]
            );
        }

        if (NAME_ENTRY_WORK->drawGrid) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &NAME_ENTRY_WORK->polys0[NAME_ENTRY_WORK->renderContext]);
            AddPrim(&g_Menu->pGfxEnv->ot[4], &NAME_ENTRY_WORK->polys50[NAME_ENTRY_WORK->renderContext]);

            for (i = 0; i < NAME_ENTRY_WORK->numPolysA0; i++) {
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &NAME_ENTRY_WORK->polysA0[i * 2 + NAME_ENTRY_WORK->renderContext]
                );
            }

            AddPrim(&g_Menu->pGfxEnv->ot[4], &NAME_ENTRY_WORK->linesD20[NAME_ENTRY_WORK->linesRenderContext]);
            AddPrim(&g_Menu->pGfxEnv->ot[4], &NAME_ENTRY_WORK->linesD50[NAME_ENTRY_WORK->linesRenderContext]);

            if (++NAME_ENTRY_WORK->caretTimer >= 0x3D) {
                NAME_ENTRY_WORK->caretTimer = 0;
            }

            if (NAME_ENTRY_WORK->caretTimer < 0x1E) {
                setXY2(
                    &NAME_ENTRY_WORK->linesCaret[g_Menu->renderContext],
                    NAME_ENTRY_WORK->caretColumn * 8 + 0x50, 0xC6,
                    NAME_ENTRY_WORK->caretColumn * 8 + 0x58, 0xC6
                );
                AddPrim(&g_Menu->pGfxEnv->ot[4], &NAME_ENTRY_WORK->linesCaret[g_Menu->renderContext]);
            }

            for (i = 0; i < 0x24; i++) {
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &NAME_ENTRY_WORK->grid.s.letters[i * 2 + NAME_ENTRY_WORK->lettersRenderContext]
                );
            }
        }
    }
}


/* SIBLING shop_menu:func_801CA404 */
void NameEntryMenu_801C97FC(void) {
    NameEntryMenu_801C8E38();
    NameEntryMenu_801C8EC8();
    NameEntryMenu_801C8F58();
    NameEntryMenu_801C90D0();
    NameEntryMenu_801C9160();
}

/* SIBLING member_change_menu:MemberChangeMenuRender */
void NameEntryMenuRender(void) {
    if (g_Menu->shouldDrawMenu) {
        NameEntryMenuUpdateWindows();
        NameEntryMenuDrawCursors();
        NameEntryMenu_801C97FC();
        NameEntryMenu_801C9338();
        NameEntryMenuRenderWindows();
    }
    NameEntryMenuRenderBackgroundDim();
}

/* SIBLING shop_menu:ShopMenuPlaySoundEffect */
void NameEntryMenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 16) | soundEffectId);
    }
}

/* SIBLING shop_menu:ShopMenuPollInput */
void NameEntryMenuPollInput(void) {
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
                NameEntryMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = MENU_INPUT_DOWN;
                NameEntryMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = MENU_INPUT_LEFT;
                NameEntryMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = MENU_INPUT_UP;
                NameEntryMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = MENU_INPUT_CONFIRM;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = MENU_INPUT_BACK;
                NameEntryMenuPlaySoundEffect(3);
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

/* SIBLING shop_menu:ShopMenuUpdateTransitionEffect */
void NameEntryMenuUpdateTransitionEffect(void) {
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

/* SIBLING shop_menu:ShopMenuUpdateAndRender */
void NameEntryMenuUpdateAndRender(void) {
    GfxEnvironment* pNextGfxEnv;
    int renderContext;

    // Debugging
    if (*D_8005917C != -1) {
#ifndef XENO_PC_PORT
        asm("break 0x400");   /* MIPS trap; host assembler can't emit it (debug-only) */
#endif
    }
    
    NameEntryMenuPollInput();
    GameCheckAndHandleSoftReset();

    // Rotate graphics environment
    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;
    
    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    NameEntryMenuUpdateTransitionEffect();
    NameEntryMenuRender();
    renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->unk1180, 0, renderContext * 0xE0);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
}

/* SIBLING member_change_menu:MemberChangeMenuInitializeCursors */
void NameEntryMenuInitializeCursors(unsigned char mode) {
    int i;

    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));
    
    switch (mode) {
        case 0:
            g_Menu->pManager->shouldRenderPointerCursors = TRUE;
            g_Menu->pCursors->unk144[0] = 1;
            g_Menu->pCursors->unk144[1] = 1;
            /* fallthrough */
        case 2:
            for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
                func_8002675C(
                    g_Menu->unk2DC, MENU_TEX_POINTER_CURSOR, 
                    &g_Menu->pCursors->polysCursor[i * 2], g_Menu->renderContext, 
                    NameEntryMenu_D_801CBEA0[i], NameEntryMenu_D_801CBEB0[i],
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

/* SIBLING member_change_menu:MemberChangeMenuFreeCursors */
void NameEntryMenuFreeCursors(void) {
    g_Menu->pManager->shouldRenderPointerCursors = FALSE;
    NameEntryMenuUpdateAndRender();
    HeapFree(g_Menu->pCursors);
}

/* SIBLING shop_menu:ShopMenuStartOpenMenuTransition */
void NameEntryMenuStartOpenMenuTransition(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    NameEntryMenuPlaySoundEffect(0x5B);
}

/* Start the close transition (state 4) with sound effect 0x5C. */
void NameEntryMenu_801C9F90(void) {
    g_Menu->transitionEffectState = 4;
    NameEntryMenuPlaySoundEffect(0x5C);
}


void NameEntryMenu_801C9FC0(u8 firstString) {
    int i;
    MenuString* pString;
    int x;

    x = 0x48;

    for (i = 0; i < 4; i++) {
        g_Menu->unk1DE0[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero(g_Menu->unk1DE0[i], sizeof(MenuString));
        if (!(i & 1)) {
            g_Menu->unk1DE0[i]->pVramBuffer = HeapAlloc(0x5CA, 0);
            g_Menu->unk1DE0[i]->vramDest.x = 0x140;
            g_Menu->unk1DE0[i]->vramDest.y = (i / 2) * 0xD + 0x4E;
            g_Menu->unk1DE0[i]->vramDest.w = 0x3A;
            g_Menu->unk1DE0[i]->vramDest.h = 0xD;
        } else {
            g_Menu->unk1DE0[i]->pVramBuffer = g_Menu->unk1DE0[i - 1]->pVramBuffer;
        }
    }

    for (i = 0; i < 3; i++) {
        pString = g_Menu->unk1DE0[i];
        pString->width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, firstString + i), pString->pVramBuffer, 0x36, i % 2
        );
        NameEntryMenu_801C5ABC(pString, i, 0, 0);
        NameEntryMenuSetVertices(pString->vertices, x, i * 0x10 + 0xA0, pString->width, 0xD);
        setUVWH(&pString->polys[g_Menu->renderContext], 0, (i / 2) * 0xD + 0x4E, pString->width, 0xD);
        pString->renderContext = g_Menu->renderContext;
        pString->unk7F = 1;
    }

    LoadImage(&g_Menu->unk1DE0[0]->vramDest, g_Menu->unk1DE0[0]->pVramBuffer);
    LoadImage(&g_Menu->unk1DE0[2]->vramDest, g_Menu->unk1DE0[2]->pVramBuffer);
    DrawSync(0);
    g_Menu->pManager->unk2E = 1;
    HeapFree(g_Menu->unk1DE0[0]->pVramBuffer);
    HeapFree(g_Menu->unk1DE0[2]->pVramBuffer);
    NameEntryMenuUpdateAndRender();
    NameEntryMenuUpdateAndRender();
}


/* Free the four strings in g_Menu->unk1DE0, then render one frame. */
void NameEntryMenu_801CA39C(void) {
    int i;

    g_Menu->pManager->unk2E = 0;
    for (i = 0; i < 4; i++) {
        HeapFree(g_Menu->unk1DE0[i]);
    }
    NameEntryMenuUpdateAndRender();
}


extern u8 g_MenuDebugEnabled;
extern void func_8003A094(void*);
extern void func_8003852C(void*);

/* Teardown (retail 0x801CA400): member_change_menu's MemberChangeMenuFree
 * sequence without the party write-back.  Two update/render frames, stop
 * drawing, render until the double buffer is back on context 0, free the
 * managers and the resource blocks NameEntryMenuInitialize loaded, unload the
 * debug sound bank, free the last manager, then free the SystemMenu block
 * MenuMain allocated. */
void NameEntryMenu_801CA400(void) {
    NameEntryMenuUpdateAndRender();
    NameEntryMenuUpdateAndRender();
    g_Menu->shouldDrawMenu = FALSE;
    NameEntryMenuUpdateAndRender();
    do {
        NameEntryMenuUpdateAndRender();
    } while (g_Menu->renderContext != 0);
    NameEntryMenu_801C505C(0);
    NameEntryMenuSetManager(0);
    NameEntryMenuSetSelectionMenu(0);
    NameEntryMenu_801C5188(0);
    NameEntryMenu_801C51EC(0);
    NameEntryMenu_801C5250(0);
    HeapFree(g_Menu->unk2DC);
    HeapFree(g_Menu->unk2E0);
    HeapFree(g_Menu->unk4E0[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        func_8003A094(g_Menu->unk2E4);
        NameEntryMenuUpdateAndRender();
        func_8003852C(g_Menu->unk2E4);
        NameEntryMenuUpdateAndRender();
        HeapFree(g_Menu->unk2E4);
    }
    NameEntryMenuUnk1E20Manager(0);
    HeapFree(g_Menu);
}


extern u8 NameEntryMenu_D_801CBEC0[0x24 * 6]; /* letter-grid glyph codes: 5 per letter, 6 apart */
extern void func_80033B34(u16* src, u8* dst, s32 count);

/* Build the 36-letter grid: one run of 0x4A textured quads (letters, then
 * the label strip), each letter rendered from its codes and uploaded to VRAM
 * (0x180 + (i % 2) * 0x20, (i / 2) * 0xD), laid out 4 columns x 9 rows; then
 * the label strip, the green frame lines and the caret line. */
void NameEntryMenu_801CA558(void) {
    u16 codes[5];
    u8 text[16];
    RECT rect;
    u8* pWork;
    int i;
    int j;

    pWork = HeapAlloc(0x2BE, 0);
    for (i = 0; i < 0x4A; i++) {
        SetPolyFT4(&NAME_ENTRY_WORK->grid.all[i]);
        setRGB0(&NAME_ENTRY_WORK->grid.all[i], 0x80, 0x80, 0x80);
        SetSemiTrans(&NAME_ENTRY_WORK->grid.all[i], 0);
        SetShadeTex(&NAME_ENTRY_WORK->grid.all[i], 1);
        NAME_ENTRY_WORK->grid.all[i].clut = g_SystemPalette1;
        NAME_ENTRY_WORK->grid.all[i].tpage = GetTPage(0, 0, 0x180, 0);
    }

    for (i = 0; i < 0x24; i++) {
        for (j = 0; j < 5; j++) {
            codes[j] = NameEntryMenu_D_801CBEC0[i * 6 + j];
        }
        func_80033B34(codes, text, 5);
        bzero(pWork, 0x2BE);
        SystemRenderStringEntry(text, pWork, 0x18, 0);
        setRECT(&rect, (i % 2) * 0x20 + 0x180, (i / 2) * 0xD, 0x1C, 0xD);
        LoadImage(&rect, pWork);
        setUVWH(&NAME_ENTRY_WORK->grid.s.letters[i * 2 + g_Menu->renderContext], (i % 2) << 7, (i / 2) * 0xD, 0x3C, 0xD);
        setXYWH(&NAME_ENTRY_WORK->grid.s.letters[i * 2 + g_Menu->renderContext], (i / 9) * 0x30 + 0x44, (i % 9) * 0x10 + 0x2E, 0x3C, 0xD);
        DrawSync(0);
    }
    HeapFree(pWork);
    NAME_ENTRY_WORK->lettersRenderContext = g_Menu->renderContext;

    setUVWH(&NAME_ENTRY_WORK->grid.s.label[g_Menu->renderContext], 0, 0xEA, NAME_ENTRY_WORK->labelWidth * 8, 0xD);
    NameEntryMenuSetVertices(
        NAME_ENTRY_WORK->vertsLabel,
        NAME_ENTRY_WORK->grid.s.label[g_Menu->renderContext].x0 + 0x50,
        NAME_ENTRY_WORK->grid.s.label[g_Menu->renderContext].y0 + 0xB6,
        NAME_ENTRY_WORK->labelWidth * 8,
        0xD
    );
    NAME_ENTRY_WORK->labelRenderContext = g_Menu->renderContext;

    for (i = 0; i < 2; i++) {
        SetLineF3(&NAME_ENTRY_WORK->linesD20[i]);
        setRGB0(&NAME_ENTRY_WORK->linesD20[i], 0, 0xFF, 0);
        SetLineF3(&NAME_ENTRY_WORK->linesD50[i]);
        setRGB0(&NAME_ENTRY_WORK->linesD50[i], 0, 0xFF, 0);
        SetLineF2(&NAME_ENTRY_WORK->linesCaret[i]);
        setRGB0(&NAME_ENTRY_WORK->linesCaret[i], 0, 0x80, 0);
    }
}


void NameEntryMenu_801CADC8(void) {
    NameEntryMenuInitializeWindow(3, 0x10, 0x9A, 0xC0, 0x3C, 1, 1, 4, 0);
    NameEntryMenu_801C9FC0(0x1D);
    func_8002675C(g_Menu->unk2DC, 0x14B, NAME_ENTRY_WORK->polysPortrait, g_Menu->renderContext, 0, 0, 0x1000);
    NameEntryMenuSetVertices(
        NAME_ENTRY_WORK->vertsPortrait,
        NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].x0 + 0x18,
        NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].y0 + 0x9E,
        NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].x1 - NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].x0,
        NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].y3 - NAME_ENTRY_WORK->polysPortrait[g_Menu->renderContext].y0
    );
    NAME_ENTRY_WORK->portraitRenderContext = g_Menu->renderContext;
    func_8002675C(g_Menu->unk2DC, 0xF9, NAME_ENTRY_WORK->polys0, g_Menu->renderContext, 0xE8, 0xB6, 0x1000);
    func_8002675C(g_Menu->unk2DC, 0xFC, NAME_ENTRY_WORK->polys50, g_Menu->renderContext, 0xE0, 0xC6, 0x1000);
    NAME_ENTRY_WORK->numPolysA0 = func_8002675C(
        g_Menu->unk2DC, 0xF0, NAME_ENTRY_WORK->polysA0, g_Menu->renderContext, 0xF4, 0x6E, 0x1000
    );
    NAME_ENTRY_WORK->renderContext = g_Menu->renderContext;
    setXYWH(&g_Menu->unk4E0[0].polys[g_Menu->renderContext], 0xF8, 0xB6, g_Menu->unk4E0[0].width, 0xD);
    setXYWH(&g_Menu->unk4E0[3].polys[g_Menu->renderContext], 0xF0, 0xC6, g_Menu->unk4E0[3].width, 0xD);
    g_Menu->unk4E0[3].renderContext = g_Menu->renderContext;
    NameEntryMenu_801CA558();
    NAME_ENTRY_WORK->drawPortrait = 1;
    g_Menu->pManager->unk47 = 1;
}


/* Render a string entry into a work buffer and upload it to VRAM
 * (0x180, 0xEA, 0x28 x 0xD). */
void NameEntryMenu_801CB1C4(void* pString) {
    RECT rect;
    void* pWork;

    pWork = HeapAlloc(0x3F6, 0);
    bzero(pWork, 0x3F6);
    SystemRenderStringEntry(pString, pWork, 0x24, 0);
    rect.x = 0x180;
    rect.y = 0xEA;
    rect.w = 0x28;
    rect.h = 0xD;
    LoadImage(&rect, pWork);
    DrawSync(0);
    HeapFree(pWork);
}


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/name_entry_menu/nonmatchings/main/misc", NameEntryMenu_801CB25C);
#else
/* NON_MATCHING (port body): 7 rows of scheduling / addu-order difference remain; retail keeps the asm. */
extern u8 D_80059171; /* FE58 argument: name-table id (characters 0-10, Gears 11-30) */

/* Start an edit: fill the 10-glyph edit buffer with blanks (0x1C) ending in
 * the 0x1F end marker, and copy name D_80059171 (g_GameState + id * 0x14,
 * 20 bytes) as the starting text. */
void NameEntryMenu_801CB25C(u8* glyphs, u8* name) {
    int i;

    for (i = 0; i < 0x14; i += 2) {
        glyphs[i] = 0x1C;
        glyphs[i + 1] = 0;
        name[i] = g_GameState.field_0x0[D_80059171 * 0x14 + i];
        name[i + 1] = g_GameState.field_0x0[D_80059171 * 0x14 + i + 1];
    }
    glyphs[0x12] = 0x1F;
    glyphs[0x13] = 0;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/name_entry_menu/nonmatchings/main/misc", NameEntryMenu_801CB2F0);
#else
/* NON_MATCHING (port body): gcc rotates the loop differently (size 72-80 vs 76); retail keeps the asm. */
/* Length, in 2-byte glyphs, of a name-entry glyph buffer: entries up to the
 * 0x1F,0x00 end marker, at most 10. */
u8 NameEntryMenu_801CB2F0(u8* glyphs) {
    int i;

    i = 0;
    while (glyphs[0] != 0x1F || glyphs[1] != 0) {
        i += 2;
        glyphs += 2;
        if (i >= 0x14) {
            break;
        }
    }
    return i / 2;
}
#endif

/* The name-entry input loop (the game's own on-screen keyboard).  The edit
 * starts from blank glyphs, showing the current name until the first change.
 * The cursor walks the letter grid (24 cell columns x 5 rows of
 * NameEntryMenu_D_801CBEC0; 0xF / 0xCF cells are skipped, 0xFF ends a row,
 * 0xCF a column).  Selecting a letter writes its code at the caret;
 * selecting the end cell (0x1F) or pressing Start finishes unless the first
 * glyph was erased.  The text, trailing 0x4F blanks trimmed, is written to
 * the up to three name-table entries listed in g_Menu->unk1E24[4..6].
 * NameEntryMenu_801CB2F0 is asm (implicitly int) in the matching build, so
 * its result is cast to u8 as retail zero-extends it. */
void NameEntryMenu_801CB33C(void) {
    u8 glyphs[0x14];
    u8 text[0x14];
    int prevRow;
    int col;
    int row;
    int prevCol;
    int index;
    int i;
    int k;
    int last;
    u8* name;
    u8 code;
    u8 dirty;
    u8 running;

    prevCol = 0xFF;
    prevRow = 0xFF;
    col = 0x16;
    row = 4;
    NameEntryMenu_801CB25C(glyphs, text);
    dirty = 0;
    NameEntryMenu_801CADC8();
    NameEntryMenu_801CB1C4(text);
    running = 1;
    NameEntryMenuStartOpenMenuTransition();
    while (g_Menu->transitionEffectState != 0) {
        NameEntryMenuUpdateAndRender();
    }
    NameEntryMenuInitializeWindow(2, 0x38, 0x26, 0xD0, 0x60, 1, 0, 4, 0);
    while (g_Menu->windowParameters[2]->unk11 == 0) {
        NameEntryMenuUpdateAndRender();
    }
    NAME_ENTRY_WORK->drawGrid = 1;
    g_Menu->pManager->unk34[0] = 1;
    g_Menu->pManager->unk34[3] = 1;
    NameEntryMenuInitializeCursors(1);
    g_Menu->pManager->shouldRenderPointerCursors = 1;

    while (running) {
        if (col != prevCol || row != prevRow) {
            func_8002675C(
                g_Menu->unk2DC, 0x108, g_Menu->pCursors->polysCursor, g_Menu->renderContext,
                col * 8 + 0x48, row * 0x10 + 0x36, 0x800
            );
            setXY3(
                &NAME_ENTRY_WORK->linesD20[g_Menu->renderContext],
                col * 8 + 0x44, row * 0x10 + 0x2E,
                col * 8 + 0x4C, row * 0x10 + 0x2E,
                col * 8 + 0x4C, row * 0x10 + 0x3A
            );
            setXY3(
                &NAME_ENTRY_WORK->linesD50[g_Menu->renderContext],
                col * 8 + 0x44, row * 0x10 + 0x2E,
                col * 8 + 0x44, row * 0x10 + 0x3A,
                col * 8 + 0x4C, row * 0x10 + 0x3A
            );
            NAME_ENTRY_WORK->linesRenderContext = g_Menu->renderContext;
            prevCol = col;
            g_Menu->pCursors->renderContexts[0] = g_Menu->renderContext;
            prevRow = row;
            g_Menu->pCursors->shouldRender[0] = 1;
        }
        if (dirty) {
            NameEntryMenu_801CB1C4(text);
            dirty = 0;
        }
        NameEntryMenuUpdateAndRender();

        switch (g_Menu->input) {
        /* Start is listed first: that source order gives retail's loop
         * invariant order; its block is cross-jumped into case 4's identical
         * end-cell tail, which is where retail lays it out. */
        case 11:
            if (glyphs[0] != 0xF) {
                running = 0;
            }
            break;
        case 5:
            if (NAME_ENTRY_WORK->caretColumn != 0) {
                NAME_ENTRY_WORK->caretColumn--;
            }
            glyphs[NAME_ENTRY_WORK->caretColumn * 2] = 0xF;
            glyphs[NAME_ENTRY_WORK->caretColumn * 2 + 1] = 0;
            func_80033B34(glyphs, text, (u8)NameEntryMenu_801CB2F0(glyphs));
            dirty = 1;
            break;
        case 4:
            index = (row + (col / 6) * 9) * 6 + col % 6;
            code = NameEntryMenu_D_801CBEC0[index];
            if (code != 0x1F) {
                if (NAME_ENTRY_WORK->caretColumn < NAME_ENTRY_WORK->labelWidth) {
                    if (code == 0xCF || code == 0xF) {
                        NameEntryMenu_D_801CBEC0[index] = 0xC3;
                    }
                    glyphs[NAME_ENTRY_WORK->caretColumn * 2] = NameEntryMenu_D_801CBEC0[index];
                    glyphs[NAME_ENTRY_WORK->caretColumn * 2 + 1] = 0;
                    func_80033B34(glyphs, text, (u8)NameEntryMenu_801CB2F0(glyphs));
                    dirty = 1;
                    NAME_ENTRY_WORK->caretColumn++;
                    NameEntryMenuPlaySoundEffect(2);
                } else {
                    NameEntryMenuPlaySoundEffect(4);
                }
            } else if (glyphs[0] != 0xF) { /* the end cell finishes like Start */
                running = 0;
            }
            break;
        case 0:
            col++;
            index = (row + (col / 6) * 9) * 6 + col % 6;
            code = NameEntryMenu_D_801CBEC0[index];
            if (code == 0xFF) {
                col = 0;
            } else if (code == 0xF || code == 0xCF) {
                col++;
            }
            break;
        case 2:
            col--;
            if (col < 0) {
                col = 0x16;
            } else {
                index = (row + (col / 6) * 9) * 6 + col % 6;
                code = NameEntryMenu_D_801CBEC0[index];
                if (code == 0xF || code == 0xCF) {
                    col--;
                }
            }
            break;
        case 1:
            row++;
            if (row >= 5) {
                row = 0;
            }
            index = (row + (col / 6) * 9) * 6 + col % 6;
            code = NameEntryMenu_D_801CBEC0[index];
            if (code == 0xCF) {
                row = 0;
            } else if (code == 0xF) {
                row++;
            }
            break;
        case 3:
            row--;
            if (row < 0) {
                row = 4;
            }
            index = (row + (col / 6) * 9) * 6 + col % 6;
            code = NameEntryMenu_D_801CBEC0[index];
            if (code == 0xF || code == 0xCF) {
                row--;
            }
            break;
        case 9:
            if (++NAME_ENTRY_WORK->caretColumn > NAME_ENTRY_WORK->labelWidth) {
                NAME_ENTRY_WORK->caretColumn--;
            }
            break;
        case 10:
            if (NAME_ENTRY_WORK->caretColumn != 0) {
                NAME_ENTRY_WORK->caretColumn--;
            }
            break;
        }
    }

    for (k = 0, last = 0; k < 3; k++) {
        for (i = 0; i < 0x14; i++) {
            name = &g_GameState.field_0x0[g_Menu->unk1E24[4 + k] * 0x14];
            name[i] = 0;
        }
        for (i = 0; i < 0x12; i++) {
            name = &g_GameState.field_0x0[g_Menu->unk1E24[4 + k] * 0x14];
            name[i] = text[i];
            if (text[i] == 0) {
                break;
            }
            if (text[i] != 0x4F) {
                last = i;
            }
        }
        for (i = last + 1; i < 0x14; i++) {
            name = &g_GameState.field_0x0[g_Menu->unk1E24[4 + k] * 0x14];
            name[i] = 0;
        }
    }

    g_Menu->pManager->unk34[0] = 0;
    g_Menu->pManager->unk34[3] = 0;
    NameEntryMenuFreeCursors();
    NAME_ENTRY_WORK->drawGrid = 0;
    NameEntryMenuFreeWindow(2);
    NameEntryMenu_801C9F90();
    while (g_Menu->translation.vz < 0x600) {
        NameEntryMenuUpdateAndRender();
    }
    g_Menu->pManager->unk47 = 0;
    NameEntryMenu_801CA39C();
    NameEntryMenuFreeWindow(3);
}


void NameEntryMenu_801CB33C(void);
void NameEntryMenu_801CA400(void);

/* Menu mode 3 entry (retail 0x801CBDBC; MenuExecute case 3).  Same shape as
 * MemberChangeMenuMain: allocate the managers, set up the selection-menu
 * VRAM rect, initialise, run the input loop (0x801CB33C), tear down. */
void NameEntryMenuMain(void) {
    NameEntryMenu_801C505C(1);
    NameEntryMenuSetManager(1);
    NameEntryMenuSetSelectionMenu(1);
    NameEntryMenu_801C5188(1);
    NameEntryMenu_801C51EC(1);
    NameEntryMenu_801C5250(1);
    NameEntryMenuUnk1E20Manager(1);
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    NameEntryMenuInitialize();
    NameEntryMenuResetRenderContext();
    NameEntryMenu_801C5EAC();
    NameEntryMenuInitializeBackground();
    NameEntryMenuInitializeWindowBorders();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->unk32A = 1;
    NameEntryMenu_801CB33C();
    NameEntryMenu_801CA400();
}

