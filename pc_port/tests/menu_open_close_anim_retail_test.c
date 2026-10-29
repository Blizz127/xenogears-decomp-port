/* Retail differential test for func_801D29A8, the menu open/close animation
 * (shared retail/port body).  The retail MIPS from disc/menu.bin and the
 * shared body run on the same RAM snapshot; every callee call and all of RAM
 * must match.  The callees are stubs that only record their arguments, except
 * func_801C7BF4 (one frame): after K frames it sets one anim slot's done byte,
 * which ends the slide loop.  MenuManager has no pointers, so both worlds
 * share one copy in guest RAM.  The native func_801E8018 receives
 * &native_menu.unk6E0 and records it as its retail address.
 * See lib/menu_retail_harness.h. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"

#define G_MENU_PTR 0x800625A0u
#define MENU_BLOCK 0x80070000u
#define MANAGER 0x800A0000u

static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
#define NFIELDS (sizeof(menu_fields) / sizeof(menu_fields[0]))
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;

enum {
    F_E8018 = 0x801E8018u, F_81E0 = 0x801C81E0u, F_E8044 = 0x801E8044u, F_5A50 = 0x801D5A50u,
    F_7BF4 = 0x801C7BF4u, F_8324 = 0x801C8324u, F_8574 = 0x801C8574u, F_28A8 = 0x801D28A8u
};
const HxCallee hx_callees[] = {
    {F_E8018, 4, "func_801E8018"}, {F_81E0, 6, "func_801C81E0"}, {F_E8044, 2, "func_801E8044"},
    {F_5A50, 2, "func_801D5A50"}, {F_7BF4, 0, "func_801C7BF4"}, {F_8324, 1, "func_801C8324"},
    {F_8574, 1, "func_801C8574"}, {F_28A8, 0, "func_801D28A8"},
};
const unsigned hx_ncallees = sizeof(hx_callees) / sizeof(hx_callees[0]);

static unsigned frames[2], done_after, done_slot;

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    (void)a;
    (void)index;
    if (fn == F_7BF4 && ++frames[side] == done_after) {
        if (side == 0)
            hx_w8(MENU_BLOCK + done_slot * 0x24 + 0x23, 1);
        else
            native_menu.unk0[done_slot * 0x24 + 0x23] = 1;
    }
    return 0;
}
void func_801E8018(s32 count, void* strings, void* ids, void* flags) {
    uint32_t s = strings == (void*)native_menu.unk6E0 ? MENU_BLOCK + 0x6E0 : hx_addr(strings);
    hx_native(F_E8018, 4, (uint32_t)count, s, hx_addr(ids), hx_addr(flags));
}
void func_801C81E0(s32 fromX, s32 fromY, s32 toX, s32 toY, u8 speed, u8 slot) {
    hx_native(F_81E0, 6, (uint32_t)fromX, (uint32_t)fromY, (uint32_t)toX, (uint32_t)toY,
              (uint32_t)speed, (uint32_t)slot);
}
void func_801E8044(s32 count, void* flags) { hx_native(F_E8044, 2, (uint32_t)count, hx_addr(flags)); }
void func_801D5A50(u8 slot, u8 charId) { hx_native(F_5A50, 2, (uint32_t)slot, (uint32_t)charId); }
void func_801C7BF4(void) { hx_native(F_7BF4, 0); }
void func_801C8324(u8 slot) { hx_native(F_8324, 1, (uint32_t)slot); }
void func_801C8574(s32 sound) { hx_native(F_8574, 1, (uint32_t)sound); }
void func_801D28A8(void) { hx_native(F_28A8, 0); }
extern void func_801D29A8(u8 open, u8 noSettle);

int main(int argc, char** argv) {
    static const uint32_t skip[][2] = {{MENU_BLOCK, MENU_BLOCK + 0x1E98}};
    static const uint32_t opens[] = {0, 1, 0x100, 0x2FF};   /* u8: 0, 1, 0, 0xFF */
    static const uint32_t settles[] = {0, 1, 0x100};        /* u8: 0, 1, 0 */
    unsigned o, ns, party, k, pre;
    HX_REQUIRE(argc == 2, "usage: test disc/menu.bin");
    hx_map();
    hx_load(argv[1], 0x801C5000u, 0);
    hx_coverage(0x801D29A8u, 0x801D2D38u);
    for (o = 0; o < 4; ++o)
    for (ns = 0; ns < 3; ++ns)
    for (party = 0; party < 8; ++party)     /* which of the three slots hold a character */
    for (k = 1; k <= 3; ++k)                /* frames until a slot reports done */
    for (pre = 0; pre < 4; ++pre) {         /* slot already done at entry (1..3), or none */
        unsigned i;
        ++hx_case;
        memset(&native_menu, 0, sizeof native_menu);
        for (i = 0; i < sizeof native_menu.unk0; ++i) native_menu.unk0[i] = (u8)(i * 7 + hx_case);
        for (i = 0; i < 3; ++i) native_menu.unk0[i * 0x24 + 0x23] = (u8)(pre == i + 1);
        native_menu.pManager = (MenuManager*)(uintptr_t)MANAGER;
        for (i = 0; i < sizeof(MenuManager); ++i) hx_w8(MANAGER + i, (uint8_t)(i * 13 + party + 1));
        for (i = 0; i < 3; ++i) hx_w8(MANAGER + 0x30 + i, (party >> i) & 1 ? (uint8_t)(i * 3 + k) : 0xFF);
        frames[0] = frames[1] = 0;
        done_after = k;
        done_slot = (party + k) % 3;
        hx_mirror_out(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
        hx_w32(G_MENU_PTR, MENU_BLOCK);
        hx_begin_case();
        hx_run_retail(0x801D29A8u, opens[o], settles[ns], 0, 0, 2000000);
        hx_retail_done();
        func_801D29A8((u8)opens[o], (u8)settles[ns]);
        hx_compare_events();
        hx_compare_ram(skip, 1);
        hx_mirror_compare(menu_fields, NFIELDS, &native_menu, MENU_BLOCK);
    }
    hx_coverage_report("func_801D29A8", NULL, 0);
    printf("MENU OPEN CLOSE ANIM PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
