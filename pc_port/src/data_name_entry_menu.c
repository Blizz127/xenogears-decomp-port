/* data_name_entry_menu.c -- name-entry menu overlay (menu mode 3, "Enter
 * Name", archive directory 0x10 file 8) .rodata / .data for the Xenogears
 * PC port.
 *
 * Same role as data_member_change_menu.c: defining these symbols removes
 * them from the zero-filled data-stub set.  Unlike that file, the values are
 * not loaded once at startup.  The name entry writes into its own letter-code
 * table (NameEntryMenu_801CB33C stores 0xC3 over a selected 0xF / 0xCF cell),
 * and retail starts every entry from a fresh copy because the field opener
 * func_800799D4 streams the overlay file again for each menu request
 * (src/field/main/misc4.c: menuLoadCommands[1] = archive file menu id + 5
 * into pOverlayBuf, before MenuMain).  That buffer is local to the opener,
 * and MenuExecute's debug route loads no overlay in the port, so
 * PcPort_NameEntryMenuLoadData() reads the same file -- archive 0x10, file
 * 8, from the user's disc -- at every mode-3 entry (MenuExecute, menu.c) and
 * copies the retail address ranges:
 *   .rodata 0x801C5000..0x801C5010  the "BISLPS-00800" save id (the rest of
 *           .rodata, 0x801C5010..0x801C5040, is NameEntryMenu_801CB33C's jump
 *           table, which the native switch carries itself);
 *   .data   0x801CBEA0..0x801CC134  window, cursor and letter-code tables.
 * No retail bytes are compiled in and no extra disc file is required.  The
 * overlay's .bss (0x801CC134..) is not referenced by the port's C code, so
 * there is nothing to re-zero. */

#include "common.h"
#include "system/archive.h"
#include "system/memory.h"
#include <stdio.h>
#include <string.h>

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int mode);

#define NAME_ENTRY_DIRECTORY 0x10
#define NAME_ENTRY_FILE 8 /* menu mode 3 + 5 */
#define NAME_ENTRY_BASE 0x801C5000u

char NameEntryMenu_D_801C5000[16];
int NameEntryMenu_D_801CBEA0[4];
int NameEntryMenu_D_801CBEB0[4];
u8 NameEntryMenu_D_801CBEC0[0x24 * 6];
u8 NameEntryMenu_D_801CBF98[4];
s32 NameEntryMenu_D_801CBF9C[30];
s32 NameEntryMenu_D_801CC014[32];
s32 NameEntryMenu_D_801CC094[32];
u_short NameEntryMenu_D_801CC114[16];

typedef struct {
    void* pData;
    u32 address;
    u32 size;
} NameEntryMenuRegion;

/* Contiguous: 0x801C5000 + 0x10, then 0x801CBEA0 .. 0x801CC134. */
static const NameEntryMenuRegion s_regions[] = {
    {NameEntryMenu_D_801C5000, 0x801C5000u, sizeof(NameEntryMenu_D_801C5000)},
    {NameEntryMenu_D_801CBEA0, 0x801CBEA0u, sizeof(NameEntryMenu_D_801CBEA0)},
    {NameEntryMenu_D_801CBEB0, 0x801CBEB0u, sizeof(NameEntryMenu_D_801CBEB0)},
    {NameEntryMenu_D_801CBEC0, 0x801CBEC0u, sizeof(NameEntryMenu_D_801CBEC0)},
    {NameEntryMenu_D_801CBF98, 0x801CBF98u, sizeof(NameEntryMenu_D_801CBF98)},
    {NameEntryMenu_D_801CBF9C, 0x801CBF9Cu, sizeof(NameEntryMenu_D_801CBF9C)},
    {NameEntryMenu_D_801CC014, 0x801CC014u, sizeof(NameEntryMenu_D_801CC014)},
    {NameEntryMenu_D_801CC094, 0x801CC094u, sizeof(NameEntryMenu_D_801CC094)},
    {NameEntryMenu_D_801CC114, 0x801CC114u, sizeof(NameEntryMenu_D_801CC114)},
};

_Static_assert(sizeof(NameEntryMenu_D_801C5000) == 0x10, "save id");
_Static_assert(sizeof(NameEntryMenu_D_801CBEA0) == 0x10, "D_801CBEA0 size");
_Static_assert(sizeof(NameEntryMenu_D_801CBEB0) == 0x10, "D_801CBEB0 size");
_Static_assert(sizeof(NameEntryMenu_D_801CBEC0) == 0xD8, "letter codes size");
_Static_assert(sizeof(NameEntryMenu_D_801CBF98) == 0x4, "D_801CBF98 size");
_Static_assert(sizeof(NameEntryMenu_D_801CBF9C) == 0x78, "D_801CBF9C size");
_Static_assert(sizeof(NameEntryMenu_D_801CC014) == 0x80, "D_801CC014 size");
_Static_assert(sizeof(NameEntryMenu_D_801CC094) == 0x80, "D_801CC094 size");
_Static_assert(sizeof(NameEntryMenu_D_801CC114) == 0x20, "D_801CC114 size");
_Static_assert(0x801CBEA0u + 0x10 + 0x10 + 0xD8 + 0x4 + 0x78 + 0x80 + 0x80 + 0x20 == 0x801CC134u,
               "the .data regions cover 0x801CBEA0..0x801CC134");

/* Copy every region from an overlay image (file 8 as loaded, image[0] =
 * 0x801C5000).  Returns 0, or -1 when the image is too short (nothing is
 * copied then). */
int PcPort_NameEntryMenuApplyImage(const u8* pImage, u32 size) {
    u32 i;

    if (pImage == NULL || size < 0x801CC134u - NAME_ENTRY_BASE) {
        return -1;
    }
    for (i = 0; i < sizeof(s_regions) / sizeof(s_regions[0]); i++) {
        memcpy(s_regions[i].pData, pImage + (s_regions[i].address - NAME_ENTRY_BASE), s_regions[i].size);
    }
    return 0;
}

/* Every mode-3 entry: read the overlay file from the user's disc archive
 * and refresh the regions, as retail's per-request reload does. */
void PcPort_NameEntryMenuLoadData(void) {
    int size;
    u8* pImage;

    ArchiveSetIndex(NAME_ENTRY_DIRECTORY, 0);
    size = ArchiveDecodeAlignedSize(NAME_ENTRY_FILE);
    if (size <= 0) {
        fprintf(stderr, "[xeno-port][name-entry] archive 0x10 file 8: bad size %d\n", size);
        return;
    }
    pImage = HeapAlloc((u_int)size, 1);
    ArchiveReadFileToBuffer(NAME_ENTRY_FILE, pImage, 0, 0x80);
    ArchiveCdDataSync(0);
    if (PcPort_NameEntryMenuApplyImage(pImage, (u32)size) != 0) {
        fprintf(stderr, "[xeno-port][name-entry] archive 0x10 file 8: image too short (%d)\n", size);
    }
    HeapFree(pImage);
}
