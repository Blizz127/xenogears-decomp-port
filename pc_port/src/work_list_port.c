#include "common.h"
#include "psx_memory.h"
#include "work_list_callback.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*WorkListCallback_t)(void*);

extern void* HeapAlloc(u_int allocationSize, u_int allocationFlags);
extern u_int HeapFree(void* allocation);

__attribute__((weak)) int PcPort_BattleMipsDispatchCallback(
    u32 callback, void* argument)
{
    (void)callback;
    (void)argument;
    return 0;
}

static void WorkListInvokeCallback(WorkListCallback_t callback, void* argument)
{
    u32 address = (u32)(uintptr_t)callback;

    if ((address & 0xE0000000u) == 0x80000000u) {
        if (PcPort_BattleMipsDispatchCallback(address, argument))
            return;
        fprintf(stderr,
                "[xeno-port][work-list] unresolved guest callback 0x%08x\n",
                address);
        abort();
    }
    callback(argument);
}

void PcPort_WorkListInvokeSavedCallback(uint32_t callback, void* argument)
{
    WorkListInvokeCallback((WorkListCallback_t)(uintptr_t)callback, argument);
}

/* PSX-layout entry: 0x1C bytes, pointer fields stored as u32. The game embeds
 * these in heap objects at fixed offsets (AnimTask: task1 +0x0, task2 +0x1C,
 * SpriteData +0x38) and the update pumps below read them at raw 32-bit
 * offsets, so the port MUST NOT widen the fields to host pointers. All code
 * and data live below 4 GiB (-no-pie), so u32 round-trips host pointers. */
typedef struct WorkListEntry {
    u32 unk0;               /* owner WorkListEntry* */
    u32 unk4;               /* payload (SpriteData*) */
    u32 onTriggerCallback;  /* WorkListCallback_t */
    u32 onFreeCallback;     /* WorkListCallback_t */
    u32 unk10 : 29;
    u32 unk10_1 : 1;
    u32 unk10_2 : 1;
    u32 unk10_3 : 1;
    u32 unk14 : 29;
    u32 unk14_1 : 1;
    u32 unk14_2 : 1;
    u32 unk14_3 : 1;
    u32 pNext;              /* next WorkListEntry* */
} WorkListEntry;

_Static_assert(sizeof(WorkListEntry) == 0x1C,
               "WorkListEntry must retain the retail packed 0x1C layout");

/* Host-pointer <-> PSX u32 pointer helpers for the entry fields above. */
#define WL_PTR(x) ((WorkListEntry*)(uintptr_t)(x))
#define WL_U32(p) ((u32)(uintptr_t)(p))

/* KUSEG address zero aliases the first byte of PS1 RAM.  Battle deliberately
 * passes zero as the timer owner, and retail TimerWorkListAddTask reads its
 * task id from address 0x10.  Do that read against emulated RAM instead of
 * dereferencing a host null pointer. */
static u32 WorkListOwnerId(const WorkListEntry* owner) {
    if (owner == NULL)
        return (*(const u32*)(g_PsxRam + 0x10)) & 0x1FFFFFFFu;
    return owner->unk10;
}

extern s32 D_80059190;
extern s32 g_NumTimerWorkListEntries;
extern s32 g_NumWorkListEntries;
extern s32 g_WorkListCurTimer;
extern WorkListEntry* D_800594C0;
extern WorkListEntry* g_TimerWorkList;
extern WorkListEntry* D_80059590;
extern WorkListEntry* g_WorkList;
extern short D_80059494;
extern void func_8001DAE8(void* arg0, u16 arg1, u32 arg2);
extern void func_800234AC(void* pSpriteData);
extern void func_8001F8E8(void* pSpriteData, u16 frameIndex, u32 animPackageAddr);
extern u8 D_8005A474[];
extern u8 D_8006BE10[];

void func_8001D2A4(void) {
    D_80059190 = 0;
}

void WorkListsReset(void) {
    g_TimerWorkList = NULL;
    g_WorkList = NULL;
    g_NumTimerWorkListEntries = 0;
    g_NumWorkListEntries = 0;
    g_WorkListCurTimer = 0;
}

void TimerWorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    u8* pEntry;

    if (g_WorkListCurTimer) {
        g_WorkListCurTimer--;
        if (g_WorkListCurTimer == 0) {
            D_80059494 = 0;
        }
        return;
    }

    D_80059590 = g_TimerWorkList;
    while (D_80059590 != NULL) {
        pEntry = (u8*)D_80059590;
        D_800594C0 = (WorkListEntry*)pEntry;
        D_80059590 = (WorkListEntry*)(uintptr_t)*(u32*)(pEntry + 0x18);
        pFnCallback = (WorkListCallback_t)(uintptr_t)*(u32*)(pEntry + 0x8);
        if (pFnCallback) {
            WorkListInvokeCallback(pFnCallback, pEntry);
        }
    }
}

void WorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    u8* pEntry;

    D_80059590 = g_WorkList;
    while (D_80059590 != NULL) {
        pEntry = (u8*)D_80059590;
        D_800594C0 = (WorkListEntry*)pEntry;
        D_80059590 = (WorkListEntry*)(uintptr_t)*(u32*)(pEntry + 0x18);
        pFnCallback = (WorkListCallback_t)(uintptr_t)*(u32*)(pEntry + 0x8);
        if (pFnCallback) {
            WorkListInvokeCallback(pFnCallback, pEntry);
        }
    }
}

/* Retail 0x8001C8DC-0x8001C940: repeatedly invoke the +0x0C free
 * callback of each list head, reloading the head after every callback. The
 * callback owns unlinking the entry, so preserving that reload/dispatch order
 * is required. Both callback slots remain packed 32-bit addresses. */
void WorkListsFreeAllEntries(void) {
    WorkListCallback_t pFnCallback;
    WorkListEntry* pEntry;

    while ((pEntry = g_TimerWorkList) != NULL) {
        pFnCallback =
            (WorkListCallback_t)(uintptr_t)pEntry->onFreeCallback;
        WorkListInvokeCallback(pFnCallback, pEntry);
    }

    while ((pEntry = g_WorkList) != NULL) {
        pFnCallback =
            (WorkListCallback_t)(uintptr_t)pEntry->onFreeCallback;
        WorkListInvokeCallback(pFnCallback, pEntry);
    }
}

/* Unlink work-list / timer-list entries owned by pTargetEntry when the
 * shared-id bits match and the sticky bit (unk14_2) is clear. Matched C from
 * work_list.c (asm 8001CE74); needed by anim-script opcode 0x96. */
void func_8001CE74(WorkListEntry* pTargetEntry) {
    WorkListCallback_t pFnOnDeleteCallback;
    WorkListEntry* pCurEntry;
    WorkListEntry* pPrevEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL;
         pCurEntry = WL_PTR(pCurEntry->pNext)) {
        if (pCurEntry->unk0 == WL_U32(pTargetEntry) &&
            (pCurEntry->unk14_2 & 1) == 0 &&
            pCurEntry->unk14 == pTargetEntry->unk10) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = WL_PTR(pCurEntry->pNext);
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = WL_PTR(pCurEntry->pNext);
            }

            pFnOnDeleteCallback =
                (WorkListCallback_t)(uintptr_t)pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                WorkListInvokeCallback(pFnOnDeleteCallback, pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL;
         pCurEntry = WL_PTR(pCurEntry->pNext)) {
        if (pCurEntry->unk0 == WL_U32(pTargetEntry) &&
            (pCurEntry->unk14_2 & 1) == 0 &&
            (pCurEntry->unk14 & 0x1FFFFFFF) == pTargetEntry->unk10) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = WL_PTR(pCurEntry->pNext);
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = WL_PTR(pCurEntry->pNext);
            }

            pFnOnDeleteCallback =
                (WorkListCallback_t)(uintptr_t)pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                WorkListInvokeCallback(pFnOnDeleteCallback, pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }
}

void func_8001D298(void) {
    D_80059190 = 0;
}

void func_8001D2B0(void* pSpriteData, s16 frameIndex) {
    u8* pData = pSpriteData;

    if ((*(u32*)(pData + 0x3C) & 0x3) != 1) {
        *(s16*)(pData + 0x34) = 0;
        return;
    }

    if (((*(u32*)(pData + 0x40) >> 20) & 0x1) != 0) {
        u8* pBase = (u8*)(uintptr_t)*(u32*)(pData + 0x20);

        *(u32*)(pData + 0x40) &= ~0x100000;
        if (*(u32*)(pBase + 0x34) != 0) {
            func_800234AC(pData);
        }
    }

    if (((*(u32*)(pData + 0x40) >> 17) & 0x1) != 0 && D_80059190 != 0) {
        u8* pEntry;

        for (pEntry = (u8*)(uintptr_t)D_80059190; pEntry != NULL;
             pEntry = (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)*(u32*)(pEntry + 0x20) + 0x38)) {
            if (pEntry == pData) {
                u8* pPackage = (u8*)(uintptr_t)*(u32*)(pData + 0x24);

                if (pPackage != D_8005A474 && pPackage != D_8006BE10 &&
                    (((*(u32*)(pData + 0x40) >> 19) & 0x1) == 0)) {
                    func_8001F8E8(pData, *(u16*)(pData + 0x34), *(u32*)(pData + 0x24));
                }

                *(s16*)(pData + 0x34) = frameIndex;
                return;
            }
        }
    }

    *(s16*)(pData + 0x34) = frameIndex;
    *(u32*)(pData + 0x40) |= 0x20000;
    *(u32*)((u8*)(uintptr_t)*(u32*)(pData + 0x20) + 0x38) = (u32)D_80059190;
    D_80059190 = (s32)(uintptr_t)pData;
}

void func_8001D468(void) {
    u8* pEntry = (u8*)(uintptr_t)D_80059190;

    while (pEntry != NULL) {
        u16 value34 = *(u16*)(pEntry + 0x34);

        if (value34 == 0) {
            *(u32*)(pEntry + 0x40) &= 0xFFFFFF03;
        } else {
            /* XENO_PC_PORT: NPC skin-palette blocks load as a single variant in
             * the port (the block header count is 1, only palette[0] is valid).
             * But func_8001DAE8 (rendering.c:282, retail-faithful) uses
             * (pEntry+0x3E & 0xF0) -- the skin index that func_8002435C packs
             * into BOTH nibbles of pEntry+0x3E (temp1.c:394-395, asm-verified) --
             * as an in-block palette index: offset = idx*32+4. For an NPC whose
             * script skin index is >0 that reads past the one loaded palette into
             * adjacent heap, so townsfolk rendered cyan/purple/red garble (and a
             * red blob). Retail's block holds every variant so idx picks the
             * right one; until the port loads the full multi-variant block, clear
             * that in-block index so each NPC uses its own valid palette[0]. This
             * only touches the palette-offset nibble (bits 20-23); Fei/party and
             * the upload-trigger bits are unaffected. See ACTIVE_HANDOFF.md. */
            *(u32*)(pEntry + 0x3C) &= 0xFF0FFFFF;
            func_8001DAE8(pEntry, value34, *(u32*)(pEntry + 0x24));
        }

        pEntry = (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)*(u32*)(pEntry + 0x20) + 0x38);
    }

    D_80059190 = 0;
}

/* ---------------------------------------------------------------------------
 * Task registration / removal, needed by the anim-script opcode 0xE0 child-
 * sprite spawn chain (func_800233A4 in game_overrides.c). Matched C for the
 * Remove/Set functions comes from work_list.c (compiled out of the port
 * build); the Add functions are ported from asm 8001CA58 / 8001CC18.
 * --------------------------------------------------------------------------- */

/* .sbss task-id counter @0x80059184: every added task gets a unique 29-bit id
 * in unk10; children born of a task inherit the owner's id in unk14, which is
 * how func_8001CE74 (opcode 0x96) finds them again. */
s32 D_80059184;
/* .sbss @0x80059464: count of timer tasks carrying the unk14_3 flag. */
s32 D_80059464;
/* .sbss @0x800591AC: when set, newly added timer tasks get unk14_3 (retail
 * uses it as a battle-scope marker so those tasks can be culled together).
 * func_80023B84 temporarily clears it while spawning if the parent's +0xB0
 * bit 8 is set. */
u8 D_800591AC;
/* .sbss @0x800591AF: HeapAlloc flags used for AnimTask allocations. */
u8 D_800591AF;

void WorkListRemoveTask(WorkListEntry* pTargetEntry);
void TimerWorkListRemoveTask(WorkListEntry* pTargetEntry);

/* asm 8001CA58: prepend pEntry to g_WorkList. unk10 keeps its top 3 bits and
 * takes the global id counter; unk14 takes the owner's id with the top 3
 * flag bits cleared; the free callback defaults to WorkListRemoveTask. */
void WorkListAddTask(WorkListEntry* pOwner, WorkListEntry* pEntry) {
    pEntry->unk0 = WL_U32(pOwner);
    pEntry->pNext = WL_U32(g_WorkList);
    g_WorkList = pEntry;
    pEntry->unk10 = D_80059184;
    D_80059184++;
    pEntry->onTriggerCallback = 0;
    pEntry->onFreeCallback = WL_U32(WorkListRemoveTask);
    pEntry->unk14 = WorkListOwnerId(pOwner);
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk14_3 = 0;
    g_NumWorkListEntries++;
}

/* asm 8001CC18: timer-list variant. Same id wiring; additionally flags the
 * entry (unk14_3, counted in D_80059464) when D_800591AC is set. */
void TimerWorkListAddTask(WorkListEntry* pOwner, WorkListEntry* pEntry) {
    pEntry->unk0 = WL_U32(pOwner);
    pEntry->onFreeCallback = WL_U32(TimerWorkListRemoveTask);
    pEntry->onTriggerCallback = 0;
    pEntry->pNext = WL_U32(g_TimerWorkList);
    g_TimerWorkList = pEntry;
    pEntry->unk14 = WorkListOwnerId(pOwner);
    pEntry->unk10 = D_80059184;
    D_80059184 = (s32)((u32)D_80059184 + 1u); /* retail ADDIU wrap */
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk14_3 = 0;
    if (D_800591AC) {
        D_80059464++;
        pEntry->unk14_3 = 1;
    }
    g_NumTimerWorkListEntries++;
}

/* Matched C from work_list.c (asm 8001CB48). The trailing ++/-- pair keeps
 * the retail quirk: a remove of an entry not on the list leaves the counter
 * unchanged instead of underflowing. */
void WorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL;
         pCurEntry = WL_PTR(pCurEntry->pNext)) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = WL_PTR(pCurEntry->pNext);
            }
            if (D_80059590 == pTargetEntry) {
                D_80059590 = WL_PTR(pTargetEntry->pNext);
            }
            break;
        }
        pPrevEntry = pCurEntry;
    }

    if (pCurEntry == NULL) {
        g_NumWorkListEntries++;
    }
    g_NumWorkListEntries--;
}

/* Matched C from work_list.c (asm 8001CD94). */
void TimerWorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL;
         pCurEntry = WL_PTR(pCurEntry->pNext)) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = WL_PTR(pCurEntry->pNext);
            }
            if (D_80059590 == pTargetEntry) {
                D_80059590 = WL_PTR(pTargetEntry->pNext);
            }
            break;
        }
        pPrevEntry = pCurEntry;
    }

    if (pTargetEntry->unk14_3) {
        D_80059464--;
    }
    g_NumTimerWorkListEntries--;
}

/* Retail 8001CE44..8001CE74: unlink before releasing the same allocation. */
void TimerWorkListDeleteTask(WorkListEntry* pTask) {
    TimerWorkListRemoveTask(pTask);
    HeapFree(pTask);
}

/* Retail 8001CD08..8001CD64; the matching work_list.c TU is excluded from
 * the native build. Keep its 0x1C-byte header and 32-bit allocation arithmetic
 * here, alongside the packed Add/Remove owners used by field and battle. */
WorkListEntry* TimerWorkListAllocateTask(void* owner, s32 dataSize) {
    WorkListEntry* entry = HeapAlloc((u32)dataSize + 0x1Cu, D_800591AF);
    TimerWorkListAddTask(owner, entry);
    entry->onFreeCallback = WL_U32(TimerWorkListDeleteTask);
    entry->unk4 = 0;
    return entry;
}

void WorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = WL_U32(callback);
}

void TimerWorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = WL_U32(callback);
}

/* Retail 8001CD7C..8001CD88: one packed32-bit load, not a host-width field. */
WorkListCallback_t WorkListTaskGetTaskCallback(WorkListEntry* pTask) {
    return (WorkListCallback_t)(uintptr_t)pTask->onTriggerCallback;
}

void WorkListTaskSetOnFreeCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onFreeCallback = WL_U32(callback);
}

/* Retail 0x8001D19C: the timer/work pair shares one allocation. */
void WorkListsDeleteTasks(WorkListEntry* pTasks) {
    WorkListRemoveTask((WorkListEntry*)((u8*)pTasks + 0x1C));
    TimerWorkListRemoveTask(pTasks);
    HeapFree(pTasks);
}

/* Retail 0x8001D1D8-0x8001D298.  Contrary to the older four-argument C
 * placeholder, the first argument is the caller-selected allocation size and
 * the free callback is the fifth o32 stack argument.  The two list records
 * occupy the first 0x38 bytes; battle owners append their payload afterward. */
WorkListEntry* WorkListsAddTasks(u32 allocationSize,
                                 WorkListEntry* timerOwner,
                                 WorkListCallback_t timerCallback,
                                 WorkListCallback_t workCallback,
                                 WorkListCallback_t onFreeCallback) {
    WorkListEntry* timerEntry =
        (WorkListEntry*)HeapAlloc(allocationSize, D_800591AF);
    WorkListEntry* workEntry;

    TimerWorkListAddTask(timerOwner, timerEntry);
    workEntry = (WorkListEntry*)((u8*)timerEntry + 0x1C);
    WorkListAddTask(timerEntry, workEntry);
    TimerWorkListSetTaskCallback(timerEntry, timerCallback);
    WorkListSetTaskCallback(workEntry, workCallback);
    WorkListTaskSetOnFreeCallback(
        timerEntry,
        onFreeCallback != NULL
            ? onFreeCallback
            : (WorkListCallback_t)WorkListsDeleteTasks);
    timerEntry->unk4 = WL_U32(timerEntry);
    workEntry->unk4 = WL_U32(timerEntry);
    return timerEntry;
}

/* Matched C from work_list.c (asm 8001D034): zero the +0x70 parent link of
 * every timer task owned by pTargetEntry whose id matches and whose unk14_1
 * flag is set. Called from the AnimTask free callback (func_80022EB8) when
 * the dying sprite's +0xB0 bit 11 is set. */
void func_8001D034(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL;
         pCurEntry = WL_PTR(pCurEntry->pNext)) {
        if (pCurEntry->unk0 == WL_U32(pTargetEntry)) {
            if (pCurEntry->unk14 == pTargetEntry->unk10 &&
                (pCurEntry->unk14_1 & 1)) {
                *(u32*)((u8*)(uintptr_t)pCurEntry->unk4 + 0x70) = 0;
            }
        }
    }
}

/* Retail 8001D0A4..8001D108; packed-pointer adapter for work_list.c's
 * decompiled timer lookup. Callback values are identities, not data pointers.
 * The 29-bit ids exclude the three upper flag bits on both entries. */
void* func_8001D0A4(void* pTask, void* pCallback) {
    WorkListEntry* pCur;
    for (pCur = g_TimerWorkList; pCur != NULL; pCur = WL_PTR(pCur->pNext)) {
        if (pCur->unk0 == WL_U32(pTask) &&
            pCur->unk14 == WorkListOwnerId(pTask) &&
            pCur->onTriggerCallback == WL_U32(pCallback)) {
            return pCur;
        }
    }
    return NULL;
}

/* Retail 8001D164..8001D19C: callback identity lookup. The delay slot at
 * 8001D190 clears v0 when the list is exhausted, so a miss returns NULL,
 * not the last visited node. Task pointers retain their packed PSX width. */
void* func_8001D164(void* pCallback) {
    WorkListEntry* pCur;
    for (pCur = g_TimerWorkList; pCur != NULL; pCur = WL_PTR(pCur->pNext)) {
        if (pCur->onTriggerCallback == WL_U32(pCallback)) {
            return pCur;
        }
    }
    return NULL;
}

/* asm 8001D3F4: unlink a sprite from the D_80059190 pending-frame chain
 * (linked through *(sprite+0x20)+0x38, the same chain func_8001D2B0 pushes
 * onto and func_8001D468 drains). */
void func_8001D3F4(void* pTargetSprite) {
    u8* pCur = (u8*)(uintptr_t)D_80059190;
    u8* pPrev = NULL;

    while (pCur != NULL) {
        u8* pNext = (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)*(u32*)(pCur + 0x20) + 0x38);

        if (pCur == (u8*)pTargetSprite) {
            if (pPrev) {
                *(u32*)((u8*)(uintptr_t)*(u32*)(pPrev + 0x20) + 0x38) = (u32)(uintptr_t)pNext;
            } else {
                D_80059190 = (s32)(uintptr_t)pNext;
            }
        } else {
            pPrev = pCur;
        }
        pCur = pNext;
    }
}
