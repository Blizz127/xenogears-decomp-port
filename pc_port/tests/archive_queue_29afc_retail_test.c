/*
 * Retail certificate for func_80029AFC (archive stream queue).
 *
 * Retail (func_80029AFC.s 0x80029AFC-0x80029EAC): NULL/empty -> -3.
 * Count until archiveIndex==0, selection-sort by unsigned index, sync,
 * snapshot g_CurArchiveOffset, zero D_80059F00 and two words below.
 * First index or pData 0: ArchiveCdSeekToFile(arg1). Else decode +
 * CdIntToPos; debug table uses PCopen/PCread, else arm CD READ_SECTOR.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

extern int func_80029AFC(StreamDataQueueEntry* pEntries, int arg1, int arg2);

s8* D_8004FE08;
s32 D_8004FE0C;
s32 D_8004FE10;
s32 D_8004FDFC;
s32 D_8004FE00;
s32 D_8004FE18;
s32 D_8004FE34;
s32 D_8004FE38;
s32 D_8004FE3C;
s32 D_8005A4DC;
s32 D_80059F00_storage[3];
asm(".globl D_80059F00\n.set D_80059F00, D_80059F00_storage + 8");
u32 D_80059F0C;
u32 D_8005A488;
u32 g_ArchiveDebugTable;
u32 g_CurArchiveOffset;
s32 g_ArchiveCurFileSector;
int g_ArchiveCurFileSize;
u_int g_ArchiveCdDriveState;
CdlLOC g_ArchiveCdCurLocation;

static unsigned s_checks;
static int s_sync;
static int s_seek;
static s32 s_seek_arg;
static int s_decode_sec;
static s32 s_decode_sec_in;
static int s_decode_sz;
static s32 s_decode_sz_in;
static int s_cdpos;
static s32 s_cdpos_sec;
static int s_cb_data;
static int s_cb_sync;
static int s_cb_ready;
static int s_ctl;
static int s_ctl_cmd;
static int s_pcopen;
static int s_pcread;
static int s_pcclose;

int ArchiveCdDataSync(int mode)
{
    (void)mode;
    s_sync++;
    return 0;
}

void ArchiveCdSeekToFile(int entryIndex)
{
    s_seek++;
    s_seek_arg = entryIndex;
}

int ArchiveDecodeSector(int entryIndex)
{
    s_decode_sec++;
    s_decode_sec_in = entryIndex;
    return 0x1234;
}

int ArchiveDecodeSizeAligned(int entryIndex)
{
    s_decode_sz++;
    s_decode_sz_in = entryIndex;
    return 0x800;
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
    p->minute = 1;
    return p;
}

void (*CdDataCallback(void (*func)()))
{
    (void)func;
    s_cb_data++;
    return 0;
}

CdlCB CdSyncCallback(CdlCB func)
{
    (void)func;
    s_cb_sync++;
    return 0;
}

CdlCB CdReadyCallback(CdlCB func)
{
    (void)func;
    s_cb_ready++;
    return 0;
}

int CdControlF(u_char cmd, u_char* param)
{
    (void)param;
    s_ctl++;
    s_ctl_cmd = cmd;
    return 1;
}

int PCopen(char* name, int flags, int perms)
{
    (void)name;
    (void)flags;
    (void)perms;
    s_pcopen++;
    return 3;
}

int PCread(int fd, char* buff, int len)
{
    (void)fd;
    (void)buff;
    (void)len;
    s_pcread++;
    return 1;
}

int PCclose(int fd)
{
    (void)fd;
    s_pcclose++;
    return 0;
}

void func_8002804C(s32 a, s32 b, s32 c, s32 d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
}

void func_8002BA40(void) {}
void func_8002AC24(void) {}
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
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
             field, (int)actual, (int)expected);
    fail("archive.queue", detail);
}

static void reset_state(void)
{
    D_8004FE08 = NULL;
    D_8004FE0C = 0;
    D_8004FE10 = 0xFF;
    D_8004FDFC = 0;
    D_8004FE00 = 0;
    D_8004FE18 = 0;
    D_8004FE34 = 0xFF;
    D_8004FE38 = 0;
    D_8004FE3C = 0xFF;
    D_8005A4DC = 0xFF;
    D_80059F00_storage[0] = 0x11;
    D_80059F00_storage[1] = 0x22;
    D_80059F00_storage[2] = 0x33;
    D_80059F0C = 0;
    D_8005A488 = 7;
    g_ArchiveDebugTable = 0;
    g_CurArchiveOffset = 0x40;
    g_ArchiveCurFileSector = 0;
    g_ArchiveCurFileSize = 0xFF;
    g_ArchiveCdDriveState = 0;
    memset(&g_ArchiveCdCurLocation, 0, sizeof(g_ArchiveCdCurLocation));
    s_sync = 0;
    s_seek = 0;
    s_decode_sec = 0;
    s_decode_sz = 0;
    s_cdpos = 0;
    s_cb_data = 0;
    s_cb_sync = 0;
    s_cb_ready = 0;
    s_ctl = 0;
    s_pcopen = 0;
    s_pcread = 0;
    s_pcclose = 0;
}

int main(void)
{
    StreamDataQueueEntry q[4];
    u8 buf_a[4];
    u8 buf_b[4];
    u8 buf_c[4];
    int rc;

    reset_state();
    rc = func_80029AFC(NULL, 0, 0);
    expect_eq_s32("null.rc", rc, -3);

    reset_state();
    memset(q, 0, sizeof(q));
    rc = func_80029AFC(q, 0, 0);
    expect_eq_s32("empty.rc", rc, -3);

    /* Sort 30,10,20 and arm CD. */
    reset_state();
    memset(q, 0, sizeof(q));
    q[0].archiveIndex = 30;
    q[0].pData = buf_a;
    q[1].archiveIndex = 10;
    q[1].pData = buf_b;
    q[2].archiveIndex = 20;
    q[2].pData = buf_c;
    rc = func_80029AFC(q, 0xABC, 0);
    expect_eq_s32("sort.rc", rc, 0);
    expect_eq_s32("sort.0", q[0].archiveIndex, 10);
    expect_eq_s32("sort.1", q[1].archiveIndex, 20);
    expect_eq_s32("sort.2", q[2].archiveIndex, 30);
    expect_eq_s32("sort.count", D_8004FDFC, 3);
    expect_eq_s32("sort.count2", D_8004FE00, 3);
    expect_eq_s32("sort.snap", D_8004FE18, 0x40);
    expect_eq_s32("sort.z0", D_80059F00_storage[2], 0);
    expect_eq_s32("sort.z1", D_80059F00_storage[1], 0);
    expect_eq_s32("sort.z2", D_80059F00_storage[0], 0);
    expect_eq_s32("sort.sync", s_sync, 1);
    expect_eq_s32("sort.sec", s_decode_sec_in, 10);
    expect_eq_s32("sort.sz", s_decode_sz_in, 10);
    expect_eq_s32("sort.pos", s_cdpos_sec, 0x1234);
    expect_eq_s32("sort.state", (s32)g_ArchiveCdDriveState, 1);
    expect_eq_s32("sort.ctl", s_ctl_cmd, CdlSetloc);
    expect_eq_s32("sort.cb", s_cb_data + s_cb_sync + s_cb_ready, 3);
    expect_eq_s32("sort.inc", (s32)D_8005A488, 8);
    expect_eq_s32("sort.arg", D_8004FE38, 0xABC);

    /* First pData NULL -> seek. */
    reset_state();
    memset(q, 0, sizeof(q));
    q[0].archiveIndex = 5;
    q[0].pData = NULL;
    rc = func_80029AFC(q, 9, 0);
    expect_eq_s32("seek.rc", rc, 0);
    expect_eq_s32("seek.n", s_seek, 1);
    expect_eq_s32("seek.arg", s_seek_arg, 9);
    expect_eq_s32("seek.count", D_8004FDFC, 0);
    expect_eq_s32("seek.size", g_ArchiveCurFileSize, 0);

    /* Debug table: PCopen/read/close. */
    reset_state();
    g_ArchiveDebugTable = 1;
    memset(q, 0, sizeof(q));
    q[0].archiveIndex = 4;
    q[0].pData = buf_a;
    rc = func_80029AFC(q, 0, 0);
    expect_eq_s32("dbg.rc", rc, 0);
    expect_eq_s32("dbg.open", s_pcopen, 1);
    expect_eq_s32("dbg.read", s_pcread, 1);
    expect_eq_s32("dbg.close", s_pcclose, 1);
    expect_eq_s32("dbg.ctl", s_ctl, 0);

    printf("ARCHIVE QUEUE 29AFC certificate PASS checks=%u\n", s_checks);
    return 0;
}
