#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
extern u16 D_800C3668[];
extern u8 D_800C3CEC[];
extern u8 D_800D2FDC[];
extern u8* D_800594F0[];
extern u8 D_800C362C[];
extern u32 D_800C3E1C[];
/* Scalar (BattleCdkG0 splits its address): its stores must not order the
 * actor-slot loads around them (see the gears.toml note). */
extern u16 D_800C3666;
extern void* D_800594BC[];
extern u8* D_800C3618[];
extern void func_800B8D7C(void);
extern void ArchiveSetIndex(s32 idx, s32 sub);
extern s32 ArchiveDecodeAlignedSize(s32 index);
extern void* HeapAlloc(u32 size, u32 flags);
extern s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_800B8354(void);
extern void func_800BC404(u32 v);
extern void func_800BB080(s32 slot);
extern void* ArchiveAllocStreamFile(s32 a0, s32 a1);
extern s32 func_80029EB0(s32 index, void* stream, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7,
                         s32 a8, s32 a9);
extern u32 HeapFree(void* p);

/* The actor's 4-bit slot: bits 30-31 of +0xA8 and 0-1 of +0xAC (low part
 * first, as func_800B905C's B905C_SLOT). */
typedef struct {
    u8 unk0[0xA8];
    u32 unkA8;
    u32 unkAC;
} B7C34Actor;
#define B7C34_SLOT(p) \
    ({ u32 lo_ = ((B7C34Actor*)(p))->unkA8 >> 30; ((((B7C34Actor*)(p))->unkAC & 3) << 2) | lo_; })

/* Load battle file pair `id` (entries id * 2 + 0x22 and + 0x23 after
 * ArchiveSetIndex(0xC, 2)).  For id 0xE3 first MoveImage the three 64x256
 * VRAM blocks at the D_800C3668 (x, y) pairs to (0x280/0x240/0x200, 0x100).
 * Read the first file into a new heap buffer (D_800594F0) and run
 * func_800B8354; bits 12-13 of the halfword at buffer + (the word at
 * buffer + 4) give a mode: when nonzero store it in D_800C362C and call
 * func_800BC404(0), then
 * mode 1 runs func_800BB080(-1) and clears D_800C3666, any other mode clears
 * the current actor's (D_800C3E1C) slot bit there and runs func_800BB080 on
 * the slot.  Then stream the second file through func_80029EB0 (stream file
 * 8, D_800594BC) when its aligned size is above 0x10, run func_800B8354
 * again, keep the buffer in D_800C3618 and free the stream. */
void func_800B7C34(s32 id) {
    RECT r;
    s32 mode;
    u8* actor;
    s32 file;
    s32 file2;

    if (id == 0xE3) {
        r.x = D_800C3668[0];
        r.y = D_800C3668[1];
        r.w = 0x40;
        r.h = 0x100;
        MoveImage(&r, 0x280, 0x100);
        r.x = D_800C3668[2];
        r.y = D_800C3668[3];
        r.w = 0x40;
        r.h = 0x100;
        MoveImage(&r, 0x240, 0x100);
        r.x = D_800C3668[4];
        r.y = D_800C3668[5];
        r.w = 0x40;
        r.h = 0x100;
        MoveImage(&r, 0x200, 0x100);
        DrawSync(0);
    }
    func_800B8D7C();
    ArchiveSetIndex(0xC, 2);
    file = id * 2 + 0x22;
    file2 = id * 2 + 0x23;
    D_800C3CEC[0] = 1;
    D_800D2FDC[0] = 1;
    D_800594F0[0] = HeapAlloc(ArchiveDecodeAlignedSize(file), 0);
    ArchiveReadFileToBuffer(file, D_800594F0[0], 0, 0x80);
    func_800B8354();
    /* offset first: as `buffer + offset` the two loads swap registers */
    mode = (*(u16*)(*(s32*)(D_800594F0[0] + 4) + (s32)D_800594F0[0]) >> 12) & 3;
    if (mode != 0) {
        D_800C362C[0] = mode;
        actor = (u8*)D_800C3E1C[0];
        func_800BC404(0);
        if (mode == 1) {
            func_800BB080(-1);
            D_800C3666 = 0;
        } else {
            D_800C3666 &= ~(1 << B7C34_SLOT(actor));
            func_800BB080(B7C34_SLOT(actor));
        }
    }
    D_800594BC[0] = ArchiveAllocStreamFile(8, 0);
    if (ArchiveDecodeAlignedSize(file2) >= 0x11) {
        func_80029EB0(file2, D_800594BC[0], 0, 0, 0, 0, 0, 0, 0, 0);
    }
    func_800B8354();
    DrawSync(0);
    D_800C3618[0] = D_800594F0[0];
    HeapFree(D_800594BC[0]);
}
/* func_80022224's by-value (x, y) pairs (texture page, CLUT). */
typedef struct {
    s16 x;
    s16 y;
} B7Pair;
typedef struct {
    u32 w[5];
} B7Blk20;
typedef struct {
    u8 unk0[0x50];
    u32 unk50;
} B7E94Sprite;
extern u32 D_8005A474[];
extern u8 D_800C3624[];
extern s16 D_800C3DF0[];
/* A scalar (split %hi at -G0) against the struct-view +0x50 stores below,
 * so its load can be scheduled between them and the flag stores, as in
 * retail; as an array it stays behind the stores. */
extern u32 D_800C3E54;
extern u8 D_800D3350[];
extern u8 D_800C35D4[];
extern void TimerWorkListAddTask(u32 owner, void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);
extern void func_80022224(void* pkg, void* file, B7Pair tex, B7Pair clut, u32 arg4);
extern void func_800BEB04(void);
extern s32 func_8001EE68(u32 a0);
extern void* func_80023B84(void* sprite, void* script, void* pkg);
extern void SpriteSetSpecialAnimFile(void* sprite, void* file);
extern void func_800245D8(u32 a0, u32 a1);
extern u32 func_800C0FAC(void* base);
extern void func_8003A89C(u32 a0, s32 a1, s32 a2);

/* Set up the sprite for the battle file func_800B7C34 loaded (D_800594F0)
 * under a temporary timer task: func_80022224 with the D_8005A474 package,
 * the file, texture page (0x380, 0x100) and CLUT (0, 0x1F4); then
 * func_800BEB04.  When func_8001EE68(package word 0) is set, spawn script
 * D_800C3DF0 of the package (table at +0x10, halfword offsets from +2) on
 * the current sprite (D_800C3E1C) and return 0; otherwise give the current
 * sprite the file as its special animation file, func_800245D8(sprite, -1)
 * and return 1.  Both the new and the current sprite get func_800C0FAC's
 * result at +0x50; then raise D_800D3350 and D_800C35D4 and run
 * func_8003A89C(D_800C3E54, 0x60, 0x78). */
u8 func_800B7E94(void) {
    B7Pair tex;
    B7Pair clut;
    u8 task[0x20];
    B7Blk20 saved;
    u8* sprite;
    u8* s;
    u8* table;
    u8 result;
    u32 v;

    sprite = (u8*)D_800C3E1C[0];
    TimerWorkListAddTask(0, task);
    *(u32*)(task + 8) = 0;
    func_800B8354();
    tex.x = 0x380;
    tex.y = 0x100;
    clut.x = 0;
    clut.y = 0x1F4;
    D_800C3624[0] = 0;
    func_80022224(D_8005A474, (void*)D_800594F0[0], tex, clut, 0);
    result = 0;
    func_800BEB04();
    if (func_8001EE68(D_8005A474[0]) != 0) {
        /* retail copies these 20 bytes and never reads them */
        saved = **(B7Blk20**)(D_800C3E1C[0] + 0x24);
        table = *(u8**)((u8*)D_8005A474 + 0x10);
        s = func_80023B84((void*)D_800C3E1C[0],
                          (u8*)(*(u16*)(table + D_800C3DF0[0] * 2 + 2) + (u32)table), D_8005A474);
    } else {
        result = 1;
        s = (u8*)D_800C3E1C[0];
        SpriteSetSpecialAnimFile(s, (void*)D_800594F0[0]);
        func_800245D8((u32)s, -1);
    }
    v = func_800C0FAC((void*)D_800594F0[0]);
    ((B7E94Sprite*)s)->unk50 = v;
    ((B7E94Sprite*)sprite)->unk50 = v;
    D_800D3350[0] = 1;
    D_800C35D4[0] = 1;
    func_8003A89C(D_800C3E54, 0x60, 0x78);
    TimerWorkListRemoveTask(task);
    return result;
}
#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


/* func_800B8048.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3E1C[];
#endif
void func_800B8048(u32 v) {
    D_800C3E1C[0] = v;
}
/* func_800B8054.s */
extern u16 D_800591B4[];
extern u8 D_800591B1[];
void func_800B8054(u16 v) {
    D_800591B4[0] = v;
    D_800591B1[0] = 0;
}
/* func_800B8068.s */
extern u8 D_800591B1[];
#ifndef XENO_PC_PORT
/* Retail passes its id (a0, which it never touches) on to func_800B7C34. */
void func_800B8068(s32 id) {
    func_800B7C34(id);
    func_800B7E94();
    D_800591B1[0] = 1;
}
#else
extern void func_800B7C34(void);
extern void func_800B7E94(void);
void func_800B8068(void) {
    func_800B7C34();
    func_800B7E94();
    D_800591B1[0] = 1;
}
#endif
