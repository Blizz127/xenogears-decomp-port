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
extern u16 g_SystemPalette2;

#ifdef XENO_PC_PORT
/* The port needs real prototypes: these return host pointers or u_short. */
extern void* GetWeaponName(s32);
extern void* GetAccessoryName(s32);
extern void* GetItemName(s32);
extern s32 SystemRenderStringEntry(void*, void*, s32, s32);
extern void func_80033B34(u16*, u8*, s32);
extern void func_801C5A7C(MenuString*, s32, s32, u8);
extern void func_801C5040(POLY_FT4*, short, short, u_char, u_char, short, short);
extern void ShopMenuSetVertices(SVECTOR*, u16, u16, u16, u16);
extern s32 func_8002675C(void*, s32, POLY_FT4*, s32, s32, s32, s32);
extern u_short ShopMenuGetCharacterEquippedItemFlags(u_char, u_char);
extern u_short ShopMenuIsCharacterFlagSet(u_short, u_char);
#endif

/* Retail 801CFF58..801D05BC (matched). Sell-side item detail for the
 * highlighted entry: renders the name into strItemDesc and the sell price
 * (half the list price, five digits with leading blanks) into str44B0,
 * which is drawn with g_SystemPalette2; marks which portraits can equip the
 * item and puts the equipped marker on those that already wear it. Returns
 * the sell price. Retail details that decided the match: the digit loop
 * shape shared with func_801D05BC, an unused 16-byte local after the RECT
 * (retail's frame has the gap), and the down-counter `j` doubling as the
 * portrait index in the portrait loop. */
int func_801CFF58(u_char id, u_char type) {
    RECT rect;
    u8 unused[16];
    u16 digits[7];
    u_char priceString[16];
    int divisors[5];
    void* work;
    u_short equipFlags;
    u_short equipped;
    u_char started;
    int sellPrice;
    int remaining;
    int quotient;
    int character;
    int j;
    int k;

    equipFlags = 0;
    divisors[0] = 1;
    divisors[1] = 10;
    divisors[2] = 100;
    divisors[3] = 1000;
    divisors[4] = 10000;
    work = HeapAlloc(0x618, 0);
    bzero(work, 0x618);
    bzero(digits, sizeof(digits));

    switch (type) {
        case ITEM_TYPE_WEAPON:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetWeaponName(id), work, 0x39, 0);
            sellPrice = g_Menu->unk330->pWeaponsData[id].price / 2;
            equipFlags = g_Menu->unk330->pWeaponsData[id].equipFlags;
            remaining = sellPrice;
            break;

        case ITEM_TYPE_ACCESSORY:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetAccessoryName(id), work, 0x39, 0);
            sellPrice = g_Menu->unk330->pAccessoriesData[id].price / 2;
            equipFlags = g_Menu->unk330->pAccessoriesData[id].equipFlags;
            remaining = sellPrice;
            break;

        case ITEM_TYPE_ITEM:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetItemName(id), work, 0x39, 0);
            sellPrice = g_Menu->unk330->pItemsData[id].price / 2;
            remaining = sellPrice;
            break;
    }

    equipped = ShopMenuGetCharacterEquippedItemFlags(id, type);

    started = FALSE;
    for (k = 0, j = 4; j > 0; k++, j--) {
        quotient = remaining / divisors[j];
        if (quotient != 0 || started) {
            ((u_char*)digits)[k * 2] = quotient + 0x10;
            remaining -= quotient * divisors[j];
            started = TRUE;
        } else {
            ((u_char*)digits)[k * 2] = 0xC3;
        }
    }
    ((u_char*)digits)[k * 2] = remaining % 10 + 0x10;

    func_80033B34(digits, priceString, 5);
    g_Menu->pShop->str44B0.width = SystemRenderStringEntry(priceString, work, 0x39, 1);

    rect.x = 320;
    rect.y = 78;
    rect.w = 60;
    rect.h = 13;
    LoadImage(&rect, work);
    DrawSync(0);

    func_801C5A7C(&g_Menu->pShop->strItemDesc, 0, 0, 0);
    func_801C5A7C(&g_Menu->pShop->str44B0, 0, 0, 0);

    g_Menu->pShop->str44B0.polys[g_Menu->renderContext].clut = g_SystemPalette2;
    func_801C5040(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 44, 18, 0, 78, g_Menu->pShop->strItemDesc.width, 13);
    func_801C5040(&g_Menu->pShop->str44B0.polys[g_Menu->renderContext], 152, 18, 0, 78, g_Menu->pShop->str44B0.width, 13);
    ShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 44, 18, g_Menu->pShop->strItemDesc.width, 13);
    ShopMenuSetVertices(g_Menu->pShop->str44B0.vertices, 152, 18, g_Menu->pShop->str44B0.width, 13);
    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    g_Menu->pShop->str44B0.renderContext = g_Menu->renderContext;

    HeapFree(work);

    if (id) {
        g_Menu->pShop->unk46A7 = TRUE;
        g_Menu->pShop->unk46B5 = TRUE;
    } else {
        g_Menu->pShop->unk46A7 = FALSE;
        g_Menu->pShop->unk46B5 = FALSE;
    }

    g_Menu->pShop->unk46A9 = 0;
    for (character = 0, j = 0; character < 16; character++) {
        if (g_Menu->availableCharacters[character]) {
            if (ShopMenuIsCharacterFlagSet(equipFlags, character)) {
                g_Menu->pShop->unk469C[j] = TRUE;
            } else {
                g_Menu->pShop->unk469C[j] = FALSE;
            }

            if (ShopMenuIsCharacterFlagSet(equipped, character)) {
                g_Menu->pShop->unk46A9 += func_8002675C(g_Menu->unk2DC, 0xE,
                    &g_Menu->pShop->polys2D0[g_Menu->pShop->unk46A9 * 2], g_Menu->renderContext,
                    D_801D21CC[j] + 14, 180, 0x1000);
            }
            j++;
        }
    }

    g_Menu->pShop->unk46A8 = g_Menu->renderContext;
    return sellPrice;
}


/* MenuShop's two MenuString lists at 0x3C30 and 0x4030 are named only in the
 * port layout (the native MenuString is larger). The PSX layout keeps them
 * inside unk3C10, so the matching build reaches them through this overlay;
 * a member-array access is what gives retail's base-first address adds. */
#ifdef XENO_PC_PORT
#define SHOP_NAME_STRINGS(shop) ((shop)->strings3C30)
#define SHOP_COUNT_STRINGS(shop) ((shop)->strings4030)
#else
typedef struct {
    /* 0x0000 */ u8 unk0[0x3C30];
    /* 0x3C30 */ MenuString names[8];
    /* 0x4030 */ MenuString counts[8];
} MenuShopListStrings;
#define SHOP_NAME_STRINGS(shop) (((MenuShopListStrings*)(shop))->names)
#define SHOP_COUNT_STRINGS(shop) (((MenuShopListStrings*)(shop))->counts)
#endif

/* Retail 801D05BC..801D0C18 (matched). Builds the eight visible rows of the
 * sell list starting at `firstItem`: the item name and the owned count
 * (five digits, leading blanks 0xC3) are rendered into the per-row name and
 * count strings, and a selected sell quantity draws an "x NN" glyph run
 * into unk1DB0. The name row's renderContext is stored twice and the count
 * row's never, as in retail (the buy list func_801CDD14 does the same).
 * Retail details that decided the match: the divisor table is filled by
 * explicit stores (an initializer becomes a .rodata copy); the digit loop
 * walks an up-counting index `k` and the fifth digit is stored through it
 * after the loop; the per-row strings are member arrays (see the overlay
 * above); and the glyph polygon base is written inline in each call. */
void func_801D05BC(int firstItem, u_char* itemIDs, u_char* itemTypes, u_char* sellQuantities, u_char* ownedQuantities) {
    RECT rect;
    u16 digits[7];
    u_char countString[16];
    int divisors[5];
    void* work;
    u_char tens;
    u_char started;
    int count;
    int quotient;
    int row;
    int j;
    int k;

    divisors[0] = 1;
    divisors[1] = 10;
    divisors[2] = 100;
    divisors[3] = 1000;
    divisors[4] = 10000;
    work = HeapAlloc(0x3F6, 0);

    for (row = 0; row < MAX_ITEMS_IN_VIEW; row++) {
        bzero(digits, sizeof(digits));
        g_Menu->pShop->unk468C[row] = 0;

        if (itemIDs[firstItem + row]) {
            count = ownedQuantities[firstItem + row];

            switch (itemTypes[firstItem + row]) {
                case ITEM_TYPE_WEAPON:
                    SHOP_NAME_STRINGS(g_Menu->pShop)[row].width = SystemRenderStringEntry(GetWeaponName(itemIDs[firstItem + row]), work, 0x24, 0);
                    break;

                case ITEM_TYPE_ACCESSORY:
                    SHOP_NAME_STRINGS(g_Menu->pShop)[row].width = SystemRenderStringEntry(GetAccessoryName(itemIDs[firstItem + row]), work, 0x24, 0);
                    break;

                case ITEM_TYPE_ITEM:
                    SHOP_NAME_STRINGS(g_Menu->pShop)[row].width = SystemRenderStringEntry(GetItemName(itemIDs[firstItem + row]), work, 0x24, 0);
                    break;
            }

            started = FALSE;
            for (k = 0, j = 4; j > 0; k++, j--) {
                quotient = count / divisors[j];
                if (quotient != 0 || started) {
                    ((u_char*)digits)[k * 2] = quotient + 0x10;
                    count -= quotient * divisors[j];
                    started = TRUE;
                } else {
                    ((u_char*)digits)[k * 2] = 0xC3;
                }
            }
            ((u_char*)digits)[k * 2] = count % 10 + 0x10;

            func_80033B34(digits, countString, 5);
            SHOP_COUNT_STRINGS(g_Menu->pShop)[row].width = SystemRenderStringEntry(countString, work, 0x24, 1);

            rect.x = (row & 1) * 24 + 384;
            rect.y = (row / 2) * 13 + 128;
            rect.w = 40;
            rect.h = 13;
            LoadImage(&rect, work);
            func_801C5A7C(&SHOP_NAME_STRINGS(g_Menu->pShop)[row], row, 0x80, 0x81);
            ShopMenuSetVertices(SHOP_NAME_STRINGS(g_Menu->pShop)[row].vertices, 0x24, 50 + row * 13,
                                SHOP_NAME_STRINGS(g_Menu->pShop)[row].width, 13);

            rect.x = (row & 1) * 24 + 384;
            rect.y = (row / 2) * 13 + 128;
            rect.w = 40;
            rect.h = 13;
            LoadImage(&rect, work);
            DrawSync(0);
            func_801C5A7C(&SHOP_COUNT_STRINGS(g_Menu->pShop)[row], row, 0x80, 0x82);
            ShopMenuSetVertices(SHOP_COUNT_STRINGS(g_Menu->pShop)[row].vertices, 0x8C, 50 + row * 13,
                                SHOP_COUNT_STRINGS(g_Menu->pShop)[row].width, 13);

            SHOP_NAME_STRINGS(g_Menu->pShop)[row].renderContext = g_Menu->renderContext;
            SHOP_NAME_STRINGS(g_Menu->pShop)[row].renderContext = g_Menu->renderContext;
            g_Menu->pShop->unk4684[row] = TRUE;

            if (sellQuantities[firstItem + row]) {
                g_Menu->pShop->unk468C[row] += func_8002675C(g_Menu->unk2DC, 0xE5,
                    (POLY_FT4*)(g_Menu->pShop->unk1DB0 + row * 0x140), g_Menu->renderContext,
                    0xB4, 0x36 + row * 13, 0x1000);

                tens = sellQuantities[firstItem + row] / 10;
                if (tens) {
                    g_Menu->pShop->unk468C[row] += func_8002675C(g_Menu->unk2DC, tens,
                        &((POLY_FT4*)(g_Menu->pShop->unk1DB0 + row * 0x140))[g_Menu->pShop->unk468C[row] * 2], g_Menu->renderContext,
                        0xBC, 0x36 + row * 13, 0x1000);
                }

                g_Menu->pShop->unk468C[row] += func_8002675C(g_Menu->unk2DC, (u_char)(sellQuantities[firstItem + row] % 10),
                    &((POLY_FT4*)(g_Menu->pShop->unk1DB0 + row * 0x140))[g_Menu->pShop->unk468C[row] * 2], g_Menu->renderContext,
                    0xC4, 0x36 + row * 13, 0x1000);

                g_Menu->pShop->unk4694[row] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->unk4684[row] = FALSE;
        }
    }

    HeapFree(work);
}


/* Retail 801D0C18..801D0E68 (matched). Applies a confirmed sale: store
 * the new gold (capped at MAX_GOLD_AMOUNT), then remove the sold units.
 * Inventory lists (uniformType) subtract each sold quantity from the
 * matching ids/quantities entry and clear the id when it reaches zero;
 * the equipment list clears the sold weapon/accessory slot of `character`
 * (weapons below id 50 live in unk6A[0], the others in unk6F[0..3]). */
void ShopMenuHandleSoldItems(u_int newGold, u_char* itemIDs, u_char* sellQuantities, int count,
                             u_char* ids, u_char* quantities, u_char* itemTypes,
                             u_char uniformType, u_char character) {
    int i;
    int j;
    int id;

    ShopMenuPlaySoundEffect(0xD1);

    g_GameState.gold = newGold;
    if (newGold > MAX_GOLD_AMOUNT) {
        g_GameState.gold = MAX_GOLD_AMOUNT;
    }

    if (uniformType) {
        for (i = 0; i < count; i++) {
            if (itemIDs[i]) {
                for (j = 0; j < count; j++) {
                    if (itemIDs[i] == ids[j]) {
                        quantities[j] -= sellQuantities[i];
                        if (quantities[j] == 0) {
                            ids[j] = 0;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            if (itemIDs[i] && sellQuantities[i]) {
                switch (itemTypes[i]) {
                    case ITEM_TYPE_WEAPON:
                        if (itemIDs[i] < 50) {
                            g_GameState.characters[character].unk6A[0] = 0;
                        } else {
                            for (j = 0, id = itemIDs[i]; j < 4; j++) {
                                if (id == g_GameState.characters[character].unk6F[j]) {
                                    g_GameState.characters[character].unk6F[j] = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ITEM_TYPE_ACCESSORY:
                        for (j = 0; j < 3; j++) {
                            if (itemIDs[i] == g_GameState.characters[character].unk74[j]) {
                                g_GameState.characters[character].unk74[j] = 0;
                                break;
                            }
                        }
                        break;
                }
            }
        }
    }
}


/* Retail 801D0E68..801D1658 (matched). The quantity-picking sell list.
 * `ids`/`quantities` are `count` parallel inventory arrays; `types` gives a
 * per-entry item type unless `uniformType` is set, in which case every entry
 * is `type`. Sellable entries (nonzero id and quantity, and no item flag
 * 0x10) are compacted into the itemIDs/ownedQuantities/sellable lists;
 * itemTypes stays uncompacted, as in retail (harmless for its callers, whose
 * entries all share one type). RIGHT/LEFT move one unit between owned and
 * sold, CONFIRM asks and hands the result to ShopMenuHandleSoldItems.
 *
 * Its switch table is now compiled into this TU's .rodata, which the linker
 * script places right after misc8's at 0x801C5028 -- the slot the
 * INCLUDE_RODATA(jtbl_801C5028) blob used to fill. Retail details that
 * decided the match: the scalar state is initialized before the VLAs are
 * allocated; gold/total/goldAfter are initialized in that order; numEntries
 * is only stored from a separate compaction index; the flag 0x10 test is
 * an if-store that jump optimization turns into sltiu; and LEFT updates
 * `total` before raising shouldUpdateGold. */
void ShopMenuSellMenu(int count, u_char* ids, u_char* quantities, u_char type, u_char uniformType, u_char* types, u_char character) {
    u_char isRunning = TRUE;
    u_char shouldInitialize = TRUE;
    u_char shouldUpdateGold = TRUE;
    int row = 0;
    int prevRow = 0xFF;
    int scroll = 0;
    int prevScroll = 0xFF;
    u_char itemIDs[count];
    u_char itemTypes[count];
    u_char sellQuantities[count];
    u_char sellable[count];
    u_char ownedQuantities[count];
    int gold = g_GameState.gold;
    int total = 0;
    int goldAfter = gold;
    int price;
    int numEntries;
    int j;
    u_char isSellable;
    int i;

    for (i = 0; i < count; i++) {
        ownedQuantities[i] = 0;
        sellable[i] = 0;
        sellQuantities[i] = 0;
        itemIDs[i] = 0;
        if (uniformType) {
            itemTypes[i] = type;
        } else {
            itemTypes[i] = types[i];
        }
    }

#ifdef XENO_PC_PORT
    /* Retail never stores numEntries when nothing is sellable (its stack
     * slot is only written inside the loop below) and then reads it for the
     * scroll limit. Give the port a defined value instead of host stack
     * garbage; the matching build is unchanged. */
    numEntries = 0;
#endif
    j = 0;
    for (i = 0; i < count; i++) {
        if (ids[i] && quantities[i]) {
            isSellable = TRUE;
            if (itemTypes[i] == ITEM_TYPE_ITEM) {
                if (g_Menu->unk330->pItemsData[ids[i]].flags & 0x10) {
                    isSellable = FALSE;
                }
            }
            if (isSellable) {
                itemIDs[j] = ids[i];
                ownedQuantities[j] = quantities[i];
                sellable[j] = TRUE;
                numEntries = ++j;
            }
        }
    }

    ShopMenuInitializeArrowCursor(0);

    while (isRunning) {
        ShopMenuUpdateAndRender();

        if (scroll != prevScroll || shouldUpdateGold) {
            func_801D05BC(scroll, itemIDs, itemTypes, sellQuantities, ownedQuantities);
            ShopMenuUpdateScrollBarHandle(0xC, 0x32, 0x3C, numEntries, scroll);
        }

        if (row != prevRow || scroll != prevScroll) {
            prevRow = row;
            price = func_801CFF58(itemIDs[scroll + row], itemTypes[scroll + row]);
            g_Menu->pManager->unk5A = TRUE;
            prevScroll = scroll;
        }

        ShopMenuUpdateArrowCursor(row, scroll, 0, 0);

        if (shouldInitialize) {
            ShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            ShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            ShopMenuStartOpenMenuTransition();
            if (uniformType) {
                ShopMenuUpdateCharacterPortraits();
            }
            while (g_Menu->transitionEffectState) {
                ShopMenuUpdateAndRender();
            }
            ShopMenuUpdateSellMenuExplanationGraphics();
            shouldInitialize = FALSE;
        }

        if (shouldUpdateGold) {
            ShopMenuUpdateGoldGraphics(gold, total, goldAfter);
            shouldUpdateGold = FALSE;
        }

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (total != 0) {
                    ShopMenuPlaySoundEffect(2);
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    isRunning = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    ShopMenuSetFinalPriceGraphics(total);
                    if (ShopMenuConfirmationWindow(0x95, 0xFF, 1)) {
                        ShopMenuHandleSoldItems(goldAfter, itemIDs, sellQuantities, count, ids, quantities, itemTypes, uniformType, character);
                    } else {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = isRunning;
                        g_Menu->pManager->shouldRenderWindow[3] = isRunning;
                        g_Menu->pManager->scrollHandleActive = isRunning;
                        prevScroll = 0xFF;
                        prevRow = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = isRunning;
                    }
                    ShopMenuFreePointerCursors();
                } else {
                    ShopMenuPlaySoundEffect(4);
                }
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                if (total != 0) {
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    if (!ShopMenuConfirmationWindow(0x92, 0xFF, 1)) {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = isRunning;
                        g_Menu->pManager->shouldRenderWindow[3] = isRunning;
                        g_Menu->pManager->scrollHandleActive = isRunning;
                        prevScroll = 0xFF;
                        prevRow = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = isRunning;
                    }
                    ShopMenuFreePointerCursors();
                }
                break;

            case MENU_INPUT_DOWN:
                if (++row >= MAX_ITEMS_IN_VIEW) {
                    row = MAX_ITEMS_IN_VIEW - 1;
                    if (++scroll > numEntries - MAX_ITEMS_IN_VIEW) {
                        scroll--;
                    }
                }
                break;

            case MENU_INPUT_UP:
                if (--row < 0) {
                    row = 0;
                    if (--scroll < 0) {
                        scroll = 0;
                    }
                }
                break;

            case MENU_INPUT_RIGHT:
                if (sellable[scroll + row]) {
                    if (ownedQuantities[scroll + row] - 1 >= 0) {
                        shouldUpdateGold = TRUE;
                        total += price;
                        sellQuantities[scroll + row]++;
                        goldAfter += price;
                        ownedQuantities[scroll + row]--;
                    }
                }
                break;

            case MENU_INPUT_LEFT:
                if (sellable[scroll + row]) {
                    if (sellQuantities[scroll + row] - 1 >= 0) {
                        total -= price;
                        shouldUpdateGold = TRUE;
                        ownedQuantities[scroll + row]++;
                        goldAfter -= price;
                        sellQuantities[scroll + row]--;
                    }
                }
                break;
        }
    }
}


/* Retail 801D1658..801D18A8 (matched). Pick a party member's portrait,
 * then list that character's three equipped accessory slots in the sell
 * menu (one of each, mixed-type list). Returns 0 when backed out, 1 when
 * a character was confirmed. */
u_char ShopMenuSellEquipmentMenu(void) {
    u_char ids[8];
    u_char quantities[8];
    u_char types[8];
    u_char result;
    int portrait;
    int prevPortrait;
    int character;
    int numItems;
    int i;

    portrait = 0;
    prevPortrait = 1;
    result = 2;
    ShopMenuUpdateCharacterPortraits();
    ShopMenuConfirmationWindowInitialize(0x98);

    do {
        ShopMenuUpdateAndRender();
        switch (g_Menu->input) {
            case MENU_INPUT_BACK:
                result = 0;
                break;

            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                result = 1;
                break;

            case MENU_INPUT_LEFT:
                if (--portrait < 0) {
                    portrait = g_Menu->pShop->numPortraits - 1;
                }
                break;

            case MENU_INPUT_RIGHT:
                if (++portrait >= g_Menu->pShop->numPortraits) {
                    portrait = 0;
                }
                break;
        }

        if (portrait != prevPortrait) {
            g_Menu->pShop->unk469C[portrait] = 1;
            g_Menu->pShop->unk469C[prevPortrait] = 0;
            prevPortrait = portrait;
        }
    } while (result == 2);

    ShopMenuConfirmationWindowFree();

    if (result != 0) {
        character = 0;
        while (portrait != 0) {
            character++;
            if (g_Menu->availableCharacters[character]) {
                portrait--;
            }
        }

        for (i = 0; i < 8; i++) {
            ids[i] = 0;
            quantities[i] = 1;
        }

        numItems = 0;
        for (i = 0; i < 3; i++) {
            if (g_GameState.characters[character].unk74[i]) {
                ids[numItems] = g_GameState.characters[character].unk74[i];
                types[numItems] = ITEM_TYPE_ACCESSORY;
                numItems++;
            }
        }

        ShopMenuSellMenu(8, ids, quantities, ITEM_TYPE_ACCESSORY, 0, types, (u_char)character);
    }

    return result;
}

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
