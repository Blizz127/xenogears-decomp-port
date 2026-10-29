/* M2 portrait-face builders retail differential: func_801D36E0 (MenuString
 * face sprite for a slot/variant/style), func_801D4F2C (portrait frame +
 * face quads).
 *
 * Native C bodies (src/menu/main/misc.c, XENO_PC_PORT arm) vs retail machine
 * code on the interpreter oracle (TEST ORACLE ONLY), identical guest memory,
 * via lib/m2_retail_diff.h. The MenuString argument is a field-wise view of
 * the retail 0x80-byte layout.
 */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "lib/m2_retail_diff.h"

#define A36E0 0x801D36E0u
#define E36E0 0x801D397Cu
#define A4F2C 0x801D4F2Cu
#define E4F2C 0x801D50ECu

#define MGR_GUEST 0x80120000u
#define STR_GUEST 0x80140000u
#define PORT_GUEST 0x80150000u
#define RES_GUEST 0x8010C000u
#define OVD_GUEST 0x801E9000u
#define GAME_GUEST 0x8006D634u
#define PAL1_GUEST 0x800595D4u
#define PAL2_GUEST 0x80059414u

SystemMenu *g_Menu;
u16 g_SystemPalette1, g_SystemPalette2;
static SystemMenu menu;
static MenuManager mgr;
static MenuString str;
static u8 port[3][0x100] __attribute__((aligned(8)));
static u8 resBuf[0x100];
u8 stateStorage[0x1000] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");
u8 ovData[0x2000] __attribute__((aligned(8)));
__asm__(".globl D_801E9B58\n.set D_801E9B58, ovData + 0xB58\n"
        ".globl D_801E9B5C\n.set D_801E9B5C, ovData + 0xB5C\n"
        ".globl D_801EA17C\n.set D_801EA17C, ovData + 0x117C\n"
        ".globl D_801EA18C\n.set D_801EA18C, ovData + 0x118C\n"
        ".globl D_801EA578\n.set D_801EA578, ovData + 0x1578\n"
        ".globl D_801EA584\n.set D_801EA584, ovData + 0x1584\n"
        ".globl D_801EA5C4\n.set D_801EA5C4, ovData + 0x15C4\n"
        ".globl D_801EA5D0\n.set D_801EA5D0, ovData + 0x15D0\n");

static u32 T851C, TE920C, TE927C, TTPAGE, T2675C;

static u32 ret_cb(u32 idx, const u32 *a) {
    u32 t = m2_callees[idx].addr;
    if (t == TTPAGE)
        return 0xBEEF0000u | (a[2] + a[3] + a[1]);
    if (t == T2675C)
        return 1;
    return 0;
}

void func_801C851C(SVECTOR *v, s32 x, s32 y, s32 w, s32 h) {
    M2_SPY(T851C, M2_U(v), (u32)x, (u32)y, (u32)w, (u32)h);
}
void func_801E920C(POLY_FT4 *f, s32 x, s32 y, s32 a, s32 b, s32 c, s32 d) {
    M2_SPY(TE920C, M2_U(f), (u32)x, (u32)y, (u32)a, (u32)b, (u32)c, (u32)d);
}
void func_801E927C(POLY_FT4 *f) { M2_SPY(TE927C, M2_U(f)); }
u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)M2_SPY(TTPAGE, (u32)tp, (u32)abr, (u32)x, (u32)y);
}
s32 func_8002675C(u8 *res, s32 glyph, void *dst, s32 ctx, s32 x, s32 y, s32 scale) {
    return (s32)M2_SPY(T2675C, M2_U(res), (u32)glyph, M2_U(dst), (u32)ctx, (u32)x, (u32)y,
                       (u32)scale);
}

extern void func_801D36E0(MenuString *pStr, s32 slot, s32 variant, s32 style);
extern void func_801D4F2C(u8 slot, u8 charId, s32 x, s32 y);

static unsigned cov36E0[(E36E0 - A36E0) / 4], cov4F2C[(E4F2C - A4F2C) / 4];

static void init_case(unsigned seed) {
    unsigned i;
    m2_pattern(&menu, sizeof menu, seed + 1u);
    m2_pattern(&mgr, sizeof mgr, seed + 2u);
    m2_pattern(&str, sizeof str, seed + 3u);
    m2_pattern(port, sizeof port, seed + 4u);
    m2_pattern(stateStorage, sizeof stateStorage, seed + 5u);
    m2_pattern(ovData, sizeof ovData, seed + 6u);
    menu.pManager = &mgr;
    menu.unk2DC = resBuf;
    menu.renderContext = (int)(seed & 1u);
    str.pVramBuffer = (void *)(uintptr_t)0x80199000u;
    for (i = 0; i < 3; i++) {
        *(u32 *)&menu.unk39C[i * 4] = (u32)(uintptr_t)port[i];
        mgr.currentCharacterIDs[i] = (u8)((seed + i * 5u) % 11u);
    }
    g_SystemPalette1 = (u16)(0x1111u + seed);
    g_SystemPalette2 = (u16)(0x2222u + seed);
    /* Signed table words (negative entries exercise the u32 shift). */
    for (i = 0; i < 12; i++) {
        if ((seed + i) % 3u == 0)
            *(u32 *)(ovData + 0x1578 + i * 4) |= 0x80000000u;
    }
}

static void run_one(unsigned fn, unsigned seed) {
    static M2Snap before, nat, ora;
    u32 args[4];
    init_case(seed);
    m2_save(&before);
    m2_trace_begin();
    g_Menu = &menu;
    if (fn == 0) {
        args[0] = M2P(&str);
        args[1] = (seed % 5u == 0) ? 0x102u : seed % 3u;
        args[2] = (seed % 4u == 0) ? 0x100u : (seed / 3u) & 1u;
        args[3] = (seed % 7u == 0) ? 0x203u : seed % 4u;
        func_801D36E0(&str, (s32)args[1], (s32)args[2], (s32)args[3]);
    } else {
        args[0] = seed % 3u;
        args[1] = (seed * 7u) & 0xFFu;
        args[2] = (seed % 4u == 0) ? 0xFFF0u + seed : 0x40u + seed;
        args[3] = (seed % 5u == 0) ? 0x1FFF0u : 0x20u + seed;
        func_801D4F2C((u8)args[0], (u8)args[1], (s32)args[2], (s32)args[3]);
    }
    m2_trace_stash();
    m2_save(&nat);
    m2_restore(&before);
    if (fn == 0)
        m2_oracle(A36E0, E36E0, cov36E0, args, 4);
    else
        m2_oracle(A4F2C, E4F2C, cov4F2C, args, 4);
    m2_save(&ora);
    m2_compare(&nat, &ora);
    m2_trace_compare();
    m2_cases++;
}

int main(void) {
    unsigned seed;

    m2_tag = "M2 FACES";
    m2_load_code("disc/menu.bin");
    m2_require(m2_word(A36E0) == 0x3C028006u, "36e0 pin");
    m2_require(m2_word(A4F2C) == 0x27BDFFC8u, "4f2c pin");

    m2_region(&mgr, MGR_GUEST, sizeof mgr, 1, "pManager");
    m2_region(port, PORT_GUEST, sizeof port, 1, "portrait bufs");
    m2_region(resBuf, RES_GUEST, sizeof resBuf, 1, "unk2DC");
    m2_region(ovData, OVD_GUEST, sizeof ovData, 1, "overlay data");
    m2_region(stateStorage, GAME_GUEST, sizeof stateStorage, 1, "g_GameState");
    m2_region(&g_SystemPalette1, PAL1_GUEST, 2, 1, "g_SystemPalette1");
    m2_region(&g_SystemPalette2, PAL2_GUEST, 2, 1, "g_SystemPalette2");
    m2_view(STR_GUEST, 0x80u);
    M2_MENUSTRING(STR_GUEST, &str);
    m2_hostblock(&str, sizeof str, "MenuString");
    m2_menu(&menu, sizeof menu);
    M2_PTRFIELD(0x2DC, menu.unk2DC);
    M2_FIELD(0x308, menu.renderContext);
    M2_PTRFIELD(0x33C, menu.pManager);
    m2_field(0x39C, &menu.unk39C[0], 4, M2F_PTR32);
    m2_field(0x3A0, &menu.unk39C[4], 4, M2F_PTR32);
    m2_field(0x3A4, &menu.unk39C[8], 4, M2F_PTR32);

    T851C = m2_jal_target(0x801D3938u);
    TE920C = m2_jal_target(0x801D390Cu);
    TE927C = m2_jal_target(0x801D3724u);
    TTPAGE = m2_jal_target(0x801D3738u);
    T2675C = m2_jal_target(0x801D4F88u);
    m2_callee(T851C, "func_801C851C", 5, ret_cb);
    m2_callee(TE920C, "func_801E920C", 7, ret_cb);
    m2_callee(TE927C, "func_801E927C", 1, ret_cb);
    m2_callee(TTPAGE, "GetTPage", 4, ret_cb);
    m2_callee(T2675C, "func_8002675C", 7, ret_cb);
    m2_scan_calls(A36E0, E36E0);
    m2_scan_calls(A4F2C, E4F2C);

    for (seed = 0; seed < 120; seed++) {
        run_one(0, seed);
        run_one(1, seed + 1000u);
    }
    m2_coverage("36e0", cov36E0, (E36E0 - A36E0) / 4, A36E0, NULL, 0);
    m2_coverage("4f2c", cov4F2C, (E4F2C - A4F2C) / 4, A4F2C, NULL, 0);
    printf("M2 FACES PASS cases=%u checks=%u\n", m2_cases, m2_checks);
    return 0;
}
