/* data_member_change_menu.c -- migrated member_change_menu overlay .data
 * (Xenogears PC port).  Same pattern/rationale as data_field.c: overlay
 * .data symbols are auto-generated as ZEROED stubs unless defined here.
 * These are the member-change menu's LAYOUT tables (window size, cursor
 * and character slot positions, texcoords) -- zeroed => a 0x0 window with
 * everything at position 0 => nothing renders.  Values are loaded at
 * startup from the user's disc/member_change_menu.bin .data (file 0x6180+,
 * VRAM base 0x801C5000; retail_data.h), never committed.  Defining them removes them from the zeroed
 * data-stub set so the menu gets real layout instead of zeros. */

int D_801CB180[4];
int D_801CB190[4];
int g_MemberChangeMenuCurserPositionsX[9];
int g_MemberChangeMenuCurserPositionsY[9];
int g_MemberChangeMenuBenchedCharPositionsX[17];
int g_MemberChangeMenuCurCharPositionsX[18];
int g_MemberChangeMenuBenchedCharPositionsY[17];
int g_MemberChangeMenuCurCharPositionsY[35];
int g_MemberChangeMenuCharTexcoordsU[19];
int g_MemberChangeMenuCharTexcoordsV[19];
int D_801CB3DC[9];
unsigned char D_801CB400[4];
int D_801CB404[30];
int D_801CB47C[32];
int D_801CB4FC[32];
unsigned short D_801CB57C[16];

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(D_801CB180) == 0x10, "D_801CB180 size");
_Static_assert(sizeof(D_801CB190) == 0x10, "D_801CB190 size");
_Static_assert(sizeof(g_MemberChangeMenuCurserPositionsX) == 0x24, "g_MemberChangeMenuCurserPositionsX size");
_Static_assert(sizeof(g_MemberChangeMenuCurserPositionsY) == 0x24, "g_MemberChangeMenuCurserPositionsY size");
_Static_assert(sizeof(g_MemberChangeMenuBenchedCharPositionsX) == 0x44, "g_MemberChangeMenuBenchedCharPositionsX size");
_Static_assert(sizeof(g_MemberChangeMenuCurCharPositionsX) == 0x48, "g_MemberChangeMenuCurCharPositionsX size");
_Static_assert(sizeof(g_MemberChangeMenuBenchedCharPositionsY) == 0x44, "g_MemberChangeMenuBenchedCharPositionsY size");
_Static_assert(sizeof(g_MemberChangeMenuCurCharPositionsY) == 0x8C, "g_MemberChangeMenuCurCharPositionsY size");
_Static_assert(sizeof(g_MemberChangeMenuCharTexcoordsU) == 0x4C, "g_MemberChangeMenuCharTexcoordsU size");
_Static_assert(sizeof(g_MemberChangeMenuCharTexcoordsV) == 0x4C, "g_MemberChangeMenuCharTexcoordsV size");
_Static_assert(sizeof(D_801CB3DC) == 0x24, "D_801CB3DC size");
_Static_assert(sizeof(D_801CB400) == 0x4, "D_801CB400 size");
_Static_assert(sizeof(D_801CB404) == 0x78, "D_801CB404 size");
_Static_assert(sizeof(D_801CB47C) == 0x80, "D_801CB47C size");
_Static_assert(sizeof(D_801CB4FC) == 0x80, "D_801CB4FC size");
_Static_assert(sizeof(D_801CB57C) == 0x20, "D_801CB57C size");

XENO_RETAIL_DATA_BEGIN(member_change_menu)
    XENO_RD(D_801CB180, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB180u), 0x10),
    XENO_RD(D_801CB190, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB190u), 0x10),
    XENO_RD(g_MemberChangeMenuCurserPositionsX, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB1A0u), 0x24),
    XENO_RD(g_MemberChangeMenuCurserPositionsY, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB1C4u), 0x24),
    XENO_RD(g_MemberChangeMenuBenchedCharPositionsX, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB1E8u), 0x44),
    XENO_RD(g_MemberChangeMenuCurCharPositionsX, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB22Cu), 0x48),
    XENO_RD(g_MemberChangeMenuBenchedCharPositionsY, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB274u), 0x44),
    XENO_RD(g_MemberChangeMenuCurCharPositionsY, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB2B8u), 0x8C),
    XENO_RD(g_MemberChangeMenuCharTexcoordsU, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB344u), 0x4C),
    XENO_RD(g_MemberChangeMenuCharTexcoordsV, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB390u), 0x4C),
    XENO_RD(D_801CB3DC, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB3DCu), 0x24),
    XENO_RD(D_801CB400, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB400u), 0x4),
    XENO_RD(D_801CB404, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB404u), 0x78),
    XENO_RD(D_801CB47C, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB47Cu), 0x80),
    XENO_RD(D_801CB4FC, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB4FCu), 0x80),
    XENO_RD(D_801CB57C, XENO_RD_MEMBER_CHANGE, XENO_RD_MENU_OFF(0x801CB57Cu), 0x20),
XENO_RETAIL_DATA_END(member_change_menu)
