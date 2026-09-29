#include "common.h"
#include "psx_memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef void (*WorkListCallback)(void*);

typedef struct TestWorkListEntry {
    u32 owner;
    u32 payload;
    u32 trigger;
    u32 on_free;
    u32 flags10;
    u32 flags14;
    u32 next;
} TestWorkListEntry;

void* WorkListsAddTasks(u32 allocation_size, void* timer_owner,
                        WorkListCallback timer_callback,
                        WorkListCallback work_callback,
                        WorkListCallback on_free_callback);
void WorkListsFreeAllEntries(void);
void TimerWorkListUpdate(void);
void WorkListUpdate(void);

s32 D_80059190;
s32 g_NumTimerWorkListEntries;
s32 g_NumWorkListEntries;
s32 g_WorkListCurTimer;
TestWorkListEntry* D_800594C0;
TestWorkListEntry* g_TimerWorkList;
TestWorkListEntry* D_80059590;
TestWorkListEntry* g_WorkList;
short D_80059494;
u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

extern u8 D_800591AF;

static u8 g_Heap[0x200];
static u32 g_AllocSize;
static u32 g_AllocFlags;
static unsigned g_FreeCalls;
static unsigned g_DispatchCalls;
static u32 g_Dispatched[2];
static void* g_DispatchArguments[2];
static unsigned g_Checks;

void* HeapAlloc(u_int size, u_int flags)
{
    g_AllocSize = size;
    g_AllocFlags = flags;
    memset(g_Heap, 0xCC, sizeof(g_Heap));
    return g_Heap;
}

u_int HeapFree(void* allocation)
{
    if (allocation != g_Heap) {
        fprintf(stderr, "WORK_LIST_PAIR FAIL wrong free pointer\n");
        return 1;
    }
    g_FreeCalls++;
    return 0;
}

int PcPort_BattleMipsDispatchCallback(u32 callback, void* argument)
{
    if (g_DispatchCalls < 2) {
        g_Dispatched[g_DispatchCalls] = callback;
        g_DispatchArguments[g_DispatchCalls] = argument;
    }
    g_DispatchCalls++;
    return 1;
}

static int require(int condition, const char* message)
{
    g_Checks++;
    if (!condition) {
        fprintf(stderr, "WORK_LIST_PAIR FAIL %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    const u32 timer_callback = 0x800BE11Cu;
    const u32 work_callback = 0x800BE1C4u;
    TestWorkListEntry* timer;
    TestWorkListEntry* work;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    g_AllocSize = g_AllocFlags = 0;
    g_FreeCalls = g_DispatchCalls = 0;
    D_800591AF = 5;

    timer = WorkListsAddTasks(0x130, NULL,
                              (WorkListCallback)(uintptr_t)timer_callback,
                              (WorkListCallback)(uintptr_t)work_callback,
                              NULL);
    work = (TestWorkListEntry*)((u8*)timer + 0x1C);
    if (!require(timer == (TestWorkListEntry*)g_Heap,
                 "returns allocation base") ||
        !require(g_AllocSize == 0x130 && g_AllocFlags == 5,
                 "uses retail caller size and heap owner") ||
        !require(g_TimerWorkList == timer && g_WorkList == work,
                 "registers paired timer/work entries") ||
        !require(timer->trigger == timer_callback &&
                 work->trigger == work_callback,
                 "stores both retail guest callbacks") ||
        !require(timer->payload == (u32)(uintptr_t)timer &&
                 work->payload == (u32)(uintptr_t)timer,
                 "pairs entries through allocation base") ||
        !require(g_NumTimerWorkListEntries == 1 &&
                 g_NumWorkListEntries == 1,
                 "increments both list counts")) {
        return 1;
    }

    TimerWorkListUpdate();
    WorkListUpdate();
    if (!require(g_DispatchCalls == 2,
                 "dispatches both guest callbacks") ||
        !require(g_Dispatched[0] == timer_callback &&
                 g_Dispatched[1] == work_callback,
                 "preserves callback addresses") ||
        !require(g_DispatchArguments[0] == timer &&
                 g_DispatchArguments[1] == work,
                 "passes the matching packed entries")) {
        return 1;
    }

    WorkListsFreeAllEntries();
    if (!require(g_TimerWorkList == NULL && g_WorkList == NULL,
                 "paired free callback unlinks both lists") ||
        !require(g_NumTimerWorkListEntries == 0 &&
                 g_NumWorkListEntries == 0,
                 "paired free restores both counts") ||
        !require(g_FreeCalls == 1, "paired allocation freed exactly once")) {
        return 1;
    }

    printf("WORK_LIST_PAIR PASS checks=%u\n", g_Checks);
    return 0;
}
