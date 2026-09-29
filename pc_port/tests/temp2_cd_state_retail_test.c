/* Retail certificate for func_8002B084 (0x8002B084-0x8002B2F0) and
 * func_8002B2F0 (0x8002B2F0-0x8002B5B4): the shared re-arm path (status != 1)
 * and the abort path (status 1 with D_8004FE34 > 0). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

extern s32 func_8002B084(u8 status);
extern s32 func_8002B2F0(u8 status);

/* state used by both bodies */
s32 D_8004FE34;
s32 D_8004FE38;
s32 D_8004FDFC;
s32 D_8005A4DC;
s32 D_8005A4A4;
s32 D_8004FDE4;
s32 D_8004FDEC;
s32 D_8004FE10;
u16 D_8004FE26;
s32 g_ArchiveCurFileSize;
s32 g_ArchiveCurFileSector;
s8* D_8004FE08;
u8 D_800596F8[0x800];
CdlLOC D_80059EF8;
void* D_80059F08;
u_int g_ArchiveCdDriveState;
u_int g_ArchiveCdDriveError;
CdlLOC g_ArchiveCdCurLocation;
ArchiveStreamFileSectionHeader* D_8004FE2C;
int D_8004FE40;

static unsigned s_checks;
static int s_ready, s_data, s_sync, s_ctl, s_ctl_cmd, s_intpos, s_seek;
static s32 s_seek_arg;

CdlCB CdReadyCallback(CdlCB f) { (void)f; s_ready++; return 0; }
void* CdDataCallback(void (*f)()) { (void)f; s_data++; return 0; }
CdlCB CdSyncCallback(CdlCB f) { (void)f; s_sync++; return 0; }
int CdControlF(u_char cmd, u_char* p) { (void)p; s_ctl++; s_ctl_cmd = cmd; return 1; }
CdlLOC* CdIntToPos(int sec, CdlLOC* p) { (void)sec; s_intpos++; return p; }
int CdGetSector(void* d, int n) { (void)d; (void)n; return 1; }
int CdPosToInt(CdlLOC* p) { (void)p; return 0x44; }
void ArchiveCdSeekToFile(int e) { s_seek++; s_seek_arg = e; }
void ArchiveCdDriveCommandHandler(u_char s, u_char* r) { (void)s; (void)r; }

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
        fail("temp2.cdstate", detail);
    }
}

static void reset(void)
{
    D_8004FE34 = 0; D_8004FE38 = 0x99; D_8004FDFC = 0x5A5A5A5A;
    D_8005A4DC = 0; D_8005A4A4 = 0; D_8004FDE4 = 0; D_8004FDEC = 0;
    D_8004FE10 = 0; D_8004FE26 = 0;
    g_ArchiveCurFileSize = 0; g_ArchiveCurFileSector = 0x44;
    D_8004FE08 = (s8*)D_800596F8; D_80059F08 = NULL;
    g_ArchiveCdDriveState = 0; g_ArchiveCdDriveError = 0;
    s_ready = s_data = s_sync = s_ctl = s_intpos = s_seek = 0;
    s_ctl_cmd = 0; s_seek_arg = 0;
}

int main(void)
{
    /* B084: other status -> re-arm with error 3 and state 0xA */
    reset();
    func_8002B084(2);
    expect_eq_s32("b084.rearm.counter", D_8005A4DC, 1);
    expect_eq_s32("b084.rearm.ready", s_ready, 1);
    expect_eq_s32("b084.rearm.intpos", s_intpos, 1);
    expect_eq_s32("b084.rearm.error", (s32)g_ArchiveCdDriveError, 3);
    expect_eq_s32("b084.rearm.state", (s32)g_ArchiveCdDriveState, 0xA);
    expect_eq_s32("b084.rearm.sync", s_sync, 1);
    expect_eq_s32("b084.rearm.ctl", s_ctl_cmd, 1);

    /* B084: abort -> both callbacks cleared, seek, no drive control */
    reset();
    D_8004FE34 = 1;
    func_8002B084(1);
    expect_eq_s32("b084.abort.ready", s_ready, 1);
    /* func_8002B084 has NO CdDataCallback call (retail listing: 0 hits) —
     * unlike func_8002B2F0/func_8002B5D0, so the data handler must stay armed. */
    expect_eq_s32("b084.abort.keep.data", s_data, 0);
    expect_eq_s32("b084.abort.size", g_ArchiveCurFileSize, 0);
    expect_eq_s32("b084.abort.seek", s_seek_arg, 0x99);
    expect_eq_s32("b084.abort.busy", D_8004FDFC, 0);
    expect_eq_s32("b084.abort.ctl", s_ctl, 0);

    /* B2F0: other status -> same re-arm shape */
    reset();
    func_8002B2F0(0);
    expect_eq_s32("b2f0.rearm.counter", D_8005A4DC, 1);
    expect_eq_s32("b2f0.rearm.error", (s32)g_ArchiveCdDriveError, 3);
    expect_eq_s32("b2f0.rearm.state", (s32)g_ArchiveCdDriveState, 0xA);
    expect_eq_s32("b2f0.rearm.ctl", s_ctl_cmd, 1);

    /* B2F0: abort -> both callbacks cleared, seek, no drive control */
    reset();
    D_8004FE34 = 1;
    func_8002B2F0(1);
    expect_eq_s32("b2f0.abort.data", s_data, 1);
    expect_eq_s32("b2f0.abort.seek", s_seek_arg, 0x99);
    expect_eq_s32("b2f0.abort.busy", D_8004FDFC, 0);
    expect_eq_s32("b2f0.abort.ctl", s_ctl, 0);

    printf("TEMP2 CD STATE certificate PASS checks=%u\n", s_checks);
    return 0;
}
