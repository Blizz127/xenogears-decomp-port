/* Pins func_801E5924 (retail 0x801E5924): x/y come from 4-byte-stride tables;
 * each LINE_F3 stores halfword vertices; the second line's middle vertex is
 * (x, y + 16).  The port once used a 2-byte stride, byte stores and x + 16
 * there (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static u8 buf[0x158];
u16 D_801E9894[8] = {1, 2, 0x123, 4, 5, 6, 7, 8};
u16 D_801E9914[8] = {9, 10, 0x045, 12, 13, 14, 15, 16};
static int verts;
static u8* MenuRawPointer(u32 o) { assert(o == 0x3A8 + 1 * 4); return buf; }
void SetLineF3(LINE_F3* p) { (void)p; }
void func_801C851C(SVECTOR* v, s32 x, s32 y, s32 w, s32 h) {
    assert(x == 0x123 && y == 0x45 && w == 0x10 && h == 0x10); (void)v; verts++;
}
#include "brackets.inc"
static u16 H(u32 o) { u16 v; memcpy(&v, buf + o, 2); return v; }
int main(void) {
    s32 k;
    g_Menu = &menu;
    func_801E5924(1);
    assert(verts == 4);
    for (k = 0; k < 2; k++) {
        u32 a = 0x50 + k * 0x18, b = 0x80 + k * 0x18;
        assert(buf[a + 4] == 0 && buf[a + 5] == 0xFF && buf[a + 6] == 0);
        assert(H(a + 8) == 0x123 && H(a + 0xA) == 0x45);
        assert(H(a + 0xC) == 0x133 && H(a + 0xE) == 0x45);
        assert(H(a + 0x10) == 0x133 && H(a + 0x12) == 0x55);
        assert(H(b + 8) == 0x123 && H(b + 0xA) == 0x45);
        assert(H(b + 0xC) == 0x123 && H(b + 0xE) == 0x55);
        assert(H(b + 0x10) == 0x133 && H(b + 0x12) == 0x55);
    }
    printf("PASS menu title bracket pins (E5924 stride, halfwords, x2)\n");
    return 0;
}
