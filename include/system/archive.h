#ifndef _XENO_ARCHIVE_H
#define _XENO_ARCHIVE_H

#include "psyq/libcd.h"

#define CD_SECTOR_SIZE 0x800
#define RETRY_COUNT 0x4

#define STREAM_FILE_HEADER_SIZE 0x24

#define ARCHIVE_HEADER_ENTRY_SIZE 0x7
#define ARCHIVE_ENTRY_NAME_BUFFER_SIZE 0x40
#define ARCHIVE_MAX_SECTIONS 0x40

#define ARCHIVE_CD_DRIVE_IDLE 0x0
#define ARCHIVE_CD_DRIVE_READ_SECTOR 0x1
#define ARCHIVE_CD_DRIVE_READ_DONE 0x2
#define ARCHIVE_CD_DRIVE_SEEK 0x3
#define ARCHIVE_CD_DRIVE_SEEK_DONE 0x4
#define ARCHIVE_CD_DRIVE_DONE 0x5
#define ARCHIVE_CD_DRIVE_READ_DATA_SECTOR 0x6
#define ARCHIVE_CD_DRIVE_HALT 0x7
#define ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR 0x8
#define ARCHIVE_CD_DRIVE_RESET_MODE 0x9
#define ARCHIVE_CD_DRIVE_ERROR 0xA
#define ARCHIVE_CD_DRIVE_HANDLE_ERROR 0xB
#define ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL 0xC

#define ARCHIVE_CD_DRIVE_ERR_SEEK 0x1
#define ARCHIVE_CD_DRIVE_ERR_GENERIC 0x2
#define ARCHIVE_CD_DRIVE_ERR_READ 0x3
#define ARCHIVE_CD_DRIVE_ERR_HALT 0x4
#define ARCHIVE_CD_DRIVE_ERR_ADPCM 0x5
#define ARCHIVE_CD_DRIVE_ERR_RESET_MODE 0x6

#define ARCHIVE_STREAM_FILE_NOT_LOADED 0x0

typedef struct {
    u_short state;
    u_short id;
    u_short size; // In # file sectors
    u_short unk2;
} ArchiveStreamFileSectionHeader;

typedef struct {
    /* 0x0 */ short archiveIndex;
    /* 0x2 */ u16 _pad;
    /* 0x4 */ void* pData;
} StreamDataQueueEntry;

extern ArchiveStreamFileSectionHeader* D_8004FE2C;
extern int D_8004FE40;

extern u32 g_ArchiveDebugTable;
extern u32 g_ArchiveHeader;
extern u32 g_ArchiveTable;
extern u32 g_CurArchiveOffset;
extern int D_8004FE18;
extern u_int g_ArchiveCdDriveState;
extern u_int g_ArchiveCdDriveError;
extern void* g_ArchiveCurStreamFile;
extern CdlLOC g_ArchiveCdCurLocation;
extern int g_ArchiveCurFileSize;

extern s32 D_8004FDFC;
extern s32 D_8004FE4C;
extern u_char* D_80059F1C;
extern s32 D_8005A49C;
extern s32 D_8005A4A4;

extern u32 D_8005A488;
extern u32 D_8005A48C;
extern u32 D_8005A490;
extern u32 D_8005A494;
extern u32 D_8005A498;
extern u32 D_8005A4A8;
extern u32 D_8005A4B4;
extern void* D_80059F08;
extern char D_80059F14[]; // CdlFilter
extern u_char D_80059F18[]; // u_char* - mode
extern u8* D_80059F1B;
extern int D_8004FE34;
extern int D_8004FE38;
extern char D_80059F15;

int ArchiveSetIndex(int directoryIndex, int entryIndex);
int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder);
int ArchiveDecodeSize(int entryIndex);
char* ArchiveGetFilePath(int entryIndex);
int ArchiveDecodeSector(int entryIndex);
void ArchiveCdSeekToFile(int entryIndex);
void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult);

#endif