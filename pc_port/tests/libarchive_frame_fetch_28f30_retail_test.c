/*
 * Retail certificate for func_80028F30 (debug-table streamed frame fetch).
 *
 * Retail (func_80028F30.s 0x80028F30-0x8002945C, frame 0x50):
 *   stream NULL -> 1 and both out-params untouched.
 *   All other paths clear *ppData and then scan the D_8004FE2C ring for
 *   state==3 && id==D_8004FE24; a miss (or index == D_8004FE40) returns 1,
 *   a hit publishes *ppData = sector buffer and
 *   *ppSection = sector + u16(+6)*0x20, then requires
 *   func_80028E60(index, u16(+6), 3) == 0 to return 0 and add u16(+6) to
 *   D_8004FE24 (otherwise 1, with *ppData already published).
 *   Debug section fetch (g_ArchiveDebugTable != 0, hFile != -1,
 *   g_ArchiveCurFileSize > 0) with D_80059F60 == 0 walks the ring from 0
 *   adding slot sizes until NOT_LOADED / D_8004FE40 and reports nothing when
 *   there is no room; in CD mode (D_80059F18[0] & 8) it first reads an 8-byte
 *   D_800596F8 probe whose first byte == 1 means "seek 0x918 and report
 *   nothing". It then reads 0x20 bytes of section header into
 *   D_8004FE08 + slot*0x800 (saved to D_80059F54), latches D_80059F5C = u16(+6)
 *   and D_8005A4B8 = u16(+8), and when the slot size u16(+4) is smaller than
 *   D_80059F5C seeks back (-0x28 in CD mode, else -0x20) and reports nothing.
 *   Otherwise the slot is marked (state 3, id D_8004FE26) and split when the
 *   remainder >= 3 (size = D_80059F5C + 1, D_80059F5C*8 sub-fields zeroed,
 *   remainder stored, ArchiveConsolidateStreamFileEntry(D_80059F5C + 1)), the
 *   0x7E0 payload is read into D_80059F58 = sector + D_80059F5C*0x20, an
 *   optional 0x118 seek runs in CD mode, and D_8004FE10 = slot,
 *   D_80059F60++, g_ArchiveCurFileSize -= 0x800.
 *   With D_80059F60 != 0 the header read goes to
 *   D_80059F54 + D_80059F60*0x20 and the payload to
 *   D_80059F58 + D_80059F60*0x7E0, slot D_8004FE10 + 1 is marked,
 *   g_ArchiveCurFileSize -= 0x800, D_80059F60++ (wrapping to 0 at
 *   D_80059F5C).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"

extern int func_80028F30(u32* ppSection, u32* ppData);

static u8 s_stream[0x8000];

void* g_ArchiveCurStreamFile;
u32 g_ArchiveDebugTable;
int g_ArchiveCurFileSize;
int D_8004FE40;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE4C;
s32 D_8004FE10;
u16 D_8004FE24;
u16 D_8004FE26;
s8* D_8004FE08;
u_char D_80059F18[4];
u8* D_80059F54;
u8* D_80059F58;
s16 D_80059F5C;
s16 D_80059F60;
u16 D_8005A4B8;
u8 D_800596F8[8];

static unsigned s_checks;
static int s_pcread;
static int s_pcread_fd;
static void* s_pcread_buf;
static int s_pcread_len;
static int s_probe_byte;
static int s_probe_len;
static void* s_probe_buf;
static void* s_read20_buf;
static int s_read20_len;
static u16 s_hdr_size;
static u16 s_hdr_frame;
static u16 s_hdr_x;
static int s_pcseek;
static int s_pcseek_fd;
static int s_pcseek_off;
static int s_pcseek_mode;

int PCread(int fd, char* buff, int len)
{
    s_pcread++;
    s_pcread_fd = fd;
    s_pcread_buf = buff;
    s_pcread_len = len;
    if (len == 8) {
        s_probe_len = len;
        s_probe_buf = buff;
        *(u8*)buff = (u8)s_probe_byte;
    } else if (len == 0x20) {
        s_read20_buf = buff;
        s_read20_len = len;
        *(u16*)(buff + 4) = s_hdr_size;
        *(u16*)(buff + 6) = s_hdr_frame;
        *(u16*)(buff + 8) = s_hdr_x;
    }
    return 1;
}

int PClseek(int fd, int offset, int mode)
{
    s_pcseek++;
    s_pcseek_fd = fd;
    s_pcseek_off = offset;
    s_pcseek_mode = mode;
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
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
                 field, (int)actual, (int)expected);
        fail("libarchive.frame", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libarchive.frame", detail);
    }
}

static u16* slot(int i)
{
    return (u16*)(s_stream + 4 + (i << 3));
}

static u8* slotbuf(int i)
{
    return (u8*)(s_stream + (*(s32*)s_stream * 8) + 0x24 + (i << 11));
}

static void reset_state(void)
{
    memset(s_stream, 0, sizeof(s_stream));
    *(s32*)s_stream = 4;

    g_ArchiveCurStreamFile = s_stream;
    g_ArchiveDebugTable = 0;
    g_ArchiveCurFileSize = 0;
    D_8004FE40 = 4;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)(s_stream + 4);
    D_8004FE4C = -1;
    D_8004FE10 = 0;
    D_8004FE24 = 0;
    D_8004FE26 = 0;
    D_8004FE08 = (s8*)((u8*)s_stream + (*(s32*)s_stream * 8) + 0x24);
    memset(D_80059F18, 0, sizeof(D_80059F18));
    D_80059F54 = NULL;
    D_80059F58 = NULL;
    D_80059F5C = 0;
    D_80059F60 = 0;
    D_8005A4B8 = 0;
    memset(D_800596F8, 0, sizeof(D_800596F8));

    s_pcread = 0;
    s_pcread_fd = 0;
    s_pcread_buf = NULL;
    s_pcread_len = 0;
    s_probe_byte = 0;
    s_probe_len = 0;
    s_probe_buf = NULL;
    s_read20_buf = NULL;
    s_read20_len = 0;
    s_hdr_size = 0;
    s_hdr_frame = 0;
    s_hdr_x = 0;
    s_pcseek = 0;
    s_pcseek_fd = 0;
    s_pcseek_off = 0;
    s_pcseek_mode = 0;
}

int main(void)
{
    u32 section = 0x5A5A5A5A;
    u32 data = 0x5A5A5A5A;
    int rc;

    /* NULL stream: both out-params untouched. */
    reset_state();
    g_ArchiveCurStreamFile = NULL;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("null.rc", rc, 1);
    expect_eq_s32("null.section", (s32)section, 0x5A5A5A5A);
    expect_eq_s32("null.data", (s32)data, 0x5A5A5A5A);

    /* CD path, no completed slot. */
    reset_state();
    slot(0)[0] = 0;
    slot(1)[0] = 3;
    slot(1)[1] = 0x99;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("cd.miss.rc", rc, 1);
    expect_eq_s32("cd.miss.data", (s32)data, 0);
    expect_eq_s32("cd.miss.section", (s32)section, 0x5A5A5A5A);

    /* CD path, completed slot: publishes sector/section and advances the id. */
    reset_state();
    slot(1)[0] = 3;
    slot(1)[1] = 0;
    slot(1)[2] = 2; /* size/count at +4 */
    slot(2)[0] = 3;
    slot(2)[1] = 0x40;
    *(u16*)(slotbuf(1) + 6) = 2;
    D_8004FE24 = 0;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("cd.hit.rc", rc, 0);
    expect_eq_ptr("cd.hit.data", (void*)(uintptr_t)data, slotbuf(1));
    expect_eq_ptr("cd.hit.section", (void*)(uintptr_t)section, slotbuf(1) + 0x40);
    expect_eq_s32("cd.hit.id", D_8004FE24, 2);
    expect_eq_s32("cd.hit.read", s_pcread, 0);

    /* CD path: func_80028E60 rejects the run, id stays and rc is 1. */
    reset_state();
    slot(1)[0] = 3;
    slot(1)[1] = 0;
    slot(1)[2] = 2;
    slot(2)[0] = 0;
    *(u16*)(slotbuf(1) + 6) = 2;
    D_8004FE24 = 0;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("cd.reject.rc", rc, 1);
    expect_eq_ptr("cd.reject.data", (void*)(uintptr_t)data, slotbuf(1));
    expect_eq_s32("cd.reject.id", D_8004FE24, 0);
    expect_eq_ptr("cd.reject.section", (void*)(uintptr_t)section, slotbuf(1) + 0x40);

    /* Debug path without a file handle falls back to the CD behaviour. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = -1;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.nofile.rc", rc, 1);
    expect_eq_s32("dbg.nofile.data", (s32)data, 0);
    expect_eq_s32("dbg.nofile.read", s_pcread, 0);

    /* New section, no room in the ring. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = 0x33;
    slot(0)[0] = 3;
    slot(0)[1] = 0x77;
    slot(0)[2] = 4;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.noroom.rc", rc, 1);
    expect_eq_s32("dbg.noroom.read", s_pcread, 0);

    /* New section, slot smaller than the header count: seek back, no data. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = 0x33;
    slot(0)[0] = 0;
    slot(0)[2] = 2;
    s_hdr_frame = 5;
    s_hdr_x = 7;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.short.rc", rc, 1);
    expect_eq_s32("dbg.short.reads", s_pcread, 1);
    expect_eq_s32("dbg.short.len", s_pcread_len, 0x20);
    expect_eq_ptr("dbg.short.buf", s_pcread_buf, slotbuf(0));
    expect_eq_s32("dbg.short.f54", (s32)(uintptr_t)D_80059F54, (s32)(uintptr_t)slotbuf(0));
    expect_eq_s32("dbg.short.f5c", D_80059F5C, 5);
    expect_eq_s32("dbg.short.a4b8", D_8005A4B8, 7);
    expect_eq_s32("dbg.short.seeks", s_pcseek, 1);
    expect_eq_s32("dbg.short.seek.off", s_pcseek_off, -0x20);
    expect_eq_s32("dbg.short.seek.mode", s_pcseek_mode, SEEK_CUR);
    expect_eq_s32("dbg.short.data", (s32)data, 0);
    expect_eq_s32("dbg.short.state", slot(0)[0], 0);

    /* CD mode short section: probe then -0x28 seek. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = 0x33;
    D_80059F18[0] = 8;
    slot(0)[0] = 0;
    slot(0)[2] = 2;
    s_hdr_frame = 5;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.cd.rc", rc, 1);
    expect_eq_s32("dbg.cd.reads", s_pcread, 2);
    expect_eq_s32("dbg.cd.probe.len", s_probe_len, 8);
    expect_eq_ptr("dbg.cd.probe.buf", s_probe_buf, D_800596F8);
    expect_eq_s32("dbg.cd.seek.off", s_pcseek_off, -0x28);

    /* CD mode probe byte 1: seek 0x918 and report nothing. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = 0x33;
    D_80059F18[0] = 8;
    s_probe_byte = 1;
    slot(0)[0] = 0;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.probe.rc", rc, 1);
    expect_eq_s32("dbg.probe.reads", s_pcread, 1);
    expect_eq_s32("dbg.probe.seek", s_pcseek, 1);
    expect_eq_s32("dbg.probe.seek.off", s_pcseek_off, 0x918);
    expect_eq_s32("dbg.probe.seek.mode", s_pcseek_mode, SEEK_CUR);

    /* New section end to end: mark, split, payload, then the ring scan. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE4C = 0x33;
    slot(0)[0] = 0;
    slot(0)[2] = 0x10;
    s_hdr_frame = 1;
    s_hdr_x = 2;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.new.rc", rc, 0);
    expect_eq_ptr("dbg.new.data", (void*)(uintptr_t)data, slotbuf(0));
    expect_eq_ptr("dbg.new.section", (void*)(uintptr_t)section, slotbuf(0) + 0x20);
    expect_eq_s32("dbg.new.f54", (s32)(uintptr_t)D_80059F54, (s32)(uintptr_t)slotbuf(0));
    expect_eq_s32("dbg.new.f58", (s32)(uintptr_t)D_80059F58, (s32)(uintptr_t)(slotbuf(0) + 0x20));
    expect_eq_s32("dbg.new.f5c", D_80059F5C, 1);
    expect_eq_s32("dbg.new.a4b8", D_8005A4B8, 2);
    expect_eq_s32("dbg.new.state", slot(0)[0], 3);
    expect_eq_s32("dbg.new.id", slot(0)[1], 0);
    expect_eq_s32("dbg.new.size", slot(0)[2], 2);
    expect_eq_s32("dbg.new.sub", slot(0)[8], 0);
    expect_eq_s32("dbg.new.rest", slot(0)[10], 0xE);
    expect_eq_s32("dbg.new.f26", D_8004FE26, 1);
    expect_eq_s32("dbg.new.f10", D_8004FE10, 0);
    expect_eq_s32("dbg.new.f60", D_80059F60, 1);
    expect_eq_s32("dbg.new.size.left", g_ArchiveCurFileSize, 0x1000);
    expect_eq_s32("dbg.new.reads", s_pcread, 2);
    expect_eq_s32("dbg.new.read1.len", s_pcread_len, 0x7E0);
    expect_eq_s32("dbg.new.seeks", s_pcseek, 0);
    expect_eq_s32("dbg.new.id.adv", D_8004FE24, 1);

    /* Continuation path: header + payload move with D_80059F60. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x2000;
    D_8004FE4C = 0x33;
    D_80059F60 = 1;
    D_80059F5C = 4;
    D_8004FE10 = 0;
    D_80059F54 = slotbuf(0);
    D_80059F58 = slotbuf(0) + 0x20;
    slot(1)[0] = 0;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.next.rc", rc, 0);
    expect_eq_s32("dbg.next.state", slot(1)[0], 3);
    expect_eq_s32("dbg.next.id", slot(1)[1], 0);
    expect_eq_s32("dbg.next.f26", D_8004FE26, 1);
    expect_eq_s32("dbg.next.f10", D_8004FE10, 1);
    expect_eq_s32("dbg.next.f60", D_80059F60, 2);
    expect_eq_s32("dbg.next.size.left", g_ArchiveCurFileSize, 0x1800);
    expect_eq_s32("dbg.next.reads", s_pcread, 2);
    expect_eq_s32("dbg.next.read1.len", s_read20_len, 0x20);
    expect_eq_ptr("dbg.next.read1.buf", s_read20_buf, slotbuf(0) + 0x20);
    expect_eq_s32("dbg.next.read2.len", s_pcread_len, 0x7E0);
    expect_eq_ptr("dbg.next.read2.buf", s_pcread_buf, slotbuf(0) + 0x20 + 0x7E0);
    expect_eq_ptr("dbg.next.data", (void*)(uintptr_t)data, slotbuf(1));

    /* Continuation wrap: D_80059F60 resets once it reaches D_80059F5C. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x2000;
    D_8004FE4C = 0x33;
    D_80059F60 = 3;
    D_80059F5C = 4;
    D_8004FE10 = 0;
    D_80059F54 = slotbuf(0);
    D_80059F58 = slotbuf(0) + 0x20;
    slot(1)[0] = 0;
    rc = func_80028F30(&section, &data);
    expect_eq_s32("dbg.wrap.rc", rc, 0);
    expect_eq_s32("dbg.wrap.f60", D_80059F60, 0);
    expect_eq_s32("dbg.wrap.f10", D_8004FE10, 1);
    expect_eq_s32("dbg.wrap.size.left", g_ArchiveCurFileSize, 0x1800);
    expect_eq_ptr("dbg.wrap.data", (void*)(uintptr_t)data, slotbuf(1));

    printf("LIBARCHIVE FRAME FETCH 28F30 certificate PASS checks=%u\n", s_checks);
    return 0;
}
