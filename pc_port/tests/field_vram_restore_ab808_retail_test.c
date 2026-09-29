/*
 * Retail certificate for func_800AB808 (VRAM restore gated by func_800AB748).
 *
 * Retail (func_800AB808.s 0x800AB808-0x800ABA94): for slots 0..4, restore
 * only when func_800AB748(i)==-1 and TIM.paddr!=0. LoadImage dest is
 * (x/2+0x300, y+0x100, w/2, h) from D_800AF5C0[i].
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern void func_800AB808(void);
extern s32 func_800AB748(s32 slot);

void* g_pGameState;
s16 D_800AF5C0[5 * 4];

static u8 s_state[0x1A20];
static u8 s_buf[0x100];
static u8 s_image[0xF20];
static u8 s_pixels[0x400];
static RECT s_prect;
static unsigned s_checks;
static int s_load_count;
static RECT s_load_rect[8];
static int s_decode_calls;
static int s_read_calls;

int ArchiveDecodeAlignedSize(int entryIndex)
{
    s_decode_calls++;
    if (entryIndex != 0x802) {
        fprintf(stderr, "ASSERTION archive.index decode=%d\n", entryIndex);
        exit(1);
    }
    return (int)sizeof(s_buf);
}

s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)
{
    (void)arg2;
    s_read_calls++;
    if (index != 0x802 || flags != 0x80 || pBuffer != s_buf) {
        fprintf(stderr, "ASSERTION archive.read\n");
        exit(1);
    }
    return 0;
}

void ArchiveCdDataSync(int mode)
{
    (void)mode;
}

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    if (allocSize == sizeof(s_buf)) {
        return s_buf;
    }
    if (allocSize == 0xF20) {
        return s_image;
    }
    fprintf(stderr, "ASSERTION heap.size 0x%x\n", allocSize);
    exit(1);
}

void HeapFree(void* p)
{
    (void)p;
}

int OpenTIM(u_long* addr)
{
    if (addr != (u_long*)s_buf) {
        fprintf(stderr, "ASSERTION opentim\n");
        exit(1);
    }
    return 0;
}

TIM_IMAGE* ReadTIM(TIM_IMAGE* timimg)
{
    void* pixels = s_pixels;

    memset(&s_prect, 0, sizeof(s_prect));
    memset(timimg, 0, sizeof(*timimg));
    timimg->prect = &s_prect;
    memcpy(&timimg->paddr, &pixels, sizeof(pixels));
    return timimg;
}

int LoadImage(RECT* rect, u_long* p)
{
    (void)p;
    if (s_load_count < 8) {
        s_load_rect[s_load_count] = *rect;
    }
    s_load_count++;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
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
    fail("vram.restore", detail);
}

static void setup_slots(void)
{
    int i;

    memset(D_800AF5C0, 0, sizeof(D_800AF5C0));
    for (i = 0; i < 5; i++) {
        D_800AF5C0[i * 4 + 0] = (s16)(i * 2);      /* x */
        D_800AF5C0[i * 4 + 1] = 0;                 /* y */
        D_800AF5C0[i * 4 + 2] = 8;                 /* w */
        D_800AF5C0[i * 4 + 3] = 1;                 /* h */
    }
}

int main(void)
{
    uintptr_t addr = (uintptr_t)s_state;

    if (addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    memset(s_state, 0, sizeof(s_state));
    g_pGameState = s_state;
    setup_slots();
    s_load_count = 0;
    s_decode_calls = 0;
    s_read_calls = 0;

    /* flags=0: AB748 slots 0-3 return -1 (restore), slot 4 returns 0 (skip). */
    expect_eq_s32("gate.s0", func_800AB748(0), -1);
    expect_eq_s32("gate.s4", func_800AB748(4), 0);

    func_800AB808();
    expect_eq_s32("decode", s_decode_calls, 1);
    expect_eq_s32("read", s_read_calls, 1);
    expect_eq_s32("loads.flags0", s_load_count, 4);
    expect_eq_s32("rect0.x", s_load_rect[0].x, 0x300);
    expect_eq_s32("rect0.y", s_load_rect[0].y, 0x100);
    expect_eq_s32("rect0.w", s_load_rect[0].w, 4);
    expect_eq_s32("rect0.h", s_load_rect[0].h, 1);
    expect_eq_s32("rect3.x", s_load_rect[3].x, 0x300 + 3);

    /* Slot 0 bit set: AB748(0)==0, so first restore skipped (3 loads). */
    *(u16*)(s_state + 0x1A16) = 0x8;
    s_load_count = 0;
    func_800AB808();
    expect_eq_s32("loads.s0skip", s_load_count, 3);

    printf("FIELD VRAM RESTORE AB808 certificate PASS checks=%u\n", s_checks);
    return 0;
}
