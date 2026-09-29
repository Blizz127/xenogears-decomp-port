#include <stdint.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/libcd.h"

/* Keep this test independent of the native linker: it supplies only the
 * archive/CD boundary used by the two functions under test. */
void* g_ArchiveCurStreamFile;
s32 g_ArchiveCurFileSector;
s32 g_ArchiveCurFileSize;
s32 D_8004FDFC;
u32 g_ArchiveDebugTable;
int CdDataSync(int mode) { (void)mode; return 0; }
s16 D_8004FE10;
s16 D_8004FE24;
s16 D_8004FE28;
s8* D_8004FE08;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE40;
s32 D_8004FE34;
s32 D_8004FE38;
u_int g_ArchiveCdDriveState;
u_int g_ArchiveCdDriveError;

static int s_setloc_sector;
static int s_read_calls;
static int s_sync_calls;
static int s_defer_sync;
static u8* s_read_destination;
static u8 s_stream[0x24 + 2 * 8 + 2 * CD_SECTOR_SIZE];

CdlLOC* CdIntToPos(int sector, CdlLOC* loc) {
    (void)loc;
    s_setloc_sector = sector;
    return loc;
}

int CdControlB(u_char command, u_char* param, u_char* result) {
    (void)param;
    (void)result;
    return command == CdlSetloc;
}

int CdRead(int sectors, u_long* buffer, int mode) {
    if (sectors != 1 || mode != CdlModeSpeed || s_read_calls != s_sync_calls)
        return 0;
    s_read_calls++;
    s_read_destination = (u8*)buffer;
    return 1;
}

int CdReadSync(int mode, u_char* result) {
    (void)result;
    if (s_defer_sync) { s_defer_sync = 0; return 1; }
    if (mode != 1 || s_sync_calls >= s_read_calls)
        return -1;
    for (int i = 0; i < CD_SECTOR_SIZE; i++)
        s_read_destination[i] = (u8)(0x40 + s_sync_calls);
    s_sync_calls++;
    return 0;
}

void ArchiveChangeStreamingFile(void* streamFile) {
    g_ArchiveCurStreamFile = streamFile;
}

void ArchiveClearStreamFileSections(void) {
    u8* stream = (u8*)g_ArchiveCurStreamFile;
    int count = *(int*)stream;
    for (int i = 0; i < count; i++) {
        ArchiveStreamFileSectionHeader* header =
            (ArchiveStreamFileSectionHeader*)(stream + 4 + i * 8);
        header->state = 0;
        header->id = 0;
        header->size = 0;
        header->unk2 = 0;
    }
}

#include "../src/archive_port.c"
#include "archive_sync_under_test.inc"

int main(void) {
    u8* first;
    u8* second;
    for (int i = 0; i < (int)sizeof(s_stream); i++)
        s_stream[i] = 0;
    u8* stream = s_stream;
    *(int*)stream = 2;
    g_ArchiveCurStreamFile = stream;
    g_ArchiveCurFileSector = 42;
    g_ArchiveCurFileSize = 2 * CD_SECTOR_SIZE;

    if (ArchiveReadFile(7, stream, 0, CdlModeStream) != 0)
        return 1;
    if (s_setloc_sector != 42 || s_read_calls != 0 || D_8004FDFC != 1)
        return 2;
    if (func_80028B14() != 0 || s_read_calls != 1 || s_sync_calls != 0)
        return 3;

    first = (u8*)(uintptr_t)(u32)func_80028B14();
    if (first == NULL || first[0] != 0x40 || s_sync_calls != 1)
        return 4;
    if (((ArchiveStreamFileSectionHeader*)(stream + 4))->state != 3)
        return 5;

    /* Consumer release is the same state transition as retail
     * func_8002945C; the next poll must be able to reuse the slot. */
    ((ArchiveStreamFileSectionHeader*)(stream + 4))->state = 0;
    if (func_80028B14() != 0 || s_read_calls != 2)
        return 6;
    second = (u8*)(uintptr_t)(u32)func_80028B14();
    if (second == NULL || second[0] != 0x41 || s_sync_calls != 2)
        return 7;
    if (D_8004FDFC != 0 || g_ArchiveCurFileSize != 0)
        return 8;
    /* Regression: field-menu wait while two sectors remain outstanding.
     * No field consumer runs during this wait. Transport must complete, and
     * both sectors must still be delivered in order afterwards. */
    g_ArchiveCurFileSize = 2 * CD_SECTOR_SIZE;
    if (ArchiveReadFile(7, stream, 0, CdlModeStream) != 0) return 9;
    ArchiveCdDataSync(0);
    if (ArchiveDataSync() != 0 || s_sync_calls != 4) return 10;
    first = (u8*)(uintptr_t)(u32)func_80028B14();
    second = (u8*)(uintptr_t)(u32)func_80028B14();
    if (!first || !second || first == second || first[0] != 0x42 || second[0] != 0x43) return 11;
    if (func_80028B14() != 0 || D_8004FE24 != 2) return 12;
    /* A finished/freed ring must not be dereferenced on a stray later poll. */
    g_ArchiveCurStreamFile = s_ArchivePortStreamRead.streamFile = (void*)1;
    if (func_80028B14() != 0) return 13;
    g_ArchiveCurStreamFile = stream;
    g_ArchiveCurFileSize = 3 * CD_SECTOR_SIZE;
    if (ArchiveReadFile(7, stream, 0, CdlModeStream) != 0) return 14;
    PcPort_ArchivePollTransport();
    s_defer_sync = 1;
    PcPort_ArchivePollTransport();
    if (s_sync_calls != 4 || !s_ArchivePortStreamRead.readPending) return 15;
    PcPort_ArchivePollTransport();
    PcPort_ArchivePollTransport();
    PcPort_ArchivePollTransport();
    /* The two-slot ring is full. Further transport polls must neither
     * overwrite a sector nor announce completion of the third one. */
    for (int i = 0; i < 10; ++i) PcPort_ArchivePollTransport();
    if (s_sync_calls != 6 || D_8004FDFC != 1) return 16;
    first = (u8*)(uintptr_t)(u32)func_80028B14();
    if (!first || first[0] != 0x44) return 17;
    ((ArchiveStreamFileSectionHeader*)(stream + 4))->state = 0;
    ArchiveCdDataSync(0);
    second = (u8*)(uintptr_t)(u32)func_80028B14();
    if (!second || second[0] != 0x45) return 18;
    first = (u8*)(uintptr_t)(u32)func_80028B14();
    if (!first || first[0] != 0x46 || func_80028B14() != 0) return 19;
    return 0;
}
