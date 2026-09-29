/* Retail certificate for psyq libc strcpy (0x8003FB84-0x8003FBC8).
 * NULL dst or NULL src -> NULL; otherwise the source string is copied
 * (including the terminator) and the destination start is returned. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static unsigned s_checks;

/* Referenced by other libc.c bodies that survive section gc. */
u_long g_RandomSeed;

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
        fail("libc.strcpy", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libc.strcpy", detail);
    }
}

int main(void)
{
    char dst[32];
    char* ret;
    char* (*pCopy)(char*, char*);

    /* Called through a pointer so the host header's nonnull attribute does not
     * reject the NULL-guard cases. */
    pCopy = (char* (*)(char*, char*))strcpy;
    memset(dst, 0x5A, sizeof(dst));
    ret = pCopy(dst, "hello");
    expect_eq_s32("copy.str", strcmp(dst, "hello"), 0);
    expect_eq_ptr("copy.ret", ret, dst);
    expect_eq_s32("copy.tail", dst[6], 0x5A);

    memset(dst, 0x5A, sizeof(dst));
    ret = pCopy(dst, "");
    expect_eq_s32("empty.str", dst[0], 0);
    expect_eq_ptr("empty.ret", ret, dst);

    expect_eq_ptr("null.dst", pCopy(NULL, "x"), NULL);
    expect_eq_ptr("null.src", pCopy(dst, NULL), NULL);

    memset(dst, 0x5A, sizeof(dst));
    ret = pCopy(dst, "a");
    expect_eq_s32("one.byte", dst[0], 'a');
    expect_eq_s32("one.nul", dst[1], 0);
    expect_eq_ptr("one.ret", ret, dst);

    printf("LIBC STRCPY certificate PASS checks=%u\n", s_checks);
    return 0;
}
