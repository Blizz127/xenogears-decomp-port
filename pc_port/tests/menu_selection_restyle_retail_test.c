/* Retail differential test for func_801CEC40, the selection menu's restyle and
 * batch submit (shared retail/port body).  The retail MIPS from disc/menu.bin
 * and the native body run on the same RAM snapshot; every callee call
 * (SetSemiTrans, SetShadeTex, func_801CE2B4) and all of RAM must match.
 * See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"
#include <stddef.h>

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define MANAGER 0x800A0000u
#define SEL 0x800B0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum { F_SEMI = 0x80043BFCu, F_SHADE = 0x80043C24u, F_BATCH = 0x801CE2B4u };
const HxCallee hx_callees[] = {
    {F_SEMI, 2, "SetSemiTrans"}, {F_SHADE, 2, "SetShadeTex"}, {F_BATCH, 3, "func_801CE2B4"},
};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

/* The libgpu flag bits in the prim's code byte (+7): 0x02 semi-transparent,
 * 0x01 shade-texture-off. */
uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    (void)index;
    (void)side;
    switch (fn) {
    case F_SEMI: { uint8_t c = hx_r8(a[0] + 7); hx_w8(a[0] + 7, a[1] ? (c | 2) : (c & ~2)); return 0; }
    case F_SHADE: { uint8_t c = hx_r8(a[0] + 7); hx_w8(a[0] + 7, a[1] ? (c | 1) : (c & ~1)); return 0; }
    default: return 0;
    }
}

#define N(fn, n, ...) hx_native(fn, n, __VA_ARGS__)
void SetSemiTrans(void* p, int abe) { N(F_SEMI, 2, hx_addr(p), (uint32_t)abe); }
void SetShadeTex(void* p, int tge) { N(F_SHADE, 2, hx_addr(p), (uint32_t)tge); }
void func_801CE2B4(s32 count, u8* list, s32 rc) { N(F_BATCH, 3, (uint32_t)count, hx_addr(list), (uint32_t)rc); }
extern void func_801CEC40(void);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint32_t counts[] = {0, 3, 28};
    unsigned pattern, render, dim, applied, rc, nt, nc;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801CEC40u, 0x801CF308u);
    for (pattern = 0; pattern < 2; ++pattern)
    for (render = 0; render < 2; ++render)
    for (dim = 0; dim < 2; ++dim)
    for (applied = 0; applied < 2; ++applied)
    for (rc = 0; rc < 4; ++rc)
    for (nt = 0; nt < 3; ++nt)
    for (nc = 0; nc < 3; ++nc) {
        uint32_t i;
        uint8_t v1192 = (uint8_t)(dim ? 1 + pattern : 0);
        uint8_t v1193 = (uint8_t)(applied ? v1192 : (v1192 ? 0 : 2));
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.pManager = (MenuManager*)(uintptr_t)MANAGER;
        native_menu.pSelectionMenu = (MenuSelectionMenu*)(uintptr_t)SEL;
        for (i = 0; i < 0x6C; ++i) hx_w8(MANAGER + i, (uint8_t)(i * 29 + pattern * 5));
        hx_w8(MANAGER + offsetof(MenuManager, shouldRenderSelectionMenu), (uint8_t)(render ? 1 + pattern : 0));
        for (i = 0; i < sizeof(MenuSelectionMenu); ++i) hx_w8(SEL + i, (uint8_t)(i * 13 + pattern * 7 + (i >> 8)));
        hx_w32(SEL + offsetof(MenuSelectionMenu, numTexts), counts[nt]);
        hx_w32(SEL + offsetof(MenuSelectionMenu, numCursors), counts[nc]);
        hx_w8(SEL + offsetof(MenuSelectionMenu, textsRenderCtx), (uint8_t)(rc & 1));
        hx_w8(SEL + offsetof(MenuSelectionMenu, cursorsRenderCtx), (uint8_t)(rc >> 1));
        hx_w8(SEL + offsetof(MenuSelectionMenu, unk1192), v1192);
        hx_w8(SEL + offsetof(MenuSelectionMenu, unk1193), v1193);
        hx_mirror_out(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801CEC40u, 0, 0, 0, 0, 2000000);
        hx_retail_done();
        func_801CEC40();
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801CEC40", NULL, 0);
    printf("MENU SELECTION RESTYLE PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
