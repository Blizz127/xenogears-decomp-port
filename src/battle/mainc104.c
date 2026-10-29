#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    u8 unk0[0x24];
    s32* unk24;
    u8 unk28[0x80];
    u32 unkA8 : 28;
    u32 mode : 2;
    u32 slot : 4 __attribute__((packed));
    u32 unkAC : 22;
    u32 unkAF : 8;
} B89Spr;

extern u8* D_800C3610[];
extern u16 D_800C3608[];
extern u8 D_800C3624[];
extern u16 D_800C3DF0[];
extern u8 D_800C3620[];
extern u8 D_800C3622[];
extern u32 D_800C3618[];
extern s32 D_800C361C[];
extern s32 D_800C360C[];
extern s32 D_800C3A6C[];
extern u8 D_800C3EB0__r[][28] asm("D_800C3EB0");
extern void func_800BC3F8(s32 a0);
extern void func_800BE790(void);
extern u8* func_800BED4C();
extern u8* func_800BEFF4(s32 slot);
extern void func_800BF3E8(u8* spr);
extern void func_800C0F70(void);
extern void func_800BC454(s32 mask);
extern s32 func_8001EE68(s32 a0);
extern void func_800BF2B8(B89Spr* spr);
extern void func_800B8354(void);
extern void ArchiveSetIndex(s32 index, s32 a1);
extern s32 ArchiveDecodeAlignedSize(s32 file);
extern s32 HeapAlloc(s32 size, s32 mode);
extern void ArchiveReadFileToBuffer(s32 file, s32 buf, s32 a2, s32 a3);
extern s32 SoundLoadWdsFile(s32 buf, s32 a1);
extern s32 func_8003BDFC(s32 a0);
extern void HeapFree(u32 p);
void func_800B8D04(void);
extern void func_800BFBA0(void);
extern void func_800BF0B4(u32 value);
extern void func_800BEE2C(s32 slot, s32 arg, s32 mode);
extern void func_80021BF8(B89Spr* spr, void* cb);
void func_800B9B30();
extern void func_800BF0C4(B89Spr* spr);

/* func_800B89FC.s, byte-exact: open the battle command window for
 * combatant `index` (main12's turn start calls it with resume 1).  Asserts
 * that no command task exists (break 1 spin), clears the slot's D_800C3608
 * bit, creates the task (func_800BED4C; +0x40 target, +0x20 index) and
 * the slot's sprite, then either resets the menu state and, for a
 * combatant whose sprite has no motion (func_8001EE68), loads sound bank
 * 7 once (D_800C3622), or (row +8 set) runs the scripted-step setup.
 * resume 0 starts the step's func_800BEE2C or the task callback;
 * otherwise func_800BF0B4(9 or 4).  The sprite slot is one 4-bit field
 * straddling the +0xA8/+0xAC words (packed), as retail reads it. */
void func_800B89FC(s32 resume, s32 index, s32 column, s32 target) {
    B89Spr* spr;
    u8* task;
    s32 buf;

    func_800BC3F8(0);
    if (D_800C3610[0] != 0) {
        for (;;) {
            __asm__ volatile("break 1024");
        }
    }
    func_800BE790();
    func_800BE790();
    D_800C3624[0] = 0;
    D_800C3608[0] &= ~(1 << index);
    task = func_800BED4C();
    D_800C3610[0] = task;
    *(s32*)(task + 0x40) = target;
    *(s32*)(task + 0x20) = index;
    func_800BF3E8(func_800BEFF4(index));
    D_800C3DF0[0] = 0;
    spr = *(B89Spr**)(D_800C3610[0] + 4);
    if (D_800C3EB0__r[index][8] == 0) {
        func_800C0F70();
        D_800C3620[0] = 0;
        func_800BC454(0xC0);
        if (func_8001EE68(*spr->unk24) == 0) {
            D_800C3622[0] = 0;
            if (D_800C3618[0] == 0 || spr->slot != D_800C361C[0]) {
                func_800BF2B8(spr);
            }
        } else {
            if (D_800C3622[0] == 0) {
                func_800B8354();
                ArchiveSetIndex(0x2C, 0);
                buf = HeapAlloc(ArchiveDecodeAlignedSize(7), 0);
                ArchiveReadFileToBuffer(7, buf, 0, 0x80);
                func_800B8354();
                func_800C0F70();
                D_800C3A6C[0] = SoundLoadWdsFile(buf, 0);
                while ((s16)func_8003BDFC(0) != 0) {
                    func_800BE790();
                }
                HeapFree(buf);
            }
            D_800C3622[0] = 1;
        }
    } else {
        func_800B8D04();
        func_800BFBA0();
        func_800BF0B4(8);
        func_800BEE2C(index, column, 0x1A);
        func_800BC454(0xC0);
    }
    if (resume == 0) {
        if (D_800C3EB0__r[index][8] != 0) {
            u8* steps = (u8*)D_800C3EB0__r;

            func_800BEE2C(index, *(u16*)(steps + D_800C360C[0] * 0x48 + 0x14E), 2);
        } else {
            func_80021BF8(spr, func_800B9B30);
            func_800BF0C4(spr);
        }
    } else {
        func_800BF0B4(D_800C3EB0__r[index][8] != 0 ? 9 : 4);
    }
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


/* func_800B8D04.s */
extern u32 D_80059464[];
#ifndef XENO_PC_PORT
extern u32 D_800C3618[];
#endif
extern void func_800B8354(void);
extern void func_800BF9EC(void);
extern u32 func_800BF720(void);
extern void func_800BE790(void);
extern void func_800BF3A4(void);
extern void HeapFree(u32 p);
void func_800B8D04(void) {
    func_800B8354();
    func_800BF9EC();
    while (D_80059464[0] != func_800BF720()) {
        func_800BE790();
    }
    func_800BF3A4();
    if (D_800C3618[0] != 0) {
        HeapFree(D_800C3618[0]);
        D_800C3618[0] = 0;
    }
}
