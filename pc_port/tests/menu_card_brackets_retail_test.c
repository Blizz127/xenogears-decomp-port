/* Retail differential test for the port body of func_801CFB48, the card-slot
 * corner brackets.  The matching build keeps the retail asm.  The retail MIPS
 * from disc/menu.bin and the native body run on the same RAM snapshot; every
 * callee call (RotTransPers3, AddPrim) and all of RAM must match.
 * The card block (g_Menu->unk32C) has different retail and host layouts
 * (MENU_CARD_OFF).  The retail run gets a retail-layout block, the native body
 * a host-layout block with the same logical values; the retail image's
 * unk32C slot is restored before the struct compare.  RotTransPers3 is
 * compared on its first 6 arguments (the last two point at stack locals), and
 * the native AddPrim target is recorded as its retail address.
 * See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"
#include <stddef.h>

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define CARD_R 0x800A0000u
#define CARD_H 0x800B0000u
#define SLOTS 0x800C0000u
#define ENV 0x800E0000u
#define HOST_CARD_OFF(o) ((o) >= 0xB94 ? (o) + (uint32_t)(offsetof(MenuUnk2, unkB94) - 0xB94) : (o))

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum { F_RTP3 = 0x8004A67Cu, F_ADDPRIM = 0x80043B48u };
const HxCallee hx_callees[] = {{F_RTP3, 6, "RotTransPers3"}, {F_ADDPRIM, 2, "AddPrim"}};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    unsigned k;
    (void)side;
    if (fn == F_RTP3) {
        for (k = 0; k < 3; ++k)
            hx_w32(a[3 + k], (uint32_t)(hx_r16(a[k]) * 5u + index) | (uint32_t)((hx_r16(a[k] + 2) + 3u * k) << 16));
        return 0x55;
    }
    return 0;
}
#define N(fn, n, ...) hx_native(fn, n, __VA_ARGS__)
int RotTransPers3(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, long* s0, long* s1, long* s2, long* p, long* flag) {
    (void)p;
    (void)flag;
    return (int)N(F_RTP3, 6, hx_addr(v0), hx_addr(v1), hx_addr(v2), hx_addr(s0), hx_addr(s1), hx_addr(s2));
}
void AddPrim(void* ot, void* p) {
    GfxEnvironment* env = (GfxEnvironment*)(uintptr_t)ENV;
    uintptr_t o = (uintptr_t)ot;
    uint32_t retail = (o >= (uintptr_t)&env->ot[0] && o < (uintptr_t)&env->ot[16])
                          ? ENV + 0x70u + (uint32_t)((unsigned long*)ot - env->ot) * 4u
                          : hx_addr(ot);
    N(F_ADDPRIM, 2, retail, hx_addr(p));
}
extern void func_801CFB48(void);

static void card_w8(uint32_t off, uint8_t v) { hx_w8(CARD_R + off, v); hx_w8(CARD_H + HOST_CARD_OFF(off), v); }
static void card_w32(uint32_t off, uint32_t v) { hx_w32(CARD_R + off, v); hx_w32(CARD_H + HOST_CARD_OFF(off), v); }

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    unsigned pattern, state, rc, present, sel;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801CFB48u, 0x801CFF64u);
    for (pattern = 0; pattern < 3; ++pattern)
    for (state = 0; state < 3; ++state)
    for (rc = 0; rc < 2; ++rc)
    for (present = 0; present < 4; ++present)
    for (sel = 0; sel < 4; ++sel) {
        uint32_t i;
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.renderContext = (int)rc;
        native_menu.unk32C = (MenuUnk2*)(uintptr_t)CARD_H;
        native_menu.pGfxEnv = (GfxEnvironment*)(uintptr_t)ENV;
        native_menu.unk4CC[0xC] = (u8)state;
        for (i = 0; i < 32; ++i) *(u32*)&native_menu.unk39C[0xC + i * 4] = SLOTS + i * 0x200u;
        for (i = 0; i < 32 * 0x200u; ++i) hx_w8(SLOTS + i, (uint8_t)(i * 41 + pattern * 17 + (i >> 9)));
        card_w32(0x4F7C, sel);
        for (i = 0; i < 32; ++i)   /* entry i; 0xFF = unused; some equal to the selected one */
            card_w8(0x4FAE + i, (uint8_t)(((i + pattern) % 5 == 0) ? 0xFF : ((i * 7 + pattern) % 6)));
        card_w8(0x4FE4, (uint8_t)(present & 1 ? 1 + pattern : 0));
        card_w8(0x4FE5, (uint8_t)(present & 2 ? 2 : 0));
        hx_mirror_out(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
        hx_w32(MENU_BLOCK + 0x32C, CARD_R);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801CFB48u, 0, 0, 0, 0, 4000000);
        hx_retail_done();
        func_801CFB48();
        hx_compare_events();
        hx_compare_ram(skip, 1);
        memcpy(hx_retail_after + (MENU_BLOCK + 0x32C - HX_BASE), &(uint32_t){CARD_H}, 4);
        hx_mirror_compare(menu_fields, sizeof(menu_fields) / sizeof(menu_fields[0]), &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801CFB48", NULL, 0);
    printf("MENU CARD BRACKETS PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
