#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/sound.h"
#include "boot_sound_banks.h"

extern void *D_80059560, *D_800595AC, *D_8006251C, *D_80062524;
u32 g_ArchiveHeader, g_ArchiveTable;

enum Fault {
    OK, NO_DIRECTORY_TABLE, NO_FILE_TABLE, NO_DIRECTORY, BAD_DIRECTORY,
    MISSING_FILE, OVERSIZED_FILE, OUT_OF_MEMORY, BLOCKING_READ_ERROR,
    REFUSED_FILE5, BUSY_FILE5, LATE_FILE5_ERROR, WDS_LOAD_ERROR
};
static const int fileBytes[] = {16, 24, 20, 25};
static unsigned char buffers[4][32], expected[4][32];
static SoundWDSEntry entries[4], previous[4];
static struct {
    enum Fault fault;
    int allocations, reads, admits, joins, loads, drains, freed, released;
    int events[40], nEvents;
    void *pending;
} run;

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "boot sound banks case %d failed: %s\n", run.fault, message);
        exit(1);
    }
}

static void event(int value)
{
    check(run.nEvents < (int)(sizeof(run.events) / sizeof(run.events[0])), "event capacity");
    run.events[run.nEvents++] = value;
}

static int capacity(int i)
{
    return (fileBytes[i] + 3) & ~3;
}

static int untouched(int i)
{
    for (int n = 0; n < (int)sizeof(buffers[i]); ++n)
        if (buffers[i][n] != 0xa5)
            return 0;
    return 1;
}

int ArchiveSetIndex(int directory, int entry)
{
    check(directory == 0 && entry == 1 && g_ArchiveHeader && g_ArchiveTable,
          "initialized retail directory selection");
    event(1);
    if (run.fault == NO_DIRECTORY) return -1;
    if (run.fault == BAD_DIRECTORY) return 5000;
    return 23;
}

int ArchiveDecodeSize(int file)
{
    check(file >= 2 && file <= 5, "WDS source index");
    event(10 + file);
    if (file == 4 && run.fault == MISSING_FILE) return 0;
    if (file == 4 && run.fault == OVERSIZED_FILE) return INT_MAX;
    return fileBytes[file - 2];
}

void *HeapAlloc(u_int bytes, u_int flags)
{
    int i = run.allocations;
    check(i < 4 && bytes == (u_int)capacity(i) && flags == 0, "valid allocation capacity");
    event(20 + i);
    if (run.fault == OUT_OF_MEMORY && i == 2) return NULL;
    ++run.allocations;
    return buffers[i];
}

s32 ArchiveReadFileToBuffer(s32 file, void *buffer, u32 arg2, u32 flags)
{
    int i = file - 2;
    check(i >= 0 && i < 3 && i == run.reads && run.allocations == 4,
          "all four allocations precede the three blocking reads");
    check(buffer == buffers[i] && arg2 == 0 && flags == 0, "blocking read arguments");
    event(30 + file);
    ++run.reads;
    if (run.fault == BLOCKING_READ_ERROR && file == 3) return -1;
    memcpy(buffer, expected[i], capacity(i));
    return 0;
}

int PcPort_ArchiveSubmitBootWds5(void *destination, int bytes)
{
    check(run.reads == 3 && !run.admits && !run.pending, "one final file admission");
    check(destination == buffers[3] && bytes == capacity(3) && untouched(3),
          "final file capacity and pristine destination at admission");
    event(35);
    ++run.admits;
    if (run.fault == REFUSED_FILE5) return -1;
    if (run.fault == BUSY_FILE5) return -2;
    run.pending = destination;
    return 0;
}

int PcPort_ArchiveCdDataSyncStatus(int mode)
{
    check(mode == 0 && run.pending == buffers[3] && !run.joins, "join accepted file once");
    check(untouched(3) && !run.loads, "file 5 remains unpublished until join");
    event(40);
    ++run.joins;
    void *destination = run.pending;
    run.pending = NULL;
    if (run.fault == LATE_FILE5_ERROR) return -1;
    memcpy(destination, expected[3], capacity(3));
    return 0;
}

SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode)
{
    int i = run.loads;
    check(i < 4 && source == (void *)buffers[i] && mode == 0, "WDS source order");
    check(run.joins == 1 && !run.pending && !memcmp(source, expected[i], capacity(i)),
          "sound consumes the complete published bytes after join");
    check(D_80059560 == &previous[0] && D_800595AC == &previous[1],
          "existing boot handles survive until all banks load");
    event(50 + i);
    ++run.loads;
    if (run.fault == WDS_LOAD_ERROR && i == 2) return NULL;
    return &entries[i];
}

int func_8003BDFC(s32 flags)
{
    check(flags == 0x10 && !run.drains && !run.freed, "drain transfers before freeing sources");
    if (run.fault == OK)
        check(D_80059560 == &entries[1] && D_800595AC == &entries[3],
              "retail handles published before SPU sync");
    ++run.drains;
    event(60);
    return 0;
}

void SoundFreeWdsEntry(SoundWDSEntry *entry)
{
    check(run.fault == WDS_LOAD_ERROR && run.drains && run.released < 2 &&
          entry == &entries[run.released], "release only newly loaded handles after draining");
    ++run.released;
}

u_int HeapFree(void *memory)
{
    check(run.freed < run.allocations && memory == buffers[run.freed] && !run.pending,
          "free owned source buffers in allocation order");
    if (run.loads)
        check(run.drains == 1, "SPU finished with source buffers");
    event(70 + run.freed++);
    return 0;
}

static void exercise(enum Fault fault, int owned, int reads, int admits, int joins, int loads)
{
    memset(&run, 0, sizeof(run));
    run.fault = fault;
    memset(buffers, 0xa5, sizeof(buffers));
    g_ArchiveHeader = fault != NO_DIRECTORY_TABLE;
    g_ArchiveTable = fault != NO_FILE_TABLE;
    D_80059560 = &previous[0];
    D_800595AC = &previous[1];
    D_8006251C = &previous[2];
    D_80062524 = &previous[3];

    check(PcPort_LoadRetailBootSoundBanks() == (fault == OK), "report complete boot bank success");

    check(run.allocations == owned && run.freed == owned && run.reads == reads &&
          run.admits == admits && run.joins == joins && run.loads == loads && !run.pending,
          "owned requests, source buffers and sound loads");
    check(run.drains == (loads != 0) && run.released == (fault == WDS_LOAD_ERROR ? 2 : 0),
          "owned sound transfers and handles");
    check(D_8006251C == &previous[2] && D_80062524 == &previous[3],
          "field globals stay with FieldMain");
    if (fault != OK) {
        check(D_80059560 == &previous[0] && D_800595AC == &previous[1],
              "failure preserves both prior boot handles");
        if (!loads) check(untouched(3), "rejected or failed file never writes its caller");
    }
}

int main(void)
{
    static const int order[] = {
        1, 12, 20, 13, 21, 14, 22, 15, 23, 32, 33, 34, 35, 40,
        50, 51, 52, 53, 60, 70, 71, 72, 73
    };
    for (int i = 0; i < 4; ++i)
        for (int n = 0; n < capacity(i); ++n)
            expected[i][n] = (unsigned char)(17 * i + 3 * n + 1);

    exercise(OK, 4, 3, 1, 1, 4);
    check(run.nEvents == (int)(sizeof(order) / sizeof(order[0])) &&
          !memcmp(order, run.events, sizeof(order)), "retail allocation, reads, loads and drain order");
    exercise(NO_DIRECTORY_TABLE, 0, 0, 0, 0, 0);
    exercise(NO_FILE_TABLE, 0, 0, 0, 0, 0);
    exercise(NO_DIRECTORY, 0, 0, 0, 0, 0);
    exercise(BAD_DIRECTORY, 0, 0, 0, 0, 0);
    exercise(MISSING_FILE, 2, 0, 0, 0, 0);
    exercise(OVERSIZED_FILE, 2, 0, 0, 0, 0);
    exercise(OUT_OF_MEMORY, 2, 0, 0, 0, 0);
    exercise(BLOCKING_READ_ERROR, 4, 2, 0, 0, 0);
    exercise(REFUSED_FILE5, 4, 3, 1, 0, 0);
    exercise(BUSY_FILE5, 4, 3, 1, 0, 0);
    exercise(LATE_FILE5_ERROR, 4, 3, 1, 1, 0);
    exercise(WDS_LOAD_ERROR, 4, 3, 1, 1, 3);
    puts("boot sound banks: allocation order, deferred WDS5 join, SPU drain and failure cleanup PASS");
    return 0;
}
