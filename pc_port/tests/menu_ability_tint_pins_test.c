/* Pins func_801DD5E8 (retail 0x801DD5E8): the 12-row loop tints both prims of
 * every non-empty row (code byte != 0x20) before the arrow/0xE00/0xF00 prims.
 * The port once skipped the row loop (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static u8 buf[0x1000];
static u8 arrow[0x100];
static uintptr_t calls[64]; static int ncalls;
static u8* MenuRawPointer(u32 o) { assert(o == 0x430); return buf; }
void func_801E8F60(s32 a, s32 b) { (void)a; (void)b; }
void func_801E8EAC(POLY_FT4* p, s32 m) { assert(m == 3); calls[ncalls++] = (uintptr_t)p; }
#include "tint.inc"
int main(void) {
    s32 i;
    g_Menu = &menu;
    menu.arrowCursors[0] = (MenuArrowCursor*)arrow;
    for (i = 0; i < 12; i++) {
        buf[i * 0x80 + 0x7D] = i & 1;                       /* current prim */
        buf[i * 0x80 + (i & 1) * 0x28 + 4] = (i == 5) ? 0x20 : 0x2C;  /* row 5 empty */
        buf[i * 0x80 + 0x77D] = 1;
    }
    func_801DD5E8(3);
    assert(ncalls == 11 * 2 + 1 + 1 + 2);
    assert(calls[0] == (uintptr_t)(buf + 0 * 0x80 + 0));
    assert(calls[1] == (uintptr_t)(buf + 0x700 + 0 * 0x80 + 0x28));
    assert(calls[2] == (uintptr_t)(buf + 1 * 0x80 + 0x28));
    printf("PASS menu ability tint pins (DD5E8 row loop)\n");
    return 0;
}
