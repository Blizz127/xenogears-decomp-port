/* M2 status-panel retail differential: func_801CD81C / func_801CDC6C.
 *
 * Native C bodies (src/menu/main/misc.c, XENO_PC_PORT arm) vs the retail
 * machine code on the interpreter oracle (TEST ORACLE ONLY), identical guest
 * memory, via pc_port/tests/lib/m2_retail_diff.h. Compared per case: every
 * byte of the panel, game state, overlay tables, palettes, x/y tables and
 * resource scratch, the host SystemMenu, and the callee trace. O0/O2/UBSan;
 * mutants must FAIL. See run_menu_m2_panel_retail_test.sh.
 */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "lib/m2_retail_diff.h"

#define PANEL_GUEST 0x80110000u
#define XTAB_GUEST 0x80111000u
#define YTAB_GUEST 0x80112000u
#define SCRATCH_GUEST 0x8010C000u
#define OV_GUEST 0x801EA000u
#define OV_SPAN 0x1000u
#define GAME_GUEST 0x8006D634u
#define GAME_SPAN 0x4600u
#define PAL1_GUEST 0x800595D4u
#define PAL2_GUEST 0x80059414u

#define D81C_ENTRY 0x801CD81Cu
#define D81C_END 0x801CDB1Cu
#define DC6C_ENTRY 0x801CDC6Cu
#define DC6C_END 0x801CE0CCu

SystemMenu *g_Menu;
u16 g_SystemPalette1;
u16 g_SystemPalette2;
u8 stateStorage[GAME_SPAN] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");
u8 ovMenu[OV_SPAN] __attribute__((aligned(8)));
__asm__(".globl D_801EA4DC\n.set D_801EA4DC, ovMenu + 0x4DC\n");
__asm__(".globl D_801EA578\n.set D_801EA578, ovMenu + 0x578\n");
__asm__(".globl D_801EA5C4\n.set D_801EA5C4, ovMenu + 0x5C4\n");

static SystemMenu menu;
static MenuCharacter panel;
static s32 xTab[24], yTab[24];
static u8 resBuf[0x400];

_Static_assert(sizeof(MenuCharacter) == 0xBEC && sizeof(POLY_FT4) == 0x28,
               "panel layout is guest layout");
_Static_assert(sizeof(menu.digits) == 9, "digit buffer");

static u32 T2675C, TC80B8, TE927C, TGETTPAGE, TE920C;

static u32 ret_cb(u32 idx, const u32 *a) {
    u32 t = m2_callees[idx].addr;
    if (t == T2675C)
        /* Cells used (one 2-poly cell per glyph, 0 when skipped): keeps the
         * builders inside their poly arrays, as the real callee does. */
        return ((a[1] * 5u) + (a[4] & 15u) + (a[5] & 3u)) % 5u != 0u;
    if (t == TGETTPAGE)
        return (0x20u + (a[2] & 0xF8u) + ((a[3] & 0xF8u) << 3) + (a[1] & 3u) +
                (a[0] & 1u)) & 0xFFFFu;
    return 0;
}

s32 func_8002675C(u8 *res, s32 glyph, void *dst, s32 ctx, s32 x, s32 y, s32 scale) {
    return (s32)M2_SPY(T2675C, M2_U(res), (u32)glyph, M2_U(dst), (u32)ctx, (u32)x,
                       (u32)y, (u32)scale);
}
void func_801C80B8(u32 value) {
    M2_SPY(TC80B8, (uintptr_t)value);
}
void func_801E927C(POLY_FT4 *face) {
    M2_SPY(TE927C, M2_U(face));
}
u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)M2_SPY(TGETTPAGE, (u32)tp, (u32)abr, (u32)x, (u32)y);
}
void func_801E920C(POLY_FT4 *face, s32 x, s32 y, s32 a, s32 b, s32 c, s32 d) {
    M2_SPY(TE920C, M2_U(face), (u32)x, (u32)y, (u32)a, (u32)b, (u32)c, (u32)d);
}

extern void func_801CD81C(MenuCharacter *panel, u8 charId, u8 slot,
                          s32 *xTable, s32 *yTable, u8 mode);
extern void func_801CDC6C(MenuCharacter *panel, u8 charId, u8 slot,
                          s32 *xTable, s32 *yTable, u8 mode);

static void w16(u8 *base, unsigned off, u16 v) {
    base[off] = (u8)v;
    base[off + 1] = (u8)(v >> 8);
}
static void w32b(u8 *base, unsigned off, u32 v) {
    w16(base, off, (u16)v);
    w16(base, off + 2, (u16)(v >> 16));
}

static const u16 h16[] = { 0, 1, 0xFFu, 0x100u, 9999, 0xFFFFu };
static const u32 h32[] = { 0, 1, 0xFFFFFFu, 0xFFFFFFFFu };
static const u32 bigx[] = { 0x10u, 0x7FFFu, 0x8000u, 0xFFFFu, 0x10000u, 0x123456u };
static const u32 bigy[] = { 0x20u, 0xFF00u, 0xFFF0u, 0xFFFFu, 0x10020u };
static const u8 slots[] = { 0, 1, 2, 5 };

static void init_case(unsigned seed, unsigned charId, unsigned rc) {
    unsigned i;
    u8 *ch = stateStorage + 0x26Cu + charId * 0xA4u;
    unsigned gear = (seed * 5u) % 20u;
    u8 *gg = stateStorage + 0x978u + gear * 0xA4u;
    s32 *t4dc = (s32 *)(ovMenu + 0x4DCu);

    m2_pattern(stateStorage, sizeof stateStorage, seed + 11u);
    m2_pattern(ovMenu, sizeof ovMenu, seed + 101u);
    m2_pattern(&panel, sizeof panel, seed + 1001u);
    m2_pattern(resBuf, sizeof resBuf, seed + 4001u);
    m2_pattern(&menu, sizeof menu, seed + 6001u);

    for (i = 0; i < 18; i++)
        t4dc[i] = (s32)(0x100u + ((seed * 37u + i * 101u) & 0xFFFu));
    t4dc[(seed * 7u) % 18] = (s32)0xFFFF;
    t4dc[(seed * 7u + 5u) % 18] = (s32)0xFFFF;

    w16(ch, 0x4Cu, h16[seed % 6]);
    w16(ch, 0x4Eu, h16[(seed + 2u) % 6]);
    w16(ch, 0x50u, h16[(seed + 1u) % 6]);
    w16(ch, 0x52u, h16[(seed + 3u) % 6]);
    ch[0xA0u] = (u8)gear;
    w32b(gg, 0x60u, h32[seed % 4]);
    w32b(gg, 0x64u, h32[(seed + 1u) % 4]);

    for (i = 0; i < 9; i++)
        menu.digits[i] = ((seed + i) % 4u == 0u) ? 0xFFu : (u8)((i * 3u + seed) % 10u);
    if (seed % 5u == 4u)
        m2_pattern(menu.digits, 9, seed + 5001u);

    for (i = 0; i < 24; i++) {
        xTab[i] = (s32)(0x40u + i * 8u + (seed & 7u));
        yTab[i] = (s32)(0x30u + i * 6u + ((seed >> 2) & 7u));
    }
    /* Adversarial coords: retail loads the E920C pair as halfwords and
     * wraps y to 16 bits; sums elsewhere are 32-bit addu. */
    xTab[16] = (s32)bigx[seed % 6];
    yTab[16] = (s32)bigy[seed % 5];
    xTab[9] = (s32)bigx[(seed + 3u) % 6];
    yTab[9] = (s32)bigy[(seed + 2u) % 5];
    if (seed % 7u == 3u) {
        xTab[12] = (s32)0x7FFFFFF8;
        yTab[13] = (s32)0x7FFFFFF0;
    }

    menu.renderContext = (int)rc;
    menu.unk2DC = resBuf;
    g_SystemPalette1 = (u16)(0x1234u + seed);
    g_SystemPalette2 = (u16)(0x5678u + seed);
}

static unsigned cov_d81c[(D81C_END - D81C_ENTRY) / 4];
static unsigned cov_dc6c[(DC6C_END - DC6C_ENTRY) / 4];

static void run_one(unsigned fn, unsigned seed, unsigned slot, unsigned mode,
                    unsigned charId, unsigned rc) {
    static M2Snap before, nat, ora;
    u32 args[6];

    init_case(seed, charId, rc);
    m2_save(&before);
    m2_trace_begin();
    g_Menu = &menu;
    if (fn)
        func_801CDC6C(&panel, (u8)charId, (u8)slot, xTab, yTab, (u8)mode);
    else
        func_801CD81C(&panel, (u8)charId, (u8)slot, xTab, yTab, (u8)mode);
    m2_trace_stash();
    m2_save(&nat);

    m2_restore(&before);
    args[0] = PANEL_GUEST;
    args[1] = charId;
    args[2] = slot;
    args[3] = XTAB_GUEST;
    args[4] = YTAB_GUEST;
    args[5] = mode;
    if (fn)
        m2_oracle(DC6C_ENTRY, DC6C_END, cov_dc6c, args, 6);
    else
        m2_oracle(D81C_ENTRY, D81C_END, cov_d81c, args, 6);
    m2_save(&ora);
    m2_compare(&nat, &ora);
    m2_trace_compare();
    m2_cases++;
}

int main(void) {
    unsigned i, seed;

    m2_tag = "M2 PANEL";
    m2_load_code("disc/menu.bin");
    m2_require(m2_word(D81C_ENTRY) == 0x27BDFFA8u, "d81c pin");
    m2_require(m2_word(DC6C_ENTRY) == 0x27BDFF98u, "dc6c pin");

    m2_region(stateStorage, GAME_GUEST, sizeof stateStorage, 1, "g_GameState");
    m2_region(ovMenu, OV_GUEST, sizeof ovMenu, 1, "overlay tables");
    m2_region(&panel, PANEL_GUEST, sizeof panel, 1, "panel");
    m2_region(xTab, XTAB_GUEST, sizeof xTab, 1, "xTable");
    m2_region(yTab, YTAB_GUEST, sizeof yTab, 1, "yTable");
    m2_region(resBuf, SCRATCH_GUEST, sizeof resBuf, 1, "unk2DC");
    m2_region(&g_SystemPalette1, PAL1_GUEST, 2, 1, "g_SystemPalette1");
    m2_region(&g_SystemPalette2, PAL2_GUEST, 2, 1, "g_SystemPalette2");
    m2_menu(&menu, sizeof menu);
    M2_PTRFIELD(0x2DC, menu.unk2DC);
    M2_FIELD(0x308, menu.renderContext);
    M2_FIELD(0x31C, menu.digits);

    T2675C = m2_jal_target(0x801CD8ECu);
    m2_require(m2_jal_target(0x801CD958u) == T2675C, "2675c sibling");
    TE927C = m2_jal_target(0x801CD984u);
    TGETTPAGE = m2_jal_target(0x801CD998u);
    TE920C = m2_jal_target(0x801CDAE0u);
    TC80B8 = m2_jal_target(0x801CDD34u);
    m2_callee(T2675C, "func_8002675C", 7, ret_cb);
    m2_callee(TC80B8, "func_801C80B8", 1, ret_cb);
    m2_callee(TE927C, "func_801E927C", 1, ret_cb);
    m2_callee(TGETTPAGE, "GetTPage", 4, ret_cb);
    m2_callee(TE920C, "func_801E920C", 7, ret_cb);
    m2_scan_calls(D81C_ENTRY, D81C_END);
    m2_scan_calls(DC6C_ENTRY, DC6C_END);

    for (seed = 0; seed < 64; seed++) {
        unsigned slot = slots[seed % 4];
        unsigned mode = seed % 3;
        unsigned charId = (seed * 3u) % 11u;
        unsigned rc = (seed >> 1) & 1u;
        run_one(0, seed, slot, mode, charId, rc);
        run_one(1, seed + 1000u, slot, mode, charId, rc);
    }
    m2_coverage("d81c", cov_d81c, (D81C_END - D81C_ENTRY) / 4, D81C_ENTRY, NULL, 0);
    m2_coverage("dc6c", cov_dc6c, (DC6C_END - DC6C_ENTRY) / 4, DC6C_ENTRY, NULL, 0);
    (void)i;
    printf("M2 PANEL PASS cases=%u checks=%u d81c_insns=%u dc6c_insns=%u\n", m2_cases,
           m2_checks, (D81C_END - D81C_ENTRY) / 4, (DC6C_END - DC6C_ENTRY) / 4);
    return 0;
}
