/* M2 menu-setup retail differential: func_801C6AA0 (party/choice setup),
 * func_801C6F70 (MenuUnk1 dim-overlay + highlight-bar primitives).
 *
 * Native C bodies (src/menu/main/misc.c, XENO_PC_PORT arm) vs retail machine
 * code on the interpreter oracle (TEST ORACLE ONLY), identical guest memory,
 * via lib/m2_retail_diff.h. Limit: partyMembers ids are drawn from 0..15 and
 * 0xFF; an id >= 16 makes retail read past availableCharacters[16] into the
 * neighbouring SystemMenu bytes, which the pointer-inflated host struct cannot
 * reproduce (not exercised, reported).
 */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "lib/m2_retail_diff.h"

#define A6AA0 0x801C6AA0u
#define E6AA0 0x801C6D4Cu
#define A6F70 0x801C6F70u
#define E6F70 0x801C72BCu

#define MGR_GUEST 0x80120000u
#define U1_GUEST 0x80126000u
#define GAME_GUEST 0x8006D634u
#define GAME_SPAN 0x2000u

SystemMenu *g_Menu;
static SystemMenu menu;
static MenuManager mgr;
static MenuUnk1 u1;
u8 D_80059460, D_80059171, D_800594CC;
u8 stateStorage[GAME_SPAN] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");

_Static_assert(offsetof(MenuManager, currentCharacterIDs) == 0x30, "mgr ids");
_Static_assert(offsetof(MenuManager, unk5C) == 0x5C, "mgr 5c");
_Static_assert(sizeof(MenuUnk1) == 0x15C && offsetof(MenuUnk1, polyG4s) == 0x50 &&
                   offsetof(MenuUnk1, lines1) == 0xC8 && offsetof(MenuUnk1, lines2) == 0xF8 &&
                   offsetof(MenuUnk1, polysDimEffect) == 0x98 &&
                   offsetof(MenuUnk1, drModes1) == 0x128 &&
                   offsetof(MenuUnk1, drawModeDimEffect) == 0x140,
               "MenuUnk1 is guest layout");

static u32 T865C, T65F4, TD22C4, T8164, TSEMI, TLINEF3, TPOLYF4, TTPAGE, TDRAWMODE;
static u32 availMask;

static u32 ret_cb(u32 idx, const u32 *a) {
    u32 t = m2_callees[idx].addr;
    if (t == T865C) {
        /* High bits set on some "absent" results: retail tests andi 0xFFFF. */
        u32 bit = (availMask >> (a[1] & 31u)) & 1u;
        return bit ? (0x10000u | (a[1] + 1u)) : ((a[1] & 1u) ? 0x30000u : 0u);
    }
    if (t == TTPAGE)
        return 0xA5000000u | ((a[2] >> 6) + (a[3] >> 8) * 16u + a[1] * 32u);
    return 0;
}

u16 func_801C865C(u16 mask, u8 index) { return (u16)M2_SPY(T865C, (u32)mask, (u32)index); }
void func_801C65F4(void) { m2_call(m2_idx(T65F4), NULL); }
void func_801D22C4(void) { m2_call(m2_idx(TD22C4), NULL); }
void func_801C8164(POLY_G4 *p, u8 r, u8 g, u8 b) {
    M2_SPY(T8164, M2_U(p), (u32)r, (u32)g, (u32)b);
}
void SetSemiTrans(void *p, int abe) { M2_SPY(TSEMI, M2_U(p), (u32)abe); }
void SetLineF3(LINE_F3 *p) { M2_SPY(TLINEF3, M2_U(p)); }
void SetPolyF4(POLY_F4 *p) { M2_SPY(TPOLYF4, M2_U(p)); }
u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)M2_SPY(TTPAGE, (u32)tp, (u32)abr, (u32)x, (u32)y);
}
void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT16 *tw) {
    M2_SPY(TDRAWMODE, M2_U(p), (u32)dfe, (u32)dtd, (u32)tpage, M2_U(tw));
}

extern void func_801C6AA0(void);
extern void func_801C6F70(void);

static unsigned cov6AA0[(E6AA0 - A6AA0) / 4], cov6F70[(E6F70 - A6F70) / 4];

static void init_case(unsigned seed) {
    unsigned i;
    u8 *gs = stateStorage;
    m2_pattern(&menu, sizeof menu, seed + 1u);
    m2_pattern(&mgr, sizeof mgr, seed + 2u);
    m2_pattern(&u1, sizeof u1, seed + 3u);
    m2_pattern(stateStorage, sizeof stateStorage, seed + 4u);
    menu.pManager = &mgr;
    menu.unk348 = &u1;
    availMask = (seed * 0x9E3779B9u) >> 7;
    if (seed % 6u == 0)
        availMask = 0;
    if (seed % 6u == 1)
        availMask = 0xFFFFu;
    for (i = 0; i < 3; i++) {
        unsigned r = (seed * 5u + i * 7u) % 11u;
        gs[0x1D34u + i] = (r < 3u) ? 0xFFu : (u8)((seed + i * 3u) & 15u);
    }
    for (i = 0; i < 16; i++)
        gs[0x26Cu + i * 0xA4u + 0xA0u] = ((seed + i) % 3u == 0u) ? 0xFFu : (u8)i;
    D_80059460 = (u8)((seed % 4u == 0) ? 0 : seed % 3u);
    D_80059171 = (u8)((seed / 4u) & 1u);
    D_800594CC = (u8)(seed * 13u);
}

static void run_one(unsigned fn, unsigned seed) {
    static M2Snap before, nat, ora;
    init_case(seed);
    m2_save(&before);
    m2_trace_begin();
    g_Menu = &menu;
    if (fn == 0)
        func_801C6AA0();
    else
        func_801C6F70();
    m2_trace_stash();
    m2_save(&nat);
    m2_restore(&before);
    if (fn == 0)
        m2_oracle(A6AA0, E6AA0, cov6AA0, NULL, 0);
    else
        m2_oracle(A6F70, E6F70, cov6F70, NULL, 0);
    m2_save(&ora);
    m2_compare(&nat, &ora);
    m2_trace_compare();
    m2_cases++;
}

int main(void) {
    unsigned seed;

    m2_tag = "M2 SETUP";
    m2_load_code("disc/menu.bin");
    m2_require(m2_word(A6AA0) == 0x3C028006u, "6aa0 pin");
    m2_require(m2_word(A6F70) == 0x27BDFFB8u, "6f70 pin");

    m2_region(&mgr, MGR_GUEST, sizeof mgr, 1, "pManager");
    m2_region(&u1, U1_GUEST, sizeof u1, 1, "unk348");
    m2_region(stateStorage, GAME_GUEST, sizeof stateStorage, 1, "g_GameState");
    m2_region(&D_80059460, 0x80059460u, 1, 1, "D_80059460");
    m2_region(&D_80059171, 0x80059171u, 1, 1, "D_80059171");
    m2_region(&D_800594CC, 0x800594CCu, 1, 1, "D_800594CC");
    m2_menu(&menu, sizeof menu);
    M2_FIELD(0x30C, menu.availableCharacters);
    M2_FIELD(0x326, menu.unk326);
    M2_FIELD(0x32B, menu.unk32B);
    M2_FIELD(0x334, menu.unk334);
    M2_FIELD(0x335, menu.unk335);
    M2_FIELD(0x336, menu.menu1Choice);
    M2_FIELD(0x337, menu.unk337);
    M2_FIELD(0x33B, menu.unk33B);
    M2_PTRFIELD(0x33C, menu.pManager);
    M2_PTRFIELD(0x348, menu.unk348);
    M2_FIELD(0x4DC, menu.selectedPartySlot);

    T865C = m2_jal_target(0x801C6B74u);
    T65F4 = m2_jal_target(0x801C6D28u);
    TD22C4 = m2_jal_target(0x801C6FACu);
    T8164 = m2_jal_target(0x801C6FF0u);
    TSEMI = m2_jal_target(0x801C700Cu);
    TLINEF3 = m2_jal_target(0x801C7024u);
    TPOLYF4 = m2_jal_target(0x801C7104u);
    TTPAGE = m2_jal_target(0x801C7210u);
    TDRAWMODE = m2_jal_target(0x801C7238u);
    m2_callee(T865C, "func_801C865C", 2, ret_cb);
    m2_callee(T65F4, "func_801C65F4", 0, ret_cb);
    m2_callee(TD22C4, "func_801D22C4", 0, ret_cb);
    m2_callee(T8164, "func_801C8164", 4, ret_cb);
    m2_callee(TSEMI, "SetSemiTrans", 2, ret_cb);
    m2_callee(TLINEF3, "SetLineF3", 1, ret_cb);
    m2_callee(TPOLYF4, "SetPolyF4", 1, ret_cb);
    m2_callee(TTPAGE, "GetTPage", 4, ret_cb);
    m2_callee(TDRAWMODE, "SetDrawMode", 5, ret_cb);
    m2_callee_deref(4, 8); /* tw: RECT on the caller's stack */
    m2_scan_calls(A6AA0, E6AA0);
    m2_scan_calls(A6F70, E6F70);

    for (seed = 0; seed < 96; seed++) {
        run_one(0, seed);
        if (seed < 8)
            run_one(1, seed + 500u);
    }
    m2_coverage("6aa0", cov6AA0, (E6AA0 - A6AA0) / 4, A6AA0, NULL, 0);
    m2_coverage("6f70", cov6F70, (E6F70 - A6F70) / 4, A6F70, NULL, 0);
    printf("M2 SETUP PASS cases=%u checks=%u\n", m2_cases, m2_checks);
    return 0;
}
