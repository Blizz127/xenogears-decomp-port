/* Native-layout regression for retail func_801DA4A8's call sequence.
 * Real production structures and extracted production body; helper calls are
 * controlled boundaries. Does not prove CD, text rendering, or heap internals. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
static SystemMenu menus[4], expected[4];
static MenuManager manager;
static ItemMenuWork allocation;
SystemMenu *g_Menu;
u8 D_801EA548[8];
static unsigned event, pattern;
static int replace_menu;
static void func_801D22F4(int n) {
    assert(event++ == 0 && n == 2);
    if (replace_menu) g_Menu = &menus[1];
}
static void func_801E8018(int n, void *strings, void *table, void *fourth) {
    assert(event++ == 1 && n == 8);
    assert(strings == g_Menu->itemMenuStrings);
    assert(table == D_801EA548);
    assert(fourth == manager.unk38);
    if (replace_menu) g_Menu = &menus[2];
}
void *HeapAlloc(u32 size, u32 flags) {
    assert(event++ == 2 && size == sizeof(ItemMenuWork) && flags == 0);
    if (replace_menu) g_Menu = &menus[3];
    memset(&allocation, pattern, sizeof allocation);
    return &allocation;
}
static void test_zero(void *p, size_t n) {
    assert(event++ == 3 && p == &allocation && n == sizeof allocation);
    assert(g_Menu->unk42C[0] == (uintptr_t)&allocation);
    memset(p, 0, n);
}
static void func_801C72BC(int mode) {
    assert(event++ == 4 && mode == 0);
    assert(g_Menu->unk42C[0] == (uintptr_t)&allocation);
    for (size_t i = 0; i < sizeof allocation; ++i)
        assert(((u8*)&allocation)[i] == 0);
}
#define bzero test_zero
#include "production.inc"
#undef bzero
int main(void) {
    assert((uintptr_t)&allocation <= UINT32_MAX);
    for (replace_menu = 0; replace_menu != 2; ++replace_menu) {
        for (pattern = 0; pattern < 256; ++pattern) {
            memset(menus, pattern, sizeof menus);
            for (int i = 0; i < 4; ++i) menus[i].pManager = &manager;
            memcpy(expected, menus, sizeof menus);
            expected[replace_menu ? 3 : 0].unk42C[0] = (u32)(uintptr_t)&allocation;
            g_Menu = &menus[0]; event = 0;
            func_801DA4A8();
            assert(event == 5);
            assert(memcmp(menus, expected, sizeof menus) == 0);
        }
    }
    printf("PASS 512 Items initializer layout/call-order cases; work=%zu bytes\n", sizeof allocation);
}
