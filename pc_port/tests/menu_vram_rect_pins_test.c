/* Pins the VRAM rectangles of two menu.bin helpers that the port once had
 * shifted (docs/port/RETAIL_DIVERGENCES.md):
 *   func_801C6D90  LoadImage  RECT {0, 0x1C0, 0x10, 1}  (16-entry CLUT row)
 *   func_801E64E0  ClearImage RECT {0x140, 0xE0, 0x40, 0x20}, colour 0,0,0
 * The bodies are extracted from src/menu/main/misc.c by the runner. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static MenuManager manager;
static u16 allocation[0x10];
static unsigned step;
void *HeapAlloc(u_int size, u_int flags) { assert(step++ == 0 && size == 0x20 && flags == 0); return allocation; }
static void clear_buffer(void *p, size_t n) { assert(step++ == 1 && p == allocation && n == 0x20); memset(p, 0, n); }
int LoadImage(RECT *r, u_long *p) {
    assert(step++ == 2 && (void *)p == allocation);
    assert(r->x == 0 && r->y == 0x1C0 && r->w == 0x10 && r->h == 1);
    assert(allocation[0] == 0 && allocation[1] == 0x7FFF && allocation[2] == 0);
    return 0;
}
int DrawSync(int mode) { assert(step++ == 3 && mode == 0); return 0; }
u_int HeapFree(void *p) { assert(step++ == 4 && p == allocation); return 0; }
static unsigned cleared;
int ClearImage(RECT *r, u_char red, u_char green, u_char blue) {
    assert(r->x == 0x140 && r->y == 0xE0 && r->w == 0x40 && r->h == 0x20);
    assert(red == 0 && green == 0 && blue == 0);
    cleared++;
    return 0;
}
#define bzero clear_buffer
#include "rects.inc"
#undef bzero
int main(void) {
    g_Menu = &menu;
    menu.pManager = &manager;
    func_801C6D90();
    assert(step == 5);
    ((u8 *)&manager)[0xB] = 1;
    func_801E64E0();
    assert(cleared == 1 && ((u8 *)&manager)[0xB] == 0);
    printf("PASS menu VRAM rect pins (C6D90 CLUT row, E64E0 clear)\n");
    return 0;
}
