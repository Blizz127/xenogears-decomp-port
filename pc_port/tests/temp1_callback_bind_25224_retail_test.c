/*
 * Retail certificate for func_80025224 (work-list callback bind).
 *
 * Retail (func_80025224.s 0x80025224-0x80025258):
 *   a1 = index -> a1 <<= 2; a1 += &D_8004FD40; a1 = *(u32*)a1;
 *   WorkListSetTaskCallback(a0, a1); return.
 * i.e. the handler stored at table entry `handlerIndex` is bound to the task.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

extern void func_80025224(void* pTask, int handlerIndex);

void (*D_8004FD40[16])(void*);

static unsigned s_checks;
static int s_calls;
static void* s_task;
static void (*s_cb)(void*);

void WorkListSetTaskCallback(void* pTask, void (*callback)(void*))
{
    s_calls++;
    s_task = pTask;
    s_cb = callback;
}

static void dummy0(void* p) { (void)p; }
static void dummy1(void* p) { (void)p; }
static void dummy2(void* p) { (void)p; }

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
        fail("temp1.callback", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp1.callback", detail);
    }
}

static void fill_table(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        D_8004FD40[i] = NULL;
    }
    D_8004FD40[0] = dummy0;
    D_8004FD40[5] = dummy1;
    D_8004FD40[15] = dummy2;
    s_calls = 0;
    s_task = NULL;
    s_cb = NULL;
}

int main(void)
{
    /* Entry 5 carries dummy1. */
    fill_table();
    func_80025224((void*)0x11, 5);
    expect_eq_s32("idx5.calls", s_calls, 1);
    expect_eq_s32("idx5.task", (s32)(uintptr_t)s_task, 0x11);
    expect_eq_ptr("idx5.cb", (void*)s_cb, (void*)dummy1);

    /* Entry 0 carries dummy0. */
    fill_table();
    func_80025224((void*)0x22, 0);
    expect_eq_ptr("idx0.cb", (void*)s_cb, (void*)dummy0);
    expect_eq_s32("idx0.task", (s32)(uintptr_t)s_task, 0x22);

    /* Entry 15 carries dummy2 (last table slot). */
    fill_table();
    func_80025224((void*)0x33, 15);
    expect_eq_ptr("idx15.cb", (void*)s_cb, (void*)dummy2);
    expect_eq_s32("idx15.calls", s_calls, 1);

    /* An empty slot binds a NULL handler rather than skipping the call. */
    fill_table();
    func_80025224((void*)0x44, 7);
    expect_eq_s32("null.calls", s_calls, 1);
    expect_eq_ptr("null.cb", (void*)s_cb, NULL);
    expect_eq_s32("null.task", (s32)(uintptr_t)s_task, 0x44);

    printf("TEMP1 CALLBACK BIND 25224 certificate PASS checks=%u\n", s_checks);
    return 0;
}
