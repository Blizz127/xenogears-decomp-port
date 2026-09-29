/* Retail certificate for func_800233A4 (0x800233A4-0x80023440). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern void* func_800233A4(void* pOwner, s32 dataSize);

static u8 s_pool[0x400];
u8 D_800591AF = 7;

static unsigned s_checks;
static int s_alloc_calls;
static s32 s_alloc_size;
static s32 s_alloc_mode;
static int s_timer_add;
static void* s_timer_owner;
static void* s_timer_task;
static int s_work_add;
static void* s_work_task;
static void* s_work_node;
static int s_init38;
static void* s_init_arg;
static void* s_timer_cb_task;
static void (*s_timer_cb)(void*);
static void* s_free_cb_task;
static void (*s_free_cb)(void*);

void* HeapAlloc(s32 size, s32 mode)
{
    s_alloc_calls++;
    s_alloc_size = size;
    s_alloc_mode = mode;
    memset(s_pool, 0, sizeof(s_pool));
    return s_pool;
}

void TimerWorkListAddTask(void* pOwner, void* pTask)
{
    s_timer_add++;
    s_timer_owner = pOwner;
    s_timer_task = pTask;
}

void WorkListAddTask(void* pTask, void* pNode)
{
    s_work_add++;
    s_work_task = pTask;
    s_work_node = pNode;
}

void func_80023804(void* p)
{
    s_init38++;
    s_init_arg = p;
}

void TimerWorkListSetTaskCallback(void* pTask, void (*pCallback)(void*))
{
    s_timer_cb_task = pTask;
    s_timer_cb = pCallback;
}

void WorkListTaskSetOnFreeCallback(void* pTask, void (*pCallback)(void*))
{
    s_free_cb_task = pTask;
    s_free_cb = pCallback;
}

void func_80022DF4(void* p) { (void)p; }
void func_80022EB8(void* p) { (void)p; }

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
                 field, (int)actual, (int)expected);
        fail("temp1.wrapper", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp1.wrapper", detail);
    }
}

int main(void)
{
    void* pOwner = (void*)0x1234;
    u8* pWrapper;
    u8* pInner;

    pWrapper = func_800233A4(pOwner, 0x40);
    pInner = pWrapper + 0x1C;
    expect_eq_ptr("ret", pWrapper, s_pool);
    expect_eq_s32("alloc.size", s_alloc_size, 0x40 + 0xEC);
    expect_eq_s32("alloc.mode", s_alloc_mode, 7);
    expect_eq_s32("alloc.calls", s_alloc_calls, 1);
    expect_eq_ptr("timer.owner", s_timer_owner, pOwner);
    expect_eq_ptr("timer.task", s_timer_task, pWrapper);
    expect_eq_ptr("work.task", s_work_task, pWrapper);
    expect_eq_ptr("work.node", s_work_node, pInner);
    expect_eq_ptr("init.arg", s_init_arg, pWrapper + 0x38);
    expect_eq_s32("init.calls", s_init38, 1);
    expect_eq_s32("wrapper.link", *(u32*)(pWrapper + 4), (s32)(uintptr_t)(pWrapper + 0x38));
    expect_eq_s32("inner.link", *(u32*)(pInner + 4), (s32)(uintptr_t)(pWrapper + 0x38));
    expect_eq_ptr("timer.cb.task", s_timer_cb_task, pWrapper);
    expect_eq_ptr("timer.cb", (void*)s_timer_cb, (void*)func_80022DF4);
    expect_eq_ptr("free.cb.task", s_free_cb_task, pWrapper);
    expect_eq_ptr("free.cb", (void*)s_free_cb, (void*)func_80022EB8);

    printf("TEMP1 WRAPPER ALLOC certificate PASS checks=%u\n", s_checks);
    return 0;
}
