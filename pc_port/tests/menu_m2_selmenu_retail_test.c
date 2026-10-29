/* M2 selection-menu sub-renderer retail differential: func_801CEC40 (dim /
 * undim restyle of the label + cursor POLY_FT4 sets, then batch submit).
 *
 * Native C body (src/menu/main/misc.c, XENO_PC_PORT arm) vs retail machine
 * code on the interpreter oracle (TEST ORACLE ONLY), identical guest memory,
 * via lib/m2_retail_diff.h.
 */
#include "common.h"
#include "system/menu.h"
#include "lib/m2_retail_diff.h"

#define AEC40 0x801CEC40u
#define EEC40 0x801CF308u
#define MGR_GUEST 0x80120000u
#define SEL_GUEST 0x80122000u

SystemMenu *g_Menu;
static SystemMenu menu;
static MenuManager mgr;
static MenuSelectionMenu sel;

_Static_assert(sizeof(MenuSelectionMenu) == 0x1194 &&
                   offsetof(MenuSelectionMenu, polysTexts) == 0x8C0 &&
                   offsetof(MenuSelectionMenu, numCursors) == 0x1188 &&
                   offsetof(MenuSelectionMenu, textsRenderCtx) == 0x1191,
               "MenuSelectionMenu is guest layout");

static u32 TSEMI, TSHADE, TE2B4;

void SetSemiTrans(void *p, int abe) { M2_SPY(TSEMI, M2_U(p), (u32)abe); }
void SetShadeTex(void *p, int tge) { M2_SPY(TSHADE, M2_U(p), (u32)tge); }
void func_801CE2B4(s32 n, void *polys, s32 ctx) {
    M2_SPY(TE2B4, (u32)n, M2_U(polys), (u32)ctx);
}

extern void func_801CEC40(void);
static unsigned covEC40[(EEC40 - AEC40) / 4];

static void run_one(unsigned seed) {
    static M2Snap before, nat, ora;
    static const s32 counts[] = { 0, 1, 2, 5, 13, 27, 28, -3 };
    m2_pattern(&menu, sizeof menu, seed + 1u);
    m2_pattern(&mgr, sizeof mgr, seed + 2u);
    m2_pattern(&sel, sizeof sel, seed + 3u);
    menu.pManager = &mgr;
    menu.pSelectionMenu = &sel;
    mgr.shouldRenderSelectionMenu = (u8)((seed % 9u == 8u) ? 0 : 1 + seed % 3u);
    sel.numTexts = counts[seed % 8u];
    sel.numCursors = counts[(seed / 8u + seed) % 8u];
    sel.textsRenderCtx = (u8)(seed & 1u);
    sel.cursorsRenderCtx = (u8)((seed >> 1) & 1u);
    sel.unk1192 = (u8)((seed % 3u == 0) ? 0 : (seed % 3u == 1) ? 1 : 0x40u);
    sel.unk1193 = (u8)((seed % 5u == 0) ? sel.unk1192 : (seed % 5u == 1) ? 0 : 7u);
    m2_save(&before);
    m2_trace_begin();
    g_Menu = &menu;
    func_801CEC40();
    m2_trace_stash();
    m2_save(&nat);
    m2_restore(&before);
    m2_oracle(AEC40, EEC40, covEC40, NULL, 0);
    m2_save(&ora);
    m2_compare(&nat, &ora);
    m2_trace_compare();
    m2_cases++;
}

int main(void) {
    unsigned seed;

    m2_tag = "M2 SELMENU";
    m2_load_code("disc/menu.bin");
    m2_require(m2_word(AEC40) == 0x3C048006u, "ec40 pin");
    m2_region(&mgr, MGR_GUEST, sizeof mgr, 1, "pManager");
    m2_region(&sel, SEL_GUEST, sizeof sel, 1, "pSelectionMenu");
    m2_menu(&menu, sizeof menu);
    M2_PTRFIELD(0x33C, menu.pManager);
    M2_PTRFIELD(0x350, menu.pSelectionMenu);

    TSEMI = m2_jal_target(0x801CECD0u);
    TSHADE = m2_jal_target(0x801CED08u);
    TE2B4 = m2_jal_target(0x801CF2C0u);
    m2_callee(TSEMI, "SetSemiTrans", 2, NULL);
    m2_callee(TSHADE, "SetShadeTex", 2, NULL);
    m2_callee(TE2B4, "func_801CE2B4", 3, NULL);
    m2_scan_calls(AEC40, EEC40);

    for (seed = 0; seed < 160; seed++)
        run_one(seed);
    m2_coverage("ec40", covEC40, (EEC40 - AEC40) / 4, AEC40, NULL, 0);
    printf("M2 SELMENU PASS cases=%u checks=%u\n", m2_cases, m2_checks);
    return 0;
}
