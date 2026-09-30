#ifndef _XENO_MENU_H
#define _XENO_MENU_H

#include "psyq/libgpu.h"
#include "system/menu_resources.h"
#include "system/sound.h"
#include "main/game.h"

// Mapped inputs
#define MENU_INPUT_RIGHT 0x0
#define MENU_INPUT_DOWN 0x1
#define MENU_INPUT_LEFT 0x2
#define MENU_INPUT_UP 0x3
#define MENU_INPUT_CONFIRM 0x4
#define MENU_INPUT_BACK 0x5
#define MENU_INPUT_IDLE 0x8

// Transition animation states
#define MENU_ANIMATION_DONE 0x0
#define MENU_OPEN_ANIMATION 0x1
#define MENU_CLOSE_ANIMATION 0x2
#define MENU_OPEN_ANIMATION_START 0x3
#define MENU_CLOSE_ANIMATION_START 0x4

#define MENU_MAX_NUM_WINDOWS 0x7
#define MENU_MAX_NUM_CURSORS 0x4
#define MENU_MAX_NUM_ARROW_CURSORS 0x2
#define MENU_WINDOW_BORDER_SIZE 0x8
#define MENU_MAX_DIGITS 0x9

// Modes for memory management functions
#define MENU_DATA_INITIALIZE 0x1
#define MENU_DATA_FREE 0x0

// TODO: Move to more fitting file
#define FONT_LETTER_HEIGHT 0xD

#define MAX_SHOP_ITEMS 0x30

#define ITEM_TYPE_WEAPON 0x0
#define ITEM_TYPE_ACCESSORY 0x1
#define ITEM_TYPE_ITEM 0x2

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u_short price;
    /* 0x4 */ u16 categoryFlags;
    /* 0x6 */ u8 flags;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 effectMagnitude;  // func_801E31C0: HP +mag*50, MP +mag*10,
                                   // stat boosts +mag; 1/2 select the special
                                   // dispatch when effectFlags bit 0 is set
    /* 0x9 */ u8 unk9;
    /* 0xA */ s16 effectFlags;     // bit15 HP restore, bit14 MP restore,
                                   // bit2 stat block, bit1 unk78 gauge,
                                   // bit0 special dispatch (E5058/E5178)
    /* 0xC */ s16 statEffectFlags; // bit15 attack, bit14 defense, bit13 ether,
                                   // bit12 etherDefence, bit11 maxHp,
                                   // bit10 maxMp; low byte = unk78 amount
    /* 0xE */ u8 unkE[0x2];
} MenuShopItem; // Size: 0x10

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u_short price;
    /* 0x6 */ u8 unk6[0xA];
} MenuShopWeapon; // Size: 0x10

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u_short price;
    /* 0x4 */ u8 unk4[0xC];
} MenuShopAccessory; // Size: 0x10

typedef struct {
    /* 0x0  */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ unsigned long ot[16];
    /* 0xB0 */ u32 unkB0;
} GfxEnvironment; // Size: 0xB4

typedef struct {
    /* 0x0   */ POLY_FT4 polysPointerCursor[2];
    /* 0x50  */ POLY_G4 polyG4s[2];
    /* 0x98  */ POLY_F4 polysDimEffect[2];
    /* 0xC8  */ LINE_F3 lines1[2];
    /* 0xF8  */ LINE_F3 lines2[2];
    /* 0x128 */ DR_MODE drModes1[2];
    /* 0x140 */ DR_MODE drawModeDimEffect[2];
    /* 0x158 */ u_char cursorRenderContext;
    /* 0x159 */ u8 unk159;
    /* 0x15A */ u8 unk15A;
    /* 0x15B */ u8 unk15B;
} MenuUnk1; // Size: 0x15C

typedef struct {
    /* 0x0   */ u8 unk0[0xB80];
    /* 0xB80 */ TIM_IMAGE tim;
    /* 0xB94 */ u8 unkB94[0x4000];
    /* 0x4B94 */ u8 unk4B94;
    /* 0x4B95 */ u8 unk4B95;
    /* 0x4B96 */ u8 unk4B96;
    /* 0x4B97 */ u8 unk4B97;
    /* 0x4B98 */ u8 unk4B98[0x5C];
    /* 0x4BF4 */ u8 unk4BF4[0x20];
    /* 0x4C14 */ u8 unk4C14[0x80];
    /* 0x4C94 */ u8 unk4C94[0x2E8];
    /* 0x4F7C */ s32 unk4F7C;
    /* 0x4F80 */ u8 unk4F80[0xB4];
} MenuUnk2; // Size: 0x5034

typedef struct {
    /* 0x0  */ u8 unk0[0x3];
    /* 0x3  */ u8 unk3;
    /* 0x4  */ u8 unk4; // Should we draw the pointer cursor on ball selection windows?
    /* 0x5  */ u8 unk5[0x4];
    /* 0x9  */ u8 shouldRenderSelectionMenu;
    /* 0xA  */ u8 unkA; // Should we draw MenuUnk5 strings?
    /* 0xB  */ u8 unkB;
    /* 0xC  */ u8 unkC[0x8];
    /* 0x14 */ u8 unk14[0x6];
    /* 0x1A */ u8 unk1A[0x6];
    /* 0x20 */ u8 shouldRenderWindow[MENU_MAX_NUM_WINDOWS];
    /* 0x27 */ u8 unk27[MENU_MAX_NUM_WINDOWS];
    /* 0x2E */ u8 unk2E;
    /* 0x2F */ u_char shouldRenderPointerCursors;
    /* 0x30 */ u8 currentCharacterIDs[0x3];
    /* 0x33 */ u8 _pad33;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ u8 unk38[0xE];
    /* 0x46 */ u8 unk46; // Should render characters?
    /* 0x47 */ u8 unk47;
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u_char scrollHandleActive;
    /* 0x4A */ u8 unk4A[0x6];
    /* 0x50 */ u8 shouldRenderArrowCursor[MENU_MAX_NUM_ARROW_CURSORS]; // Should render arrow cursor (array, size 3?)
    /* 0x52 */ u8 unk52[0x8];
    /* 0x5A */ u8 unk5A;
    /* 0x5B */ u8 unk5B;
    /* 0x5C */ u8 unk5C[0x10];
} MenuManager; // Size: 0x6C

// A menu with N options, each having a cursor and some text/content
typedef struct {
    /* 0x0    */ POLY_FT4 polysCursors[28 * 2]; // Cursor for each menu option
    /* 0x8C0  */ POLY_FT4 polysTexts[28 * 2]; // Texts / content for each menu option
    /* 0x1180 */ RECT unk1180;
    /* 0x1188 */ int numCursors;
    /* 0x118C */ int numTexts;
    /* 0x1190 */ u_char cursorsRenderCtx;
    /* 0x1191 */ u_char textsRenderCtx;
    /* 0x1192 */ u_char unk1192; // If set to true, and the field below is not true, fade out the menu gfx
    /* 0x1193 */ u_char unk1193; // 
} MenuSelectionMenu; // Size: 0x1194

typedef struct {
    /* 0x0    */ POLY_FT4 polys0[16 * 2]; // Unknown size
    /* 0x500  */ POLY_FT4 polys500[16 * 2]; // Unknown size
    /* 0xA00  */ u8 unkA00[0xA00];
    /* 0x1400 */ int unk1400; // polys0 count
    /* 0x1404 */ int unk1404; // polys500 count
    /* 0x1408 */ u_char unk1408; // polys0 render context
    /* 0x1409 */ u_char unk1409; // polys500 render context
    /* 0x140A */ u8 unk140A;
    /* 0x140B */ u8 unk140B;
} MenuUnk5; // Size: 0x140C

typedef struct {
    /* 0x0  */ MenuShopWeapon* pWeaponsData;
    /* 0x4  */ MenuShopAccessory* pAccessoriesData;
    /* 0x8  */ u8 unk8[0x14];
    /* 0x1C */ MenuShopItem* pItemsData;
    /* 0x20 */ u8 unk20[0x98];
    /* 0xB8 */ s32 unkB8;
    /* 0xBC */ s32 unkBC;
    /* 0xC0 */ u8 unkC0[0xC];
} MenuUnk6; // Size: 0xCC

typedef struct {
    /* 0x0 */ u8 unk0[0xDEC];
} MenuUnk7; // Size: 0xDEC

// Character Info
typedef struct {
    /* 0x0   */ POLY_FT4 polysDescriptionStrings[18];
    /* 0x2D0 */ POLY_FT4 polys2D0[2];
    /* 0x320 */ POLY_FT4 polysFixedLabelTail[8];
    /* 0x460 */ POLY_FT4 polysPortraitSmall[2];
    /* 0x4B0 */ POLY_FT4 polys4B0[2];
    /* 0x500 */ POLY_FT4 polysLevelString[6];
    /* 0x5F0 */ POLY_FT4 polys5F0[6];
    /* 0x6E0 */ POLY_FT4 polysHpString[10];
    /* 0x870 */ POLY_FT4 polysMaxHpString[10];
    /* 0xA00 */ POLY_FT4 polysMpString[6];
    /* 0xAF0 */ POLY_FT4 polysMaxMpString[6];
    /* 0xBE0 */ u_char levelStringLength;
    /* 0xBE1 */ u8 unkBE1; // Num polys5F0
    /* 0xBE2 */ u_char hpStringLength;
    /* 0xBE3 */ u_char maxHpStringLength;
    /* 0xBE4 */ u_char mpStringLength;
    /* 0xBE5 */ u_char maxMpStringLength;
    /* 0xBE6 */ u_char renderContext;
    /* 0xBE7 */ u8 unkBE7; // Is active / should render?
    /* 0xBE8 */ u_char descriptionStringsLength;
    /* 0xBE9 */ u8 unkBE9;
    /* 0xBEA */ u8 unkBEA[0x2];
} MenuCharacter; // Size: 0xBEC

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ RECT vramDest;
    /* 0x78 */ void* pVramBuffer;
    /* 0x7C */ u8 unk7C; // Palette?
    /* 0x7D */ u8 renderContext;
    /* 0x7E */ u8 width;
    /* 0x7F */ u8 unk7F;
} MenuString; // Size: 0x80

/* Items submenu content.  The offsets describe the retail 32-bit layout;
 * embedded MenuStrings naturally expand in the native 64-bit port.  The
 * description bundle remains a four-byte PSX pointer slot (lw semantics),
 * matching SystemMenu.unk42C and the port's below-4GB heap convention. */
typedef struct {
    /* 0x0000 */ MenuString itemNames[16];
    /* 0x0800 */ MenuString itemCounts[16];
    /* 0x1000 */ MenuString selectedItemName;
    /* 0x1080 */ MenuString selectedItemCount;
    /* 0x1100 */ MenuString selectedItemDescription;
    /* 0x1180 */ u32 descriptionBundle;
    /* 0x1184 */ u8 rowVisible[16];
    /* 0x1194 */ u8 descriptionVisible;
    /* 0x1195 */ u8 _pad1195[3];
} ItemMenuWork; // Retail size: 0x1198

/* Abilities submenu content, allocated by func_801DC1D4 and stored in the
 * truncated PSX-pointer slot SystemMenu.unk42C[1] (retail g_Menu + 0x430) --
 * the same convention Items established at unk42C[0].  Retail size 0x1094;
 * the embedded MenuStrings expand naturally in the native 64-bit port.
 *
 * NOT ItemMenuWork: that struct's 0x1080 is a MenuString, this one's is a
 * four-byte pointer slot.  Reusing the Items type would compile and look
 * right while writing a pointer into a MenuString's vertex data.
 *
 * Unnamed spans are sized filler with no observed access. */
typedef struct {
    /* 0x0000 */ MenuString strings[32];
    /* 0x1000 */ MenuString unk1000String; /* 33rd string slot, immediately
                                      * past strings[32].  Single observed
                                      * writer: func_801DC3D8's epilogue
                                      * (func_801D36E0 sprite setup, once per
                                      * build).  Observed reader: func_801D1640,
                                      * which draws it every frame while the
                                      * pManager->unk4A[0] content latch is
                                      * set.
                                      * Declared, NOT filler: native MenuString
                                      * is 0x98 (POLY_FT4's 8-byte u_long tag
                                      * inflates it from PSX 0x80), so writing
                                      * one through a u8[0x80] span would put
                                      * renderContext (native +0x91) on
                                      * unk1090[1] -- the row-13 rowFlags
                                      * spill -- silently corrupting it. */
    /* 0x1080 */ u32 abilityBank;    /* LZSSHeapDecompress result, C72BC
                                      * mode 2; freed by 0x12. PSX-width. */
    /* 0x1084 */ u8 rowFlags[0xC];   /* per-row flags, indexed by the 0..0xB
                                      * cursor (bound proven by func_801DDF24's
                                      * cursor arithmetic: slti 0xC / bgez, and
                                      * the +-2 paged variants).  func_801DC3D8
                                      * writes, func_801DDF24 tests bit 0x80 to
                                      * gate the confirm dispatch into
                                      * func_801DD790, func_801DCE60 reads.
                                      * Each site does `addu <base>,<cursor>`
                                      * one instruction before the access. */
    /* 0x1090 */ u8 unk1090[0x2];    /* rowFlags spill: func_801DC3D8 builds
                                      * rows 0..0xD and writes 0x1084+i for all
                                      * of them, so rows 12/13 land here.  The
                                      * cursor is bounded at 0xC (func_801DDF24),
                                      * so these two entries are built but
                                      * unreachable by cursor. */
    /* 0x1092 */ u8 unk1092;         /* scalar -- NOT indexed (accessed direct
                                      * off the work pointer in func_801DCE60) */
    /* 0x1093 */ u8 unk1093;         /* tail pad to 0x1094 */
} AbilityMenuWork; // Retail size: 0x1094

/* Built by func_801D3488 into SystemMenu.unk440 (retail g_Menu + 0x440),
 * HeapAlloc'd once and guarded by MenuManager.unk5C[0xB] (retail +0x67).
 * SHARED BY SEVEN CALLERS across Abilities / Equip / Gear / Status, so the
 * layout is load-bearing well beyond the slice that first defines it.
 *
 * NAME IS DELIBERATELY STRUCTURAL.  The shape is unambiguous -- func_8002675C
 * takes the polys (2 calls x 4 POLY_FT4) and func_801C851C takes the vertex
 * groups (4 calls x 4 SVECTOR), with the geometry read from
 * polys[i*2 + renderContext] at the exact POLY_FT4 vertex offsets -- but what
 * the four quads DEPICT is not: sprite-table indices 0x164 + i out of
 * SystemMenu.unk2DC, bank selected by D_801EA16C[arg0], and arg0 differs per
 * caller.  Rename when the atlas proves what they are. */
typedef struct {
    /* 0x000 */ POLY_FT4 polys[8];      /* 4 quads as double-buffered pairs */
    /* 0x140 */ SVECTOR vertices[16];   /* 4 groups of 4, one group per quad */
    /* 0x1C0 */ u_char renderContext;
    /* 0x1C1 */ u8 unk1C1[0x3];         /* pad to 0x1C4; no observed access */
} MenuUnk440Work; // Size: 0x1C4

typedef struct {
    /* 0x0   */ POLY_FT4 polysWindowBorderCorners[8];
    /* 0x140 */ POLY_FT4 polysWindowBorderTop[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x1E0 */ POLY_FT4 polysWindowBorderBottom[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x280 */ POLY_FT4 polysWindowBorderLeft[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x320 */ POLY_FT4 polysWindowBorderRight[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x3C0 */ POLY_FT4 polysScrollBarEmpty[2];
    /* 0x410 */ POLY_FT4 polysScrollBarEnds[4];
    /* 0x4B0 */ POLY_G4 polysBackground[2];
    /* 0x4F8 */ DR_MODE drawModes[2];
    /* 0x510 */ SVECTOR vertsWindowBorderCorners[16];
    /* 0x590 */ SVECTOR vertsWindowBorderTop1[4]; // First half of top border
    /* 0x5B0 */ SVECTOR vertsWindowBorderTop2[4]; // Second half of top border
    /* 0x5D0 */ SVECTOR vertsWindowBorderBottom1[4]; // First half of bottom border
    /* 0x5F0 */ SVECTOR vertsWindowBorderBottom2[4]; // Second half of bottom border
    /* 0x610 */ SVECTOR vertsWindowBorderLeft1[4]; // First half of left border
    /* 0x630 */ SVECTOR vertsWindowBorderLeft2[4]; // Second half of left border
    /* 0x650 */ SVECTOR vertsWindowBorderRight1[4]; // First half of right border
    /* 0x670 */ SVECTOR vertsWindowBorderRight2[4]; // Second half of right border
    /* 0x690 */ SVECTOR vertsBackground[4];
    /* 0x6B0 */ SVECTOR vertsScrollBarEmpty[4];
    /* 0x6D0 */ SVECTOR vertsScrollBarEnds[8];
    /* 0x710 */ s32 unk710;
    /* 0x714 */ s32 unk714; // If 0, use default transform matrix when rendering
    /* 0x718 */ int zIndex;
    /* 0x71C */ u_char renderContext;
    /* 0x71D */ u_char hasScrollBar;
    /* 0x71E */ u8 _pad71E[0x2];
} MenuWindow; // Size: 0x720

typedef struct {
    /* 0x0  */  POLY_FT4 polysCursor[8];
    /* 0x140 */ u8 shouldRender[0x4]; // Should render / isActive?
    /* 0x144 */ u8 unk144[0x4]; // Should update positions?
    /* 0x148 */ u8 renderContexts[0x4];
} MenuPointerCursors; // Size: 0x14C

typedef struct {
    /* 0x0  */ u_short x;
    /* 0x2  */ u_short y;
    /* 0x4  */ u_short width;
    /* 0x6  */ u_short height;
    /* 0x8  */ u_short unk8; // Width?
    /* 0xA  */ u_short unkA; // Height?
    /* 0xC  */ int zIndex;
    /* 0x10 */ u_char index;
    /* 0x11 */ u_char unk11;
    /* 0x12 */ u_char unk12;
    /* 0x13 */ u_char hasScrollBar;
    /* 0x13 */ u8 unk14[0x4];
} MenuWindowParameters; // Size: 0x18

// Shop data
typedef struct {
    /* 0x0    */ POLY_FT4 polysCharacterPortraits[9*2];
    /* 0x2D0  */ POLY_FT4 polys2D0[9*2]; // Letter 'E' (for Equip) on character portraits if item is equipped
    /* 0x5A0  */ POLY_FT4 polysExplanations[4 * 2];
    /* 0x6E0  */ u8 unk6E0[0x5A0];
    /* 0xC80  */ POLY_FT4 polysGoldBefore[9 * 2];
    /* 0xF50  */ POLY_FT4 polysTotalPrice[9 * 2];
    /* 0x1220 */ POLY_FT4 polysFinalPrice[9 * 2];
    /* 0x14F0 */ u8 unk14F0[0x5F0];
    /* 0x1AE0 */ POLY_FT4 polysGoldAfter[9 * 2];
    /* 0x1DB0 */ u8 unk1DB0[0xA00];
    /* 0x27B0 */ POLY_FT4 polys27B0[9 * 3*2];
    /* 0x3020 */ POLY_FT4 polys3020[9 * 3*2];
    /* 0x3890 */ LINE_F3 linesPortraitHighlight1[9 * 2]; // Red highlight line around portraits, 1st half
    /* 0x3A40 */ LINE_F3 linesPortraitHighlight2[9 * 2]; // Red highlight line around portraits, 2nd half
    /* 0x3BF0 */ LINE_F2 lines3BF0[2]; // White line between gold amounts?
#ifdef XENO_PC_PORT
    /* Retail 801CCFF4 renders these as eight pairs of MenuString records. */
    /* 0x3C10 */ u8 unk3C10[0x20];
    /* 0x3C30 */ MenuString strings3C30[8];
    /* 0x4030 */ MenuString strings4030[8];
#else
    /* 0x3C10 */ u8 unk3C10[0x820];
#endif
    /* 0x4430 */ MenuString strItemDesc;
    /* 0x44B0 */ MenuString str44B0;
#ifdef XENO_PC_PORT
    /* 0x4530 */ u8 unk4530[0x80];
    /* 0x45B0 */ MenuString str45B0;
#else
    /* 0x4530 */ u8 unk4530[0x100];
#endif
    /* 0x4630 */ void* pItemDescriptions;
    /* 0x4634 */ void* pWeaponDescriptions;
    /* 0x4638 */ void* pAccessoryDescriptions;
    /* 0x463C */ u8 unk643C[0x18];
    /* 0x4654 */ u_char curItemQuantities[MAX_SHOP_ITEMS];
    /* 0x4684 */ u8 unk4684[0x8];
    /* 0x468C */ u8 unk468C[0x8];
    /* 0x4694 */ u8 unk4694[0x8];
    /* 0x469C */ u8 unk469C[0x9];
    /* 0x46A5 */ u_char numPortraits;
    /* 0x46A6 */ u_char portraitsRenderCtx;
    /* 0x46A7 */ u8 unk46A7;
    /* 0x46A8 */ u8 unk46A8;
    /* 0x46A9 */ u8 unk46A9;
    /* 0x56AA */ u_char explanationsRenderCtx;
    /* 0x46AB */ u_char explanationsLen;
    /* 0x46AC */ u_char goldBeforeRenderCtx;
    /* 0x46AD */ u_char goldBeforeStrLen;
    /* 0x46AE */ u_char totalPriceRenderCtx;
    /* 0x46AF */ u_char totalPriceStrLen;
    /* 0x46B0 */ u_char goldAfterRenderCtx;
    /* 0x46B1 */ u_char goldAfterStrLen;
    /* 0x46B2 */ u8 unk46B2;
    /* 0x46B3 */ u_char finalPriceRenderCtx;
    /* 0x46B4 */ u_char finalPriceStrLen;
    /* 0x46B5 */ u8 unk46B5;
    /* 0x46B6 */ u8 unk46B6[0x6];
    /* 0x46BC */ u8 unk46BC[0x9]; // Number string Lengths
    /* 0x46C5 */ u8 unk46C5[0x9]; // Number string Lengths
    /* 0x46CE */ u8 unk46CE[0x9]; // Render contexts
    /* 0x46D7 */ u8 unk46D7[0x9]; // Render contexts
    /* 0x46E0 */ u16 unk46E0[0x10];
    /* 0x4700 */ u16 unk4700[0x10];
    /* 0x4720 */ u8 unk4720[0x65];
    /* 0x4785 */ u8 unk4785;
    /* 0x4786 */ u8 unk4786[0x2];
} MenuShop; // Size: 4788

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ u_char renderContext;
    /* 0x71 */ u8 unk71[0x3];
} MenuScrollBarHandle; // Size: 0x74

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ int curAnimFrame;
    /* 0x74 */ u_char animFrameDuration;
    /* 0x75 */ u_char renderContext;
    /* 0x76 */ u8 unk76;
    /* 0x77 */ u8 unk77;
} MenuArrowCursor; // Size: 0x78

typedef struct {
    /* 0x0    */ u8 unk0[108];
    /* 0x6C   */ GfxEnvironment gfxEnvs[2];
    /* 0x1D4  */ GfxEnvironment* pGfxEnv;
    /* 0x1D8  */ SVECTOR rotation;
    /* 0x1E0  */ VECTOR translation;
    /* 0x1F0  */ MATRIX matTransform;
    /* 0x210  */ u8 unk1D8[0x8];
    // Note: The next 8 bytes (4 shorts) could be a SVECTOR
    /* 0x218  */ undefined16 unk218;
    /* 0x21A  */ undefined16 unk21A;
    /* 0x21C  */ undefined16 unk21C;
    /* 0x21E  */ u8 unk21E[0x2];
    // Note: the next 16 bytes (4 longs) could be a VECTOR
    /* 0x220  */ undefined32 unk220;
    /* 0x224  */ undefined32 unk224;
    /* 0x228  */ undefined32 unk228;
    /* 0x22D  */ u8 unk22D[0xAB];
    /* 0x2D8  */ u32 unk2D8;
    /* 0x2DC  */ void* unk2DC; // Resources / Textured polys
    /* 0x2E0  */ void* unk2E0; // Pointer to resources (bin 3)
    /* 0x2E4  */ SoundFile* unk2E4; // Pointer to SEDS file
    /* 0x2E8  */ undefined32 unk2E8;
    /* 0x2EC  */ u8 unk2EC[0x1C];
    /* 0x308  */ int renderContext;
    /* 0x30C  */ u_char availableCharacters[0x10];
    /* 0x31C  */ u_char digits[0x9]; // Buffer for numbers parsed into a string
    /* 0x325  */ u_char input;
    /* 0x326  */ u8 unk326;
    /* 0x327  */ u_char shouldDrawMenu;
    /* 0x328  */ u8 unk328;
    /* 0x329  */ u_char transitionEffectState;
    /* 0x32A  */ u8 unk32A; // Sound effects enabled?
    /* 0x32B  */ u8 unk32B;
    /* 0x32C  */ MenuUnk2* unk32C;
    /* 0x330  */ MenuUnk6* unk330;
    /* 0x334  */ u8 unk334;
    /* 0x335  */ u8 unk335;
    /* 0x336  */ u_char menu1Choice;
    /* 0x337  */ u8 unk337; // Menu 1 prev choice?
    /* 0x338  */ u_char menu2Choice;
    /* 0x339  */ u8 unk339; // Menu 2 prev choice?
    /* 0x33A  */ u8 unk33A;
    /* 0x33B  */ u8 unk33B;
    /* 0x33C */ MenuManager* pManager;
    /* 0x340  */ u8 unk340[0x8];
    /* 0x348  */ MenuUnk1* unk348;
    /* 0x34C  */ u8 unk34C[0x4];
    /* 0x350  */ MenuSelectionMenu* pSelectionMenu;
    /* 0x354  */ MenuUnk5* unk354;
    /* 0x358  */ u8 unk358[0xC];
    /* 0x364  */ MenuWindow* windows[MENU_MAX_NUM_WINDOWS];
    /* 0x380  */ MenuWindowParameters* windowParameters[MENU_MAX_NUM_WINDOWS];
    /* 0x39C  */ u8 unk39C[0x8C];
    /* 0x428  */ MenuPointerCursors* pCursors;
    /* 0x42C  */ u32 unk42C[4]; // Truncated PSX-pointer slots (native heap is <4GB)
    /* 0x43C  */ MenuScrollBarHandle* pScrollHandle;
    /* 0x440  */ u8 unk440[0x4];
    /* 0x444  */ MenuArrowCursor* arrowCursors[MENU_MAX_NUM_ARROW_CURSORS];
    /* 0x44C  */ u8 unk44C[0x4];
    /* 0x450  */ MenuShop* pShop;
    /* 0x454  */ u8 unk454[0x18];

    // Window borders
    /* 0x46C  */ u32 unk46C; // UV?
    /* 0x470  */ s32 texPage0;
    /* 0x474  */ s32 clutX0;
    /* 0x478  */ s32 clutY0;
    /* 0x47C  */ s32 texPageX0;
    /* 0x480  */ s32 texPageY0;
    /* 0x484  */ u32 unk484; // UV?
    /* 0x488  */ s32 texPage1;
    /* 0x48C  */ s32 clutX1;
    /* 0x490  */ s32 clutY1;
    /* 0x494  */ s32 texPageX1;
    /* 0x498  */ s32 texPageY1;
    /* 0x49C  */ u32 unk49C; // UV?
    /* 0x4A0  */ s32 texPage2;
    /* 0x4A4  */ s32 clutX2;
    /* 0x4A8  */ s32 clutY2;
    /* 0x4AC  */ s32 texPageX2;
    /* 0x4B0  */ s32 texPageY2;
    /* 0x4B4  */ u32 unk4B4; // UV?
    /* 0x4B8  */ s32 texPage3;
    /* 0x4BC  */ s32 clutX3;
    /* 0x4C0  */ s32 clutY3;
    /* 0x4C4  */ s32 texPageX3;
    /* 0x4C8  */ s32 texPageY3;

    /* 0x4CC  */ u8 unk4CC[0x10];
    /* 0x4DC  */ u8 selectedPartySlot;
    /* 0x4DD  */ u8 _pad4DD[0x3];
    /* 0x4E0  */ MenuString unk4E0[4];
    /* 0x6E0  */ MenuString unk6E0[8];
    /* 0xAE0  */ MenuString unkAE0[6];
    /* 0xDE0  */ u8 unkDE0[0x300];
    /* 0x10E0 */ MenuString itemMenuStrings[8];
    /* 0x14E0 */ u8 unk14E0[0x900];
    /* 0x1DE0 */ MenuString* unk1DE0[4];
    /* 0x1DF0 */ MenuCharacter* benchedCharacters[MAX_BENCHED_PARTY_MEMBERS];
    /* 0x1E08 */ MenuCharacter* currentCharacters[MAX_PARTY_MEMBERS];
    /* 0x1E14 */ u8 unk1E14[MAX_GAME_CHARACTERS];
    /* 0x1E1F */ u8 unk1E1F;
    /* 0x1E20 */ MenuUnk7* unk1E20;
    /* 0x1E24 */ u8 unk1E24[0x8];
    /* 0x1E2C */ void* pShopEntries;
    /* 0x1E30 */ u_char shopItemIDs[MAX_SHOP_ITEMS];
    /* 0x1E60 */ u_char shopItemTypes[MAX_SHOP_ITEMS];
    /* 0x1E90 */ u8 unk1E90[0x4];
    /* 0x1E94 */ u8 unk1E94;
    /* Ordinary RAM; MenuProcessControllerInput uses volatile accesses locally
     * to retain its retail load/store schedule without qualifying all readers. */
    /* 0x1E95 */ u8 unk1E95;
    /* 0x1E96 */ u8 unk1E96[2];
} SystemMenu; // Size: 0x1E98

extern SystemMenu* g_Menu;
extern u8 g_MenuDebugEnabled;

/* Menu-body access to the retail 32-bit pointer slots and card-event words
 * in g_Menu, for bodies shared by the matching build and the port.  Retail
 * reads and writes g_Menu + offset directly.  The port routes through
 * helpers defined in src/menu/main/misc.c (MenuRawPointer,
 * MenuStoreRawPointer, MenuCardEvent, MenuSetCardEvent), which keep the PSX width inside the
 * native-inflated SystemMenu. */
#ifdef XENO_PC_PORT
#define MENU_RAW(offset) MenuRawPointer(offset)
#define MENU_STORE_RAW(offset, p) MenuStoreRawPointer((offset), (p))
#define MENU_CARD_EVENT(slot) MenuCardEvent(slot)
#define MENU_SET_CARD_EVENT(slot, ev) MenuSetCardEvent((slot), (ev))
#else
#define MENU_RAW(offset) (*(u8**)((u8*)g_Menu + (offset)))
#define MENU_STORE_RAW(offset, p) (*(void**)((u8*)g_Menu + (offset)) = (p))
#define MENU_CARD_EVENT(slot) \
    (*(u32*)(*(u8**)((u8*)g_Menu + 0x32C) + 0x4FEC + (slot) * 4))
#define MENU_SET_CARD_EVENT(slot, ev) (MENU_CARD_EVENT(slot) = (ev))
#endif

/* Retail MenuUnk2 (g_Menu->unk32C) byte offset -> this build's offset.  The
 * embedded TIM_IMAGE holds pointers, so on the 64-bit host every field from
 * +0xB94 on sits later by offsetof(unkB94) - 0xB94; on the PSX it is the
 * identity, so retail code generation is unchanged.  Use it for every raw
 * card-buffer offset: ((u8*)g_Menu->unk32C)[MENU_CARD_OFF(0x4FE4)]. */
/* Calls whose retail code leaves $a0 unset (the callee ignores it).  The port
 * passes a real argument; retail keeps the argument-less call shape.
 * MENU_KR(f) is the same for calls retail makes unprototyped. */
#ifdef XENO_PC_PORT
#define MENU_CALL_D32B4() func_801D32B4(0)
#define MENU_CALL_C881C(port) func_801C881C(port)
#define MENU_KR(f) f
#else
#define MENU_CALL_D32B4() ((s32 (*)())func_801D32B4)()
#define MENU_CALL_C881C(port) ((s32 (*)())func_801C881C)()
#define MENU_KR(f) ((void (*)())f)
#endif

/* Retail dereferences these menu pointers without a NULL check; on the PS1 a
 * NULL one reads guest address 0 (low RAM, no fault).  The port heap lives in
 * emulated PSX RAM, so map NULL to PSX_ADDR(0) instead of faulting on the
 * host or skipping the work.  Identity on the PSX build. */
#ifdef XENO_PC_PORT
#define MENU_GUEST_PTR(p) ((p) != NULL ? (p) : (__typeof__(p))PSX_ADDR(0))
#else
#define MENU_GUEST_PTR(p) (p)
#endif

/* Retail divides with no zero check (menu code has no break 7 sequence), so
 * on the R3000 x / 0 leaves LO = -1 for a non-negative dividend and +1 for a
 * negative one.  The host would trap, so the port reproduces that quotient.
 * Plain division on the PSX build. */
#ifdef XENO_PC_PORT
#define MENU_DIV(a, b) ((b) != 0 ? (a) / (b) : ((a) < 0 ? 1 : -1))
#else
#define MENU_DIV(a, b) ((a) / (b))
#endif

/* The unk340[0] / unk340[4] side buffers hold 4-byte PSX-width pointers. */
#define MENU_SIDE_BUF(o) ((u8*)(uintptr_t)*(u32*)&g_Menu->unk340[o])

#ifdef XENO_PC_PORT
#define MENU_CARD_OFF(off) \
    ((off) >= 0xB94 ? (off) + (s32)(offsetof(MenuUnk2, unkB94) - 0xB94) : (off))
#else
#define MENU_CARD_OFF(off) (off)
#endif

#endif
