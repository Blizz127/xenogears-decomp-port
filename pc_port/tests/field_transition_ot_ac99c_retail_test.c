/*
 * Retail certificate for func_800AC99C (FE60 OT insert).
 *
 * Retail (func_800AC99C.s 0x800AC99C-0x800ACB8C): insert D_800AF7F0+idx*0x34
 * then D_800AF788+idx*0x34 so pSecond.tag sees the post-insert OT. Phase y0
 * is written at group+idx*0x50+{0x6A,0x7E,0x92,0xA6}, not idx*0x30.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "psx_memory.h"

extern void func_800AC99C(void);

unsigned char g_FieldTransitionPackets[0xD0];
asm(".globl D_800AF788\n.set D_800AF788, g_FieldTransitionPackets");
asm(".globl D_800AF7F0\n.set D_800AF7F0, g_FieldTransitionPackets + 0x68");
asm(".globl D_800AF824\n.set D_800AF824, g_FieldTransitionPackets + 0x9C");

void* D_800AF770;
int g_FieldCurRenderContextIndex;
void* g_FieldCurRenderContext;
uint8_t g_PsxRam[4];

static u8 s_heap[0x1000];
static u8 s_ctx[0x8100];
static unsigned s_checks;

static u32 pack24(const void* p)
{
    return PsxMemory_GuestAddr(p) & 0x00FFFFFF;
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
    fail("transition.ot", detail);
}

static void expect_eq_u32(const char* field, u32 actual, u32 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=0x%x expected=0x%x",
             field, actual, expected);
    fail("transition.ot", detail);
}

/* Retail ACACC..ACB64 chains each sprite before its draw-mode packet and
 * repeats the other context's first phase across the four strips. */
static void check_all_groups(unsigned idx, unsigned first_phase, u8 *setup_tail)
{
    u32 previous=pack24(setup_tail);
    for(unsigned row=0;row<16;++row) {
        u8 *group=s_heap+row*0x100;
        u8 *sprites=group+0x60+idx*0x50;
        u8 *modes=group+idx*0x30;
        unsigned phase=row==0?first_phase:255;
        for(unsigned col=0;col<4;++col) {
            u8 *sprite=sprites+col*0x14,*mode=modes+col*0x0c;
            expect_eq_u32("all.sprt.link",*(u32*)sprite&0xffffff,previous);
            expect_eq_u32("all.mode.link",*(u32*)mode&0xffffff,pack24(sprite));
            expect_eq_s32("all.phase",*(s16*)(sprite+0xa),phase);
            previous=pack24(mode);
        }
    }
}

int main(void)
{
    u32* otHead;
    u8* group;
    u8* pFirst;
    u8* pSecond;

    memset(g_FieldTransitionPackets, 0, sizeof(g_FieldTransitionPackets));
    memset(s_heap, 0, sizeof(s_heap));
    memset(s_ctx, 0, sizeof(s_ctx));
    D_800AF770 = s_heap;
    g_FieldCurRenderContext = s_ctx;
    otHead = (u32*)(s_ctx + 0x80D4);

    /* idx=0: pFirst=AF7F0, pSecond=AF788; other-context y0 at +0xBA. */
    *(u32*)(g_FieldTransitionPackets + 0x68) = 0xAC000000;
    *(u32*)g_FieldTransitionPackets = 0xAD000000;
    *otHead = 0x12ABCDEF;
    *(s16*)(s_heap + 0xBA) = 5;
    g_FieldCurRenderContextIndex = 0;
    func_800AC99C();

    pFirst = g_FieldTransitionPackets + 0x68;
    pSecond = g_FieldTransitionPackets;
    expect_eq_u32("i0.pFirst.lo", *(u32*)pFirst & 0x00FFFFFF, 0x00ABCDEF);
    expect_eq_u32("i0.pFirst.hi", *(u32*)pFirst & 0xFF000000, 0xAC000000);
    expect_eq_u32("i0.pSecond.lo", *(u32*)pSecond & 0x00FFFFFF, pack24(pFirst));
    expect_eq_u32("i0.pSecond.hi", *(u32*)pSecond & 0xFF000000, 0xAD000000);
    expect_eq_u32("i0.ot.lo", *otHead & 0x00FFFFFF, pack24(s_heap + 0xF00 + 0x24));
    expect_eq_s32("i0.phase0", *(s16*)(s_heap + 0x6A), 4);
    expect_eq_s32("i0.phase1", *(s16*)(s_heap + 0x7E), 4);
    expect_eq_s32("i0.phase3", *(s16*)(s_heap + 0xA6), 4);

    group = s_heap;
    expect_eq_u32("i0.sprt0.lo", *(u32*)(group + 0x60) & 0x00FFFFFF,
                  pack24(pSecond));
    expect_eq_u32("i0.drm0.lo", *(u32*)(group + 0x00) & 0x00FFFFFF,
                  pack24(group + 0x60));
    check_all_groups(0,4,pSecond);

    /* idx=1: phase dest is +0x50, not +0x30. */
    memset(g_FieldTransitionPackets, 0, sizeof(g_FieldTransitionPackets));
    memset(s_heap, 0, sizeof(s_heap));
    *(u32*)(g_FieldTransitionPackets + 0x68 + 0x34) = 0xAE000000;
    *(u32*)(g_FieldTransitionPackets + 0x34) = 0xAF000000;
    *otHead = 0x12000001;
    *(s16*)(s_heap + 0x6A) = 9;
    g_FieldCurRenderContextIndex = 1;
    func_800AC99C();

    pFirst = g_FieldTransitionPackets + 0x68 + 0x34;
    pSecond = g_FieldTransitionPackets + 0x34;
    expect_eq_u32("i1.pFirst.lo", *(u32*)pFirst & 0x00FFFFFF, 0x00000001);
    expect_eq_u32("i1.pSecond.lo", *(u32*)pSecond & 0x00FFFFFF, pack24(pFirst));
    expect_eq_s32("i1.phase.right", *(s16*)(s_heap + 0xBA), 8);
    expect_eq_s32("i1.phase.wrong30", *(s16*)(s_heap + 0x9A), 0);
    expect_eq_s32("i1.phase.left", *(s16*)(s_heap + 0x6A), 9);
    expect_eq_u32("i1.sprt.lo", *(u32*)(s_heap + 0xB0) & 0x00FFFFFF,
                  pack24(pSecond));
    expect_eq_u32("i1.drm.lo", *(u32*)(s_heap + 0x30) & 0x00FFFFFF,
                  pack24(s_heap + 0xB0));
    check_all_groups(1,8,pSecond);

    printf("FIELD TRANSITION OT AC99C certificate PASS checks=%u\n", s_checks);
    return 0;
}
