#ifndef _XENO_ARCHIVE_H
#define _XENO_ARCHIVE_H
#include "system/archive_disc.h"

#include "psyq/libcd.h"

#ifdef XENO_PC_PORT
int ArchiveClearStreamFileSections(void);
/* Game-thread service for a pending archive CD mode change or logical seek. */
extern void (*g_PcPortArchiveModeService)(void);
/* Keep the accepted boot destination until the blocking archive join completes. */
int PcPort_ArchiveSubmitBootWds5(void* destination, int capacity);
/* A blocking join returns zero on success or -1 on a failed transfer. */
int PcPort_ArchiveCdDataSyncStatus(int mode);
/* The arena owns its portrait destination until the CD join. */
int PcPort_ArchiveSubmitBattlingPortraits(void* destination, int capacity);
int PcPort_ArchiveOwnsBattlingPortraits(void);
/* 80080644 reads one two-sector record out of arena file 6. Its caller joins
 * the previous record before submitting the next, then joins before upload. */
int PcPort_ArchiveSubmitBattlingPortraitRecord(unsigned slot, void *destination, int capacity);
/* Submit in directory 0x1c. Keep the destination alive through the field's
 * poll (0 = ready, ARCHIVE_READ_RETRY = busy, -1 = failed) or teardown join. */
int PcPort_ArchiveSubmitFieldSong(int index, void* destination, int capacity);
int PcPort_ArchiveFieldSongIndex(void);
int PcPort_ArchivePollFieldSong(void);
void PcPort_ArchiveFinishFieldSong(void);
void PcPort_ArchiveBeginFieldFrame(int vblanks);
void PcPort_ArchiveEndFieldFrame(void);
void PcPort_ArchiveLeaveFieldFrame(void);
/* Nonzero while an ordinary file read owns its SDK READY generation. */
extern uint64_t g_PcPortArchiveFileRegistration;
extern void (*g_PcPortArchiveQueueService)(void);
/* Set while a file-completion hook owns the active game-thread stack frame.
 * New reads can be retried after the hook returns. */
extern void (*g_PcPortArchiveCancelFilteredQueue)(void);
#define ARCHIVE_READ_RETRY (-2)
#define ARCHIVE_FIELD_WDS_CLOCK 0x10000
enum {
    ARCHIVE_PROLOGUE_WDS_INDEX = 0x65,
    ARCHIVE_PROLOGUE_WDS_ORIGIN = 121890,
    ARCHIVE_PROLOGUE_WDS_SECTOR = 115251
};
int PcPort_ArchiveDriveAcceptNotification(u_char status, const u_char* result);
void PcPort_ArchiveDriveCommand(u_char command, u_char* param);
void PcPort_ArchiveDriveClearCallback(void);
void PcPort_ArchiveRefuseRecovery(void);
int PcPort_ArchiveHasSeparateTransfer(void);
int PcPort_ArchiveSaveReadyCallback(void);
int PcPort_ArchiveRestoreReadyCallback(void);
#endif

#ifdef XENO_PC_PORT
/* The file READY handler first transfers the Mode 2 address and both XA
 * subheaders, then consumes the user payload in separate word transfers. */
typedef struct {
    CdlLOC position;
    u8 subheader[4];
    u8 repeatedSubheader[4];
} ArchiveCdSectorHeader;
extern ArchiveCdSectorHeader D_80059EF8;
extern s32 D_8005A4DC;
#endif

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
#ifdef XENO_PC_PORT
extern u32 D_80059F08;
#else
extern void* D_80059F08;
#endif
extern u32 D_80059F0C;
extern char D_80059F14[]; // CdlFilter
#ifdef XENO_PC_PORT
extern u_char D_80059F18[4];
/* Retail clears the mode parameter backwards starting at its last byte. */
#define D_80059F1B (D_80059F18[3])
#else
extern u_char D_80059F18[];
extern u8* D_80059F1B;
#endif
extern int D_8004FE34;
extern int D_8004FE38;
#ifdef XENO_PC_PORT
extern s32 D_8004FE3C;
#endif
extern char D_80059F15;

int ArchiveSetIndex(int directoryIndex, int entryIndex);
int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder);
int ArchiveDecodeSize(int entryIndex);
char* ArchiveGetFilePath(int entryIndex);
int ArchiveDecodeSector(int entryIndex);
void ArchiveCdSeekToFile(int entryIndex);
void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult);

#endif
