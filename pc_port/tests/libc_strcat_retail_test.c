/* Retail certificate for psyq libc strcat (0x8003FA78-0x8003FB20).
 * NULL dst or NULL src -> NULL; otherwise src is appended to dst (terminator
 * included) and dst is returned. The same-end-address case also returns NULL. */
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
        fail("libc.strcat", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libc.strcat", detail);
    }
}

int main(void)
{
    char buf[32];
    char* ret;
    char* (*pCat)(char*, char*);
    char same[8];

    pCat = (char* (*)(char*, char*))strcat;

    strcpy(buf, "ab");
    ret = pCat(buf, "cd");
    expect_eq_s32("cat.str", strcmp(buf, "abcd"), 0);
    expect_eq_ptr("cat.ret", ret, buf);

    strcpy(buf, "");
    ret = pCat(buf, "xy");
    expect_eq_s32("empty.str", strcmp(buf, "xy"), 0);
    expect_eq_ptr("empty.ret", ret, buf);

    strcpy(buf, "ab");
    ret = pCat(buf, "");
    expect_eq_s32("src.empty", strcmp(buf, "ab"), 0);

    expect_eq_ptr("null.dst", pCat(NULL, "x"), NULL);

    strcpy(buf, "ab");
    expect_eq_ptr("null.src", pCat(buf, NULL), NULL);
    expect_eq_s32("null.src.keep", strcmp(buf, "ab"), 0);

    /* dst and src ending at the same address -> retail returns NULL. */
    strcpy(same, "abc");
    expect_eq_ptr("same.end", pCat(same, same + 3), NULL);

    printf("LIBC STRCAT certificate PASS checks=%u\n", s_checks);
    return 0;
}
