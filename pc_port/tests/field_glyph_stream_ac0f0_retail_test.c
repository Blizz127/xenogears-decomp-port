/*
 * Retail certificate for func_800AC0F0 (FE60 glyph strip).
 *
 * Retail (func_800AC0F0.s 0x800AC0F0-0x800AC304): remaining is D_800AF780;
 * 0x0D consumes 1 byte; sourceIndex==1 uses signed div-by-7 (mult
 * 0x92492493) for MoveImage src ((raw%7)*9+0x380, (raw/7)*0x10+0x100).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern void* func_800AC0F0(void* pStream, s32 destinationX, s32 renderContext);
extern intptr_t func_800ABFDC(u8*,s32*);

s32 D_800AF780;
uint8_t g_PsxRam[4];

static unsigned s_checks;
static int s_move_count;
static int s_load_count;
static RECT s_move_rect;
static int s_move_dx;
static int s_move_dy;
static u16 s_glyph[0x10];
static const uint8_t *s_provider;
static int s_check_pixels;
static unsigned s_provider_calls;
static uint32_t s_expected_code=0x8140;
static s32 *s_flag;

const uint8_t *PcPortKromFont(uint32_t code,size_t length) {
    assert(code==s_expected_code && length==30);
    if(s_flag)assert(*s_flag==0);
    ++s_provider_calls;return s_provider;
}

long Krom2RawAdd(unsigned long code)
{
    (void)code;
    return (long)(uintptr_t)PcPortKromFont(code,30);
}

int MoveImage(RECT* rect, int x, int y)
{
    s_move_rect = *rect;
    s_move_dx = x;
    s_move_dy = y;
    s_move_count++;
    return 0;
}

int LoadImage(RECT* rect, u_long* p)
{
    (void)rect;
    if(s_check_pixels && s_load_count==0) {
        const u8 *bytes=(const u8*)p;
        for(unsigned i=0;i<288;++i) {
            u8 expected=i<270 && (i%18==7 || i%18==8)?255:0;
            assert(bytes[i]==expected);
        }
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
    fail("glyph.stream", detail);
}

int main(void)
{
    u8 stream[0x40];
    void* ret;

    memset(stream, 0, sizeof(stream));
    memset(s_glyph, 0, sizeof(s_glyph));

    /* remaining<=0: no glyphs, 0x1C empty LoadImages, pointer unchanged. */
    D_800AF780 = 0;
    s_load_count = 0;
    s_move_count = 0;
    ret = func_800AC0F0(stream, 0x300, 1);
    expect_eq_s32("empty.ret", (s32)((u8*)ret - stream), 0);
    expect_eq_s32("empty.remain", D_800AF780, 0);
    expect_eq_s32("empty.loads", s_load_count, 0x1C);
    expect_eq_s32("empty.moves", s_move_count, 0);

    /* 0x0D: consume 1 byte, fill 0x1C empties. */
    memset(stream, 0, sizeof(stream));
    stream[0] = 0x0D;
    D_800AF780 = 8;
    s_load_count = 0;
    ret = func_800AC0F0(stream, 0x300, 0);
    expect_eq_s32("cr.ret", (s32)((u8*)ret - stream), 1);
    expect_eq_s32("cr.remain", D_800AF780, 7);
    expect_eq_s32("cr.loads", s_load_count, 0x1C);

    /* sourceIndex==1, raw=8: /7 -> q=1 r=1; x=0x389 y=0x110. */
    memset(stream, 0, sizeof(stream));
    stream[0] = 0x85;
    stream[1] = 0x48;
    stream[2] = 0x0D;
    D_800AF780 = 8;
    s_load_count = 0;
    s_move_count = 0;
    ret = func_800AC0F0(stream, 0x300, 2);
    expect_eq_s32("tile.ret", (s32)((u8*)ret - stream), 3);
    expect_eq_s32("tile.remain", D_800AF780, 5);
    expect_eq_s32("tile.moves", s_move_count, 1);
    expect_eq_s32("tile.mx", s_move_rect.x, 0x389);
    expect_eq_s32("tile.my", s_move_rect.y, 0x110);
    expect_eq_s32("tile.mw", s_move_rect.w, 9);
    expect_eq_s32("tile.mh", s_move_rect.h, 0x10);
    expect_eq_s32("tile.dx", s_move_dx, 0x300);
    expect_eq_s32("tile.dy", s_move_dy, 0x20);
    expect_eq_s32("tile.loads", s_load_count, 0x1B);

    u16 high_glyph[16];
    for(unsigned i=0;i<16;++i)high_glyph[i]=0x8001;
    assert((uintptr_t)high_glyph>UINT32_MAX);
    s_provider=(const uint8_t*)high_glyph;
    memset(stream,0,sizeof stream);stream[0]=0x81;stream[1]=0x40;stream[2]=0x0d;
    D_800AF780=8;s_load_count=s_move_count=0;s_check_pixels=1;
    ret=func_800AC0F0(stream,0x300,1);
    expect_eq_s32("font.ret",(s32)((u8*)ret-stream),3);
    expect_eq_s32("font.loads",s_load_count,28);
    expect_eq_s32("font.moves",s_move_count,0);
    expect_eq_s32("font.calls",s_provider_calls,1);

    /* Retail ABFF8..AC028 selects exactly [8540,8880) as atlas indices.
     * Check transport for every code with both high host pointer and -1. */
    for(unsigned code=0;code<65536;++code)for(unsigned rejected=0;rejected<2;++rejected) {
        u8 encoded[]={(u8)(code>>8),(u8)code};s32 flag=99;
        s_provider=rejected?(const uint8_t*)(intptr_t)-1:(const uint8_t*)high_glyph;
        s_expected_code=code;s_flag=&flag;
        unsigned before=s_provider_calls;
        intptr_t raw=func_800ABFDC(encoded,&flag);
        if(code>=0x8540 && code<0x8880) {
            assert(flag==1 && raw==(intptr_t)(code-0x8540) && s_provider_calls==before);
        } else {
            assert(flag==0 && raw==(intptr_t)s_provider && s_provider_calls==before+1);
        }
        s_flag=NULL;++s_checks;
    }

    printf("FIELD GLYPH STREAM AC0F0 certificate PASS checks=%u\n", s_checks);
    return 0;
}
