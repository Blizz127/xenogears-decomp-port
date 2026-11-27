#ifndef XENO_ARCHIVE_READY_H
#define XENO_ARCHIVE_READY_H

#include <stdint.h>
#include "types.h"
#include "psyq/libcd.h"

enum {
    ARCHIVE_QUEUE_READY = 0x8002ac24u,
    ARCHIVE_FILE_READY = 0x8002b084u,
    ARCHIVE_RING_READY = 0x8002b2f0u,
    ARCHIVE_ADPCM_READY = 0x8002b5d0u
};

uint64_t PcPort_ArchiveClockUsec(void);
uint64_t PcPort_ArchiveFieldClockAt(uint64_t wall);
int PcPort_ArchiveGetWdsSector(int expectedSector, void* destination);
void PcPort_ArchiveWaitForQueueTick(void);
void PcPort_ArchiveFileReady(u_char status, u_char* result);
int PcPort_ArchiveSuspendReady(u32 address);
int PcPort_ArchiveQueueRequiresSeek(int firstLba, int sectors, int nextLba);

#endif
