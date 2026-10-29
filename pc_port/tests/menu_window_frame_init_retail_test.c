/* Retail differential test for func_801E53CC, a window's frame-primitive init
 * (shared retail/port body).  The retail MIPS from disc/menu.bin and the
 * shared body run on the same RAM snapshot; every libgpu call and all of RAM
 * must match.  The libgpu callees are stubs with small deterministic effects
 * (SetPoly* write len/code, SetSemiTrans / SetShadeTex flip the code bits,
 * GetTPage / GetClut return junk above bit 15 so retail's u_short masking is
 * exercised).  SetDrawMode's texture-window pointer is a stack address in
 * both worlds, so its RECT is recorded by value.  MenuWindow and MenuManager
 * have no pointers; both worlds share them in guest RAM.
 * See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define WINDOWS 0x800A0000u
#define WINDOW_STRIDE 0x800u
#define MANAGER 0x800C0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
#define NFIELDS (sizeof(menu_fields) / sizeof(menu_fields[0]))
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum {
    F_POLYG4 = 0x80043CC4u, F_POLYFT4 = 0x80043CB0u, F_SEMI = 0x80043BFCu, F_SHADE = 0x80043C24u,
    F_TPAGE = 0x80043A1Cu, F_CLUT = 0x80043A58u, F_DRAWMODE = 0x800454DCu
};
const HxCallee hx_callees[] = {
    {F_POLYG4, 1, "SetPolyG4"}, {F_POLYFT4, 1, "SetPolyFT4"}, {F_SEMI, 2, "SetSemiTrans"},
    {F_SHADE, 2, "SetShadeTex"}, {F_TPAGE, 4, "GetTPage"}, {F_CLUT, 2, "GetClut"},
    {F_DRAWMODE, 5, "SetDrawMode"},
};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

static uint32_t rect_digest(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    return (((x & 0xFFFF) * 31u + (y & 0xFFFF)) * 31u + (w & 0xFFFF)) * 31u + (h & 0xFFFF);
}

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    (void)index;
    switch (fn) {
    case F_POLYG4: hx_w8(a[0] + 3, 8); hx_w8(a[0] + 7, 0x38); return 0;
    case F_POLYFT4: hx_w8(a[0] + 3, 9); hx_w8(a[0] + 7, 0x2C); return 0;
    case F_SEMI: hx_w8(a[0] + 7, (uint8_t)(a[1] ? hx_r8(a[0] + 7) | 2 : hx_r8(a[0] + 7) & ~2)); return 0;
    case F_SHADE: hx_w8(a[0] + 7, (uint8_t)(a[1] ? hx_r8(a[0] + 7) | 1 : hx_r8(a[0] + 7) & ~1)); return 0;
    case F_TPAGE: return 0xA5A50000u | ((a[0] * 0x80u + a[1] * 0x20u + (a[2] >> 6) + (a[3] << 4)) & 0xFFFFu);
    case F_CLUT: return 0x5A5A0000u | (((a[1] << 6) | ((a[0] >> 4) & 63u)) & 0xFFFFu);
    case F_DRAWMODE:
        if (side == 0) /* retail: a[4] is the guest stack RECT; record it by value */
            ((uint32_t*)a)[4] = rect_digest(hx_r16(a[4]), hx_r16(a[4] + 2), hx_r16(a[4] + 4), hx_r16(a[4] + 6));
        hx_w32(a[0] + 4, 0xE1000000u | (a[3] & 0x7FFu) | (a[2] << 9) | (a[1] << 10));
        hx_w32(a[0] + 8, a[4]);
        return 0;
    }
    return 0;
}
void SetPolyG4(POLY_G4* p) { hx_native(F_POLYG4, 1, hx_addr(p)); }
void SetPolyFT4(POLY_FT4* p) { hx_native(F_POLYFT4, 1, hx_addr(p)); }
void SetSemiTrans(void* p, int abe) { hx_native(F_SEMI, 2, hx_addr(p), (uint32_t)abe); }
void SetShadeTex(void* p, int tge) { hx_native(F_SHADE, 2, hx_addr(p), (uint32_t)tge); }
u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)hx_native(F_TPAGE, 4, (uint32_t)tp, (uint32_t)abr, (uint32_t)x, (uint32_t)y);
}
u_short GetClut(int x, int y) { return (u_short)hx_native(F_CLUT, 2, (uint32_t)x, (uint32_t)y); }
void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT16* tw) {
    hx_native(F_DRAWMODE, 5, hx_addr(p), (uint32_t)dfe, (uint32_t)dtd, (uint32_t)tpage,
              rect_digest((uint16_t)tw->x, (uint16_t)tw->y, (uint16_t)tw->w, (uint16_t)tw->h));
}
extern void func_801E53CC(u8 windowIndex);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint32_t indices[] = {0, 1, 2, 3, 4, 5, 6, 0x103};   /* u8: 0x103 -> 3 */
    unsigned w, pattern;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801E53CCu, 0x801E56E8u);
    for (w = 0; w < sizeof indices / sizeof indices[0]; ++w)
    for (pattern = 0; pattern < 16; ++pattern) {
        unsigned i;
        s32* tex = &native_menu.texPage0;
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.pManager = (MenuManager*)(uintptr_t)MANAGER;
        for (i = 0; i < MENU_MAX_NUM_WINDOWS; ++i)
            native_menu.windows[i] = (MenuWindow*)(uintptr_t)(WINDOWS + i * WINDOW_STRIDE);
        for (i = 0; i < (uint32_t)(&native_menu.texPageY3 - tex) + 1; ++i)  /* texPage0 .. texPageY3 */
            tex[i] = (s32)((i * 0x45u + pattern * 0x3Bu + 7u) & 0x3FFu);
        for (i = 0; i < MENU_MAX_NUM_WINDOWS * WINDOW_STRIDE; ++i)
            hx_w8(WINDOWS + i, (uint8_t)(i * 29u + pattern + (i >> 11)));
        for (i = 0; i < sizeof(MenuManager); ++i) hx_w8(MANAGER + i, (uint8_t)(0xC0 + i + pattern));
        hx_mirror_out(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801E53CCu, indices[w], 0, 0, 0, 2000000);
        hx_retail_done();
        func_801E53CC((u8)indices[w]);
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801E53CC", NULL, 0);
    printf("MENU WINDOW FRAME INIT PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
