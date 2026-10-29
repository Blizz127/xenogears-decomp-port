/* Host port body of the battle file's sprite setup.  The byte-exact C is
 * retail-only in mainc100.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include <stdlib.h>
#include "battle_host_guest.h"

/* func_80022224's by-value (x, y) pairs (texture page, CLUT). */
typedef struct {
    s16 x;
    s16 y;
} B7Pair;

extern u8 D_8005A474[];
extern void TimerWorkListAddTask(void* owner, void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);
extern void func_80022224(void* pkg, void* file, B7Pair tex, B7Pair clut, u32 arg4);
extern void func_800BEB04(void);
extern void func_800B8354(void);
extern int func_8001EE68(u8* word);
extern void* func_80023B84(void* sprite, void* script, void* pkg);
extern void SpriteSetSpecialAnimFile(void* sprite, void* file);
extern void func_800245D8(void* sprite, s16 animIndex);
extern u32 func_800C0FAC(u8* base);
extern void func_8003A89C(void* manager, s32 level, s32 steps);

/* Retail keeps the temporary timer task in its stack frame; a work-list
 * entry's links are packed 32-bit words, which a host stack address cannot
 * hold, so the port takes the task from a small static pool indexed by call
 * depth (one frame per nested entry, as the stack would give).  Both
 * func_800B8354 and func_800BEB04 wait through func_800BE790, which can run
 * other tasks while this one sits on the timer list; a deeper nesting than
 * the pool aborts rather than reuse a listed entry. */
enum { kB7E94PoolDepth = 4 };
static u8 s_b7e94_tasks[kB7E94PoolDepth][0x20] __attribute__((aligned(4)));
static unsigned s_b7e94_depth;

/* func_800B7E94.s: set up the sprite for the battle file func_800B7C34
 * loaded (D_800594F0) under a temporary timer task: func_80022224 with the
 * D_8005A474 package, the file, texture page (0x380, 0x100) and CLUT
 * (0, 0x1F4); then func_800BEB04.  When func_8001EE68(package word 0) is
 * set, spawn script D_800C3DF0 of the package (table at +0x10, halfword
 * offsets from +2) on the current sprite (D_800C3E1C) and return 0;
 * otherwise give the current sprite the file as its special animation file,
 * func_800245D8(sprite, -1) and return 1.  Both the new and the current
 * sprite get func_800C0FAC's result at +0x50; then raise D_800D3350 and
 * D_800C35D4 and run func_8003A89C(D_800C3E54, 0x60, 0x78).  Retail also
 * copies 20 bytes from the pointer at sprite +0x24 into its frame and never
 * reads them; that dead copy is omitted.  The sprites are guest words;
 * D_800594F0 / D_800C3E54 are the native file buffer / audio manager words
 * and go through bg_arg. */
u32 func_800B7E94(void) {
    B7Pair tex;
    B7Pair clut;
    u32 sprite;
    u32 s;
    u32 table;
    u32 result;
    u32 v;
    u8* task;

    if (s_b7e94_depth >= kB7E94PoolDepth) {
        fprintf(stderr, "[xeno-port] func_800B7E94 nested deeper than %d frames\n", kB7E94PoolDepth);
        abort();
    }
    task = s_b7e94_tasks[s_b7e94_depth++];
    sprite = BG_U32(0x800C3E1C); /* D_800C3E1C */
    TimerWorkListAddTask(NULL, task);
    *(u32*)(task + 8) = 0;
    func_800B8354();
    tex.x = 0x380;
    tex.y = 0x100;
    clut.x = 0;
    clut.y = 0x1F4;
    BG_U8(0x800C3624) = 0; /* D_800C3624 */
    func_80022224(D_8005A474, bg_arg(BG_U32(0x800594F0)), tex, clut, 0); /* D_800594F0 */
    result = 0;
    func_800BEB04();
    if (func_8001EE68((u8*)(uintptr_t)*(u32*)D_8005A474) != 0) {
        table = *(u32*)(D_8005A474 + 0x10);
        s = BG_ADDR(func_80023B84(
            bg_arg(BG_U32(0x800C3E1C)),
            bg_arg((u32)BG_U16(table + (u32)(s32)BG_S16(0x800C3DF0) * 2 + 2) + table), /* D_800C3DF0 */
            D_8005A474));
    } else {
        result = 1;
        s = BG_U32(0x800C3E1C);
        SpriteSetSpecialAnimFile(bg_arg(s), bg_arg(BG_U32(0x800594F0)));
        func_800245D8(bg_arg(s), -1);
    }
    v = func_800C0FAC((u8*)bg_arg(BG_U32(0x800594F0)));
    BG_U32(s + 0x50) = v;
    BG_U32(sprite + 0x50) = v;
    BG_U8(0x800D3350) = 1; /* D_800D3350 */
    BG_U8(0x800C35D4) = 1; /* D_800C35D4 */
    func_8003A89C(bg_arg(BG_U32(0x800C3E54)), 0x60, 0x78); /* D_800C3E54 */
    TimerWorkListRemoveTask(task);
    s_b7e94_depth--;
    return result;
}
#endif
