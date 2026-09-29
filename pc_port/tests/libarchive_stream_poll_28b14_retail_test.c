/*
 * Retail certificate for func_80028B14 (streaming sector poll).
 *
 * Retail (func_80028B14.s 0x80028B14-0x80028E60, frame 0x38):
 *   g_ArchiveCurStreamFile NULL -> 0; buffers = stream + n*8 + 0x24 where
 *   n = *(s32*)stream.
 *   g_ArchiveDebugTable == 0 (CD path): scan the D_8004FE2C ring for
 *   state==3 && id==D_8004FE24, bump D_8004FE24 and return that slot's
 *   +slot*0x800 buffer. The scan runs n times but the "no match" test compares
 *   the counter against D_8004FE40, not n.
 *   debug path: D_8004FE4C == -1 or g_ArchiveCurFileSize <= 0 -> 0. Otherwise
 *   rotate D_8004FE10 over the D_8004FE40 ring slots (wrapping to 0) until the
 *   slot state is ARCHIVE_STREAM_FILE_NOT_LOADED; if the rotation ends the
 *   walked slot is loaded -> 0. Mark the slot state 3 and PCread(0x800) into
 *   its sector buffer with up to three func_8002804C(i,0,0xFF,0) retries
 *   (all four failures -> 0). g_ArchiveCurFileSize -= 0x800; > 0 returns the
 *   sector. Otherwise g_ArchiveCurFileSize = 0, PCclose with up to three
 *   func_8002804C(i,0,0,0xFF) retries, then D_8004FE4C = -1 and the next
 *   D_8004FE0C queue entry (index D_8004FE10 + 1): its archiveIndex goes to
 *   D_80059F0C, its pData must be non-NULL, ArchiveGetFilePath + PCopen retries
 *   (up to three func_8002804C(i,0xFF,0,0)) then
 *   g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(next) and D_8004FDFC--.
 *   An empty/absent next entry (index 0 or NULL pData) or D_8004FE0C == 0
 *   clears g_ArchiveCurFileSize and D_8004FDFC. Every path returns the sector
 *   it read except the failure paths, which return 0.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

extern s32 func_80028B14(void);

void* g_ArchiveCurStreamFile;
u32 g_ArchiveDebugTable;
int g_ArchiveCurFileSize;
int D_8004FE40;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE4C;
s32 D_8004FE0C;
s32 D_8004FE10;
u16 D_8004FE24;
u32 D_80059F0C;
s32 D_8004FDFC;

static u8 s_stream[0x8000];
/* The retail queue entries are 8 bytes (u16 index at +0, 32-bit pData at +4);
 * the host struct is 16 bytes, so emulate the retail layout with raw bytes. */
static u8 s_queueBuf[4 * 8];

static unsigned s_checks;
static int s_pcread;
static int s_pcread_fd;
static void* s_pcread_buf;
static int s_pcread_len;
static int s_pcread_ret;
static int s_pcopen;
static const char* s_pcopen_name;
static int s_pcopen_ret;
static int s_pcclose;
static int s_pcclose_idx;
static int s_pcclose_seq[4];
static int s_retry;
static s32 s_retry_a;
static s32 s_retry_b;
static s32 s_retry_c;
static s32 s_retry_d;
static int s_decode_aligned;
static s32 s_decode_aligned_in;
static int s_decode_aligned_ret;
static int s_filepath;
static s32 s_filepath_in;

int ArchiveDecodeSizeAligned(int entryIndex)
{
    s_decode_aligned++;
    s_decode_aligned_in = entryIndex;
    return s_decode_aligned_ret;
}

char* ArchiveGetFilePath(int entryIndex)
{
    s_filepath++;
    s_filepath_in = entryIndex;
    return "next.bin";
}

int PCread(int fd, char* buff, int len)
{
    s_pcread++;
    s_pcread_fd = fd;
    s_pcread_buf = buff;
    s_pcread_len = len;
    return s_pcread_ret;
}

int PCopen(char* name, int flags, int perms)
{
    (void)flags;
    (void)perms;
    s_pcopen++;
    s_pcopen_name = name;
    return s_pcopen_ret;
}

int PCclose(int fd)
{
    (void)fd;
    s_pcclose++;
    return s_pcclose_seq[s_pcclose_idx++];
}

void func_8002804C(s32 a, s32 b, s32 c, s32 d)
{
    s_retry++;
    s_retry_a = a;
    s_retry_b = b;
    s_retry_c = c;
    s_retry_d = d;
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
        fail("libarchive.poll", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("libarchive.poll", detail);
    }
}

/* Ring slot accessors: slots live at stream+4, sector buffers at stream+n*8+0x24. */
static u16* slot(int i)
{
    return (u16*)(s_stream + 4 + (i << 3));
}

static u8* slotbuf(int i)
{
    return s_stream + (*(s32*)s_stream * 8) + 0x24 + (i << 11);
}

static void queue_entry(int i, u16 index, const void* pData)
{
    *(u16*)(s_queueBuf + (i * 8)) = index;
    *(u32*)(s_queueBuf + (i * 8) + 4) = (u32)(uintptr_t)pData;
}

static void reset_state(void)
{
    memset(s_stream, 0, sizeof(s_stream));
    memset(s_queueBuf, 0, sizeof(s_queueBuf));
    *(s32*)s_stream = 4;

    g_ArchiveCurStreamFile = s_stream;
    g_ArchiveDebugTable = 0;
    g_ArchiveCurFileSize = 0;
    D_8004FE40 = 4;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)(s_stream + 4);
    D_8004FE4C = 0x11;
    D_8004FE0C = 0;
    D_8004FE10 = 0;
    D_8004FE24 = 0;
    D_80059F0C = 0;
    D_8004FDFC = 0;

    s_pcread = 0;
    s_pcread_fd = 0;
    s_pcread_buf = NULL;
    s_pcread_len = 0;
    s_pcread_ret = 1;
    s_pcopen = 0;
    s_pcopen_name = NULL;
    s_pcopen_ret = 0x22;
    s_pcclose = 0;
    s_pcclose_idx = 0;
    memset(s_pcclose_seq, 0, sizeof(s_pcclose_seq));
    s_retry = 0;
    s_retry_a = 0;
    s_retry_b = 0;
    s_retry_c = 0;
    s_retry_d = 0;
    s_decode_aligned = 0;
    s_decode_aligned_in = 0;
    s_decode_aligned_ret = 0x1000;
    s_filepath = 0;
    s_filepath_in = 0;
}

int main(void)
{
    s32 rc;

    /* NULL stream. */
    reset_state();
    g_ArchiveCurStreamFile = NULL;
    rc = func_80028B14();
    expect_eq_s32("null.rc", rc, 0);

    /* CD path: matching slot state/id. */
    reset_state();
    D_8004FE24 = 0x10;
    slot(0)[0] = 2;
    slot(1)[0] = 3;
    slot(1)[1] = 0x10;
    slot(2)[0] = 3;
    slot(2)[1] = 0x11;
    rc = func_80028B14();
    expect_eq_ptr("cd.slot1", (void*)(uintptr_t)rc, slotbuf(1));
    expect_eq_s32("cd.id.bump", D_8004FE24, 0x11);
    expect_eq_s32("cd.slot0.keep", slot(0)[0], 2);
    expect_eq_s32("cd.slot2.keep", slot(2)[0], 3);

    /* CD path: no slot tagged with the current id -> 0, id unchanged. */
    reset_state();
    D_8004FE24 = 0x10;
    slot(0)[0] = 3;
    slot(0)[1] = 0x20;
    slot(1)[0] = 3;
    slot(1)[1] = 0x21;
    slot(2)[0] = 1;
    slot(3)[0] = 0;
    rc = func_80028B14();
    expect_eq_s32("cd.miss.rc", rc, 0);
    expect_eq_s32("cd.miss.id", D_8004FE24, 0x10);

    /* The scan is bounded by the stream's slot count while the "no match"
     * test still compares against D_8004FE40 (retail quirk). */
    reset_state();
    *(s32*)s_stream = 2;
    D_8004FE40 = 3;
    D_8004FE24 = 0x10;
    slot(0)[0] = 3;
    slot(0)[1] = 0x99;
    slot(1)[0] = 3;
    slot(1)[1] = 0x99;
    rc = func_80028B14();
    expect_eq_ptr("cd.quirk.rc", (void*)(uintptr_t)rc,
                  s_stream + (2 * 8) + 0x24 + (2 << 11));
    expect_eq_s32("cd.quirk.id", D_8004FE24, 0x11);

    /* Debug path: handle/idle guards. */
    reset_state();
    g_ArchiveDebugTable = 1;
    D_8004FE4C = -1;
    rc = func_80028B14();
    expect_eq_s32("dbg.nofile.rc", rc, 0);

    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0;
    rc = func_80028B14();
    expect_eq_s32("dbg.empty.rc", rc, 0);
    expect_eq_s32("dbg.empty.read", s_pcread, 0);

    /* Debug path: rotate to a free slot, read 0x800, file continues. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE10 = 1;
    slot(0)[0] = 3;
    slot(1)[0] = 0;
    rc = func_80028B14();
    expect_eq_ptr("dbg.read.rc", (void*)(uintptr_t)rc, slotbuf(1));
    expect_eq_s32("dbg.read.reads", s_pcread, 1);
    expect_eq_s32("dbg.read.fd", s_pcread_fd, 0x11);
    expect_eq_ptr("dbg.read.buf", s_pcread_buf, slotbuf(1));
    expect_eq_s32("dbg.read.len", s_pcread_len, 0x800);
    expect_eq_s32("dbg.read.state", slot(1)[0], 3);
    expect_eq_s32("dbg.read.rotor", D_8004FE10, 2);
    expect_eq_s32("dbg.read.size", g_ArchiveCurFileSize, 0x1000);
    expect_eq_s32("dbg.read.close", s_pcclose, 0);
    expect_eq_s32("dbg.read.retry", s_retry, 0);

    /* Debug path: rotor wraps past D_8004FE40. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    D_8004FE10 = 3;
    slot(3)[0] = 3;
    slot(0)[0] = 0;
    rc = func_80028B14();
    expect_eq_ptr("dbg.wrap.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.wrap.rotor", D_8004FE10, 1);
    expect_eq_s32("dbg.wrap.state", slot(0)[0], 3);

    /* Debug path: full ring whose scanned slot is loaded -> 0. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    slot(0)[0] = 3;
    slot(1)[0] = 3;
    slot(2)[0] = 2;
    slot(3)[0] = 3;
    rc = func_80028B14();
    expect_eq_s32("dbg.full.rc", rc, 0);
    expect_eq_s32("dbg.full.read", s_pcread, 0);

    /* Debug path: PCread retries four times -> 0, file size untouched. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1800;
    s_pcread_ret = 0;
    rc = func_80028B14();
    expect_eq_s32("dbg.readfail.rc", rc, 0);
    expect_eq_s32("dbg.readfail.reads", s_pcread, 4);
    expect_eq_s32("dbg.readfail.retries", s_retry, 4);
    expect_eq_s32("dbg.readfail.retry.a", s_retry_a, 3);
    expect_eq_s32("dbg.readfail.retry.b", s_retry_b, 0);
    expect_eq_s32("dbg.readfail.retry.c", s_retry_c, 0xFF);
    expect_eq_s32("dbg.readfail.retry.d", s_retry_d, 0);
    expect_eq_s32("dbg.readfail.size", g_ArchiveCurFileSize, 0x1800);

    /* Debug path: short file -> close, queue the next entry. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    s_pcclose_seq[0] = 0;
    queue_entry(2, 0x1234, s_stream);
    D_8004FE0C = (s32)(uintptr_t)s_queueBuf;
    D_8004FE10 = 0;
    D_8004FDFC = 5;
    rc = func_80028B14();
    expect_eq_ptr("dbg.next.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.next.size0", g_ArchiveCurFileSize, 0x1000);
    expect_eq_s32("dbg.next.close", s_pcclose, 1);
    expect_eq_s32("dbg.next.rotor", D_8004FE10, 2);
    expect_eq_s32("dbg.next.index", (s32)D_80059F0C, 0x1234);
    expect_eq_s32("dbg.next.path", s_filepath_in, 0x1234);
    expect_eq_s32("dbg.next.open", s_pcopen, 1);
    expect_eq_s32("dbg.next.open.name", strcmp(s_pcopen_name, "next.bin"), 0);
    expect_eq_s32("dbg.next.fd", D_8004FE4C, 0x22);
    expect_eq_s32("dbg.next.decoded", s_decode_aligned_in, 0x1234);
    expect_eq_s32("dbg.next.busy", D_8004FDFC, 4);
    expect_eq_s32("dbg.next.retry", s_retry, 0);

    /* Debug path: close retries four times, then continues anyway. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    s_pcclose_seq[0] = 1;
    s_pcclose_seq[1] = 1;
    s_pcclose_seq[2] = 1;
    s_pcclose_seq[3] = 1;
    queue_entry(2, 0x77, s_stream);
    D_8004FE0C = (s32)(uintptr_t)s_queueBuf;
    D_8004FE10 = 0;
    rc = func_80028B14();
    expect_eq_ptr("dbg.closefail.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.closefail.closes", s_pcclose, 4);
    expect_eq_s32("dbg.closefail.retries", s_retry, 4);
    expect_eq_s32("dbg.closefail.retry.a", s_retry_a, 3);
    expect_eq_s32("dbg.closefail.retry.b", s_retry_b, 0);
    expect_eq_s32("dbg.closefail.retry.c", s_retry_c, 0);
    expect_eq_s32("dbg.closefail.retry.d", s_retry_d, 0xFF);
    expect_eq_s32("dbg.closefail.index", (s32)D_80059F0C, 0x77);

    /* Debug path: PCopen never succeeds but the read still continues. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    s_pcopen_ret = -1;
    queue_entry(2, 0x77, s_stream);
    D_8004FE0C = (s32)(uintptr_t)s_queueBuf;
    D_8004FE10 = 0;
    D_8004FDFC = 2;
    rc = func_80028B14();
    expect_eq_ptr("dbg.openfail.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.openfail.opens", s_pcopen, 4);
    expect_eq_s32("dbg.openfail.retries", s_retry, 4);
    expect_eq_s32("dbg.openfail.retry.a", s_retry_a, 3);
    expect_eq_s32("dbg.openfail.retry.b", s_retry_b, 0xFF);
    expect_eq_s32("dbg.openfail.retry.c", s_retry_c, 0);
    expect_eq_s32("dbg.openfail.retry.d", s_retry_d, 0);
    expect_eq_s32("dbg.openfail.size", g_ArchiveCurFileSize, 0x1000);
    expect_eq_s32("dbg.openfail.busy", D_8004FDFC, 1);

    /* Debug path: next entry index 0 clears the stream state. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    queue_entry(2, 0, s_stream);
    D_8004FE0C = (s32)(uintptr_t)s_queueBuf;
    D_8004FE10 = 0;
    D_8004FDFC = 3;
    rc = func_80028B14();
    expect_eq_ptr("dbg.emptynext.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.emptynext.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("dbg.emptynext.busy", D_8004FDFC, 0);
    expect_eq_s32("dbg.emptynext.fd", D_8004FE4C, -1);
    expect_eq_s32("dbg.emptynext.open", s_pcopen, 0);

    /* Debug path: next entry with NULL pData also clears the stream state. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    queue_entry(2, 0x99, NULL);
    D_8004FE0C = (s32)(uintptr_t)s_queueBuf;
    D_8004FE10 = 0;
    D_8004FDFC = 3;
    rc = func_80028B14();
    expect_eq_ptr("dbg.nulldata.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.nulldata.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("dbg.nulldata.busy", D_8004FDFC, 0);
    expect_eq_s32("dbg.nulldata.path", s_filepath, 0);
    expect_eq_s32("dbg.nulldata.open", s_pcopen, 0);

    /* Debug path: no queue at all clears the busy flag and returns the sector. */
    reset_state();
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x800;
    D_8004FE0C = 0;
    D_8004FDFC = 3;
    rc = func_80028B14();
    expect_eq_ptr("dbg.noqueue.rc", (void*)(uintptr_t)rc, slotbuf(0));
    expect_eq_s32("dbg.noqueue.busy", D_8004FDFC, 0);
    expect_eq_s32("dbg.noqueue.fd", D_8004FE4C, -1);
    expect_eq_s32("dbg.noqueue.size", g_ArchiveCurFileSize, 0);

    printf("LIBARCHIVE STREAM POLL 28B14 certificate PASS checks=%u\n", s_checks);
    return 0;
}
