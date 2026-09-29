/* Pins the g_GameState party-record stride (0xA4) used by the shared menu
 * bodies func_801E4258 and func_801E42AC; the port once stepped 0x28, i.e.
 * the wrong character's record for any index > 0
 * (docs/port/RETAIL_DIVERGENCES.md).  Bodies are extracted by the runner. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "main/game.h"
#include "system/menu.h"
GameState g_GameState;
#include "records.inc"
static u8 *rec(int idx) { return (u8 *)&g_GameState + 0x978 + idx * 0xA4; }
int main(void) {
    /* The retail ctx arg is an s32 holding a pointer: keep it below 4 GiB. */
    u8 *low = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(low != MAP_FAILED && (uintptr_t)low < 0x80000000u);
    u8 *ctx = low, *tbl10 = low + 0x1000, *tbl0c = low + 0x2000;
    /* ctx+0x0C and ctx+0x10 are adjacent 4-byte retail pointer slots, so
     * each host pointer is stored just before the call that reads it. */
    memset(&g_GameState, 0, sizeof g_GameState);
    /* func_801E4258: record 2, table row 3 -> rec[0x70] = row[8], rec[0x72] = row[0xA] */
    rec(2)[8] = 3;
    *(u16 *)(tbl10 + 3 * 0x14 + 8) = 0x1234;
    *(u16 *)(tbl10 + 3 * 0x14 + 0xA) = 0x5678;
    *(u8 **)(ctx + 0x10) = tbl10;   /* 4258 table: 0x14-byte rows */
    func_801E4258(ctx, 2);
    assert(*(u16 *)(rec(2) + 0x70) == 0x1234 && *(u16 *)(rec(2) + 0x72) == 0x5678);
    assert(*(u16 *)(rec(0) + 0x70) == 0 && *(u16 *)((u8 *)&g_GameState + 0x978 + 2 * 0x28 + 0x70) == 0);
    /* func_801E42AC: record 1, table row 2; rec[0x3F] written even when not lowering */
    rec(1)[3] = 2;
    *(u16 *)(rec(1) + 0x38) = 5;
    *(u16 *)(tbl0c + 2 * 0x10 + 6) = 9;
    tbl0c[2 * 0x10 + 0xC] = 0x11; tbl0c[2 * 0x10 + 0xD] = 0x22; tbl0c[2 * 0x10 + 0xE] = 0x33;
    *(u8 **)(ctx + 0x0C) = tbl0c;   /* 42AC table: 0x10-byte rows */
    func_801E42AC((s32)(uintptr_t)ctx, 1);
    assert(*(u16 *)(rec(1) + 0x3A) == 9 && rec(1)[0x3C] == 0x11 && rec(1)[0x3D] == 0x22 && rec(1)[0x3E] == 0x33);
    assert(rec(1)[0x3F] == 0x33 && *(u16 *)(rec(1) + 0x38) == 5);
    /* lowering case */
    *(u16 *)(tbl0c + 2 * 0x10 + 6) = 4;
    func_801E42AC((s32)(uintptr_t)ctx, 1);
    assert(*(u16 *)(rec(1) + 0x38) == 4);
    /* func_801E4998: ((val / 10) * 2) / 9 rounded down to a multiple of 10,
     * val = record-set word at g_GameState + 0x9DC + idx * 0xA4 */
    {
        u8 *model = low + 0x3000;
        *(u32 *)((u8 *)&g_GameState + 0x9DC + 1 * 0xA4) = 1000; /* 1000/10*2/9 = 22 -> 20 */
        *(u8 **)(ctx + 0x4C + 1 * 4) = model;
        func_801E4998((s32)(uintptr_t)ctx, 1);
        assert(*(u16 *)(model + 0x5C8 + 0x24) == 20);
        *(u32 *)((u8 *)&g_GameState + 0x9DC + 1 * 0xA4) = 4000; /* 4000/10*2/9 = 88 -> 80 */
        func_801E4998((s32)(uintptr_t)ctx, 1);
        assert(*(u16 *)(model + 0x5C8 + 0x24) == 80);
    }
    /* func_801E35BC: stat record (0x26C + stat * 0xA4) raised by
     * char[0x5B] * slot[0x11], slot table indexed by charIdx, capped at stat[0x4E] */
    {
        u8 *base = (u8 *)&g_GameState + 0x26C, *slots = low + 0x4000;
        base[2 * 0xA4 + 0x5B] = 3;                     /* char 2 */
        *(u16 *)(base + 1 * 0xA4 + 0x4C) = 10;           /* stat record 1 */
        *(u16 *)(base + 1 * 0xA4 + 0x4E) = 100;
        slots[4 * 0x28 + 0x11] = 5;                      /* slot 4 */
        *(u8 **)(ctx + 0x20 + 2 * 4) = slots;
        func_801E35BC(ctx, 2, 1, 4, 0);
        assert(*(u16 *)(base + 1 * 0xA4 + 0x4C) == 25);
        *(u16 *)(base + 1 * 0xA4 + 0x4E) = 30;
        func_801E35BC(ctx, 2, 1, 4, 0);
        assert(*(u16 *)(base + 1 * 0xA4 + 0x4C) == 30);
        /* reverse: base record = 0x978 + gs[0x30C + char * 0xA4] * 0xA4 */
        ((u8 *)&g_GameState)[0x30C + 2 * 0xA4] = 3;
        *(u32 *)((u8 *)&g_GameState + 0x978 + 3 * 0xA4 + 0x64) = 50;
        *(u32 *)((u8 *)&g_GameState + 0x978 + 3 * 0xA4 + 0x60) = 20;
        func_801E35BC(ctx, 2, 1, 4, 1);
        assert(*(u32 *)((u8 *)&g_GameState + 0x978 + 3 * 0xA4 + 0x60) == 25);
    }
    /* func_801E41C0: record 3, table row 1 (0x18-byte rows at *(ctx + 8));
     * rec[0x9F] written even when rec[0x60] is not lowered */
    {
        u8 *tbl08 = low + 0x5000;
        rec(3)[2] = 1;
        *(u32 *)(tbl08 + 1 * 0x18 + 4) = 777;
        tbl08[1 * 0x18 + 0x14] = 1; tbl08[1 * 0x18 + 0x15] = 2;
        tbl08[1 * 0x18 + 0x16] = 3; tbl08[1 * 0x18 + 0x17] = 4;
        *(u8 **)(ctx + 8) = tbl08;
        func_801E41C0((s32)(uintptr_t)ctx, 3);
        assert(*(u32 *)(rec(3) + 0x60) == 777 && *(u32 *)(rec(3) + 0x64) == 777);
        assert(rec(3)[0x98] == 1 && rec(3)[0x9E] == 2 && rec(3)[0x9D] == 3 && rec(3)[0x9F] == 4);
    }
    printf("PASS menu party-record pins (E4258, E42AC, E41C0, E4998, E35BC)\n");
    return 0;
}
