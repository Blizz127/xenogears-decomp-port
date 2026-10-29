/* Retail differential test for func_801CDC6C, the HP / MaxHP / MP / MaxMP
 * digit rows of a portrait panel (shared retail/port body).  The retail MIPS
 * from disc/menu.bin and the shared body run on the same RAM snapshot; every
 * callee call and all of RAM must match.  func_801C80B8 is a stub that writes
 * g_Menu->digits as retail does (nine MSD-first decimal digits, leading zeros
 * blanked to 0xFF) into each world's own SystemMenu.  func_8002675C records
 * its seven arguments and returns 0 or 1 (a function of them), which keeps
 * the row lengths inside the panel's prim arrays.  MenuCharacter and
 * GameState have no pointers: both worlds share the panel, the x/y tables and
 * g_GameState (bound to its retail address) in guest RAM.
 * See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "menu_retail_harness.h"

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define PANEL 0x800A0000u
#define XTABLE 0x800B0000u
#define YTABLE 0x800B0100u
#define ATLAS 0x800C0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
#define NFIELDS (sizeof(menu_fields) / sizeof(menu_fields[0]))
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum { F_80B8 = 0x801C80B8u, F_675C = 0x8002675Cu };
const HxCallee hx_callees[] = {{F_80B8, 1, "func_801C80B8"}, {F_675C, 7, "func_8002675C"}};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    (void)index;
    if (fn == F_80B8) {
        uint8_t d[9];
        uint32_t value = a[0], divisor = 100000000u;
        unsigned i;
        for (i = 0; i < 9; i++) {
            d[i] = (uint8_t)(value / divisor);
            value %= divisor;
            divisor /= 10;
        }
        for (i = 1; i < 9; i++) {
            if (d[i] != 0) {
                if (d[i - 1] == 0) d[i - 1] = 0xFF;
                break;
            }
            d[i - 1] = 0xFF;
        }
        for (i = 0; i < 9; i++) {
            if (side == 0)
                hx_w8(MENU_BLOCK + 0x31C + i, d[i]);
            else
                native_menu.digits[i] = d[i];
        }
        return 0;
    }
    if (fn == F_675C) return (a[1] + a[4] + a[5]) & 1;
    return 0;
}
s32 func_8002675C(u8* atlas, s32 glyph, void* polys, s32 rc, s32 x, s32 y, s32 scale) {
    return (s32)hx_native(F_675C, 7, hx_addr(atlas), (uint32_t)glyph, hx_addr(polys), (uint32_t)rc,
                          (uint32_t)x, (uint32_t)y, (uint32_t)scale);
}
void func_801C80B8(u32 value) { hx_native(F_80B8, 1, value); }
extern void func_801CDC6C(MenuCharacter* panel, u8 charId, u8 slot, s32* xTable, s32* yTable,
                          u8 mode);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint32_t modes[] = {0, 1, 2, 0x100};       /* u8: 0x100 -> 0 */
    static const uint32_t slots[] = {0, 1, 2, 0x103};       /* u8: 0x103 -> 3 */
    static const uint8_t chars[] = {0, 3, 7};
    static const u16 hp[] = {0, 7, 42, 999, 1234};          /* > 999 drops a digit */
    static const u16 mp[] = {0, 9, 10, 99, 100};
    static const u32 gear_hp[] = {0, 5, 12345, 99999, 123456};
    unsigned m, sl, c, rc, v;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801CDC6Cu, 0x801CE0CCu);
    for (m = 0; m < 4; ++m)
    for (sl = 0; sl < 4; ++sl)
    for (c = 0; c < 3; ++c)
    for (rc = 0; rc < 2; ++rc)
    for (v = 0; v < 5; ++v) {
        unsigned i;
        u8 ch = chars[c], gear = (u8)((ch + v) & 7);
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        native_menu.unk2DC = (void*)(uintptr_t)ATLAS;
        native_menu.renderContext = (s32)rc;
        for (i = 0; i < 9; ++i) native_menu.digits[i] = (u8)(0xA0 + i);
        for (i = 0; i < sizeof(MenuCharacter); ++i) hx_w8(PANEL + i, (uint8_t)(i * 37u + hx_case));
        for (i = 0; i < 16; ++i) {
            hx_w32(XTABLE + i * 4, (uint32_t)(s32)(i * 0x11 + v * 3 - 0x40));
            hx_w32(YTABLE + i * 4, (uint32_t)(s32)(i * 0x0D + c * 5 - 0x20));
        }
        g_GameState.characters[ch].hp = hp[v];
        g_GameState.characters[ch].maxHp = hp[(v + 2) % 5];
        g_GameState.characters[ch].mp = mp[v];
        g_GameState.characters[ch].maxMp = mp[(v + 3) % 5];
        g_GameState.characters[ch].gearId = gear;
        g_GameState.gears[gear].hp = gear_hp[v];
        g_GameState.gears[gear].maxHp = gear_hp[(v + 1) % 5];
        hx_mirror_out(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_w32(HX_STACK_TOP + 0x10, YTABLE);      /* retail stack args 5 and 6 */
        hx_w32(HX_STACK_TOP + 0x14, modes[m]);
        hx_begin_case();
        hx_run_retail(0x801CDC6Cu, PANEL, ch, slots[sl], XTABLE, 4000000);
        hx_retail_done();
        func_801CDC6C((MenuCharacter*)(uintptr_t)PANEL, ch, (u8)slots[sl],
                      (s32*)(uintptr_t)XTABLE, (s32*)(uintptr_t)YTABLE, (u8)modes[m]);
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801CDC6C", NULL, 0);
    printf("MENU PANEL DIGITS PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
