#ifndef XENO_RENDERER_CD_H
#define XENO_RENDERER_CD_H

#include <stdint.h>
#include "psx/types.h"

/* Native CD controller records. Location fields have packed BCD values.
 * The ISO directory result stores byte length in a 32-bit field on both hosts. */
typedef struct {
    unsigned char minute, second, sector, track;
} CdlLOC;

typedef struct {
    unsigned char file, chan;
    unsigned short pad;
} CdlFILTER;

typedef struct {
    CdlLOC pos;
    uint32_t size;
    char name[16];
} CdlFILE;

typedef void (*CdlCB)(unsigned char interrupt, unsigned char *response);

enum {
    CdlNop = 1, CdlSetloc = 2, CdlReadN = 6, CdlStandby = 7,
    CdlStop = 8, CdlPause = 9, CdlSetfilter = 13, CdlSetmode = 14,
    CdlGetlocP = 17, CdlGetTN = 19, CdlSeekL = 21, CdlReadS = 27
};
enum {
    CdlNoIntr = 0, CdlDataReady = 1, CdlComplete = 2, CdlDiskError = 5
};
enum {
    CdlStatError = 1, CdlStatStandby = 2, CdlStatSeekError = 4,
    CdlStatShellOpen = 16, CdlStatRead = 32
};
enum {
    CdlModeSF = 1 << 3, CdlModeSize0 = 1 << 4, CdlModeSize1 = 1 << 5,
    CdlModeRT = 1 << 6, CdlModeSpeed = 1 << 7
};

/* CdReadFile counts 32-bit words per 2048-byte user-data sector. */
enum { SECTOR_SIZE = 2048 / 4 };

#define ENCODE_BCD(decimal) ((((decimal) / 10) * 16) | ((decimal) % 10))
#define DECODE_BCD(bcd) (((bcd) >> 4) * 10 + ((bcd) & 15))

#ifdef __cplusplus
extern "C" {
#endif

void PsyX_CDFS_Init(const char *path, int track, int sector_bytes);
void PsyX_CDFS_Init_Mem(const unsigned int *image, int bytes, int track, int sector_bytes);
CdlFILE *CdSearchFile(CdlFILE *entry, char *path);
int CdReadFile(char *path, u_long *destination, int bytes);

int CdInit(void);
int CdDiskReady(int mode);
int CdSetDebug(int level);
int CdLastCom(void);
char *CdComstr(unsigned char command);
void CdFlush(void);
CdlLOC *CdIntToPos(int sector, CdlLOC *position);
int CdPosToInt(CdlLOC *position);

int CdControl(unsigned char command, unsigned char *parameters, unsigned char *response);
int CdControlB(unsigned char command, unsigned char *parameters, unsigned char *response);
int CdControlF(unsigned char command, unsigned char *parameters);
int CdSync(int mode, unsigned char *response);
int CdReady(int mode, unsigned char *response);
CdlCB CdReadyCallback(CdlCB handler);
CdlCB CdSyncCallback(CdlCB handler);
void *CdDataCallback(void (*handler)(void));
int CdRead(int sectors, u_long *destination, int mode);
int CdReadSync(int mode, unsigned char *response);
CdlCB CdReadCallback(CdlCB handler);
int CdGetSector(void *destination, int words);

/* Port-owned asynchronous transport and movie functions. */
int PsyX_CdDataSectorRate(void);
int PsyX_CdReadTransportBusy(void);
int PsyX_CdCancelReadSector(void *destination);
int PsyX_CdPollReadSector(void *destination, int sector);
int PsyX_CdReadManagedSectors(uint32_t sector, uint32_t count, void *destination);
int PsyX_CdReadFileLogical(const CdlLOC *position, int sectors, void *destination);
uint64_t PsyX_CdFileCursorReceiptAt(int sector);

int PsyX_CdAuthorizeLid(int (*verify)(int));
int PsyX_CdSetLidOpen(int open);
int PsyX_CdLidOpen(void);
int PsyX_CdInsertImage(const char *path, int (*verify)(int));

int PsyX_CdSuspendReadyCallback(CdlCB handler, uint64_t *ticket);
int PsyX_CdRestoreReadyCallback(uint64_t ticket, CdlCB handler, uint64_t *registration);
int PsyX_CdClaimReadyCallback(CdlCB handler, uint64_t *registration);
int PsyX_CdReleaseReadyCallback(CdlCB handler, uint64_t registration);
int PsyX_CdSetlocForReadyCallback(const CdlLOC *position, CdlCB handler, uint64_t registration);
int PsyX_CdPollReadSectorWithReadyCallback(void *destination, int sector, CdlCB handler,
                                          uint64_t registration);
int PsyX_CdControlWithSyncCallback(unsigned char command, unsigned char *parameters, CdlCB handler);
int PsyX_CdPollSyncCallback(CdlCB handler);
int PsyX_CdClearSyncCallback(CdlCB handler);
int PsyX_CdStartSeekWithSyncCallback(const CdlLOC *position, CdlCB handler);
int PsyX_CdStartIdlePauseWithSyncCallback(CdlCB handler);
int PsyX_CdStartActivePauseWithSyncCallback(CdlCB handler, uint64_t (*clock)(void));
int PsyX_CdStartTerminalPauseWithSyncCallback(CdlCB handler, uint64_t (*clock)(void));

int PsyX_CdStartTimedFileSetloc(const CdlLOC *position, CdlCB ready, CdlCB sync,
                               uint64_t *registration, uint64_t (*clock)(void));
/* The WDS loader may retry through the ordinary archive route if the
 * drive left the expected source sector before it accepted the seek. */
enum { PSYX_CD_FILE_ORIGIN_MISMATCH = -2 };
int PsyX_CdStartTimedFileSetlocFrom(int origin, const CdlLOC *position, CdlCB ready, CdlCB sync,
                                   uint64_t *registration, uint64_t (*clock)(void));
int PsyX_CdStartFileReadN(CdlCB ready, uint64_t registration, CdlCB sync);
int PsyX_CdPollFileReadN(int sector, CdlCB ready, uint64_t registration);
int PsyX_CdEndFileReadN(CdlCB ready, uint64_t registration, int pause);
int PsyX_CdSeekAfterFileReadN(const CdlLOC *position, CdlCB ready, uint64_t registration, CdlCB sync);
int PsyX_CdPauseAfterFileReadN(CdlCB ready, uint64_t registration, CdlCB sync, uint64_t (*clock)(void));

int PsyX_CdStartMovieReadS(const CdlLOC *position, unsigned char mode, const CdlFILTER *filter,
                          CdlCB ready, uint64_t *registration);
int PsyX_CdStartMovieReadSRange(const CdlLOC *position, unsigned char mode, const CdlFILTER *filter,
                               CdlCB ready, uint64_t *registration, int first, int end);
int PsyX_CdStopMovieReadS(CdlCB ready, uint64_t registration, int (*restore)(void));
int PsyX_CdDeliveringMovieReadS(void);
int PsyX_CdMovieSectorLba(void);
int PsyX_CdParkMovieReadS(CdlCB ready, uint64_t registration);
int PsyX_CdMovieReadSAtEnd(CdlCB ready, uint64_t registration);
int PsyX_CdMarkSkippedStreamSector(CdlCB handler);
int PsyX_CdRetireSkippedStreamSector(CdlCB handler);
int PsyX_CdRetireStreamNotice(CdlCB handler);
void PsyX_CdMovieOpeningSector(int first_chunk, unsigned int final_picture);
int PsyX_CdMovieOpeningReady(CdlCB ready, uint64_t registration, unsigned int uploaded);
void PsyX_CdPaceMoviePicture(void);
void PsyX_CdMovieTerminalSector(unsigned int frame, unsigned int end, int first, int last);
unsigned long long PsyX_CdBeginMovieTerminalFade(CdlCB ready, uint64_t registration,
                                                unsigned int uploaded, unsigned int end);
int PsyX_CdMovieTerminalReady(CdlCB ready, uint64_t registration, unsigned int uploaded, unsigned int end);

int PsyX_CdPollReadSectorForReadyOwner(void *destination, int sector, CdlCB handler,
                                      uint64_t registration);
int PsyX_CdStartPauseWithSyncCallback(CdlCB handler);
int PsyX_CdStartFileSetloc(const CdlLOC *position, CdlCB ready, CdlCB sync, uint64_t *registration);

#ifdef __cplusplus
}
#endif
#endif
