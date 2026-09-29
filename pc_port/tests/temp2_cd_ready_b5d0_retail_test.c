/*
 * Retail certificate for func_8002B5D0 (CdReadyCallback stream handler).
 *
 * Retail (func_8002B5D0.s 0x8002B5D0-0x8002B8B0, frame 0x28):
 *   status != 1 -> D_8005A4DC += 1 then the "no room" re-arm path.
 *   status == 1 with D_8004FE34 > 0 (abort): clear the ready + data callbacks,
 *   zero g_ArchiveCurFileSize, ArchiveCdSeekToFile(D_8004FE38), D_8004FDFC = 0.
 *   status == 1 with g_ArchiveCurFileSize <= 0: clear the ready callback and the
 *   size.
 *   status == 1 with a live stream: rotate D_8004FE10 over D_8004FE40 slots to a
 *   NOT_LOADED one; a loaded walk target is the "no room" path (no
 *   D_8005A4DC bump). The found slot requires CdGetSector(D_80059EF8, 3) +
 *   CdPosToInt == g_ArchiveCurFileSector; a mismatch bumps D_8004FDEC, swallows
 *   0x200 words into D_800596F8, bumps D_8005A4DC and re-arms. On a match the
 *   slot is marked state 1 with id D_8004FE26 (then bumped), 0x200 words land in
 *   D_8004FE08 + index*0x800, g_ArchiveCurFileSize -= 0x800 and
 *   g_ArchiveCurFileSector += 1, returning while the size stays positive.
 *   "No room" path: clear the ready callback into D_80059F08,
 *   CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation), then either
 *   error 3 + state 0xA (D_8005A4DC < 3) or a busy-wait with D_8005A4DC = 0,
 *   error 4, D_8005A4A4 += 1 and state 0xA; both end with
 *   CdSyncCallback(&ArchiveCdDriveCommandHandler) and CdControlF(1, NULL).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

extern void func_8002B5D0(u8 status);

static u8 s_stream[0x8000];

int g_ArchiveCurFileSize;
int D_8004FE40;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE10;
u16 D_8004FE26;
s8* D_8004FE08;
s32 D_8004FDEC;
s32 D_8005A4DC;
CdlLOC D_80059EF8;
u8 D_800596F8[0x800];
void* D_80059F08;
s32 D_8005A4A4;
s32 D_8004FE34;
s32 D_8004FE38;
s32 D_8004FDFC;
s32 g_ArchiveCurFileSector;
u_int g_ArchiveCdDriveState;
u_int g_ArchiveCdDriveError;
CdlLOC g_ArchiveCdCurLocation;

static unsigned s_checks;
static int s_ready;
static int s_data;
static int s_sync;
static void* s_sync_arg;
static int s_ctl;
static int s_ctl_cmd;
static int s_get;
static void* s_get_buf;
static int s_get_size;
static int s_pos;
static int s_pos_ret;
static int s_intpos;
static s32 s_intpos_sec;
static int s_seek;
static s32 s_seek_arg;

CdlCB CdReadyCallback(CdlCB func)
{
    s_ready++;
    return func;
}

void* CdDataCallback(void (*func)())
{
    s_data++;
    return (void*)func;
}

CdlCB CdSyncCallback(CdlCB func)
{
    s_sync++;
    s_sync_arg = (void*)func;
    return func;
}

int CdControlF(u_char com, u_char* param)
{
    (void)param;
    s_ctl++;
    s_ctl_cmd = com;
    return 1;
}

int CdGetSector(void* madr, int size)
{
    s_get++;
    s_get_buf = madr;
    s_get_size = size;
    return 1;
}

int CdPosToInt(CdlLOC* p)
{
    (void)p;
    s_pos++;
    return s_pos_ret;
}

CdlLOC* CdIntToPos(int sec, CdlLOC* p)
{
    s_intpos++;
    s_intpos_sec = sec;
    return p;
}

void ArchiveCdSeekToFile(int entryIndex)
{
    s_seek++;
    s_seek_arg = entryIndex;
}

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
        fail("temp2.ready", detail);
    }
}

static void expect_eq_ptr(const char* field, const void* actual, const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual != expected) {
        snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
                 field, actual, expected);
        fail("temp2.ready", detail);
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
    memset(D_800596F8, 0, sizeof(D_800596F8));
    *(s32*)s_stream = 4;

    g_ArchiveCurFileSize = 0x1000;
    D_8004FE40 = 4;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)(s_stream + 4);
    D_8004FE10 = 0;
    D_8004FE26 = 7;
    D_8004FE08 = (s8*)((u8*)s_stream + (*(s32*)s_stream * 8) + 0x24);
    D_8004FDEC = 0;
    D_8005A4DC = 0;
    D_80059F08 = NULL;
    D_8005A4A4 = 0;
    D_8004FE34 = 0;
    D_8004FE38 = 0;
    D_8004FDFC = 0x5A5A5A5A;
    g_ArchiveCurFileSector = 0x44;
    g_ArchiveCdDriveState = 0;
    g_ArchiveCdDriveError = 0;
    memset(&g_ArchiveCdCurLocation, 0, sizeof(g_ArchiveCdCurLocation));

    s_ready = 0;
    s_data = 0;
    s_sync = 0;
    s_sync_arg = NULL;
    s_ctl = 0;
    s_ctl_cmd = 0;
    s_get = 0;
    s_get_buf = NULL;
    s_get_size = 0;
    s_pos = 0;
    s_pos_ret = 0x44;
    s_intpos = 0;
    s_intpos_sec = 0;
    s_seek = 0;
    s_seek_arg = 0;
}

int main(void)
{
    /* Other status: bump the retry counter and re-arm the drive. */
    reset_state();
    func_8002B5D0(2);
    expect_eq_s32("other.retries", D_8005A4DC, 1);
    expect_eq_s32("other.ready", s_ready, 1);
    expect_eq_s32("other.get", s_get, 0);
    expect_eq_s32("other.intpos", s_intpos, 1);
    expect_eq_s32("other.intpos.sec", s_intpos_sec, 0x44);
    expect_eq_s32("other.error", g_ArchiveCdDriveError, 3);
    expect_eq_s32("other.state", (s32)g_ArchiveCdDriveState, 0xA);
    expect_eq_s32("other.sync", s_sync, 1);
    expect_eq_ptr("other.sync.arg", s_sync_arg, &ArchiveCdDriveCommandHandler);
    expect_eq_s32("other.ctl", s_ctl, 1);
    expect_eq_s32("other.ctl.cmd", s_ctl_cmd, 1);
    expect_eq_ptr("other.f08", D_80059F08, NULL);

    /* Abort request. */
    reset_state();
    D_8004FE34 = 1;
    D_8004FE38 = 0x123;
    D_8004FDFC = 5;
    func_8002B5D0(1);
    expect_eq_s32("abort.ready", s_ready, 1);
    expect_eq_s32("abort.data", s_data, 1);
    expect_eq_s32("abort.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("abort.seek", s_seek, 1);
    expect_eq_s32("abort.seek.arg", s_seek_arg, 0x123);
    expect_eq_s32("abort.busy", D_8004FDFC, 0);
    expect_eq_s32("abort.ctl", s_ctl, 0);
    expect_eq_s32("abort.intpos", s_intpos, 0);

    /* Idle stream. */
    reset_state();
    g_ArchiveCurFileSize = 0;
    func_8002B5D0(1);
    expect_eq_s32("idle.ready", s_ready, 1);
    expect_eq_s32("idle.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("idle.retries", D_8005A4DC, 0);
    expect_eq_s32("idle.get", s_get, 0);
    expect_eq_s32("idle.ctl", s_ctl, 0);

    /* Sector lands in the free slot. */
    reset_state();
    func_8002B5D0(1);
    expect_eq_s32("read.gets", s_get, 2);
    expect_eq_ptr("read.toc.buf", s_get_buf, slotbuf(0));
    expect_eq_s32("read.payload.size", s_get_size, 0x200);
    expect_eq_s32("read.state", slot(0)[0], 1);
    expect_eq_s32("read.id", slot(0)[1], 7);
    expect_eq_s32("read.nextid", D_8004FE26, 8);
    expect_eq_s32("read.rotor", D_8004FE10, 1);
    expect_eq_s32("read.size", g_ArchiveCurFileSize, 0x800);
    expect_eq_s32("read.sector", g_ArchiveCurFileSector, 0x45);
    expect_eq_s32("read.pos", s_pos, 1);
    expect_eq_s32("read.ctl", s_ctl, 0);

    /* Last sector: the size hits zero and the ready callback is cleared. */
    reset_state();
    g_ArchiveCurFileSize = 0x800;
    func_8002B5D0(1);
    expect_eq_s32("last.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("last.ready", s_ready, 1);
    expect_eq_s32("last.state", slot(0)[0], 1);
    expect_eq_s32("last.ctl", s_ctl, 0);

    /* Position mismatch: drop the sector and re-arm. */
    reset_state();
    s_pos_ret = 0x99;
    func_8002B5D0(1);
    expect_eq_s32("mismatch.pos", s_pos, 1);
    expect_eq_s32("mismatch.drop", D_8004FDEC, 1);
    expect_eq_s32("mismatch.gets", s_get, 2);
    expect_eq_s32("mismatch.state", slot(0)[0], 0);
    expect_eq_s32("mismatch.retries", D_8005A4DC, 1);
    expect_eq_s32("mismatch.error", g_ArchiveCdDriveError, 3);
    expect_eq_s32("mismatch.ctl", s_ctl, 1);

    /* Full ring with the retry budget spent: error 4 and the fault counter. */
    reset_state();
    D_8005A4DC = 3;
    slot(0)[0] = 1;
    slot(1)[0] = 1;
    slot(2)[0] = 1;
    slot(3)[0] = 1;
    func_8002B5D0(1);
    expect_eq_s32("full.gets", s_get, 0);
    expect_eq_s32("full.retries", D_8005A4DC, 0);
    expect_eq_s32("full.error", g_ArchiveCdDriveError, 4);
    expect_eq_s32("full.faults", D_8005A4A4, 1);
    expect_eq_s32("full.state", (s32)g_ArchiveCdDriveState, 0xA);
    expect_eq_s32("full.sync", s_sync, 1);
    expect_eq_s32("full.ctl", s_ctl, 1);
    expect_eq_s32("full.size", g_ArchiveCurFileSize, 0x1000);

    printf("TEMP2 CD READY B5D0 certificate PASS checks=%u\n", s_checks);
    return 0;
}
