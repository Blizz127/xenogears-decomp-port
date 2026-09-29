#include "boot_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "../include/xg_plat/disc.h"

#ifndef BOOT_CERT_DIRECT_DISC
#include "common.h"
#include "system/archive.h"
extern u32 g_ArchiveTable;
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize,
                                       s32 arg3, u32 flags);
#endif

#define BOOT_STR_MAGIC 0x0160
#define BOOT_STR_PICTURE 0x8001
#define BOOT_TABLE_LBA 0x18
#define BOOT_TABLE_SECTORS 0x10
#define BOOT_TABLE_BYTES (BOOT_TABLE_SECTORS * 0x800)

static unsigned char* s_str;
static int s_strPrefixSize;
static int s_strFileSize;
static int s_strLba;
static int s_strNsect;
static int s_strEntry;
static unsigned char* s_title;
static int s_titleSize;
static int s_titleOff;
static int s_titleNs;
static int s_titleLen;
static int s_idxOff[PC_PORT_BOOT_STR_INDEX_MAX];
static int s_idxNs[PC_PORT_BOOT_STR_INDEX_MAX];
static int s_idxLen[PC_PORT_BOOT_STR_INDEX_MAX];
static int s_idxCount;
static int s_loaded;

static int BootStrEntry(void)
{
#ifdef BOOT_MUTANT_WRONG_STR
    return 2; /* capsule-launch STR, not Opening */
#else
    return PC_PORT_BOOT_INTRO_STR_ENTRY;
#endif
}

/* Raw user-sector reads from the Disc 1 image via the disc/storage layer. */
static int BootDiscReadRaw(int lba, int count, void* dst)
{
    if (lba < 0 || count < 0)
        return -1;
    return xg_plat_disc_read_sectors((uint32_t)lba, (uint32_t)count, dst);
}

int PcPort_BootDiscReadSectors(int lba, int count, void* dst)
{
#ifndef BOOT_CERT_DIRECT_DISC
    if (g_ArchiveTable)
        return ArchiveReadFileFromCdSector(lba, dst, count * 0x800, 0, 0);
#endif
    return BootDiscReadRaw(lba, count, dst);
}

static int BootReadTable(unsigned char* table)
{
#ifndef BOOT_CERT_DIRECT_DISC
    if (g_ArchiveTable) {
        memcpy(table, (const void*)(uintptr_t)g_ArchiveTable, BOOT_TABLE_BYTES);
        return 1;
    }
#endif
    return PcPort_BootDiscReadSectors(BOOT_TABLE_LBA, BOOT_TABLE_SECTORS, table) == 0;
}

static void BootDecodeEntry(const unsigned char* table, int entry, int* lba, int* size)
{
    const unsigned char* e = table + (entry - 1) * 7;
    *lba = e[0] | (e[1] << 8) | (e[2] << 16);
    *size = e[3] | (e[4] << 8) | (e[5] << 16) | (e[6] << 24);
}

static int BootParsePictureHdr(const unsigned char* sec, int* ns, int* flen)
{
    unsigned mag = (unsigned)(sec[0] | (sec[1] << 8));
    unsigned typ = (unsigned)(sec[2] | (sec[3] << 8));
    unsigned sin = (unsigned)(sec[4] | (sec[5] << 8));
    unsigned n = (unsigned)(sec[6] | (sec[7] << 8));

    if (mag != BOOT_STR_MAGIC || sin != 0 || typ != BOOT_STR_PICTURE)
        return 0;
    *ns = n > 0 ? (int)n : 1;
    *flen = (int)(sec[12] | (sec[13] << 8) | (sec[14] << 16) | (sec[15] << 24));
    return 1;
}

int PcPort_BootAssetsLoad(void)
{
    unsigned char table[BOOT_TABLE_BYTES];
    unsigned char sec[0x800];
    int lba, size, nsect, off, frames, ns, flen, back, last_off, last_ns, last_len;
    int entry, prefix_ns;
    unsigned char* buf;

    if (s_loaded)
        return 1;
#ifdef BOOT_MUTANT_SKIP_INTRO
    return 0;
#endif

    memset(table, 0, sizeof(table));
    if (!BootReadTable(table))
        return 0;
    entry = BootStrEntry();
    BootDecodeEntry(table, entry, &lba, &size);
    if (lba <= 0 || size < 0x800)
        return 0;
    nsect = size / 0x800;
    /* Table size for file 15 overruns into later STRs (file 16 @ 98917,
     * file 17 "Playing with mom"). Last-picture search must stay inside
     * the Opening span: next archive LBA minus this LBA. */
#ifndef BOOT_MUTANT_OVERRUN_TITLE
    {
        int next_lba = 0, next_size = 0;
        BootDecodeEntry(table, entry + 1, &next_lba, &next_size);
        if (next_lba > lba && next_lba - lba < nsect)
            nsect = next_lba - lba;
    }
#endif
    prefix_ns = PC_PORT_BOOT_INTRO_STR_PREFIX_SECTORS;
    if (prefix_ns > nsect)
        prefix_ns = nsect;
    buf = (unsigned char*)malloc((size_t)prefix_ns * 0x800);
    if (!buf)
        return 0;
    if (PcPort_BootDiscReadSectors(lba, prefix_ns, buf) != 0) {
        free(buf);
        return 0;
    }
    if ((buf[0] | (buf[1] << 8)) != BOOT_STR_MAGIC) {
        free(buf);
        return 0;
    }

    frames = 0;
    off = 0;
    while (off < nsect && frames < PC_PORT_BOOT_STR_INDEX_MAX) {
        if (PcPort_BootDiscReadSectors(lba + off, 1, sec) != 0)
            break;
        if (BootParsePictureHdr(sec, &ns, &flen)) {
            s_idxOff[frames] = off;
            s_idxNs[frames] = ns;
            s_idxLen[frames] = flen;
            frames++;
            off += ns;
        } else {
            off++;
        }
    }
    s_idxCount = frames;

    last_off = 0;
    last_ns = 1;
    last_len = 0;
    for (back = nsect - 1; back >= 0 && back >= nsect - 512; back--) {
        if (PcPort_BootDiscReadSectors(lba + back, 1, sec) != 0)
            break;
        if (BootParsePictureHdr(sec, &ns, &flen)) {
            if (back + ns > nsect)
                continue; /* do not truncate a picture that straddles the bound */
            last_off = back;
            last_ns = ns;
            last_len = flen;
            break;
        }
    }
    if (last_ns > 32)
        last_ns = 32;

    s_str = buf;
    s_strPrefixSize = prefix_ns * 0x800;
    s_strFileSize = size;
    s_strLba = lba;
    s_strNsect = nsect;
    s_strEntry = entry;
    s_titleOff = last_off;
    s_titleNs = last_ns;
    s_titleLen = last_len;
#ifdef BOOT_MUTANT_FONT_PLACEHOLDER
    s_title = NULL;
    s_titleSize = 0;
#else
    s_titleSize = last_ns * 0x800;
    s_title = (unsigned char*)malloc((size_t)s_titleSize);
    if (!s_title) {
        free(buf);
        s_str = NULL;
        s_strPrefixSize = 0;
        return 0;
    }
    if (PcPort_BootDiscReadSectors(lba + last_off, last_ns, s_title) != 0) {
        free(s_title);
        s_title = NULL;
        s_titleSize = 0;
        free(buf);
        s_str = NULL;
        return 0;
    }
#endif
    s_loaded = 1;
    printf("[xeno-port][boot] intro STR payload file=%d lba=%d size=%d prefix=%d indexed=%d\n",
           entry, lba, size, s_strPrefixSize, s_idxCount);
    if (s_titleSize > 0)
        printf("[xeno-port][boot] title graphic Opening STR last picture off=%d sectors=%d size=%d\n",
               last_off, last_ns, s_titleSize);
    fflush(stdout);
    return 1;
}

void PcPort_BootAssetsUnload(void)
{
    free(s_str);
    s_str = NULL;
    free(s_title);
    s_title = NULL;
    s_strPrefixSize = 0;
    s_strFileSize = 0;
    s_titleSize = 0;
    s_idxCount = 0;
    s_loaded = 0;
}

int PcPort_BootAssetsLoaded(void)
{
    return s_loaded;
}

const unsigned char* PcPort_BootIntroStrPayload(int* size)
{
    if (size)
        *size = s_strPrefixSize;
    return s_str;
}

const unsigned char* PcPort_BootTitleGraphicPayload(int* size)
{
    if (size)
        *size = s_titleSize;
    return s_title;
}

int PcPort_BootTitleUsedRetailGraphic(void)
{
#ifdef BOOT_MUTANT_FONT_PLACEHOLDER
    return 0;
#else
    return s_loaded && s_titleSize > 0 && s_title != NULL;
#endif
}

int PcPort_BootIntroStrLba(void)
{
    return s_strLba;
}

int PcPort_BootIntroStrFileSize(void)
{
    return s_strFileSize;
}

int PcPort_BootIntroStrSectorCount(void)
{
    return s_strNsect;
}

int PcPort_BootIntroStrEntry(void)
{
    return s_strEntry;
}

int PcPort_BootTitleFrameSectorOff(void)
{
    return s_titleOff;
}

int PcPort_BootTitleFrameSectors(void)
{
    return s_titleNs;
}

int PcPort_BootTitleFrameLen(void)
{
    return s_titleLen;
}

int PcPort_BootStrIndexCount(void)
{
    return s_idxCount;
}

int PcPort_BootStrIndexOff(int i)
{
    if (i < 0 || i >= s_idxCount)
        return 0;
    return s_idxOff[i];
}

int PcPort_BootStrIndexNs(int i)
{
    if (i < 0 || i >= s_idxCount)
        return 0;
    return s_idxNs[i];
}

int PcPort_BootStrIndexLen(int i)
{
    if (i < 0 || i >= s_idxCount)
        return 0;
    return s_idxLen[i];
}
