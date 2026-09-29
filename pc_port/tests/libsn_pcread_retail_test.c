/* Retail certificate for psyq libsn PCread (0x8004C398-0x8004C458).
 * len == 0 -> 0 and no transfer; otherwise up to 0x8000 bytes per
 * func_8004C458(0, fd, chunk, buff) call, accumulating the returned counts; a
 * short read stops the loop and a -1 result returns -1. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

extern int PCread(int fd, char* buff, int len);

static unsigned s_checks;
static int s_calls;
static int s_arg0;
static int s_fd;
static int s_chunk;
static char* s_buf;
static int s_ret_seq[4];
static int s_ret_idx;

int func_8004C458(int a0, int fd, int size, char* buff)
{
    (void)fd;
    s_calls++;
    s_arg0 = a0;
    s_fd = fd;
    s_chunk = size;
    s_buf = buff;
    return s_ret_seq[s_ret_idx++];
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
        fail("libsn.pcread", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libsn.pcread", detail);
    }
}

int main(void)
{
    static char buf[0x20000];
    int rc;

    /* len == 0: no transfer. */
    s_calls = 0; s_ret_idx = 0;
    rc = PCread(3, buf, 0);
    expect_eq_s32("zero.rc", rc, 0);
    expect_eq_s32("zero.calls", s_calls, 0);

    /* One full short transfer. */
    s_calls = 0; s_ret_idx = 0;
    s_ret_seq[0] = 0x100;
    rc = PCread(7, buf, 0x100);
    expect_eq_s32("one.rc", rc, 0x100);
    expect_eq_s32("one.calls", s_calls, 1);
    expect_eq_s32("one.arg0", s_arg0, 0);
    expect_eq_s32("one.fd", s_fd, 7);
    expect_eq_s32("one.chunk", s_chunk, 0x100);
    expect_eq_ptr("one.buf", s_buf, buf);

    /* Large length splits into 0x8000 chunks and completes. */
    s_calls = 0; s_ret_idx = 0;
    s_ret_seq[0] = 0x8000;
    s_ret_seq[1] = 0x8000;
    rc = PCread(1, buf, 0x10000);
    expect_eq_s32("big.rc", rc, 0x10000);
    expect_eq_s32("big.calls", s_calls, 2);
    expect_eq_s32("big.chunk", s_chunk, 0x8000);

    /* Short read stops the loop early. */
    s_calls = 0; s_ret_idx = 0;
    s_ret_seq[0] = 0x10;
    s_ret_seq[1] = 0x10;
    rc = PCread(2, buf, 0x10000);
    expect_eq_s32("short.rc", rc, 0x10);
    expect_eq_s32("short.calls", s_calls, 1);

    /* -1 aborts. */
    s_calls = 0; s_ret_idx = 0;
    s_ret_seq[0] = -1;
    rc = PCread(2, buf, 0x100);
    expect_eq_s32("err.rc", rc, -1);
    expect_eq_s32("err.calls", s_calls, 1);

    printf("LIBSN PCREAD certificate PASS checks=%u\n", s_checks);
    return 0;
}
