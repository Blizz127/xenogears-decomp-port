/* Mutable CD command state from the US main executable. The hardware IRQ
 * handler and movie diagnostics share these words through their retail names. */
#include "data_alias.h"
#include <stddef.h>
#include "types.h"
#include "system/archive.h"

_Static_assert(sizeof(u_int) == 4 && sizeof(int) == 4 && sizeof(u32) == 4,
               "archive CD state and counters are PSX words");
_Static_assert(sizeof(CdlLOC) == 4, "CD location stores four MSF/track bytes");

/* 800596F8..80059F03 is one sector of discard space followed by the
 * three DMA header words. 80059F00 names the last of those words. */
typedef struct {
    u8 discard[CD_SECTOR_SIZE];
    ArchiveCdSectorHeader header;
} ArchiveCdReadStorage;

_Alignas(4) static ArchiveCdReadStorage s_archiveCdReadStorage __attribute__((used));
_Static_assert(offsetof(ArchiveCdReadStorage, header) == 0x800 &&
               offsetof(ArchiveCdSectorHeader, repeatedSubheader) == 8 &&
               sizeof(ArchiveCdSectorHeader) == 12 && sizeof(ArchiveCdReadStorage) == 0x80c,
               "archive DMA storage must keep its original byte offsets");

XG_SIZED_DATA_ALIAS(D_800596F8, s_archiveCdReadStorage, 0, 2048);
XG_SIZED_DATA_ALIAS(D_80059EF8, s_archiveCdReadStorage, 2048, 12);
XG_SIZED_DATA_ALIAS(D_80059F00, s_archiveCdReadStorage, 2056, 4);

s32 D_8005A4DC;

/* Initialized to zero by the retail .sdata image at 8004FE1C/20 and
 * 8004FE38/3C. The channel and the following queue cursor are separate words. */
u_int g_ArchiveCdDriveState;
u_int g_ArchiveCdDriveError;
int D_8004FE38;
s32 D_8004FE3C;

/* The retail startup BSS clear includes these parameters. The filter at
 * 80059F14 has its own storage; the last byte of the mode is 80059F1B. */
/* SW/LW at 8002AF08/8002A8A8 exchange a retail code address. */
u32 D_80059F08;
_Alignas(4) CdlLOC g_ArchiveCdCurLocation;
_Alignas(4) u_char D_80059F18[4];

/* Retail ArchiveInit clears each counter independently. The movie diagnostics
 * also write D_8005A4A8 and D_8005A4B4. */
u32 D_8005A488;
u32 D_8005A48C;
u32 D_8005A490;
u32 D_8005A494;
u32 D_8005A498;
u32 D_8005A4A8;
u32 D_8005A4B4;
