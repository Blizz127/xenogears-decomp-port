/* Pins func_801C8324's done test (retail 0x801C8324): the 8.8 accumulator
 * is divided by 256, rounding toward zero.  The port once used >> 8, which
 * floors negative accumulators and ended the open animation a tick early
 * (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
#include "anim.inc"
int main(void) {
    MenuOpenAnim *s;
    g_Menu = &menu;
    s = MENU_OPEN_ANIM(1);
    memset(s, 0, sizeof(*s));
    s->curX = 100; s->toX = 100; s->stepX = 0x100; s->dirX = 1; s->speed = 1;
    s->accX = 0x80; /* one step: 0x80 - 0x100 = -0x80 */
    func_801C8324(1);
    assert(s->accX == -0x80 && s->done == 0); /* -0x80/256 == 0; >>8 gives -1 */
    s->accX = -0x80; s->speed = 1;             /* -> -0x180: /256 = -1 */
    func_801C8324(1);
    assert(s->done == 1);
    memset(s, 0, sizeof(*s));
    s->curY = 50; s->toY = 50; s->stepX = 0x80; s->stepY = 0x100; s->dirY = 1;
    s->speed = 1; s->accY = 0x80;
    func_801C8324(1);
    assert(s->accY == -0x80 && s->done == 0);
    printf("PASS menu open anim pins (C8324 acc / 256)\n");
    return 0;
}
