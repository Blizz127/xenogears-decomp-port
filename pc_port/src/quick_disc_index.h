#ifndef XENO_QUICK_DISC_INDEX_H
#define XENO_QUICK_DISC_INDEX_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* The retail validator checks sector 23, copies sixteen 2048-byte sectors
 * into the FAT, then takes 122 directory bytes from sector 40. Keep these
 * raw sectors from the digest stream until the SDK accepts its exact file. */
typedef struct PcPortDiscIndex {
    unsigned char sectors[18][2352];
} PcPortDiscIndex;

static inline void PcPort_CaptureDiscIndex(PcPortDiscIndex *index, uint64_t offset,
                                           const unsigned char *chunk, size_t bytes)
{
    const uint64_t first = 23u * 2352u;
    const uint64_t end = first + sizeof *index;
    if (!index || offset >= end || offset + bytes <= first) return;
    uint64_t begin = offset > first ? offset : first;
    uint64_t stop = offset + bytes < end ? offset + bytes : end;
    memcpy((unsigned char *)index + (begin - first), chunk + (begin - offset), stop - begin);
}

static inline int PcPort_DiscIndexMatches(const PcPortDiscIndex *index, int disc)
{
    const unsigned char *id = index->sectors[0] + 24;
    const unsigned char *directory = index->sectors[17] + 24;
    return (disc == 1 || disc == 2) && id[3] == '0' + disc &&
           !memcmp(id + 4, "_XEN", 4) && directory[0x78] == disc && directory[0x79] == 0;
}

static inline void PcPort_InstallDiscIndex(const PcPortDiscIndex *index,
                                          unsigned char *table, unsigned char *directory)
{
    for (int i = 0; i < 16; ++i)
        memcpy(table + 2048 * i, index->sectors[1 + i] + 24, 2048);
    memcpy(directory, index->sectors[17] + 24, 122);
}

int PcPort_VerifyDiscIndex(int fd, int disc, PcPortDiscIndex *index);

#endif
