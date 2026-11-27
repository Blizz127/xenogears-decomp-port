#include "archive_ready.h"
#include "system/archive.h"
#include "xg_plat/timing.h"
#ifdef _WIN32
#include <synchapi.h>
#include "win32_clock.h"
typedef SRWLOCK ArchiveLock;
#define ARCHIVE_LOCK_INIT SRWLOCK_INIT
#define archive_lock AcquireSRWLockExclusive
#define archive_unlock ReleaseSRWLockExclusive
#else
#include <pthread.h>
#include <time.h>
typedef pthread_mutex_t ArchiveLock;
#define ARCHIVE_LOCK_INIT PTHREAD_MUTEX_INITIALIZER
#define archive_lock pthread_mutex_lock
#define archive_unlock pthread_mutex_unlock
#endif

#pragma weak xg_plat_timing_speed

uint64_t PcPort_ArchiveClockUsec(void) {
    /* CdSync can sample this clock under the SDK's CD lock. The platform
     * speed getter reads SDL atomics and this mutex protects only the clock. */
    static ArchiveLock clockLock = ARCHIVE_LOCK_INIT;
    static uint64_t lastWallUsec, archiveUsec;
    static int lastSpeed;
    archive_lock(&clockLock);
#ifdef _WIN32
    uint64_t wallUsec = PcPort_Win32MonotonicUsec();
    if (wallUsec != 0) {
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
        uint64_t wallUsec = (uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_nsec / 1000;
#endif
        if (lastSpeed)
            archiveUsec += (wallUsec - lastWallUsec) * lastSpeed;
        else
            archiveUsec = wallUsec;
        lastWallUsec = wallUsec;
        /* Keep elapsed time at the old rate when a new speed is observed. */
        lastSpeed = xg_plat_timing_speed ? xg_plat_timing_speed() : XG_PLAT_SPEED_MIN;
    }
    uint64_t result = archiveUsec;
    archive_unlock(&clockLock);
    return result;
}

/* The field's renderer waits for a scheduled number of NTSC VBlanks. Host
 * rendering can overrun that budget while the original field does the same
 * work in time. The measured WDS and adjacent SMDS alone use this clock.
 * Between the two render boundaries their real READY/ACK deadlines can
 * mature at fractional-frame times, including during Vsync(-1). */
static ArchiveLock fieldClockLock = ARCHIVE_LOCK_INIT;
static struct {
    enum { FIELD_CD_WALL, FIELD_CD_RENDER, FIELD_CD_BOUNDARY } phase;
    uint64_t guest, wall, frameUsec;
} fieldClock;

static uint64_t FieldClockAt(uint64_t wall) {
    uint64_t elapsed = wall > fieldClock.wall ? wall - fieldClock.wall : 0;
    if (fieldClock.phase == FIELD_CD_BOUNDARY) return fieldClock.guest;
    if (fieldClock.phase == FIELD_CD_RENDER && elapsed > fieldClock.frameUsec)
        elapsed = fieldClock.frameUsec;
    return fieldClock.guest + elapsed;
}

uint64_t PcPort_ArchiveFieldClockAt(uint64_t wall) {
    archive_lock(&fieldClockLock);
    uint64_t now = FieldClockAt(wall);
    archive_unlock(&fieldClockLock);
    return now;
}

void PcPort_ArchiveBeginFieldFrame(int vblanks) {
    uint64_t wall = PcPort_ArchiveClockUsec();
    archive_lock(&fieldClockLock);
    fieldClock.guest = FieldClockAt(wall);
    fieldClock.wall = wall;
    /* US Disc 1 measured 566,120 CPU clocks per NTSC noninterlaced VBlank
     * at 33.8688 MHz. Preserve the renderer's script-selected frame rate. */
    fieldClock.frameUsec = (uint64_t)(vblanks > 0 ? vblanks : 1) * 566120 * 10000 / 338688;
    fieldClock.phase = FIELD_CD_RENDER;
    archive_unlock(&fieldClockLock);
}

void PcPort_ArchiveEndFieldFrame(void) {
    uint64_t wall = PcPort_ArchiveClockUsec();
    archive_lock(&fieldClockLock);
    fieldClock.guest = fieldClock.phase == FIELD_CD_RENDER ?
        fieldClock.guest + fieldClock.frameUsec : FieldClockAt(wall);
    fieldClock.phase = FIELD_CD_BOUNDARY;
    archive_unlock(&fieldClockLock);
}

/* A real blocking drive join or a field exit has no following render edge.
 * Continue from the same timestamp on the free-running drive clock until
 * the field starts another frame. */
void PcPort_ArchiveLeaveFieldFrame(void) {
    uint64_t wall = PcPort_ArchiveClockUsec();
    archive_lock(&fieldClockLock);
    fieldClock.guest = FieldClockAt(wall);
    fieldClock.wall = wall;
    fieldClock.phase = FIELD_CD_WALL;
    archive_unlock(&fieldClockLock);
}

int PcPort_ArchiveGetWdsSector(int expectedSector, void* destination) {
    return CdGetSector(&D_80059EF8, 3) &&
           CdPosToInt(&D_80059EF8.position) == expectedSector &&
           CdGetSector(destination, CD_SECTOR_SIZE / 4);
}

extern int PcPort_ArchiveFieldWdsActive(void) __attribute__((weak));

void PcPort_ArchiveWaitForQueueTick(void) {
    if (PcPort_ArchiveFieldWdsActive && PcPort_ArchiveFieldWdsActive())
        PcPort_ArchiveLeaveFieldFrame();
#ifdef _WIN32
    SDL_Delay(1);
#else
    const struct timespec delay = { 0, 1000000 };
    nanosleep(&delay, NULL);
#endif
}

extern s32 func_8002B084(u_char status);
extern s32 func_8002B2F0(u_char status);
extern void func_8002B5D0(u_char status);

/* The host bodies for these three original one-argument handlers have a
 * different C ABI from CdlCB. Keep their SDK entry points type-correct. */
void PcPort_ArchiveFileReady(u_char status, u_char* result) {
    (void)result;
    func_8002B084(status);
}

static void ArchiveRingReady(u_char status, u_char* result) {
    (void)result;
    func_8002B2F0(status);
}

static void ArchiveAdpcmReady(u_char status, u_char* result) {
    (void)result;
    func_8002B5D0(status);
}

static const struct {
    u32 address;
    CdlCB callback;
} readyHandlers[] = {
    {ARCHIVE_FILE_READY, PcPort_ArchiveFileReady},
    {ARCHIVE_RING_READY, ArchiveRingReady},
    {ARCHIVE_ADPCM_READY, ArchiveAdpcmReady},
    {0, NULL}
};

/* The retail slot contains only code addresses. The native SDK generation
 * belongs to the outstanding callback exchange and never enters guest RAM. */
static ArchiveLock readyLock = ARCHIVE_LOCK_INIT;
static uint64_t savedTicket;

static int ArchiveSaveReadyAt(unsigned i) {
    uint64_t ticket;
    if (!PsyX_CdSuspendReadyCallback(readyHandlers[i].callback, &ticket))
        return 0;
    savedTicket = ticket;
    D_80059F08 = readyHandlers[i].address;
    return 1;
}

int PcPort_ArchiveSuspendReady(u32 address) {
    int saved = 0;
    archive_lock(&readyLock);
    for (unsigned i = 0; i < sizeof(readyHandlers) / sizeof(readyHandlers[0]); ++i) {
        if (readyHandlers[i].address == address) {
            saved = ArchiveSaveReadyAt(i);
            break;
        }
    }
    archive_unlock(&readyLock);
    return saved;
}

int PcPort_ArchiveSaveReadyCallback(void) {
    int saved = 0;
    archive_lock(&readyLock);
    for (unsigned i = 0; i < sizeof(readyHandlers) / sizeof(readyHandlers[0]); ++i) {
        if (ArchiveSaveReadyAt(i)) {
            saved = 1;
            break;
        }
    }
    archive_unlock(&readyLock);
    return saved;
}

int PcPort_ArchiveRestoreReadyCallback(void) {
    int restored = 0;
    archive_lock(&readyLock);
    if (savedTicket) {
        for (unsigned i = 0; i < sizeof(readyHandlers) / sizeof(readyHandlers[0]); ++i) {
            if (readyHandlers[i].address == D_80059F08) {
                restored = PsyX_CdRestoreReadyCallback(savedTicket, readyHandlers[i].callback, NULL);
                break;
            }
        }
        if (!restored)
            PsyX_CdRestoreReadyCallback(savedTicket, NULL, NULL);
        savedTicket = 0;
    }
    archive_unlock(&readyLock);
    return restored;
}
