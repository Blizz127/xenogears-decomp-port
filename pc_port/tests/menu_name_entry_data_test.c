/* Name entry (menu mode 3) per-entry data refill: PcPort_NameEntryMenuLoadData
 * (pc_port/src/data_name_entry_menu.c) must read archive 0x10 file 8 and fill
 * the overlay's .rodata save id and .data tables (0x801CBEA0..0x801CC134) on
 * EVERY entry.  The archive / heap calls are stubbed here and serve the
 * user's disc/name_entry_menu.bin, i.e. the same file-8 bytes the port reads
 * from the disc archive at runtime. */
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char NameEntryMenu_D_801C5000[16];
extern int NameEntryMenu_D_801CBEA0[4];
extern int NameEntryMenu_D_801CBEB0[4];
extern u8 NameEntryMenu_D_801CBEC0[0x24 * 6];
extern u8 NameEntryMenu_D_801CBF98[4];
extern s32 NameEntryMenu_D_801CBF9C[30];
extern s32 NameEntryMenu_D_801CC014[32];
extern s32 NameEntryMenu_D_801CC094[32];
extern u_short NameEntryMenu_D_801CC114[16];
extern int PcPort_NameEntryMenuApplyImage(const u8* pImage, u32 size);
extern void PcPort_NameEntryMenuLoadData(void);

static u8 image[0x7800];
static int dir = -1, reads, syncs, allocs, frees, badCalls;
static void* lastAlloc;

#define FAIL(...) do { fprintf(stderr, "FAIL name entry data: " __VA_ARGS__); fputc('\n', stderr); exit(1); } while (0)

int ArchiveSetIndex(int directoryIndex, int entryIndex) {
    dir = entryIndex == 0 ? directoryIndex : -1;
    return 0;
}
int ArchiveDecodeAlignedSize(unsigned int entryIndex) {
    if (dir != 0x10 || entryIndex != 8) {
        badCalls++;
        return 0;
    }
    return (int)sizeof image;
}
s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags) {
    if (dir != 0x10 || index != 8 || arg2 != 0 || pBuffer != lastAlloc) {
        badCalls++;
        return -1;
    }
    (void)flags;
    memcpy(pBuffer, image, sizeof image);
    reads++;
    return 0;
}
void ArchiveCdDataSync(int mode) {
    (void)mode;
    syncs++;
}
void* HeapAlloc(u_int allocSize, u_int allocFlags) {
    (void)allocFlags;
    allocs++;
    lastAlloc = malloc(allocSize);
    memset(lastAlloc, 0xEE, allocSize);
    return lastAlloc;
}
u_int HeapFree(void* pMem) {
    if (pMem != lastAlloc) {
        badCalls++;
    }
    frees++;
    free(pMem);
    lastAlloc = NULL;
    return 0;
}

static const struct {
    void* p;
    uint32_t address;
    uint32_t size;
    const char* name;
} regions[] = {
    {NameEntryMenu_D_801C5000, 0x801C5000u, 0x10, "D_801C5000 (save id)"},
    {NameEntryMenu_D_801CBEA0, 0x801CBEA0u, 0x10, "D_801CBEA0"},
    {NameEntryMenu_D_801CBEB0, 0x801CBEB0u, 0x10, "D_801CBEB0"},
    {NameEntryMenu_D_801CBEC0, 0x801CBEC0u, 0xD8, "D_801CBEC0 (letter codes)"},
    {NameEntryMenu_D_801CBF98, 0x801CBF98u, 0x4, "D_801CBF98"},
    {NameEntryMenu_D_801CBF9C, 0x801CBF9Cu, 0x78, "D_801CBF9C"},
    {NameEntryMenu_D_801CC014, 0x801CC014u, 0x80, "D_801CC014"},
    {NameEntryMenu_D_801CC094, 0x801CC094u, 0x80, "D_801CC094"},
    {NameEntryMenu_D_801CC114, 0x801CC114u, 0x20, "D_801CC114"},
};
#define NREGIONS (sizeof regions / sizeof regions[0])

static void zero_regions(void) {
    for (unsigned i = 0; i < NREGIONS; i++) memset(regions[i].p, 0, regions[i].size);
}

static void check_loaded(const char* when) {
    uint32_t covered = 0;
    for (unsigned i = 0; i < NREGIONS; i++) {
        const u8* want = image + (regions[i].address - 0x801C5000u);
        int nonzero = 0;
        for (uint32_t k = 0; k < regions[i].size; k++) nonzero |= ((const u8*)regions[i].p)[k];
        if (!nonzero) FAIL("%s: %s still zero", when, regions[i].name);
        if (memcmp(regions[i].p, want, regions[i].size) != 0) FAIL("%s: %s differs from the image", when, regions[i].name);
        if (regions[i].address >= 0x801CBEA0u) covered += regions[i].size;
    }
    if (covered != 0x801CC134u - 0x801CBEA0u) FAIL("%s: .data coverage %#x", when, covered);
}

int main(void) {
    FILE* f = fopen("disc/name_entry_menu.bin", "rb");
    if (!f || fread(image, 1, sizeof image, f) != sizeof image) FAIL("cannot read disc/name_entry_menu.bin");
    fclose(f);

    /* 1. First entry from the zero-filled state. */
    zero_regions();
    PcPort_NameEntryMenuLoadData();
    if (reads != 1 || syncs < 1 || allocs != 1 || frees != 1 || badCalls) {
        FAIL("archive/heap sequence reads=%d syncs=%d allocs=%d frees=%d bad=%d", reads, syncs, allocs, frees, badCalls);
    }
    check_loaded("first load");
    if (memcmp(NameEntryMenu_D_801C5000, "BISLPS-00800", 13) != 0) FAIL("save id is not BISLPS-00800");
    if (memcmp(NameEntryMenu_D_801CBEC0, image + 0x6EC0, 6) != 0) FAIL("letter-code row 0 differs from the image");
    printf("save id \"%s\"; letter-code row 0 = %02X %02X %02X %02X %02X %02X\n", NameEntryMenu_D_801C5000,
           NameEntryMenu_D_801CBEC0[0], NameEntryMenu_D_801CBEC0[1], NameEntryMenu_D_801CBEC0[2],
           NameEntryMenu_D_801CBEC0[3], NameEntryMenu_D_801CBEC0[4], NameEntryMenu_D_801CBEC0[5]);

    /* 2. A session selects a skip cell (NameEntryMenu_801CB33C stores 0xC3
     * over a 0xF / 0xCF cell); the next entry must start from the image. */
    int cell = -1;
    for (int k = 0; k < (int)sizeof NameEntryMenu_D_801CBEC0; k++) {
        if (NameEntryMenu_D_801CBEC0[k] == 0x0F || NameEntryMenu_D_801CBEC0[k] == 0xCF) {
            cell = k;
            break;
        }
    }
    if (cell < 0) FAIL("no 0xF / 0xCF skip cell in the letter codes");
    u8 before = NameEntryMenu_D_801CBEC0[cell];
    NameEntryMenu_D_801CBEC0[cell] = 0xC3;
    NameEntryMenu_D_801CBF9C[3] ^= 0x5A5A; /* and another table touched */
    PcPort_NameEntryMenuLoadData();
    if (reads != 2 || badCalls) FAIL("second entry did not read the file again (reads=%d)", reads);
    if (NameEntryMenu_D_801CBEC0[cell] != before) FAIL("second load left cell %d at %02X", cell, NameEntryMenu_D_801CBEC0[cell]);
    check_loaded("second load");
    printf("skip cell %d (row %d): %02X -> C3 by the session -> %02X after the next load\n", cell, cell / 6, before,
           NameEntryMenu_D_801CBEC0[cell]);

    /* 3. A short image is refused without touching the tables. */
    zero_regions();
    if (PcPort_NameEntryMenuApplyImage(image, 0x7000) == 0) FAIL("short image accepted");
    for (unsigned i = 0; i < NREGIONS; i++) {
        for (uint32_t k = 0; k < regions[i].size; k++)
            if (((const u8*)regions[i].p)[k]) FAIL("short image wrote %s", regions[i].name);
    }
    printf("NAME ENTRY DATA PASS regions=%u\n", (unsigned)NREGIONS);
    return 0;
}
