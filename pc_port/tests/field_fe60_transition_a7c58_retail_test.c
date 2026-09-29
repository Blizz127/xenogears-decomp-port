/*
 * Retail certificate for func_800A7C58 (FE60 title/field 2D transition).
 *
 * Retail (func_800A7C58.s 0x800A7C58-0x800A8310): archive 0xA9 LoadImage
 * RECT 0x140,0 x 0xC0,0x100; overlay HeapAlloc leftover a0=8 when
 * D_8004F370!=0 else (D_800ADB30&0xFFFFFF)+0xFFE2CFF8; first MoveImage
 * dest y is D_800ADB78<<8; both context isrgb24 bytes cleared.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "field/main.h"

extern void func_800A7C58(void);

s32 D_800ADB84;
/* Test-input drain called by misc5.c; the queue is always empty here. */
int ControllerPopState(void) { return 0; }
void ControllerResetState(void) {}
s32 D_800B00E4;
s32 D_800ADB78;
s32 D_800B06A0;
s32 D_800AFE74;
s32 D_800ADB74;
s32 D_800ADB7C;
s32 D_800ADB80;
s32 D_800ADB50;
s32 D_800ADB60;
s32 D_800ADB3C;
s32 D_800ADB38;
s32 D_800ADB6C;
s32 D_800B2264;
s32 D_800AFE84;
s32 D_8004F370;
s32 D_8004F300;
s32 D_801E89E0;
void* D_800ADB20;
void* D_800ADB30;
u16 D_800C3900;
s16 D_800C3A20;
s16 D_800C3A2A;
s16 D_800C3A2E;
s16 D_800C3A36;
s16 D_800C3A38;
s16 D_800C3A3A;
int g_FieldSystemMode;
RenderContext g_FieldRenderContexts[2];
RenderContext* g_FieldCurRenderContext;
uint8_t g_PsxRam[4];

static unsigned s_checks;
static u8 s_a9[0x40];
static u8 s_snap[0x40];
static u8 s_trans[0x100];
static u8 s_adb20[0x10];

static int s_alloc_count;
static u32 s_alloc_size[8];
static u32 s_alloc_flags[8];
static int s_free_count;
static void* s_free_ptr[8];
static int s_load_count;
static RECT s_load_rect;
static void* s_load_src;
static int s_store_count;
static RECT s_store_rect;
static void* s_store_dst;
static int s_move_count;
static RECT s_move_rect[4];
static int s_move_x[4];
static int s_move_y[4];
static int s_decode_a9;
static int s_setindex[8];
static int s_setindex_count;
static int s_seek;
static s32 s_seek_arg;
static int s_7394;
static int s_73e8;
static int s_708c;
static int s_7218;
static int s_732c;
static int s_732c_arg;
static int s_7948;
static int s_74f8;
static int s_convert;
static int s_e7fd4;
static int s_particles;
static int s_85788;
static int s_acc58;
static int s_sync_flush;
static int s_render_sync;
static int s_consol;
static int s_otag;
static int s_75910;
static int s_77dac;
static int s_7554c;
static int s_poll;
static int s_fade;
static s32 s_fade_vol;
static s32 s_fade_frames;
static int s_vsync;
static int s_43b0;
static int s_77884;
static int s_77ab4;
static int s_70488;
static int s_70508;
static int s_85738;
static int s_user;
static u_int s_user_tag;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    if (s_alloc_count < 8) {
        s_alloc_size[s_alloc_count] = allocSize;
        s_alloc_flags[s_alloc_count] = allocFlags;
    }
    s_alloc_count++;
    if (allocFlags == 1) {
        return s_trans;
    }
    if (allocSize == 0x18000) {
        return s_snap;
    }
    return s_a9;
}

void HeapFree(void* p)
{
    if (s_free_count < 8) {
        s_free_ptr[s_free_count] = p;
    }
    s_free_count++;
}

void HeapConsolidate(void)
{
    s_consol++;
}

void HeapChangeCurrentUser(u_int userTag, char** pContentTypes)
{
    (void)pContentTypes;
    s_user++;
    s_user_tag = userTag;
}

int ArchiveDecodeAlignedSize(int entryIndex)
{
    if (entryIndex == 0xA9) {
        s_decode_a9++;
        return 0x20;
    }
    return 0x10;
}

s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)
{
    (void)index;
    (void)pBuffer;
    (void)arg2;
    (void)flags;
    return 0;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    (void)entryIndex;
    if (s_setindex_count < 8) {
        s_setindex[s_setindex_count] = directoryIndex;
    }
    s_setindex_count++;
    return 0;
}

void ArchiveCdSeekOrPause(s32 entryIndex)
{
    s_seek++;
    s_seek_arg = entryIndex;
}

void func_8002A2D0(s32 entryIndex)
{
    ArchiveCdSeekOrPause(entryIndex);
}

int LoadImage(RECT* rect, u_long* p)
{
    s_load_count++;
    s_load_rect = *rect;
    s_load_src = p;
    return 0;
}

int StoreImage(RECT* rect, u_long* p)
{
    s_store_count++;
    s_store_rect = *rect;
    s_store_dst = p;
    return 0;
}

int MoveImage(RECT* rect, int x, int y)
{
    if (s_move_count < 4) {
        s_move_rect[s_move_count] = *rect;
        s_move_x[s_move_count] = x;
        s_move_y[s_move_count] = y;
    }
    s_move_count++;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

int Vsync(int mode)
{
    (void)mode;
    s_vsync++;
    return 0;
}

DISPENV* PutDispEnv(DISPENV* env)
{
    return env;
}

DRAWENV* PutDrawEnv(DRAWENV* env)
{
    return env;
}

void func_800A7394(void)
{
    s_7394++;
}

void func_800A73E8(void)
{
    s_73e8++;
}

void func_800A708C(void)
{
    s_708c++;
}

void func_800A7218(void)
{
    s_7218++;
}

void func_800A732C(s32 count)
{
    s_732c++;
    s_732c_arg = count;
    D_800B00E4 = 0;
    D_800ADB78 = 1;
}

void func_800A7948(void)
{
    s_7948++;
}

void func_800A74F8(void)
{
    s_74f8++;
}

void FieldImageConvert24BitTo15Bit(void)
{
    s_convert++;
}

void func_801E7FD4(void)
{
    s_e7fd4++;
}

void FieldParticlesFreeAll(void)
{
    s_particles++;
}

void func_80085788(void)
{
    s_85788++;
}

void func_800ACC58(void)
{
    s_acc58++;
}

void FieldRenderSyncAndFlush(void)
{
    s_sync_flush++;
}

void FieldRenderSync(void)
{
    s_render_sync++;
}

void FieldClearAndSwapOTagInternal(void)
{
    s_otag++;
}

void func_80075910(void)
{
    s_75910++;
}

void func_80077DAC(void)
{
    s_77dac++;
}

void func_8007554C(void)
{
    s_7554c++;
}

void FieldPollControllers(void)
{
    s_poll++;
}

void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames)
{
    s_fade++;
    s_fade_vol = targetVolume;
    s_fade_frames = fadeFrames;
}

void func_801D43B0(void)
{
    s_43b0++;
}

void func_80077884(void)
{
    s_77884++;
}

void func_80077AB4(void)
{
    s_77ab4++;
}

void func_80070488(void)
{
    s_70488++;
}

void func_80070508(void)
{
    s_70508++;
}

void func_80085738(void)
{
    s_85738++;
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
    fail("fe60.trans", detail);
}

static void expect_eq_ptr(const char* field, const void* actual,
                          const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
             field, actual, expected);
    fail("fe60.trans", detail);
}

static void reset_state(void)
{
    memset(g_FieldRenderContexts, 0, sizeof(g_FieldRenderContexts));
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 1;
    g_FieldCurRenderContext = &g_FieldRenderContexts[0];
    D_800ADB84 = 0;
    D_800B00E4 = 0xFF;
    D_800ADB78 = 0xFF;
    D_800B06A0 = 0;
    D_800AFE74 = 0xFF;
    D_800ADB74 = 0;
    D_800ADB7C = 0;
    D_800ADB80 = 0;
    D_800ADB50 = 0xFF;
    D_800ADB60 = 0xFF;
    D_800ADB3C = 0;
    D_800ADB38 = 0;
    D_800ADB6C = 0;
    D_800B2264 = 0;
    D_800AFE84 = 0;
    D_8004F370 = 1;
    D_800ADB20 = s_adb20;
    D_800ADB30 = (void*)(uintptr_t)0x801E0000u;
    D_800C3900 = 0;
    D_800C3A20 = 7;
    D_800C3A2A = 0;
    D_800C3A2E = 0;
    D_800C3A36 = 0;
    D_800C3A38 = 0;
    D_800C3A3A = 0;
    g_FieldSystemMode = 0;
    s_alloc_count = 0;
    s_free_count = 0;
    s_load_count = 0;
    s_store_count = 0;
    s_move_count = 0;
    s_decode_a9 = 0;
    s_setindex_count = 0;
    s_seek = 0;
    s_seek_arg = 0;
    s_7394 = 0;
    s_73e8 = 0;
    s_708c = 0;
    s_7218 = 0;
    s_732c = 0;
    s_7948 = 0;
    s_74f8 = 0;
    s_convert = 0;
    s_e7fd4 = 0;
    s_particles = 0;
    s_85788 = 0;
    s_acc58 = 0;
    s_sync_flush = 0;
    s_render_sync = 0;
    s_consol = 0;
    s_otag = 0;
    s_75910 = 0;
    s_77dac = 0;
    s_7554c = 0;
    s_poll = 0;
    s_fade = 0;
    s_fade_vol = -1;
    s_fade_frames = -1;
    s_vsync = 0;
    s_43b0 = 0;
    s_77884 = 0;
    s_77ab4 = 0;
    s_70488 = 0;
    s_70508 = 0;
    s_85738 = 0;
    s_user = 0;
    s_user_tag = 0;
}

int main(void)
{
    int i;
    u32 overlaySize;

    /* Mode 0, leftover HeapAlloc(8,1), frame-budget exit. */
    reset_state();
    D_800ADB74 = 0;
    D_8004F370 = 1;
    func_800A7C58();
    expect_eq_s32("m0.load", s_load_count, 1);
    expect_eq_s32("m0.load.x", s_load_rect.x, 0x140);
    expect_eq_s32("m0.load.y", s_load_rect.y, 0);
    expect_eq_s32("m0.load.w", s_load_rect.w, 0xC0);
    expect_eq_s32("m0.load.h", s_load_rect.h, 0x100);
    expect_eq_ptr("m0.load.src", s_load_src, s_a9);
    expect_eq_s32("m0.store", s_store_count, 1);
    expect_eq_s32("m0.store.w", s_store_rect.w, 0xC0);
    expect_eq_s32("m0.store.h", s_store_rect.h, 0x100);
    expect_eq_ptr("m0.store.dst", s_store_dst, s_snap);
    expect_eq_s32("m0.seek", s_seek, 1);
    expect_eq_s32("m0.seek.arg", s_seek_arg, 7);
    expect_eq_s32("m0.set0", s_setindex[0], 0x18);
    expect_eq_s32("m0.adb7c", D_800ADB7C, 1);
    expect_eq_s32("m0.7948", s_7948, 1);
    expect_eq_s32("m0.move", s_move_count, 2);
    expect_eq_s32("m0.move0.y", s_move_rect[0].y, 0x100);
    expect_eq_s32("m0.move0.w", s_move_rect[0].w, 0x1E0);
    expect_eq_s32("m0.move0.h", s_move_rect[0].h, 0xE0);
    expect_eq_s32("m0.move0.dstx", s_move_x[0], 0);
    expect_eq_s32("m0.move0.dsty", s_move_y[0], 0x100);
    expect_eq_s32("m0.move1.y", s_move_rect[1].y, 0x100);
    expect_eq_s32("m0.move1.w", s_move_rect[1].w, 0x140);
    expect_eq_s32("m0.move1.dsty", s_move_y[1], 0);
    expect_eq_s32("m0.rgb0", g_FieldRenderContexts[0].dispEnv.isrgb24, 0);
    expect_eq_s32("m0.rgb1", g_FieldRenderContexts[1].dispEnv.isrgb24, 0);
    expect_eq_s32("m0.convert", s_convert, 1);
    expect_eq_s32("m0.fade", D_800C3A38, 0xFF);
    expect_eq_s32("m0.mode", D_800ADB74, 0);
    expect_eq_s32("m0.adb6c", D_800ADB6C, -1);
    expect_eq_s32("m0.adb3c", D_800ADB3C, 0x20);
    expect_eq_s32("m0.adb38", D_800ADB38, 1);
    expect_eq_s32("m0.adb50", D_800ADB50, 0);
    expect_eq_s32("m0.user", (s32)s_user_tag, 8);
    expect_eq_s32("m0.particles", s_particles, 1);

    overlaySize = 0;
    for (i = 0; i < s_alloc_count && i < 8; i++) {
        if (s_alloc_flags[i] == 1) {
            overlaySize = s_alloc_size[i];
        }
    }
    expect_eq_s32("m0.overlay", (s32)overlaySize, 8);

    /* Mode 2: skip LoadImage and 24-bit convert. */
    reset_state();
    D_800ADB74 = 2;
    D_800AFE84 = 1;
    func_800A7C58();
    expect_eq_s32("m2.load", s_load_count, 0);
    expect_eq_s32("m2.77dac", s_77dac, 1);
    expect_eq_s32("m2.7554c", s_7554c, 1);
    expect_eq_s32("m2.732c", s_732c_arg, 9);
    expect_eq_s32("m2.convert", s_convert, 0);
    expect_eq_s32("m2.70488", s_70488, 0);
    expect_eq_s32("m2.adb50", D_800ADB50, 1);
    expect_eq_s32("m2.mode", D_800ADB74, 0);

    /* Circle skip: system mode, flag 0x80, button 0x20. */
    reset_state();
    D_800ADB74 = 0;
    D_800C3A3A = 1;
    g_FieldSystemMode = 1;
    D_800ADB80 = 0x80;
    D_800C3900 = 0x20;
    func_800A7C58();
    expect_eq_s32("cir.poll", s_poll, 1);
    expect_eq_s32("cir.fade", s_fade, 1);
    expect_eq_s32("cir.vol", s_fade_vol, 0);
    expect_eq_s32("cir.frames", s_fade_frames, 10);
    expect_eq_s32("cir.convert", s_convert, 1);

    /* Overlay formula when D_8004F370==0: 0x801E0000 -> 0xCFF8. */
    reset_state();
    D_800ADB74 = 0;
    D_8004F370 = 0;
    D_800ADB30 = (void*)(uintptr_t)0x801E0000u;
    func_800A7C58();
    overlaySize = 0;
    for (i = 0; i < s_alloc_count && i < 8; i++) {
        if (s_alloc_flags[i] == 1) {
            overlaySize = s_alloc_size[i];
        }
    }
    expect_eq_s32("ovl.size", (s32)overlaySize, 0xCFF8);
    expect_eq_s32("ovl.decode", s_decode_a9, 2);

    /* D_800B2264 teardown of D_800ADB20. */
    reset_state();
    D_800B2264 = 1;
    func_800A7C58();
    expect_eq_s32("b2264.e7fd4", s_e7fd4, 1);
    expect_eq_s32("b2264.rsync", s_render_sync, 1);
    expect_eq_ptr("b2264.free0", s_free_ptr[0], s_adb20);

    printf("FIELD FE60 TRANSITION A7C58 certificate PASS checks=%u\n",
           s_checks);
    return 0;
}
