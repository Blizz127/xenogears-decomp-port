/*
 * Retail certificate for func_800A74F8 (party slot 1/2 capture).
 *
 * Retail (func_800A74F8.s 0x800A74F8-0x800A7740): HeapAlloc 0x14000 into
 * D_8005A418 / D_8005A41C (not g_PartyDataBuffers). Mode 2 streams
 * skins[1..2]+5 and LZSS into those blocks; else StoreImage (0x200,0) and
 * (0x200,0x80).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "system/archive.h"

extern void func_800A74F8(void);

s32 D_800ADB74;
s32 g_GamePartyMemberSkins[3];
s32 g_GamePartyMembers[3];
void* D_8005A418;
void* D_8005A41C;
void* g_PartyStreamDataPointers[3];
void* g_PartyDataBuffers[3];

static u8 s_buf_a[0x40];
static u8 s_buf_b[0x40];
static u8 s_stream[0x40];
static u8 s_party[0x40];
static unsigned s_checks;
static int s_alloc_14000;
static int s_alloc_flags0;
static int s_pin_count;
static void* s_pin[4];
static int s_store_count;
static RECT s_store_rect[4];
static void* s_store_dst[4];
static int s_lzss_count;
static void* s_lzss_src;
static void* s_lzss_dst;
static int s_setindex_dir;
static int s_decode_index;
static int s_29afc;
static int s_sync;
static StreamDataQueueEntry s_reqs[4];

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    if (allocSize == 0x14000) {
        s_alloc_14000++;
        if (allocFlags == 0) {
            s_alloc_flags0++;
        }
        if (s_alloc_14000 == 1) {
            return s_buf_a;
        }
        if (s_alloc_14000 == 2) {
            return s_buf_b;
        }
    }
    if (allocFlags == 1) {
        return s_stream;
    }
    fprintf(stderr, "ASSERTION heap.size 0x%x flags %u\n", allocSize,
            allocFlags);
    exit(1);
}

void HeapFree(void* p)
{
    (void)p;
}

void HeapPinBlock(void* p)
{
    if (s_pin_count < 4) {
        s_pin[s_pin_count] = p;
    }
    s_pin_count++;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    (void)entryIndex;
    s_setindex_dir = directoryIndex;
    return 0;
}

int ArchiveDecodeAlignedSize(int entryIndex)
{
    s_decode_index = entryIndex;
    return (int)sizeof(s_stream);
}

s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)
{
    (void)index;
    (void)pBuffer;
    (void)arg2;
    (void)flags;
    return 0;
}

void ArchiveCdDataSync(int mode)
{
    (void)mode;
    s_sync++;
}

s32 func_80029AFC(StreamDataQueueEntry* entries, s32 arg1, s32 arg2)
{
    (void)arg1;
    (void)arg2;
    s_29afc++;
    s_reqs[0] = entries[0];
    s_reqs[1] = entries[1];
    return 0;
}

u32 LZSSDecompress(void* source, void* destination)
{
    s_lzss_src = source;
    s_lzss_dst = destination;
    s_lzss_count++;
    return 0;
}

int StoreImage(RECT* rect, u_long* p)
{
    if (s_store_count < 4) {
        s_store_rect[s_store_count] = *rect;
        s_store_dst[s_store_count] = p;
    }
    s_store_count++;
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
    fail("party.capture", detail);
}

static void expect_eq_ptr(const char* field, const void* actual,
                          const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s", field);
    fail("party.capture", detail);
}

static void reset_state(void)
{
    g_GamePartyMemberSkins[0] = 0xFF;
    g_GamePartyMemberSkins[1] = 0xFF;
    g_GamePartyMemberSkins[2] = 0xFF;
    g_GamePartyMembers[0] = 0xFF;
    g_GamePartyMembers[1] = 0xFF;
    g_GamePartyMembers[2] = 0xFF;
    memset(g_PartyStreamDataPointers, 0, sizeof(g_PartyStreamDataPointers));
    g_PartyDataBuffers[1] = s_party;
    g_PartyDataBuffers[2] = s_party;
    D_8005A418 = NULL;
    D_8005A41C = NULL;
    s_alloc_14000 = 0;
    s_alloc_flags0 = 0;
    s_pin_count = 0;
    s_store_count = 0;
    s_lzss_count = 0;
    s_setindex_dir = -1;
    s_decode_index = -1;
    s_29afc = 0;
    s_sync = 0;
}

int main(void)
{
    /* VRAM capture path (D_800ADB74 != 2). */
    reset_state();
    D_800ADB74 = 0;
    func_800A74F8();
    expect_eq_s32("vram.allocs", s_alloc_14000, 2);
    expect_eq_s32("vram.flags0", s_alloc_flags0, 2);
    expect_eq_ptr("vram.a418", D_8005A418, s_buf_a);
    expect_eq_ptr("vram.a41c", D_8005A41C, s_buf_b);
    expect_eq_s32("vram.pin", s_pin_count, 2);
    expect_eq_ptr("vram.pin0", s_pin[0], s_buf_a);
    expect_eq_s32("vram.stores", s_store_count, 2);
    expect_eq_s32("vram.r0.x", s_store_rect[0].x, 0x200);
    expect_eq_s32("vram.r0.y", s_store_rect[0].y, 0);
    expect_eq_s32("vram.r0.w", s_store_rect[0].w, 0x140);
    expect_eq_s32("vram.r0.h", s_store_rect[0].h, 0x80);
    expect_eq_ptr("vram.dst0", s_store_dst[0], s_buf_a);
    expect_eq_s32("vram.r1.y", s_store_rect[1].y, 0x80);
    expect_eq_ptr("vram.dst1", s_store_dst[1], s_buf_b);
    expect_eq_s32("vram.lzss", s_lzss_count, 0);

    /* Mode 2: stream skins[1]+5, LZSS into D_8005A418. */
    reset_state();
    D_800ADB74 = 2;
    g_GamePartyMemberSkins[1] = 7;
    g_GamePartyMemberSkins[2] = 0xFF;
    g_GamePartyMembers[1] = 3;
    g_GamePartyMembers[2] = 0xFF;
    func_800A74F8();
    expect_eq_s32("m2.setindex", s_setindex_dir, 4);
    expect_eq_s32("m2.decode", s_decode_index, 12);
    expect_eq_ptr("m2.stream1", g_PartyStreamDataPointers[1], s_stream);
    expect_eq_s32("m2.req0.idx", s_reqs[0].archiveIndex, 12);
    expect_eq_ptr("m2.req0.p", s_reqs[0].pData, s_stream);
    expect_eq_s32("m2.req1.idx", s_reqs[1].archiveIndex, 0);
    expect_eq_s32("m2.29afc", s_29afc, 1);
    expect_eq_s32("m2.sync", s_sync, 1);
    expect_eq_s32("m2.lzss", s_lzss_count, 1);
    expect_eq_ptr("m2.lzss.src", s_lzss_src, s_stream);
    expect_eq_ptr("m2.lzss.dst", s_lzss_dst, s_buf_a);
    expect_eq_ptr("m2.a418", D_8005A418, s_buf_a);

    printf("FIELD PARTY CAPTURE A74F8 certificate PASS checks=%u\n", s_checks);
    return 0;
}
