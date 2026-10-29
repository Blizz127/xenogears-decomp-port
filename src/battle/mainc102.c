#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
extern u32 D_800D39C8[];
extern s32 D_800595AC[];
extern void func_800BC404(u32 v);
extern void func_800BFC80(u32 a0, u32 a1, u32 a2);
extern void func_800245D8(u32 a0, u32 a1);
extern void func_800B39C0(s32 a0, s32 a1, s32 r, s32 g, s32 b);
extern void func_800BE790(void);
extern void func_800A9FF0(s32 idx);
extern void func_800BADD4(u32 i);
extern void func_800C0F70(void);
extern void func_800B8354(void);
extern void ArchiveSetIndex(s32 idx, s32 sub);
extern s32 ArchiveDecodeAlignedSize(s32 file);
extern void* HeapAlloc(s32 size, s32 mode);
extern void ArchiveReadFileToBuffer(s32 file, void* buf, s32 a2, s32 a3);
extern s32 SoundLoadWdsFile(void* buf, s32 a1);
extern s16 func_8003BDFC(s32 a0);
extern void HeapFree(u32 p);
extern void func_8003852C(u32 p);

typedef struct {
    u16 index;
    u16 unk2;
    void* buf;
} BattleFileEntry;
typedef struct {
    u16 file;
    s16 sound;
} BattleSoundBank;
extern BattleSoundBank D_800C35DC[];
extern void func_80029AFC(BattleFileEntry* list, s32 a1, s32 a2);
extern void SoundAddSedsEntry(void* seds);
extern void SoundFreeWdsEntry(s32 wds);
extern void func_80039E60(s32 id);
extern s32 func_8003A5D0(s32 id);

/* func_800B838C.s, byte-exact: play sound bank `bank`.  Load archive 0x2C
 * file D_800C35DC[bank].file (the SEDS) and, as the WDS, that file + 1 +
 * sub (bank 8 ignores sub) through a func_80029AFC list; register them
 * (SoundAddSedsEntry, SoundLoadWdsFile) and wait until func_8003BDFC(0)
 * is 0; start sound (SEDS +0x14 halfword << 16) + D_800C35DC[bank].sound
 * + sub with func_80039E60 and wait while func_8003A5D0 returns nonzero;
 * then release the SEDS (func_8003852C), the WDS entry and both buffers.
 * Codegen: the list has four entries (retail's frame), three used; the
 * WDS index is written as file + 1, then += sub (a single expression
 * folds the 1 into sub).  Matches under the CDK compiler only. */
void func_800B838C(s32 bank, s32 sub) {
    BattleFileEntry list[4];
    u8* seds;
    void* wdsBuf;
    s32 wds;
    s32 id;

    func_800C0F70();
    func_800B8354();
    ArchiveSetIndex(0x2C, 1);
    list[0].index = D_800C35DC[bank].file;
    seds = HeapAlloc(ArchiveDecodeAlignedSize(list[0].index), 0);
    list[0].buf = seds;
    if (bank == 8) {
        list[1].index = D_800C35DC[8].file + 1;
    } else {
        list[1].index = D_800C35DC[bank].file + 1;
        list[1].index += sub;
    }
    wdsBuf = HeapAlloc(ArchiveDecodeAlignedSize(list[1].index), 0);
    list[1].buf = wdsBuf;
    list[2].buf = NULL;
    list[2].index = 0;
    func_80029AFC(list, 0, 0);
    func_800B8354();
    SoundAddSedsEntry(seds);
    wds = SoundLoadWdsFile(wdsBuf, 0);
    while (func_8003BDFC(0) != 0) {
        func_800BE790();
    }
    id = D_800C35DC[bank].sound + sub + (*(u16*)(seds + 0x14) << 16);
    func_80039E60(id);
    while (func_8003A5D0(id) != 0) {
        func_800BE790();
    }
    func_8003852C((u32)seds);
    SoundFreeWdsEntry(wds);
    HeapFree((u32)wdsBuf);
    HeapFree((u32)seds);
}

/* Entry i of the pointer table at D_800C3EB0 + 0x8C8C.  An inline helper on
 * purpose: its copies make retail's loops big enough that loop optimisation
 * hoists only the 0x8C8C (and, in the smaller loop, 0x15) constants. */
static inline u8* B853C_Actor(s32 i) {
    u8* base = D_800C3EB0;

    return *(u8**)((u8*)(i * 4 + base) + 0x8C8C);
}

/* func_800B853C.s: mode 0 calls func_800BC404(-1), then func_800BFC80(e, 0, 2)
 * and func_800245D8(e, 0x18) for each of the three table entries e that is
 * set and whose +0xAF byte is not 0x15.  Mode 2 calls func_800BC404(0) and
 * func_800B39C0(0x28, 2, 0xFF, 0xFF, 0xFF), func_800245D8(e, 5) for the same
 * entries, then func_800BE790 40 times.  Every mode then runs func_800A9FF0
 * and func_800BADD4 for slots 3..10, loads archive 0x2C file 5 through
 * SoundLoadWdsFile into D_800595AC, calls func_800BE790 until
 * func_8003BDFC(0) is 0, frees both buffers and clears D_800C3EB0[0x8DA8]. */
void func_800B853C(s32 mode) {
    u8* base;
    s32 i;
    s32 n;
    void* buf;

    switch (mode) {
    case 0: {
        s32 j;
        u8* actor;

        func_800BC404(-1);
        for (j = 0; j != 3; j++) {
            actor = B853C_Actor(j);
            if (actor != NULL && (s8)actor[0xAF] != 0x15) {
                func_800BFC80((u32)actor, 0, 2);
                func_800245D8((u32)actor, 0x18);
            }
        }
        break;
    }
    case 1:
        break;
    case 2:
        func_800BC404(0);
        func_800B39C0(0x28, 2, 0xFF, 0xFF, 0xFF);
        for (i = 0; i != 3; i++) {
            u8* actor = B853C_Actor(i);

            if (actor != NULL && (s8)actor[0xAF] != 0x15) {
                func_800245D8((u32)actor, 5);
            }
        }
        for (i = 0x28; i > 0; i--) {
            func_800BE790();
        }
        break;
    }
    i = 3;
    n = 0xB;
    for (; i != n; i++) {
        func_800A9FF0(i);
        func_800BADD4(i);
    }
    func_800C0F70();
    func_800B8354();
    ArchiveSetIndex(0x2C, 0);
    buf = HeapAlloc(ArchiveDecodeAlignedSize(5), 1);
    ArchiveReadFileToBuffer(5, buf, 0, 0x80);
    func_800B8354();
    D_800595AC[0] = SoundLoadWdsFile(buf, 0);
    while (func_8003BDFC(0) != 0) {
        func_800BE790();
    }
    HeapFree((u32)buf);
    HeapFree(D_800D39C8[0]);
    base = (u8*)(u32)D_800C3EB0;
    *(base + 0x8000 + 0xDA8) = 0;
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


/* func_800B8774.s */
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 D_8005919C[];
extern u8 D_800591AD[];
#ifndef XENO_PC_PORT
extern u32 D_800D2D54[];
#endif
extern void func_800BE790(void);
extern void DrawSync(u32 v);
extern void func_800BADD4(u32 i);
extern void GfxFreeWorkBuffers(void);
extern void WorkListsFreeAllEntries(void);
extern void HeapFree(u32 p);
extern void func_800A9F94(void);
extern void func_800A4820(void);
void func_800B8774(void) {
    u8* base = (u8*)(u32)D_800C3EB0;
    u32 i;
    u32 n;

    if (*(u32*)(base + 0x8000 + 0xC84) == 0) {
        func_800BE790();
    }
    DrawSync(0);
    i = 0;
    n = 0xB;
    for (; i != n; i++) {
        func_800BADD4(i);
    }
    GfxFreeWorkBuffers();
    WorkListsFreeAllEntries();
    func_8003852C(D_8005919C[0]);
    HeapFree(D_8005919C[0]);
    D_800591AD[0] = 0;
    DrawSync(0);
    func_800A9F94();
    func_800A4820();
    HeapFree(D_800D2D54[0]);
}
/* func_800B8840.s */
extern u8 D_800591AD[];
extern u32 D_80059464[];
extern u8 D_800591AC[];
extern u32 D_800591A8[];
extern u32 D_80050104[];
extern void func_800BED30(void);
extern void func_800BE108(void);
extern void WorkListsReset(void);
extern void func_800BB7F8(void);
extern void GfxAllocateWorkBuffers(u32 a0, u32 a1);
extern void func_800BCD8C(void);
extern void func_800B7C28(void);
extern void func_800B89F4(void);
void func_800B8840(void) {
    D_800591AD[0] = 1;
    D_80059464[0] = 0;
    D_800591AC[0] = 0;
    D_800591A8[0] = 0x2000;
    func_800BED30();
    func_800BE108();
    WorkListsReset();
    func_800BB7F8();
    GfxAllocateWorkBuffers(0x5000, 0);
    func_800BCD8C();
    func_800B7C28();
    func_800B89F4();
    D_80050104[0] = 0;
}
