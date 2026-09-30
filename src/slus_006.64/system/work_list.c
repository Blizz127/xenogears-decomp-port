#include "common.h"
#include "system/memory.h"

#ifdef XENO_PC_PORT
#include <stdint.h>
#endif

/* A lot of this TU has been matched already, but due to SBSS/SDATA symbols
 * issues it's currently unable to be compiled in. */

// TU is compiled on GCC 2.7.2 or 2.6.0

typedef void (*WorkListCallback_t)(void*);

typedef struct {
    struct WorkListEntry* unk0; // 
    void* unk4; // pSpriteData, at least in some cases. Could be a more general pointer to data
    WorkListCallback_t onTriggerCallback;
    WorkListCallback_t onFreeCallback;

    // Flags, shrug
    u32 unk10: 29;
    u32 unk10_1: 1;
    u32 unk10_2: 1;
    u32 unk10_3 : 1;

    u32 unk14: 29;
    u32 unk14_1: 1;
    u32 unk14_2: 1;
    u32 unk14_3 : 1;

    struct WorkListEntry* pNext;
} WorkListEntry;


/* Retail SBSS globals.  Their definitions are kept in the extracted BSS
 * segment; declaring the corresponding commons here lets the -G8 compiler
 * emit the retail gp-relative accesses. */
s32 D_80059184;
int g_NumTimerWorkListEntries;
int g_NumWorkListEntries;
WorkListEntry* D_800594C0;
WorkListEntry* g_TimerWorkList;
WorkListEntry* D_80059590;
WorkListEntry* g_WorkList;


void WorkListsFreeAllEntries(void) {
    WorkListEntry* pList;

    for (pList = g_TimerWorkList; g_TimerWorkList != NULL; pList = g_TimerWorkList) {
        pList->onFreeCallback(pList);
    }

    for (pList = g_WorkList; g_WorkList != NULL; pList = g_WorkList) {
        pList->onFreeCallback(pList);
    }
}

extern s32 g_NumTimerWorkListEntries;
extern s32 g_NumWorkListEntries;
extern s32 g_WorkListCurTimer;
extern WorkListEntry* D_800594C0;
extern WorkListEntry* g_TimerWorkList;
extern WorkListEntry* D_80059590;
extern WorkListEntry* g_WorkList;
extern short D_80059494;

void WorkListsReset(void) {
    g_TimerWorkList = NULL;
    g_WorkList = NULL;
    g_NumTimerWorkListEntries = 0;
    g_NumWorkListEntries = 0;
    g_WorkListCurTimer = 0;
}
/*
void WorkListsReset(void) {
    g_TimerWorkList = NULL;
    g_WorkList = NULL;
    g_NumTimerWorkListEntries = 0;
    g_NumWorkListEntries = 0;
    g_WorkListCurTimer = 0;
}
*/

void TimerWorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    u8* pEntry;
    WorkListEntry* pNext;

    if (g_WorkListCurTimer) {
        g_WorkListCurTimer--;
        if (g_WorkListCurTimer == 0) {
            D_80059494 = 0;
        }
    } else {
        D_80059590 = g_TimerWorkList;
        while (D_80059590 != NULL) {
            /* Retail reads the link and the callback before publishing the
             * cursor globals. */
            pEntry = (u8*)D_80059590;
            pNext = (WorkListEntry*)(uintptr_t)*(u32*)(pEntry + 0x18);
            pFnCallback = (WorkListCallback_t)(uintptr_t)*(u32*)(pEntry + 0x8);
            D_800594C0 = (WorkListEntry*)pEntry;
            D_80059590 = pNext;
            if (pFnCallback) {
                pFnCallback(pEntry);
            }
        }
    }
}
/*
void TimerWorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    WorkListEntry* pEntry;

    // Count down timer
    if (g_WorkListCurTimer) {
        g_WorkListCurTimer--;
        if (g_WorkListCurTimer == 0) {
            D_80059494 = 0;
        }
    
    // Timer has reached zero
    } else {
        D_80059590 = g_TimerWorkList;
        while (D_80059590) {
            pEntry = D_80059590;
            D_800594C0 = pEntry;
            D_80059590 = pEntry->pNext;
            pFnCallback = pEntry->onTriggerCallback;
            if (pFnCallback) {
                pFnCallback(pEntry);
            }
        }  
    }
}
*/

void WorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    u8* pEntry;
    WorkListEntry* pNext;

    D_80059590 = g_WorkList;
    while (D_80059590 != NULL) {
        pEntry = (u8*)D_80059590;
        pNext = (WorkListEntry*)(uintptr_t)*(u32*)(pEntry + 0x18);
        pFnCallback = (WorkListCallback_t)(uintptr_t)*(u32*)(pEntry + 0x8);
        D_800594C0 = (WorkListEntry*)pEntry;
        D_80059590 = pNext;
        if (pFnCallback) {
            pFnCallback(pEntry);
        }
    }
}
/*
void WorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    WorkListEntry* pEntry;

    D_80059590 = g_WorkList;
    while (D_80059590) {
        pEntry = D_80059590;
        D_800594C0 = pEntry;
        D_80059590 = pEntry->pNext;
        pFnCallback = pEntry->onTriggerCallback; 
        if (pFnCallback) {
            pFnCallback(pEntry);
        }
    }
}
*/

extern s32 D_80059184;
extern void WorkListRemoveTask(WorkListEntry* pEntry);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", WorkListAddTask);
#else
void WorkListAddTask(void* data, WorkListEntry* pEntry) {
    u32 old14 = *(u32*)((u8*)pEntry + 0x14);
    u32 old10 = *(u32*)((u8*)pEntry + 0x10);
    s32 timer = D_80059184;
    u32 taskFlags = *(u32*)((u8*)data + 0x10);

    pEntry->unk0 = (WorkListEntry*)data;
    pEntry->pNext = g_WorkList;
    g_WorkList = pEntry;

    /* Set entry+0x10: merge timer counter into low 29 bits */
    *(u32*)((u8*)pEntry + 0x10) = (old10 & 0xE0000000) | (timer & 0x1FFFFFFF);

    /* Set entry+0x14: merge data task flags, clear bits 29-31 */
    old14 = (old14 & 0xE0000000) | (taskFlags & 0x1FFFFFFF);
    old14 &= 0xDFFFFFFF;
    old14 &= 0xBFFFFFFF;
    old14 &= 0x7FFFFFFF;
    *(u32*)((u8*)pEntry + 0x14) = old14;

    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = WorkListRemoveTask;
    D_80059184 = timer + 1;
    g_NumWorkListEntries++;
}
#endif

extern u8 D_800591AF;
void WorkListDeleteTask(WorkListEntry* pTask);

WorkListEntry* WorkListAllocateTask(void* data, int dataSize) {
    WorkListEntry* pEntry;

    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), D_800591AF);
    WorkListAddTask(data, pEntry);
    pEntry->onFreeCallback = WorkListDeleteTask;
    return pEntry;
}

void WorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = pCurEntry->pNext;
            }
            
            if (D_80059590 == pTargetEntry) {
                D_80059590 = pTargetEntry->pNext;
            }

            break;
        }

        pPrevEntry = pCurEntry;
    } 
    
    // Target entry was not found
    if (pCurEntry == NULL) {
        g_NumWorkListEntries++;
    }
    
    g_NumWorkListEntries--;
}

void WorkListDeleteTask(WorkListEntry* pTask) {
    WorkListRemoveTask(pTask);
    HeapFree(pTask);
}

extern void TimerWorkListRemoveTask(WorkListEntry* pEntry);
extern u8 D_800591AC;
extern s32 D_80059464;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", TimerWorkListAddTask);
#else
void TimerWorkListAddTask(void* data, WorkListEntry* pEntry) {
    u32 old14, old10, taskFlags;
    s32 timer;
    u32* pTimerList;
    u8 isMainList;

    pEntry->unk0 = (WorkListEntry*)data;
    taskFlags = *(u32*)((u8*)data + 0x10);
    pTimerList = (u32*)g_TimerWorkList;
    pEntry->onFreeCallback = TimerWorkListRemoveTask;
    pEntry->onTriggerCallback = NULL;
    g_TimerWorkList = pEntry;

    old14 = *(u32*)((u8*)pEntry + 0x14);
    old10 = *(u32*)((u8*)pEntry + 0x10);
    timer = D_80059184;

    *(u32*)((u8*)pEntry + 0x14) = (old14 & 0xE0000000) | (taskFlags & 0x1FFFFFFF);
    old10 = (old10 & 0xE0000000) | (timer & 0x1FFFFFFF);
    *(u32*)((u8*)pEntry + 0x10) = old10;
    *(u32*)((u8*)pEntry + 0x18) = (u32)pTimerList;
    D_80059184 = timer + 1;

    isMainList = D_800591AC;
    if (isMainList) {
        D_80059464++;
        *(u32*)((u8*)pEntry + 0x14) |= 0x80000000;
    } else {
        *(u32*)((u8*)pEntry + 0x14) &= 0x7FFFFFFF;
    }
    g_NumTimerWorkListEntries++;
}
#endif

extern u8 D_800591AF;
void TimerWorkListDeleteTask(WorkListEntry* pTask);

WorkListEntry* TimerWorkListAllocateTask(void* data, int dataSize) {
    WorkListEntry* pEntry;
    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), D_800591AF);
    TimerWorkListAddTask(data, pEntry);
    pEntry->onFreeCallback = &TimerWorkListDeleteTask;
    pEntry->unk4 = 0;
    return pEntry;
}

void WorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = callback;
}

void TimerWorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = callback;
}

void WorkListTaskSetOnFreeCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onFreeCallback = callback;
}

WorkListCallback_t WorkListTaskGetTaskCallback(WorkListEntry* pTask) {
    return pTask->onTriggerCallback;
}

WorkListCallback_t WorkListTaskGetOnFreeCallback(WorkListEntry* pTask) {
    return pTask->onFreeCallback;
}

void TimerWorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = pCurEntry->pNext;
            }
            
            if (D_80059590 == pTargetEntry) {
                D_80059590 = pTargetEntry->pNext;
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

void TimerWorkListDeleteTask(WorkListEntry* pTask) {
    TimerWorkListRemoveTask(pTask);
    HeapFree(pTask);
}

// Unlink target entry from lists if certain flags are met
void func_8001CE74(WorkListEntry* pTargetEntry) {
    WorkListCallback_t pFnOnDeleteCallback;
    WorkListEntry* pCurEntry;
    WorkListEntry* pPrevEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (
            pCurEntry->unk0 == pTargetEntry && 
            (pCurEntry->unk14_2 & 1) == 0 && 
            pCurEntry->unk14 == pTargetEntry->unk10
        ) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = pCurEntry->pNext;
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = pCurEntry->pNext;
            }
            
            pFnOnDeleteCallback = pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                pFnOnDeleteCallback(pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (
            pCurEntry->unk0 == pTargetEntry && 
            (pCurEntry->unk14_2 & 1) == 0 && 
            (pCurEntry->unk14 & 0x1FFFFFFF) == pTargetEntry->unk10
        ) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = pCurEntry->pNext;
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = pCurEntry->pNext;
            }
            
            pFnOnDeleteCallback = pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                pFnOnDeleteCallback(pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }
}

// Set pCurEntry->unk4->unk70 of target entry if unk14_1 flag is set
void func_8001D034(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->unk0 == pTargetEntry) {
            if (
                pCurEntry->unk14 == pTargetEntry->unk10 &&
                (pCurEntry->unk14_1 & 1)
            ) {
                *(u32*)(pCurEntry->unk4 + 0x70) = 0;
            }
        }   
    }
}

void* func_8001D0A4(void* pTask, void* pCallback) {
    void* pCur = g_TimerWorkList;
    while (pCur != NULL) {
        if (*(void**)pCur == pTask) {
            u32 curFlags = *(u32*)((u8*)pCur + 0x14) & 0x1FFFFFFF;
            u32 taskFlags = *(u32*)((u8*)pTask + 0x10) & 0x1FFFFFFF;
            if (curFlags == taskFlags) {
                if (*(void**)((u8*)pCur + 0x08) == pCallback) return pCur;
            }
        }
        pCur = *(void**)((u8*)pCur + 0x18);
    }
    return NULL;
}

void* func_8001D10C(void* pTask) {
    void* pCur = g_TimerWorkList;
    while (pCur != NULL) {
        if (*(void**)pCur == pTask) {
            u32 curFlags = *(u32*)((u8*)pCur + 0x14) & 0x1FFFFFFF;
            u32 taskFlags = *(u32*)((u8*)pTask + 0x10) & 0x1FFFFFFF;
            if (curFlags == taskFlags) return pCur;
        }
        pCur = *(void**)((u8*)pCur + 0x18);
    }
    return NULL;
}

/* Returns the first timer-list entry whose trigger callback is pCallback,
 * or NULL when there is none (retail returns NULL, not the last entry). */
void* func_8001D164(void* pCallback) {
    void* pCur;
    for (pCur = g_TimerWorkList; pCur != NULL; pCur = *(void**)((u8*)pCur + 0x18)) {
        if (*(void**)((u8*)pCur + 0x08) == pCallback) {
            return pCur;
        }
    }
    return NULL;
}

void WorkListsDeleteTasks(WorkListEntry* pTasks) {
    WorkListRemoveTask(pTasks + 1);
    TimerWorkListRemoveTask(pTasks);
    HeapFree(pTasks);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", WorkListsAddTasks);
#else
WorkListEntry* WorkListsAddTasks(void* timerData, void* timerCb, void* workCb, void* onFreeCb) {
    WorkListEntry* pEntry;
    WorkListEntry* pWorkEntry;
    pEntry = HeapAlloc(sizeof(WorkListEntry) * 2, D_800591AF);
    TimerWorkListAddTask(timerData, pEntry);
    pWorkEntry = (WorkListEntry*)((u8*)pEntry + sizeof(WorkListEntry));
    WorkListAddTask(pEntry, pWorkEntry);
    TimerWorkListSetTaskCallback(pEntry, timerCb);
    WorkListSetTaskCallback(pWorkEntry, workCb);
    if (onFreeCb != NULL) {
        WorkListTaskSetOnFreeCallback(pEntry, onFreeCb);
    } else {
        WorkListTaskSetOnFreeCallback(pEntry, &WorkListsDeleteTasks);
    }
    pEntry->unk4 = pEntry;
    pWorkEntry->unk4 = pEntry;
    return pEntry;
}
#endif



// This may be the start of a new TU
#ifdef XENO_PC_PORT
extern s32 D_80059190;
#else
/* This TU's own .sbss word: retail reaches it %gp_rel, which cc1 only emits
 * for a same-TU definition (plain externs assemble absolute here). */
s32 D_80059190;
#endif
extern void func_8001DAE8(void* arg0, u16 arg1, u32 arg2);
extern void func_800234AC(void* pSpriteData);
extern void func_8001F8E8(void* pSpriteData, u16 frameIndex, u32 animPackageAddr);
extern u8 D_8005A474[];
extern u8 D_8006BE10[];

void func_8001D298(void) {
    D_80059190 = 0;
}

void func_8001D2A4(void) {
    D_80059190 = 0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", func_8001D2B0);
#else
// Change Sprite Animation Frame maybe?
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
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", func_8001D3F4);
#else
// Unlink SpriteData entry from list
void func_8001D3F4(u8* pTarget) {
    u8* pPrev = NULL;
    u8* pCur = (u8*)(uintptr_t)D_80059190;
    while (pCur != NULL) {
        u32 pNext;
        if (pCur == pTarget) {
            u32 pTargetSub = *(u32*)(pCur + 0x20);
            if (pPrev != NULL) {
                u32 pPrevSub = *(u32*)(pPrev + 0x20);
                *(u32*)(pPrevSub + 0x38) = *(u32*)(pTargetSub + 0x38);
            } else {
                D_80059190 = *(u32*)(pTargetSub + 0x38);
            }
        } else {
            pPrev = pCur;
        }
        pNext = *(u32*)(*(u32*)(pCur + 0x20) + 0x38);
        pCur = (u8*)(uintptr_t)pNext;
    }
}
#endif

void func_8001D468(void) {
    u8* pEntry = (u8*)(uintptr_t)D_80059190;

    while (pEntry != NULL) {
        u16 value34 = *(u16*)(pEntry + 0x34);

        if (value34 == 0) {
            *(u32*)(pEntry + 0x40) &= 0xFFFFFF03;
        } else {
            func_8001DAE8(pEntry, value34, *(u32*)(pEntry + 0x24));
        }

        pEntry = (u8*)(uintptr_t)*(u32*)((u8*)(uintptr_t)*(u32*)(pEntry + 0x20) + 0x38);
    }

    D_80059190 = 0;
}
