#include "common.h"


#ifndef XENO_PC_PORT
/* func_800BEC18.s, byte-exact: update the two pads' button state in the
 * D_800C3EB0 block at +0x8C58 (held[2], press[2], rel[2], then a 4-entry
 * history).  Pad 0 takes ControllerGetButtonState(0): held, newly pressed
 * (~old & cur) and released (old & ~cur).  Pad 1 takes (1): newly pressed
 * (~old & cur), held, and, as in retail, a released mask computed from
 * pad 0's held state (rel[1] = held[0] & ~cur).  When pad 0's held state
 * differs from the newest history entry, the history shifts down one
 * entry and entry 0 takes pad 0's held/press/rel and D_800D30E4[0].
 * Codegen:
 * - cur is one u32 variable for both results, with the first narrowed by
 *   (u16): retail keeps both in a1 (andi for the first, a plain copy for
 *   the second).  ControllerGetButtonState is declared as defined (int).
 * - Every access goes through the BattlePadState view at base + 0x8C58.
 *   With *(u16*)(base + K) reads, the four new history values load before
 *   the third entry copy (13/70); a local pad pointer moves the base
 *   register (42/70). */
extern u8 D_800C3EB0[];
extern u16 D_800D30E4[];
extern int ControllerGetButtonState(int controllerIndex);
typedef struct BattlePadHist { u16 held; u16 press; u16 rel; u16 time; } BattlePadHist;
typedef struct BattlePadState {
    u16 held[2];
    u16 press[2];
    u16 rel[2];
    BattlePadHist hist[4];
} BattlePadState;

void func_800BEC18(void) {
    u8* base;
    u32 cur;
    u16 old;

    cur = (u16)ControllerGetButtonState(0);
    base = D_800C3EB0;
    old = ((BattlePadState*)(base + 0x8C58))->held[0];
    ((BattlePadState*)(base + 0x8C58))->held[0] = cur;
    ((BattlePadState*)(base + 0x8C58))->press[0] = ~old & cur;
    ((BattlePadState*)(base + 0x8C58))->rel[0] = old & ~cur;
    cur = ControllerGetButtonState(1);
    ((BattlePadState*)(base + 0x8C58))->press[1] = ~((BattlePadState*)(base + 0x8C58))->held[1] & cur;
    ((BattlePadState*)(base + 0x8C58))->held[1] = cur;
    ((BattlePadState*)(base + 0x8C58))->rel[1] = ((BattlePadState*)(base + 0x8C58))->held[0] & ~cur;
    if (((BattlePadState*)(base + 0x8C58))->held[0] != ((BattlePadState*)(base + 0x8C58))->hist[0].held) {
        ((BattlePadState*)(base + 0x8C58))->hist[3] = ((BattlePadState*)(base + 0x8C58))->hist[2];
        ((BattlePadState*)(base + 0x8C58))->hist[2] = ((BattlePadState*)(base + 0x8C58))->hist[1];
        ((BattlePadState*)(base + 0x8C58))->hist[1] = ((BattlePadState*)(base + 0x8C58))->hist[0];
        ((BattlePadState*)(base + 0x8C58))->hist[0].held = ((BattlePadState*)(base + 0x8C58))->held[0];
        ((BattlePadState*)(base + 0x8C58))->hist[0].press = ((BattlePadState*)(base + 0x8C58))->press[0];
        ((BattlePadState*)(base + 0x8C58))->hist[0].rel = ((BattlePadState*)(base + 0x8C58))->rel[0];
        ((BattlePadState*)(base + 0x8C58))->hist[0].time = D_800D30E4[0];
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


/* func_800BED30.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3E20[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3610[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2E54[];
#endif
#ifdef XENO_PC_PORT
#define BATTLE_C3610_RESET() (*(u32*)PSX_ADDR(0x800C3610) = 0)
#else
#define BATTLE_C3610_RESET() (D_800C3610[0] = 0)
#endif
void func_800BED30(void) {
    D_800C3E20[0] = 0;
    BATTLE_C3610_RESET();
    D_800D2E54[0] = 0;
}
#undef BATTLE_C3610_RESET
