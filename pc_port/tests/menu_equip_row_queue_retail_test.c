/* Retail differential test for func_801CE860, which projects and queues the
 * Equip-row bar quads and glyph strips (shared retail/port body).  The retail
 * MIPS from disc/menu.bin and the native body run on the same RAM snapshot;
 * every callee call (RotTransPers4, AddPrim, func_801CE198) and all of RAM
 * must match.  See lib/menu_retail_harness.h.
 * RotTransPers4's last two arguments point at the caller's stack locals and
 * are not compared.  The native AddPrim target in the host GfxEnvironment (8-byte
 * ot entries) is recorded as its retail address (ot at +0x70, 4-byte entries). */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"
#include <stddef.h>

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define BUF 0x800B0000u
#define ENV 0x800E0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum { F_RTP4 = 0x8004A73Cu, F_ADDPRIM = 0x80043B48u, F_STRIP = 0x801CE198u };
const HxCallee hx_callees[] = {
    {F_RTP4, 8, "RotTransPers4"}, {F_ADDPRIM, 2, "AddPrim"}, {F_STRIP, 4, "func_801CE198"},
};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    unsigned k;
    (void)side;
    if (fn == F_RTP4) {
        for (k = 0; k < 4; ++k)
            hx_w32(a[4 + k], (uint32_t)(hx_r16(a[k]) * 3u + index) | (uint32_t)((hx_r16(a[k] + 2) + 5u * k) << 16));
        return 0x1234;
    }
    return 0;
}

#define N(fn, n, ...) hx_native(fn, n, __VA_ARGS__)
int RotTransPers4(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* v3, long* s0, long* s1, long* s2, long* s3,
                  long* p, long* flag) {
    (void)p;
    (void)flag;
    return (int)N(F_RTP4, 8, hx_addr(v0), hx_addr(v1), hx_addr(v2), hx_addr(v3), hx_addr(s0), hx_addr(s1),
                   hx_addr(s2), hx_addr(s3));
}
void AddPrim(void* ot, void* p) {
    GfxEnvironment* env = (GfxEnvironment*)(uintptr_t)ENV;
    uintptr_t o = (uintptr_t)ot;
    uint32_t retail = (o >= (uintptr_t)&env->ot[0] && o < (uintptr_t)&env->ot[16])
                          ? ENV + 0x70u + (uint32_t)((unsigned long*)ot - env->ot) * 4u
                          : hx_addr(ot);
    N(F_ADDPRIM, 2, retail, hx_addr(p));
}
void func_801CE198(s32 count, SVECTOR* verts, POLY_FT4* polys, s32 rc) {
    N(F_STRIP, 4, (uint32_t)count, hx_addr(verts), hx_addr(polys), (uint32_t)rc);
}
extern void func_801CE860(void);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint8_t active[] = {0x00, 0x7F, 0x55, 0x2A, 0x41};
    unsigned pattern, preview, mask;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801CE860u, 0x801CEB5Cu);
    for (pattern = 0; pattern < 6; ++pattern)
    for (preview = 0; preview < 2; ++preview)
    for (mask = 0; mask < sizeof active; ++mask) {
        uint32_t i;
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.pGfxEnv = (GfxEnvironment*)(uintptr_t)ENV;
        *(u32*)&native_menu.unk358[4] = BUF;
        for (i = 0; i < 0x3400; ++i) hx_w8(BUF + i, (uint8_t)(i * 31 + pattern * 11 + (i >> 7)));
        for (i = 0; i < 7; ++i) {
            hx_w8(BUF + 0x32EA + i, (uint8_t)((active[mask] >> i) & 1 ? 1 + pattern : 0));
            hx_w8(BUF + 0x32E3 + i, (uint8_t)((i + pattern) & 1));
            hx_w8(BUF + 0x32DC + i, (uint8_t)((i + pattern + 1) & 1));
        }
        hx_w8(BUF + 0x32F2, (uint8_t)(preview ? 1 + pattern : 0));
        hx_mirror_out(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801CE860u, 0, 0, 0, 0, 2000000);
        hx_retail_done();
        func_801CE860();
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801CE860", NULL, 0);
    printf("MENU EQUIP ROW QUEUE PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
