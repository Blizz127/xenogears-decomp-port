#ifndef XENO_CD_INTERFACE_H
#define XENO_CD_INTERFACE_H

#include "psyq/sys/types.h"
#ifdef XENO_PC_PORT
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The game sends the first three BCD bytes to Setloc. The fourth byte
 * occupies the mode/track position in raw sector headers and directory data. */
typedef struct {
    unsigned char minute, second, sector, track;
} CdlLOC;

typedef struct {
    unsigned char file, chan;
    unsigned short reserved;
} CdlFILTER;

typedef struct {
    unsigned char val0, val1, val2, val3;
} CdlATV;

typedef void (*CdlCB)(unsigned char interrupt, unsigned char *response);

/* Drive command bytes, response interrupts, and status/mode bits follow
 * the CD-ROM command and register tables at psx-spx.consoledev.net/cdromdrive/. */
enum {
    CdlNop = 0x01, CdlSetloc = 0x02, CdlReadN = 0x06,
    CdlStop = 0x08, CdlPause = 0x09, CdlDemute = 0x0c,
    CdlSetfilter = 0x0d, CdlSetmode = 0x0e, CdlGetlocP = 0x11,
    CdlGetTN = 0x13, CdlSeekL = 0x15, CdlReadS = 0x1b
};
enum {
    CdlNoIntr = 0, CdlDataReady = 1, CdlComplete = 2, CdlDiskError = 5
};
enum {
    CdlStatError = 1 << 0, CdlStatStandby = 1 << 1, CdlStatSeekError = 1 << 2,
    CdlStatShellOpen = 1 << 4, CdlStatRead = 1 << 5,
    CdlStatSeek = 1 << 6, CdlStatPlay = 1 << 7
};
enum {
    CdlModeSF = 1 << 3, CdlModeSize0 = 1 << 4, CdlModeSize1 = 1 << 5,
    CdlModeRT = 1 << 6, CdlModeSpeed = 1 << 7
};
/* ArchiveReadFile's ring-buffer flag (func_80085560); never sent to the drive. */
enum { CdlModeStream = 0x100 };

/* Signatures follow the system archive, sound, and src/slus_006.64/psyq/libcd
 * translation units. Buffer lengths for CdGetSector are measured in words. */
int CdInit(void);
void CdFlush(void);
int CdSetDebug(int level);
int CdLastCom(void);
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
int CdDataSync(int mode);
int CdRead(int sectors, u_long *destination, int mode);
int CdReadSync(int mode, unsigned char *response);
CdlCB CdReadCallback(CdlCB handler);
int CdGetSector(void *destination, int words);
int CdMix(CdlATV *levels);

#ifndef XENO_PC_PORT
/* The console's event.c resets the controller and configures cdread.c. */
int CdReset(int mode);
int CdReadMode(int mode);

/* cd_read_retry() samples the shell-open bit and read mode before restarting. */
int CdStatus(void);
int CdMode(void);

/* CdLastPos returns the controller's stored position for the retry's Setloc. */
CdlLOC *CdLastPos(void);

/* cb_read() uses the asynchronous DMA path when a data callback is installed. */
int CdGetSector2(void *destination, int words);
#else
/* Native disc backend used by archive_port.c, archive_ready.c and
 * movie_player.c. pc_port/patches supplies the matching PsyCross definitions. */
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
#endif

#ifdef __cplusplus
}
#endif
#endif
