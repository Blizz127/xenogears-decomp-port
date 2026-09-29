/* Retail certificates for psyq libgpu ClearOTag (0x80044A20-0x80044AD8) and
 * MoveImage (0x8004495C-0x80044A20). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern u_long* ClearOTag(u_long* ot, int n);
extern int MoveImage(RECT* pRect, int x, int y);

/* ClearOTag's globals */
char g_GraphDebugLevel;
int (*g_GpuPrintf)(char*, ...);
char D_8005698C[4];
char D_800191B0[4] = "OT";
char D_800191A4[4] = "MV";

/* MoveImage's packet and dispatch state */
u32 D_80056980[4];
u32 D_80056984;
u32 D_80056988;
void* D_800568C8;

static unsigned s_checks;
static int s_printf_calls;
static void* s_printf_ot;
static int s_printf_n;
static int s_helper_calls;
static char* s_helper_fmt;
static void* s_helper_rect;
static int s_helper_ret;
static int s_move_calls;
static void* s_move_ctx;
static void* s_move_pkt;
static s32 s_move_len;
static u32 s_move_arg;
static int s_move_ret;

int func_8004463C(char* pFormat, RECT* pRect)
{
    s_helper_calls++;
    s_helper_fmt = pFormat;
    s_helper_rect = pRect;
    return s_helper_ret;
}

static int GpuPrintfStub(char* fmt, ...)
{
    (void)fmt;
    s_printf_calls++;
    return 0;
}

static int MoveStub(void* pCtx, void* pPkt, s32 nLen, u32 arg4)
{
    s_move_calls++;
    s_move_ctx = pCtx;
    s_move_pkt = pPkt;
    s_move_len = nLen;
    s_move_arg = arg4;
    return s_move_ret;
}

static u8 s_dispatch[0x40];

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
        fail("libgpu.ot", detail);
    }
}

/* Aliasing-clean 4-byte load (production spells these as *(u32*) puns and
 * compiles -w; this TU is -Werror). */
static u32 load_u32(const void* p)
{
    u32 v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libgpu.ot", detail);
    }
}

int main(void)
{
    u_long ot[4];
    u_long* pRet;
    RECT rect;
    int rc;
    int i;

    /* ---- ClearOTag ---- */
    ot[0] = 0xAA000000; ot[1] = 0xBB000000; ot[2] = 0xCC000000; ot[3] = 0xDD000000;
    *(u32*)D_8005698C = 0x12345678;
    g_GraphDebugLevel = 0;
    g_GpuPrintf = GpuPrintfStub;
    s_printf_calls = 0;
    pRet = ClearOTag(ot, 4);
    expect_eq_ptr("clear.ret", pRet, &ot[3]);
    for (i = 0; i < 3; i++) {
        expect_eq_s32("clear.link", (s32)load_u32(&ot[i]),
                      (s32)(((u32)(uintptr_t)&ot[i + 1]) & 0x00FFFFFFu));
    }
    expect_eq_s32("clear.last", (s32)load_u32(&ot[3]),
                  (s32)(((u32)(uintptr_t)D_8005698C) & 0x00FFFFFFu));
    expect_eq_s32("clear.no.printf", s_printf_calls, 0);

    ot[0] = 0xAA000000; ot[1] = 0xBB000000;
    g_GraphDebugLevel = 2;
    s_printf_ot = NULL; s_printf_n = 0;
    ClearOTag(ot, 2);
    expect_eq_s32("clear.printf", s_printf_calls, 1);

    /* ---- MoveImage ---- */
    rect.x = 5; rect.y = 6; rect.w = 7; rect.h = 8;
    memset(D_80056980, 0, sizeof(D_80056980));
    memset(s_dispatch, 0, sizeof(s_dispatch));
    D_800568C8 = s_dispatch;
    *(void**)(s_dispatch + 8) = (void*)&MoveStub;
    *(u32*)(s_dispatch + 0x18) = 0x1234;
    s_helper_calls = 0; s_helper_ret = 1;
    s_move_calls = 0; s_move_ret = 0x42;
    rc = MoveImage(&rect, 0x1111, 0x2222);
    expect_eq_s32("move.rc", rc, 0x42);
    expect_eq_s32("move.helper", s_helper_calls, 1);
    expect_eq_ptr("move.fmt", s_helper_fmt, D_800191A4);
    expect_eq_ptr("move.rect", s_helper_rect, &rect);
    expect_eq_s32("move.pkt0", (s32)D_80056980[0], (s32)load_u32(&rect.x));
    expect_eq_s32("move.xy", (s32)D_80056984, (s32)0x22221111);
    expect_eq_s32("move.pkt8", (s32)D_80056980[2], (s32)load_u32(&rect.w));
    expect_eq_s32("move.calls", s_move_calls, 1);
    expect_eq_s32("move.ctx", (s32)(uintptr_t)s_move_ctx, 0x1234);
    /* Same address MoveImage passes (&D_80056980[-2]); laundered through
     * uintptr_t so -Warray-bounds cannot see the negative offset. */
    expect_eq_ptr("move.buf", s_move_pkt,
                  (const void*)((uintptr_t)D_80056980 - 8u));
    expect_eq_s32("move.len", s_move_len, 0x14);
    expect_eq_s32("move.arg4", (s32)s_move_arg, 0);

    /* helper returning 0 -> -1 with no dispatch */
    s_helper_ret = 0;
    s_move_calls = 0;
    rc = MoveImage(&rect, 1, 2);
    expect_eq_s32("move.fail.rc", rc, -1);
    expect_eq_s32("move.fail.calls", s_move_calls, 0);

    printf("LIBGPU OT+IMAGE certificate PASS checks=%u\n", s_checks);
    return 0;
}
