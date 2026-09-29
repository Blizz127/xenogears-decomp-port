/*
 * Retail certificate for func_800AC3AC (FE60 transition packet setup).
 *
 * Retail (func_800AC3AC.s 0x800AC3AC-0x800AC998): SetPolyGT4 at D_800AF788
 * and D_800AF7F0; RGB0/1 of first and RGB2/3 of second are 0xFF. Copy each
 * POLY_GT4 0x34 bytes to +0x34. HeapAlloc(0x1000,1) with no NULL check.
 * 16 groups of 8 SPRT; per-strip x0 written to left (+0x60) and right
 * (+0xB0). GetTPage(1,0,0x300+strip*0x40,0) twice per strip.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern void func_800AC3AC(void);

unsigned char g_FieldTransitionPackets[0xD0];
asm(".globl D_800AF788\n.set D_800AF788, g_FieldTransitionPackets");
asm(".globl D_800AF7F0\n.set D_800AF7F0, g_FieldTransitionPackets + 0x68");
asm(".globl D_800AF824\n.set D_800AF824, g_FieldTransitionPackets + 0x9C");

void* D_800AF770;
uint8_t g_PsxRam[4];

static u8 s_heap[0x1000];
static unsigned s_checks;
static int s_gt4_count;
static void* s_gt4[4];
static int s_sprt_count;
static int s_semi_count;
static int s_semi_abe[32];
static int s_tpage_count;
static int s_tpage_x[160];
static int s_tpage_y[160];
static int s_tpage_tp[160];
static int s_tpage_abr[160];
static int s_clut_count;
static int s_clut_y[64];
static int s_draw_count;
static DR_MODE* s_draw_p[160];
static int s_draw_tpage[160];
static int s_alloc_size;
static int s_alloc_flags;

void SetPolyGT4(POLY_GT4* p)
{
    p->code = 0x3C;
    if (s_gt4_count < 4) {
        s_gt4[s_gt4_count++] = p;
    }
}

void SetSprt(SPRT* p)
{
    p->code = 0x64;
    s_sprt_count++;
}

void SetSemiTrans(void* p, int abe)
{
    (void)p;
    if (s_semi_count < 32) {
        s_semi_abe[s_semi_count] = abe;
    }
    s_semi_count++;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    if (s_tpage_count < 160) {
        s_tpage_tp[s_tpage_count] = tp;
        s_tpage_abr[s_tpage_count] = abr;
        s_tpage_x[s_tpage_count] = x;
        s_tpage_y[s_tpage_count] = y;
    }
    s_tpage_count++;
    return (u_short)((x & 0x3FF) | ((y & 0x100) << 2) | (abr << 5) | (tp << 7));
}

u_short GetClut(int x, int y)
{
    (void)x;
    if (s_clut_count < 64) {
        s_clut_y[s_clut_count] = y;
    }
    s_clut_count++;
    return (u_short)(y << 6);
}

void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT* tw)
{
    (void)dfe;
    (void)dtd;
    if (tw != NULL) {
        fprintf(stderr, "ASSERTION draw.tw nonnull\n");
        exit(1);
    }
    if (s_draw_count < 160) {
        s_draw_p[s_draw_count] = p;
        s_draw_tpage[s_draw_count] = tpage;
    }
    s_draw_count++;
}

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    s_alloc_size = (int)allocSize;
    s_alloc_flags = (int)allocFlags;
    return s_heap;
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
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
             field, (int)actual, (int)expected);
    fail("transition.packet", detail);
}

int main(void)
{
    POLY_GT4* first;
    POLY_GT4* second;
    POLY_GT4* firstCopy;
    POLY_GT4* secondCopy;
    SPRT* g0;
    SPRT* g1;
    u8* packet0;

    memset(g_FieldTransitionPackets, 0xAA, sizeof(g_FieldTransitionPackets));
    memset(s_heap, 0xBB, sizeof(s_heap));
    D_800AF770 = (void*)(uintptr_t)0xDEAD;

    func_800AC3AC();

    first = (POLY_GT4*)g_FieldTransitionPackets;
    second = (POLY_GT4*)(g_FieldTransitionPackets + 0x68);
    firstCopy = (POLY_GT4*)(g_FieldTransitionPackets + 0x34);
    secondCopy = (POLY_GT4*)(g_FieldTransitionPackets + 0x9C);
    packet0 = s_heap;
    g0 = (SPRT*)(packet0 + 0x60);
    g1 = (SPRT*)(s_heap + 0x100 + 0x60);

    expect_eq_s32("gt4.count", s_gt4_count, 2);
    expect_eq_s32("gt4.0", (s32)(s_gt4[0] == first), 1);
    expect_eq_s32("gt4.1", (s32)(s_gt4[1] == second), 1);
    expect_eq_s32("first.r0", first->r0, 0xFF);
    expect_eq_s32("first.r2", first->r2, 0);
    expect_eq_s32("second.r0", second->r0, 0);
    expect_eq_s32("second.r2", second->r2, 0xFF);
    expect_eq_s32("first.x1", first->x1, 0x280);
    expect_eq_s32("first.y2", first->y2, 0x18);
    expect_eq_s32("second.y0", second->y0, 0xC8);
    expect_eq_s32("second.y3", second->y3, 0xE0);
    expect_eq_s32("first.u1", first->u1, 2);
    expect_eq_s32("second.v2", second->v2, 2);
    expect_eq_s32("fade.tpage0.x", s_tpage_x[0], 0x3C0);
    expect_eq_s32("fade.tpage0.y", s_tpage_y[0], 0x100);
    expect_eq_s32("fade.tpage0.abr", s_tpage_abr[0], 2);
    expect_eq_s32("fade.tpage1.x", s_tpage_x[1], 0x3C0);
    expect_eq_s32("copy.first.r0", firstCopy->r0, 0xFF);
    expect_eq_s32("copy.second.r0", secondCopy->r0, 0);
    expect_eq_s32("copy.second.y0", secondCopy->y0, 0xC8);
    expect_eq_s32("alloc.size", s_alloc_size, 0x1000);
    expect_eq_s32("alloc.flags", s_alloc_flags, 1);
    expect_eq_s32("heap.ptr", (s32)(D_800AF770 == s_heap), 1);
    expect_eq_s32("sprt.count", s_sprt_count, 0x10);
    expect_eq_s32("g0.x0", g0->x0, 0x40);
    expect_eq_s32("g0.y0", g0->y0, 0);
    expect_eq_s32("g0.v0", g0->v0, 0);
    expect_eq_s32("g0.w", g0->w, 0x80);
    expect_eq_s32("g0.h", g0->h, 0x10);
    expect_eq_s32("g0.r0", g0->r0, 0x80);
    expect_eq_s32("g1.y0", g1->y0, 0x10);
    expect_eq_s32("g1.v0", g1->v0, 0x10);
    expect_eq_s32("g0.right0.x0", ((SPRT*)(packet0 + 0xB0))->x0, 0x40);
    expect_eq_s32("g0.left1.x0", ((SPRT*)(packet0 + 0x74))->x0, 0xC0);
    expect_eq_s32("g0.right1.x0", ((SPRT*)(packet0 + 0xC4))->x0, 0xC0);
    expect_eq_s32("g0.right3.x0", ((SPRT*)(packet0 + 0xEC))->x0, 0x1C0);
    expect_eq_s32("strip0.tpage.x", s_tpage_x[2], 0x300);
    expect_eq_s32("strip0.tpage.abr", s_tpage_abr[2], 0);
    expect_eq_s32("strip1.tpage.x", s_tpage_x[4], 0x340);
    expect_eq_s32("draw0.off", (s32)((u8*)s_draw_p[0] - packet0), 0);
    expect_eq_s32("draw1.off", (s32)((u8*)s_draw_p[1] - packet0), 0x30);
    expect_eq_s32("tpage.count", s_tpage_count, 2 + 0x10 * 8);
    expect_eq_s32("draw.count", s_draw_count, 0x10 * 8);
    expect_eq_s32("clut.count", s_clut_count, 2 + 0x10);
    expect_eq_s32("semi.fade0", s_semi_abe[0], 1);
    expect_eq_s32("semi.sprite0", s_semi_abe[2], 0);

    printf("FIELD TRANSITION PACKET AC3AC certificate PASS checks=%u\n",
           s_checks);
    return 0;
}
