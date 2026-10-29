/* Retail differential test for func_801D1EE0, the menu pointer cursor and
 * highlight bar (shared retail/port body).  The retail MIPS from
 * disc/menu.bin and the native body run on the same RAM snapshot; the
 * func_8002675C call and all of RAM must match.  The per-option x/y tables
 * D_801E9A00/D_801E9A2C come from the retail overlay image (the native body
 * is bound to their retail addresses).  See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"
#include <stddef.h>

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define MANAGER 0x800A0000u
#define CUR 0x800B0000u
#define ATLAS 0x800E0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum { F_ATLAS = 0x8002675Cu };
const HxCallee hx_callees[] = {{F_ATLAS, 7, "func_8002675C"}};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    (void)a;
    (void)side;
    return fn == F_ATLAS ? 1 + (index & 3) : 0;
}
#define N(fn, n, ...) hx_native(fn, n, __VA_ARGS__)
s32 func_8002675C(u8* atlas, s32 id, void* buf, s32 rc, s32 x, s32 y, s32 scale) {
    return (s32)N(F_ATLAS, 7, hx_addr(atlas), (uint32_t)id, hx_addr(buf), (uint32_t)rc, (uint32_t)x, (uint32_t)y,
                  (uint32_t)scale);
}
extern void func_801D1EE0(s32 selected, s32 buildBar);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint32_t bars[] = {0, 1, 0x100, 0x1A5};
    unsigned pattern, selected, bar, rc;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801D1EE0u, 0x801D22C4u);
    for (pattern = 0; pattern < 3; ++pattern)
    for (selected = 0; selected < 11; ++selected)
    for (bar = 0; bar < 4; ++bar)
    for (rc = 0; rc < 2; ++rc) {
        uint32_t i;
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.renderContext = (int)rc;
        native_menu.unk2DC = (void*)(uintptr_t)ATLAS;
        native_menu.unk348 = (MenuUnk1*)(uintptr_t)CUR;
        native_menu.pManager = (MenuManager*)(uintptr_t)MANAGER;
        for (i = 0; i < 0x6C; ++i) hx_w8(MANAGER + i, (uint8_t)(i * 29 + pattern * 5));
        for (i = 0; i < 0x200; ++i) hx_w8(CUR + i, (uint8_t)(i * 37 + pattern * 13 + selected));
        hx_w8(CUR + offsetof(MenuUnk1, unk15B), (uint8_t)(0x30 + pattern * 0x41 + selected * 3));
        hx_mirror_out(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801D1EE0u, selected, bars[bar], 0, 0, 2000000);
        hx_retail_done();
        func_801D1EE0((s32)selected, (s32)bars[bar]);
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801D1EE0", NULL, 0);
    printf("MENU POINTER CURSOR PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
