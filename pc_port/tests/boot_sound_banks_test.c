#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/sound.h"
#include "boot_sound_banks.h"

extern void *D_80059560;
extern void *D_800595AC;
extern void *D_8006251C;
extern void *D_80062524;

static unsigned char s_buffers[4][32];
static SoundWDSEntry s_entries[4];
static int s_allocCount;
static int s_loadCount;
static int s_freeCount;
static int s_syncCalls;
static int s_transferSyncFlags;
static int s_events[32];
static int s_eventCount;

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "boot sound banks: FAIL: %s\n", message);
        exit(1);
    }
}

static void event(int value)
{
    s_events[s_eventCount++] = value;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    check(directoryIndex == 0 && entryIndex == 1,
          "retail archive selection is 0/1");
    event(1);
    return 0;
}

int ArchiveDecodeSize(int entryIndex)
{
    check(entryIndex >= 2 && entryIndex <= 5, "decoded file is 2..5");
    event(10 + entryIndex);
    return 0x100 + entryIndex;
}

void *HeapAlloc(int size, int flags)
{
    check(s_allocCount < 4, "only four source buffers are allocated");
    check(size == 0x102 + s_allocCount && flags == 0,
          "allocation uses decoded size and retail flags");
    event(20 + s_allocCount);
    return s_buffers[s_allocCount++];
}

void ArchiveReadFileToBuffer(int fileIndex, void *buffer, int arg2, int arg3)
{
    check(fileIndex >= 2 && fileIndex <= 5, "read file is 2..5");
    check(buffer == s_buffers[fileIndex - 2], "read uses matching allocation");
    check(arg2 == 0 && arg3 == 0, "read preserves retail zero arguments");
    ((unsigned char *)buffer)[0] = (unsigned char)fileIndex;
    event(30 + fileIndex);
}

int ArchiveCdDataSync(int mode)
{
    check(mode == 0, "archive sync mode");
    s_syncCalls++;
    event(40);
    return 0;
}

SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode)
{
    int index = ((unsigned char *)source)[0] - 2;
    check(index == s_loadCount && mode == 0,
          "WDS loads files 2..5 in order with automatic allocation");
    event(50 + index);
    s_loadCount++;
    return &s_entries[index];
}

s16 func_8003BDFC(s32 flags)
{
    s_transferSyncFlags = flags;
    event(60);
    return 0;
}

void HeapFree(void *memory)
{
    check(s_freeCount < 4 && memory == s_buffers[s_freeCount],
          "source buffers are freed in retail order");
    event(70 + s_freeCount);
    s_freeCount++;
}

int main(void)
{
    static const int expected[] = {
        1,
        12, 20, 32,
        13, 21, 33,
        14, 22, 34,
        15, 23, 35,
        40,
        50, 51, 52, 53,
        60,
        70, 71, 72, 73,
    };
    int i;

    memset(s_buffers, 0, sizeof(s_buffers));
    memset(s_entries, 0, sizeof(s_entries));
    D_80059560 = D_800595AC = D_8006251C = D_80062524 = (void *)1;

    PcPort_LoadRetailBootSoundBanks();

    check(s_syncCalls == 1, "all four reads share one archive sync");
    check(s_transferSyncFlags == 0x10, "SPU transfer queue is drained");
    check(D_80059560 == &s_entries[1], "second WDS handle is published");
    check(D_800595AC == &s_entries[3], "fourth WDS handle is published");
    check(D_8006251C == (void *)1 && D_80062524 == (void *)1,
          "field copies are not published before FieldMain");
    check(s_eventCount == (int)(sizeof(expected) / sizeof(expected[0])),
          "event count matches retail sequence");
    for (i = 0; i < s_eventCount; i++) {
        check(s_events[i] == expected[i], "event order matches retail sequence");
    }

    puts("boot sound banks: PASS");
    return 0;
}
