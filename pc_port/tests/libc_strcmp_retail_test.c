/* Retail certificate for psyq libc strcmp (0x8003FB20-0x8003FB84).
 * NULL/NULL -> 0, NULL first -> -1, NULL second -> 1; otherwise the first
 * differing byte pair is returned as a signed difference of unsigned bytes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static unsigned s_checks;
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
        fail("libc.strcmp", detail);
    }
}

int main(void)
{
    int (*pCmp)(char*, char*);

    pCmp = (int (*)(char*, char*))strcmp;

    expect_eq_s32("eq", pCmp("abc", "abc"), 0);
    expect_eq_s32("empty", pCmp("", ""), 0);
    expect_eq_s32("prefix", pCmp("ab", "abc") < 0, 1);
    expect_eq_s32("longer", pCmp("abc", "ab") > 0, 1);
    expect_eq_s32("diff", pCmp("abd", "abc") > 0, 1);
    expect_eq_s32("highbit", pCmp("\xFF", "\x01") > 0, 1);
    expect_eq_s32("null.null", pCmp(NULL, NULL), 0);
    expect_eq_s32("null.first", pCmp(NULL, "a"), -1);
    expect_eq_s32("null.second", pCmp("a", NULL), 1);
    expect_eq_s32("nul.vs.byte", pCmp("a", "a\x01") < 0, 1);

    printf("LIBC STRCMP certificate PASS checks=%u\n", s_checks);
    return 0;
}
