/*
 * Retail certificate for func_80029EB0 (archive stream-file arm).
 *
 * Retail (func_80029EB0.s 0x80029EB0-0x8002A260, frame 0x50):
 *   pStreamFile NULL or *pStreamFile < 2 -> -4
 *   archiveIndex <= 0 or ArchiveDecodeSize(archiveIndex) <= 0 -> -3
 *   ArchiveCdDataSync(0); D_8004FE18 = g_CurArchiveOffset; zero the three
 *   stream words at D_80059EF8 (D_80059F00 and two words below)
 *   ArchiveChangeStreamingFile(pStreamFile); latch D_80059F0C, sector, size,
 *   D_8004FE08 = pStreamFile + nStreamSectors*8 + 0x24, D_8004FE2C = +4,
 *   D_8004FE40 = nStreamSectors, D_8004FDFC = 1, D_8004FE38 = arg2 & 0xFFFF,
 *   D_8004FE10/0C/34/D_8005A4DC/26/28 = 0 (D_8004FE24 is untouched),
 *   D_80059F24-2C = args 4-6,
 *   D_80059F30/34/38 = args 7-9, D_80059F3C/40/44/48/4C/50 = 0,
 *   ArchiveClearStreamFileSections(), CdIntToPos(sector, &g_ArcCdCurLocation)
 * then either
 *   debug table: ArchiveGetFilePath -> PCopen retry (3 extra tries with
 *   func_8002804C(i,0xFF,0,0)) -> do { func_8002B8B0(0,0);
 *   func_8002BF38(0,0); } while (D_8004FDFC > 0) -> PCclose retry, returning
 *   -6 when the close keeps failing, else clearing D_8004FDFC and
 *   g_ArchiveCurFileSize and returning 0;
 *   or CD: g_ArchiveCdDriveState = READ_SECTOR, CdDataCallback(&func_8002BB50),
 *   CdSyncCallback(&ArchiveCdDriveCommandHandler), CdReadyCallback(&func_8002B5D0),
 *   D_8005A488++ and CdControlF(CdlSetloc, &g_ArchiveCdCurLocation).
 * The trailing mode arguments are u16 stack arguments (retail lhu).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

extern int func_80029EB0(s32 archiveIndex, s32* pStreamFile, s32 arg2, s32 arg3,
                         u16 arg4, u16 arg5, u16 arg6, u16 arg7, u16 arg8, u16 arg9);

s8* D_8004FE08;
s32 D_8004FE0C;
s32 D_8004FE10;
s32 D_8004FE18;
s16 D_8004FE24;
s16 D_8004FE26;
s16 D_8004FE28;
s32 D_8004FE34;
s32 D_8004FE38;
s32 D_8004FE40;
s32 D_8004FDFC;
s32 D_8005A4DC;
ArchiveStreamFileSectionHeader* D_8004FE2C;
u32 D_80059F0C;
s32 D_80059F04;
u16 D_80059F24;
u16 D_80059F28;
u16 D_80059F2C;
u16 D_80059F30;
u16 D_80059F34;
u16 D_80059F38;
s32 D_80059F3C;
u16 D_80059F40;
u16 D_80059F44;
u16 D_80059F48;
s32 D_80059F4C;
s32 D_80059F50;
u32 D_8005A488;
u32 g_ArchiveDebugTable;
u32 g_CurArchiveOffset;
s32 g_ArchiveCurFileSector;
int g_ArchiveCurFileSize;
u_int g_ArchiveCdDriveState;
CdlLOC g_ArchiveCdCurLocation;

/* D_80059F00 sits at the top of the three-word block retail zeroes. */
static s32 s_streamWords[3];
asm(".globl D_80059EF8\n.set D_80059EF8, s_streamWords");
asm(".globl D_80059F00\n.set D_80059F00, s_streamWords + 8");

static unsigned s_checks;
static int s_sync;
static int s_decode_size;
static s32 s_decode_size_in;
static int s_decode_size_ret;
static int s_decode_sector;
static s32 s_decode_sector_in;
static int s_decode_aligned;
static s32 s_decode_aligned_in;
static int s_change_streaming;
static int s_clear_sections;
static int s_cdpos;
static s32 s_cdpos_sec;
static CdlLOC* s_cdpos_arg;
static int s_cb_data;
static void (*s_cb_data_arg)();
static int s_cb_sync;
static void* s_cb_sync_arg;
static int s_cb_ready;
static void* s_cb_ready_arg;
static int s_ctl;
static int s_ctl_cmd;
static void* s_ctl_arg;
static int s_pcopen;
static const char* s_pcopen_name;
static int s_pcopen_ret;
static int s_pcclose;
static int s_pcclose_calls;
static int s_pcclose_seq[4];
static int s_retry;
static s32 s_retry_a;
static s32 s_retry_b;
static s32 s_retry_c;
static s32 s_retry_d;
static int s_pump_b8b0;
static s32 s_pump_b8b0_a;
static int s_pump_bf38;
static s32 s_pump_bf38_a;
static int s_drain_on_pump;

int ArchiveCdDataSync(int mode)
{
    (void)mode;
    s_sync++;
    return 0;
}

int ArchiveDecodeSize(int entryIndex)
{
    s_decode_size++;
    s_decode_size_in = entryIndex;
    return s_decode_size_ret;
}

int ArchiveDecodeSector(int entryIndex)
{
    s_decode_sector++;
    s_decode_sector_in = entryIndex;
    return 0x1234;
}

int ArchiveDecodeAlignedSize(int entryIndex)
{
    s_decode_aligned++;
    s_decode_aligned_in = entryIndex;
    return 0x800;
}

int ArchiveChangeStreamingFile(void* pStreamFile)
{
    (void)pStreamFile;
    s_change_streaming++;
    return 0;
}

int ArchiveClearStreamFileSections(void)
{
    s_clear_sections++;
    return 0;
}

char* ArchiveGetFilePath(int entryIndex)
{
    (void)entryIndex;
    return "dbg.bin";
}

CdlLOC* CdIntToPos(int sector, CdlLOC* p)
{
    s_cdpos++;
    s_cdpos_sec = sector;
    s_cdpos_arg = p;
    p->minute = 1;
    return p;
}

void (*CdDataCallback(void (*func)()))
{
    s_cb_data++;
    s_cb_data_arg = func;
    return 0;
}

CdlCB CdSyncCallback(CdlCB func)
{
    s_cb_sync++;
    s_cb_sync_arg = (void*)func;
    return 0;
}

CdlCB CdReadyCallback(CdlCB func)
{
    s_cb_ready++;
    s_cb_ready_arg = (void*)func;
    return 0;
}

int CdControlF(u_char cmd, u_char* param)
{
    s_ctl++;
    s_ctl_cmd = cmd;
    s_ctl_arg = param;
    return 1;
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
    return s_pcclose_seq[s_pcclose_calls++];
}

void func_8002804C(s32 a, s32 b, s32 c, s32 d)
{
    s_retry++;
    s_retry_a = a;
    s_retry_b = b;
    s_retry_c = c;
    s_retry_d = d;
}

void func_8002B8B0(s32 a, s32 b)
{
    (void)b;
    s_pump_b8b0++;
    s_pump_b8b0_a = a;
}

void func_8002BF38(s32 a, s32 b)
{
    (void)b;
    s_pump_bf38++;
    s_pump_bf38_a = a;
    if (s_drain_on_pump)
        D_8004FDFC = 0;
}

void func_8002BB50(void) {}
void func_8002B5D0(void) {}
void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult)
{
    (void)status;
    (void)pResult;
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
        fail("archive.arm", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("archive.arm", detail);
    }
}

static void reset_state(void)
{
    D_8004FE08 = NULL;
    D_8004FE0C = 0x5A5A5A5A;
    D_8004FE10 = 0x5A5A5A5A;
    D_8004FE18 = 0;
    D_8004FE24 = 0x5A5A;
    D_8004FE26 = 0x5A5A;
    D_8004FE28 = 0x5A5A;
    D_8004FE34 = 0x5A5A5A5A;
    D_8004FE38 = 0x5A5A5A5A;
    D_8004FE40 = 0x5A5A5A5A;
    D_8004FDFC = 0x5A5A5A5A;
    D_8005A4DC = 0x5A5A5A5A;
    D_80059F0C = 0x5A5A5A5A;
    D_80059F04 = 0x5A5A5A5A;
    D_80059F24 = 0x5A5A;
    D_80059F28 = 0x5A5A;
    D_80059F2C = 0x5A5A;
    D_80059F30 = 0x5A5A;
    D_80059F34 = 0x5A5A;
    D_80059F38 = 0x5A5A;
    D_80059F3C = 0x5A5A5A5A;
    D_80059F40 = 0x5A5A;
    D_80059F44 = 0x5A5A;
    D_80059F48 = 0x5A5A;
    D_80059F4C = 0x5A5A5A5A;
    D_80059F50 = 0x5A5A5A5A;
    D_8005A488 = 7;
    g_ArchiveDebugTable = 0;
    g_CurArchiveOffset = 0x40;
    g_ArchiveCurFileSector = 0;
    g_ArchiveCurFileSize = 0x5A5A5A5A;
    g_ArchiveCdDriveState = 0;
    memset(&g_ArchiveCdCurLocation, 0, sizeof(g_ArchiveCdCurLocation));
    s_streamWords[0] = 0x5A5A5A5A;
    s_streamWords[1] = 0x5A5A5A5A;
    s_streamWords[2] = 0x5A5A5A5A;

    s_sync = 0;
    s_decode_size = 0;
    s_decode_size_in = 0;
    s_decode_size_ret = 1;
    s_decode_sector = 0;
    s_decode_sector_in = 0;
    s_decode_aligned = 0;
    s_decode_aligned_in = 0;
    s_change_streaming = 0;
    s_clear_sections = 0;
    s_cdpos = 0;
    s_cdpos_sec = 0;
    s_cdpos_arg = NULL;
    s_cb_data = 0;
    s_cb_data_arg = NULL;
    s_cb_sync = 0;
    s_cb_sync_arg = NULL;
    s_cb_ready = 0;
    s_cb_ready_arg = NULL;
    s_ctl = 0;
    s_ctl_cmd = 0;
    s_ctl_arg = NULL;
    s_pcopen = 0;
    s_pcopen_name = NULL;
    s_pcopen_ret = 5;
    s_pcclose = 0;
    s_pcclose_calls = 0;
    memset(s_pcclose_seq, 0, sizeof(s_pcclose_seq));
    s_retry = 0;
    s_retry_a = 0;
    s_retry_b = 0;
    s_retry_c = 0;
    s_retry_d = 0;
    s_pump_b8b0 = 0;
    s_pump_b8b0_a = 0;
    s_pump_bf38 = 0;
    s_pump_bf38_a = 0;
    s_drain_on_pump = 1;
}

int main(void)
{
    /* Backing store covers pStreamFile + slotCount*8 + 0x24, which the
     * function points D_8004FE08 into. */
    u8 streamBytes[0x60];
    s32* stream = (s32*)streamBytes;
    int rc;

    memset(streamBytes, 0, sizeof(streamBytes));
    stream[0] = 3;

    /* Validation window. */
    reset_state();
    rc = func_80029EB0(0x123, NULL, 4, 0, 1, 2, 3, 4, 5, 6);
    expect_eq_s32("null.rc", rc, -4);
    expect_eq_s32("null.touched", s_sync, 0);

    reset_state();
    stream[0] = 1;
    rc = func_80029EB0(0x123, stream, 4, 0, 1, 2, 3, 4, 5, 6);
    expect_eq_s32("short.rc", rc, -4);
    stream[0] = 3;

    reset_state();
    rc = func_80029EB0(0, stream, 4, 0, 1, 2, 3, 4, 5, 6);
    expect_eq_s32("zero.index.rc", rc, -3);
    expect_eq_s32("zero.index.decode", s_decode_size, 0);

    reset_state();
    s_decode_size_ret = 0;
    rc = func_80029EB0(0x123, stream, 4, 0, 1, 2, 3, 4, 5, 6);
    expect_eq_s32("empty.decode.rc", rc, -3);
    expect_eq_s32("empty.decode.in", s_decode_size_in, 0x123);
    expect_eq_s32("empty.decode.sync", s_sync, 0);

    /* CD arm path. */
    reset_state();
    rc = func_80029EB0(0x123, stream, 4, 0, 0xAA01, 0xAA02, 0xAA03, 0xAA04,
                       0xAA05, 0xAA06);
    expect_eq_s32("arm.rc", rc, 0);
    expect_eq_s32("arm.sync", s_sync, 1);
    expect_eq_s32("arm.decode.size", s_decode_size_in, 0x123);
    expect_eq_s32("arm.decode.sector", s_decode_sector_in, 0x123);
    expect_eq_s32("arm.decode.aligned", s_decode_aligned_in, 0x123);
    expect_eq_s32("arm.change.streaming", s_change_streaming, 1);
    expect_eq_s32("arm.clear.sections", s_clear_sections, 1);
    expect_eq_s32("arm.snap", D_8004FE18, 0x40);
    expect_eq_s32("arm.z0", s_streamWords[0], 0);
    expect_eq_s32("arm.z1", s_streamWords[1], 0);
    expect_eq_s32("arm.z2", s_streamWords[2], 0);
    expect_eq_s32("arm.f0c", (s32)D_80059F0C, 0x123);
    expect_eq_s32("arm.sector", g_ArchiveCurFileSector, 0x1234);
    expect_eq_s32("arm.size", g_ArchiveCurFileSize, 0x800);
    expect_eq_ptr("arm.buf", D_8004FE08, (s8*)stream + 0x24 + 3 * 8);
    expect_eq_ptr("arm.slots", D_8004FE2C, (u8*)stream + 4);
    expect_eq_s32("arm.slotcount", D_8004FE40, 3);
    expect_eq_s32("arm.busy", D_8004FDFC, 1);
    expect_eq_s32("arm.arg2", D_8004FE38, 4);
    expect_eq_s32("arm.zero.10", D_8004FE10, 0);
    expect_eq_s32("arm.zero.0c", D_8004FE0C, 0);
    expect_eq_s32("arm.zero.34", D_8004FE34, 0);
    expect_eq_s32("arm.zero.a4dc", D_8005A4DC, 0);
    expect_eq_s32("arm.keep.24", D_8004FE24, 0x5A5A);
    expect_eq_s32("arm.zero.26", D_8004FE26, 0);
    expect_eq_s32("arm.zero.28", D_8004FE28, 0);
    expect_eq_s32("arm.mode.x0", D_80059F24, 0xAA01);
    expect_eq_s32("arm.mode.x1", D_80059F28, 0xAA02);
    expect_eq_s32("arm.mode.x2", D_80059F2C, 0xAA03);
    expect_eq_s32("arm.mode.y0", D_80059F30, 0xAA04);
    expect_eq_s32("arm.mode.y1", D_80059F34, 0xAA05);
    expect_eq_s32("arm.mode.y2", D_80059F38, 0xAA06);
    expect_eq_s32("arm.zero.3c", D_80059F3C, 0);
    expect_eq_s32("arm.zero.40", D_80059F40, 0);
    expect_eq_s32("arm.zero.44", D_80059F44, 0);
    expect_eq_s32("arm.zero.48", D_80059F48, 0);
    expect_eq_s32("arm.zero.4c", D_80059F4C, 0);
    expect_eq_s32("arm.zero.50", D_80059F50, 0);
    expect_eq_s32("arm.cdpos", s_cdpos, 1);
    expect_eq_s32("arm.cdpos.sec", s_cdpos_sec, 0x1234);
    expect_eq_ptr("arm.cdpos.arg", s_cdpos_arg, &g_ArchiveCdCurLocation);
    expect_eq_s32("arm.state", (s32)g_ArchiveCdDriveState, ARCHIVE_CD_DRIVE_READ_SECTOR);
    expect_eq_s32("arm.cb.data", s_cb_data, 1);
    expect_eq_ptr("arm.cb.data.arg", s_cb_data_arg, &func_8002BB50);
    expect_eq_s32("arm.cb.sync", s_cb_sync, 1);
    expect_eq_ptr("arm.cb.sync.arg", s_cb_sync_arg, &ArchiveCdDriveCommandHandler);
    expect_eq_s32("arm.cb.ready", s_cb_ready, 1);
    expect_eq_ptr("arm.cb.ready.arg", s_cb_ready_arg, &func_8002B5D0);
    expect_eq_s32("arm.ctl", s_ctl, 1);
    expect_eq_s32("arm.ctl.cmd", s_ctl_cmd, CdlSetloc);
    expect_eq_ptr("arm.ctl.arg", s_ctl_arg, &g_ArchiveCdCurLocation);
    expect_eq_s32("arm.inc", (s32)D_8005A488, 8);
    expect_eq_s32("arm.no.pcopen", s_pcopen, 0);

    /* arg2 is latched masked to 16 bits. */
    reset_state();
    rc = func_80029EB0(0x123, stream, 0x12345, 0, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("mask.rc", rc, 0);
    expect_eq_s32("mask.arg2", D_8004FE38, 0x2345);
    expect_eq_s32("mask.mode", D_80059F24, 0);

    /* Debug table success: open, pump, close. */
    reset_state();
    g_ArchiveDebugTable = 1;
    s_pcopen_ret = 5;
    s_pcclose_seq[0] = 0;
    rc = func_80029EB0(0x123, stream, 0, 0, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("dbg.rc", rc, 0);
    expect_eq_s32("dbg.open", s_pcopen, 1);
    expect_eq_s32("dbg.open.name", strcmp(s_pcopen_name, "dbg.bin"), 0);
    expect_eq_s32("dbg.fd", D_80059F04, 5);
    expect_eq_s32("dbg.pump.b8b0", s_pump_b8b0, 1);
    expect_eq_s32("dbg.pump.b8b0.arg", s_pump_b8b0_a, 0);
    expect_eq_s32("dbg.pump.bf38", s_pump_bf38, 1);
    expect_eq_s32("dbg.pump.bf38.arg", s_pump_bf38_a, 0);
    expect_eq_s32("dbg.close", s_pcclose, 1);
    expect_eq_s32("dbg.busy", D_8004FDFC, 0);
    expect_eq_s32("dbg.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("dbg.ctl", s_ctl, 0);
    expect_eq_s32("dbg.retry", s_retry, 0);

    /* Debug table: open retries 3 times, then closes. */
    reset_state();
    g_ArchiveDebugTable = 1;
    s_pcopen_ret = -1;
    s_pcclose_seq[0] = 0;
    rc = func_80029EB0(0x123, stream, 0, 0, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("dbg.openfail.rc", rc, 0);
    expect_eq_s32("dbg.openfail.opens", s_pcopen, 4);
    expect_eq_s32("dbg.openfail.retries", s_retry, 4);
    expect_eq_s32("dbg.openfail.retry.a", s_retry_a, 3);
    expect_eq_s32("dbg.openfail.retry.b", s_retry_b, 0xFF);
    expect_eq_s32("dbg.openfail.retry.c", s_retry_c, 0);
    expect_eq_s32("dbg.openfail.retry.d", s_retry_d, 0);
    expect_eq_s32("dbg.openfail.close", s_pcclose, 1);

    /* Debug table: one close retry then success. */
    reset_state();
    g_ArchiveDebugTable = 1;
    s_pcopen_ret = 5;
    s_pcclose_seq[0] = 1;
    s_pcclose_seq[1] = 0;
    rc = func_80029EB0(0x123, stream, 0, 0, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("dbg.closeretry.rc", rc, 0);
    expect_eq_s32("dbg.closeretry.closes", s_pcclose, 2);
    expect_eq_s32("dbg.closeretry.retries", s_retry, 1);
    expect_eq_s32("dbg.closeretry.retry.a", s_retry_a, 1);
    expect_eq_s32("dbg.closeretry.retry.b", s_retry_b, 0);
    expect_eq_s32("dbg.closeretry.retry.c", s_retry_c, 0);
    expect_eq_s32("dbg.closeretry.retry.d", s_retry_d, 0xFF);

    /* Debug table: close never succeeds -> -6. */
    reset_state();
    g_ArchiveDebugTable = 1;
    s_pcopen_ret = 5;
    s_pcclose_seq[0] = 3;
    rc = func_80029EB0(0x123, stream, 0, 0, 0, 0, 0, 0, 0, 0);
    expect_eq_s32("dbg.closefail.rc", rc, -6);
    expect_eq_s32("dbg.closefail.closes", s_pcclose, 1);
    expect_eq_s32("dbg.closefail.retries", s_retry, 1);
    expect_eq_s32("dbg.closefail.retry.a", s_retry_a, 3);
    expect_eq_s32("dbg.closefail.retry.d", s_retry_d, 0xFF);

    printf("ARCHIVE STREAM ARM 29EB0 certificate PASS checks=%u\n", s_checks);
    return 0;
}
