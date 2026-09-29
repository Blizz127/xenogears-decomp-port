/* Retail certificate for GfxFreeWorkBuffers (0x80024FB8-0x80024FE4).
 * Frees the work-buffer block and then resets the sprite/image lists, in that
 * order (retail: HeapFree(g_GfxWorkBuffers) then func_8001D2A4). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

extern void GfxFreeWorkBuffers(void);

void* g_GfxWorkBuffers;

static unsigned s_checks;
static int s_seq;
static int s_free_order;
static void* s_free_arg;
static int s_reset_order;

void HeapFree(void* pMem)
{
    s_seq++;
    s_free_order = s_seq;
    s_free_arg = pMem;
}

void func_8001D2A4(void)
{
    s_seq++;
    s_reset_order = s_seq;
}

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
        fail("temp1.gfxfree", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp1.gfxfree", detail);
    }
}

int main(void)
{
    static int sentinel;

    g_GfxWorkBuffers = &sentinel;
    s_seq = 0;
    GfxFreeWorkBuffers();
    expect_eq_ptr("free.arg", s_free_arg, &sentinel);
    expect_eq_s32("free.order", s_free_order, 1);
    expect_eq_s32("reset.order", s_reset_order, 2);
    expect_eq_s32("calls", s_seq, 2);

    g_GfxWorkBuffers = NULL;
    s_seq = 0;
    GfxFreeWorkBuffers();
    expect_eq_ptr("null.arg", s_free_arg, NULL);
    expect_eq_s32("null.calls", s_seq, 2);

    printf("TEMP1 GFX FREE certificate PASS checks=%u\n", s_checks);
    return 0;
}
