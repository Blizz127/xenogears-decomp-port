#include "common.h"
#include "psyq/libgte.h"
#ifndef XENO_PC_PORT
s32 func_8002D180(void *arg0, void *arg1);
#endif

#ifdef XENO_PC_PORT
/* Execute GTE operations through the host adapter, not PsyQ's assembler
 * placeholders (which the x86 assembler also accepts as raw data). */
#include <psx/inline_c.h>
#else
#include "psyq/inline_c.h"
#endif
#include "system/memory.h"
#include "system/archive.h"
#include "system/model.h"
#include "psyq/pc.h"
#ifdef XENO_PC_PORT
#include <psx/gtereg.h>
#include <stdio.h>
#include <stdlib.h>
#include "guest_prim_link.h"
#include "psx_memory.h"
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002AC24);
#endif

extern CdlLOC D_80059EF8;
extern s32 D_8004FDE4;
extern s32 D_8004FDEC;
extern s8* D_8004FE08;
extern u8 D_800596F8[];
extern s32 g_ArchiveCurFileSector;
extern s32 D_8005A4DC;
extern s32 D_8004FE10;
extern u16 D_8004FE26;

/* Transcribed from asm/slus_006.64/nonmatchings/system/temp2/func_8002B084.s
 * (0x8002B084-0x8002B2F0, 155 instructions). CdReadyCallback handler for the
 * sector-read state: status != 1 (or an abort request, or a position mismatch)
 * takes the shared re-arm path — D_8005A4DC counter, CdReadyCallback(NULL) into
 * D_80059F08, CdIntToPos, then error 3 + state 0xA or the busy-wait/error 4/
 * D_8005A4A4++ variant, ending in CdSyncCallback + CdControlF(1, NULL). Status 1
 * with a live stream pulls the sector position (3 words) plus the payload:
 * a full 0x800 sector as 0x200 words, or a partial one as (size+3)/4 words with
 * the remainder cleared through D_800596F8. Matching positions advance
 * D_8004FE08 by 0x800 and decrement g_ArchiveCurFileSize, and once that reaches
 * zero (or on abort) the ready callback is cleared and ArchiveCdSeekToFile(
 * D_8004FE38) + D_8004FDFC = 0 end the stream. A position mismatch bumps
 * D_8004FDE4 and re-arms instead. */
#ifdef XENO_PC_PORT
__attribute__((weak))
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002B084);
#else
s32 func_8002B084(u8 status) {
    s32 size;

    if ((status & 0xFF) != 1) {
        goto rearm;
    }
    if (D_8004FE34 > 0) {
        goto finish;
    }
    size = g_ArchiveCurFileSize;
    if (size >= 0x800) {
        CdGetSector(&D_80059EF8, 3);
        CdGetSector(D_8004FE08, 0x200);
    } else if (size > 0) {
        CdGetSector(&D_80059EF8, 3);
        CdGetSector(D_8004FE08, (size + 3) / 4);
        CdGetSector(D_800596F8, 0x200 - ((size + 3) / 4));
    }

    if (CdPosToInt(&D_80059EF8) != g_ArchiveCurFileSector) {
        D_8004FDE4 += 1;
        goto rearm;
    }
    g_ArchiveCurFileSector += 1;
    D_8004FE08 += 0x800;
    g_ArchiveCurFileSize -= 0x800;
    if (g_ArchiveCurFileSize > 0) {
        return 0;
    }

finish:
    CdReadyCallback(NULL);
    g_ArchiveCurFileSize = 0;
    ArchiveCdSeekToFile(D_8004FE38);
    D_8004FDFC = 0;
    return 0;

rearm:
    D_8005A4DC += 1;
    D_80059F08 = (void*)(uintptr_t)CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    if (D_8005A4DC < 3) {
        g_ArchiveCdDriveError = 3;
        g_ArchiveCdDriveState = 0xA;
    } else {
        {
            s32 v1;
            s32 v0;
            for (v1 = 0x270F; v1 >= 0; v1--) {
                for (v0 = 0x7CF; v0 >= 0; v0--) {
                    ;
                }
            }
        }
        D_8005A4DC = 0;
        g_ArchiveCdDriveError = 4;
        D_8005A4A4 += 1;
        g_ArchiveCdDriveState = 0xA;
    }
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(1, NULL);
    return 0;
}
#endif

/* Transcribed from asm/slus_006.64/nonmatchings/system/temp2/func_8002B2F0.s
 * (0x8002B2F0-0x8002B5B4, 184 instructions). The ring-slot variant of the CD
 * ready handler: status != 1 bumps D_8005A4DC and re-arms; an abort request
 * (D_8004FE34 > 0) clears both callbacks, zeroes the size, seeks to D_8004FE38
 * and clears D_8004FDFC; a dead stream just clears the ready callback and the
 * size. Otherwise it rotates D_8004FE10 over the D_8004FE40 ring slots to a
 * NOT_LOADED one (a loaded walk target re-arms without the counter bump),
 * verifies CdPosToInt against g_ArchiveCurFileSector — a mismatch bumps
 * D_8004FDEC, swallows 0x200 words into D_800596F8 and re-arms — and on a match
 * marks the slot state 1 with id D_8004FE26 (then bumped), reads 0x200 words into
 * D_8004FE08 + index*0x800, decrements the size by 0x800 and advances the sector;
 * a size that stays positive returns, otherwise the ready callback is cleared and
 * the size zeroed. The re-arm path matches func_8002B084's (error 3 or the
 * busy-wait/error 4 variant, then CdSyncCallback + CdControlF(1, NULL)). */
#ifdef XENO_PC_PORT
__attribute__((weak))
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002B2F0);
#else
s32 func_8002B2F0(u8 status) {
    u8* pSlot;
    s32 nSectors;
    s16 i;
    s32 index;

    if ((status & 0xFF) != 1) {
        goto rearm_bump;
    }
    if (D_8004FE34 > 0) {
        CdReadyCallback(NULL);
        CdDataCallback(NULL);
        g_ArchiveCurFileSize = 0;
        ArchiveCdSeekToFile(D_8004FE38);
        D_8004FDFC = 0;
        return 0;
    }
    if (g_ArchiveCurFileSize <= 0) {
        goto clear_ready;
    }

    pSlot = (u8*)D_8004FE2C;
    index = 0;
    if (D_8004FE40 > 0) {
        nSectors = D_8004FE40;
        i = 0;
        do {
            index = D_8004FE10;
            pSlot = (u8*)D_8004FE2C + (index << 3);
            D_8004FE10 = index + 1;
            if (D_8004FE10 >= nSectors) {
                D_8004FE10 = 0;
            }
            if (*(u16*)pSlot == 0) {
                break;
            }
            i++;
        } while (i < nSectors);
    }
    if (*(u16*)pSlot != 0) {
        goto rearm;
    }

    CdGetSector(&D_80059EF8, 3);
    if (CdPosToInt(&D_80059EF8) != g_ArchiveCurFileSector) {
        D_8004FDEC += 1;
        CdGetSector(D_800596F8, 0x200);
        goto rearm_bump;
    }
    *(u16*)pSlot = 1;
    *(u16*)(pSlot + 2) = D_8004FE26;
    D_8004FE26 = D_8004FE26 + 1;
    CdGetSector(D_8004FE08 + (index << 11), 0x200);
    g_ArchiveCurFileSize -= 0x800;
    g_ArchiveCurFileSector += 1;
    if (g_ArchiveCurFileSize > 0) {
        return 0;
    }

clear_ready:
    CdReadyCallback(NULL);
    g_ArchiveCurFileSize = 0;
    return 0;

rearm_bump:
#ifdef TEMP2_CDSTATE_MUTANT_NO_BUMP
    ;
#else
    D_8005A4DC += 1;
#endif
rearm:
    D_80059F08 = (void*)(uintptr_t)CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    if (D_8005A4DC < 3) {
        g_ArchiveCdDriveError = 3;
        g_ArchiveCdDriveState = 0xA;
    } else {
        s32 v1;
        s32 v0;
        for (v1 = 0x270F; v1 >= 0; v1--) {
            for (v0 = 0x7CF; v0 >= 0; v0--) {
                ;
            }
        }
        D_8005A4DC = 0;
        g_ArchiveCdDriveError = 4;
        D_8005A4A4 += 1;
        g_ArchiveCdDriveState = 0xA;
    }
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(1, NULL);
    return 0;
}
#endif

extern s32 D_8004FE10;
extern u16 D_8004FE26;
extern s8* D_8004FE08;
extern s32 D_80059F04;
extern s32 g_ArchiveCurFileSector;
extern void func_8002804C(s32, s32, s32, s32);

extern s32 D_8004FDEC;
extern s32 D_8005A4DC;
extern CdlLOC D_80059EF8;
extern u8 D_800596F8[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002B5D0);
#else
/* Transcribed from asm/slus_006.64/nonmonmatchings/system/temp2/func_8002B5D0.s
 * (0x8002B5D0-0x8002B8B0, frame 0x28). CdReadyCallback handler for the
 * streaming read: on status 1 it queues the next sector into a free ring slot,
 * on any other status (or a full ring) it re-arms the drive.
 *   D_8004FE34 > 0 (explicit abort): clear both callbacks, zero
 *   g_ArchiveCurFileSize, ArchiveCdSeekToFile(D_8004FE38), D_8004FDFC = 0.
 *   g_ArchiveCurFileSize <= 0: clear the ready callback and the size.
 *   Status 1 with a live stream: rotate D_8004FE10 over D_8004FE40 slots to a
 *   NOT_LOADED one; a loaded walk target means "no room" (below).
 *   Found: CdGetSector(D_80059EF8, 3) + CdPosToInt must equal
 *   g_ArchiveCurFileSector, otherwise D_8004FDEC += 1, swallow 0x200 words
 *   into D_800596F8, bump D_8005A4DC and take the re-arm path. On a match the
 *   slot is marked state 1 with id D_8004FE26 (then bumped), 0x200 words are
 *   read into D_8004FE08 + index*0x800, g_ArchiveCurFileSize -= 0x800 and
 *   g_ArchiveCurFileSector += 1, returning while the size stays positive and
 *   otherwise clearing the ready callback and the size.
 *   No room / other status: D_8005A4DC += 1 (other status only), clear the
 *   ready callback into D_80059F08, CdIntToPos(g_ArchiveCurFileSector,
 *   &g_ArchiveCdCurLocation), then either error 3 with state 0xA
 *   (D_8005A4DC < 3) or a busy-wait, D_8005A4DC = 0, error 4,
 *   D_8005A4A4 += 1 and state 0xA. Both end with
 *   CdSyncCallback(&ArchiveCdDriveCommandHandler) and CdControlF(1, NULL).
 * Note: retail reads an undefined slot pointer when D_8004FE40 <= 0 (only
 * reachable with an empty ring); this C probes slot 0. The busy-wait is a
 * delay loop, written as a nested counter loop. */
void func_8002B5D0(u8 status) {
    u8* pSlot;
    s32 nSectors;
    s16 i;
    s32 index;
    s32 v0;
    s32 v1;

    if (status != 1) {
        D_8005A4DC += 1;
        goto no_room;
    }

    if (D_8004FE34 > 0) {
        CdReadyCallback(NULL);
        CdDataCallback(NULL);
        g_ArchiveCurFileSize = 0;
        ArchiveCdSeekToFile(D_8004FE38);
        D_8004FDFC = 0;
        return;
    }

    if (g_ArchiveCurFileSize <= 0) {
        goto stop;
    }

    pSlot = (u8*)D_8004FE2C;
    index = 0;
    if (D_8004FE40 > 0) {
        nSectors = D_8004FE40;
        i = 0;
        do {
            index = D_8004FE10;
            pSlot = (u8*)D_8004FE2C + (index << 3);
            D_8004FE10 = index + 1;
            if (D_8004FE10 >= nSectors) {
                D_8004FE10 = 0;
            }
            if (*(u16*)pSlot == 0) {
                break;
            }
            i++;
        } while (i < nSectors);
    }
    if (*(u16*)pSlot != 0) {
        goto no_room;
    }

    CdGetSector(&D_80059EF8, 3);
    if (CdPosToInt(&D_80059EF8) != g_ArchiveCurFileSector) {
        D_8004FDEC += 1;
        CdGetSector(D_800596F8, 0x200);
        D_8005A4DC += 1;
        goto no_room;
    }

    v0 = D_8004FE26;
#ifdef TEMP2_B5D0_MUTANT_MARK_STATE3
    *(u16*)pSlot = 3;
#else
    *(u16*)pSlot = 1;
#endif
    *(u16*)(pSlot + 2) = (u16)v0;
    D_8004FE26 = (u16)(v0 + 1);
    CdGetSector((void*)(D_8004FE08 + (index << 11)), 0x200);
    g_ArchiveCurFileSize -= 0x800;
    g_ArchiveCurFileSector += 1;
    if (g_ArchiveCurFileSize > 0) {
        return;
    }

stop:
    CdReadyCallback(NULL);
    g_ArchiveCurFileSize = 0;
    return;

no_room:
    D_80059F08 = (void*)(uintptr_t)CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    if (D_8005A4DC < 3) {
        g_ArchiveCdDriveError = 3;
        v0 = 0xA;
    } else {
        for (v1 = 0x270F; v1 >= 0; v1--) {
            for (v0 = 0x7CF; v0 >= 0; v0--) {
                ;
            }
        }
        v0 = D_8005A4A4;
        D_8005A4DC = 0;
        g_ArchiveCdDriveError = 4;
        D_8005A4A4 = v0 + 1;
        v0 = 0xA;
    }

    g_ArchiveCdDriveState = (u_int)v0;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(1, NULL);
}
#endif


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002B8B0);
#else
/* Transcribed from asm/slus_006.64/nonmatchings/system/temp2/func_8002B8B0.s
 * (0x8002B8B0-0x8002BA40, frame 0x20). Queue one 0x800 sector from the open
 * PC file into the ring:
 *   g_ArchiveCurFileSize <= 0 -> clear it and return.
 *   Otherwise rotate D_8004FE10 over the D_8004FE40 ring slots (wrapping to 0)
 *   until a NOT_LOADED slot is found; a full ring whose walked slot is loaded
 *   just clears g_ArchiveCurFileSize.
 *   The found slot is marked state 1 with id D_8004FE26 (then bumped) and
 *   PCread(0x800) fills D_8004FE08 + index*0x800 with up to three
 *   func_8002804C(i,0,0xFF,0) retries; a read that never succeeds still falls
 *   through to the accounting below.
 *   Then g_ArchiveCurFileSize -= 0x800 and g_ArchiveCurFileSector += 1, and
 *   g_ArchiveCurFileSize is cleared when it did not stay positive. */
void func_8002B8B0(void) {
    u8* pSlot;
    s32 nSectors;
    s16 i;
    s32 index;

    if (g_ArchiveCurFileSize > 0) {
        nSectors = D_8004FE40;
        if (nSectors > 0) {
            i = 0;
            do {
                index = D_8004FE10;
                pSlot = (u8*)D_8004FE2C + (index << 3);
                D_8004FE10 = index + 1;
                if (D_8004FE10 >= nSectors) {
                    D_8004FE10 = 0;
                }
                if (*(u16*)pSlot == 0) {
                    break;
                }
                i++;
            } while (i < nSectors);
            if (*(u16*)pSlot != 0) {
                g_ArchiveCurFileSize = 0;
                goto tail;
            }
        } else {
            /* Retail reads an undefined slot pointer on this path; treat an
             * unpublished ring as "no room" instead. */
            g_ArchiveCurFileSize = 0;
            goto tail;
        }

#ifdef TEMP2_B8B0_MUTANT_MARK_STATE2
        *(u16*)pSlot = 2;
#else
        *(u16*)pSlot = 1;
#endif
        *(u16*)(pSlot + 2) = D_8004FE26;
        D_8004FE26 = D_8004FE26 + 1;

        for (i = 0; i < 4; i++) {
            if (PCread(D_80059F04, (char*)(D_8004FE08 + (index << 11)), 0x800) != 0) {
                break;
            }
            func_8002804C(i, 0, 0xFF, 0);
        }

        g_ArchiveCurFileSize -= 0x800;
        g_ArchiveCurFileSector += 1;
    }

tail:
    if (g_ArchiveCurFileSize > 0) {
        return;
    }
    g_ArchiveCurFileSize = 0;
}
#endif

extern s32 D_8004FE00;
extern s32 D_8004FDFC;

void func_8002BA40(void) {
    D_8004FDFC = D_8004FE00;
}

extern s32 D_8004FE40;
extern s16 D_8004FE28;
extern s32 D_8004FE38;
extern s32 g_ArchiveCurFileSize;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002BA58);
#else
void func_8002BA58(void) {
    s32 slotCount = D_8004FE40;
    u8* slot = (u8*)D_8004FE2C;
    s16 i = 0;
    u8* sector;

    if (slotCount > 0) {
        u16 seq = (u16)D_8004FE28;
        for (i = 0; i < slotCount; i++, slot += 8) {
            if (*(u16*)(slot + 0) == 1 && *(u16*)(slot + 2) == seq) {
                break;
            }
        }
    }
    if (i == slotCount) {
        return;
    }

    /* Found matching slot — mark as processed */
    *(u16*)(slot + 0) = 3;
    D_8004FE28++;

    if (g_ArchiveCurFileSize > 0) return;

    if (D_8004FDFC < 2) {
        g_ArchiveCurFileSize = 0;
        CdDataCallback(0);
        ArchiveCdSeekToFile(D_8004FE38);
        D_8004FDFC = 0;
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002BB50);
#endif

/* ---- func_8002BF38: synchronous VRAM stream section decoder -----------------
 * asm 8002BF38-8002C30C. Consumes at most ONE 0x800 stream sector per call,
 * selected from the slot ring at D_8004FE2C (slots {u16 state,u16 id,..}, 8B
 * stride): the slot with state==1 and id==D_8004FE28. Sector data lives at
 * D_8004FE08 + slot*0x800.
 *
 * State machine: while D_80059F50 (strips remaining) is 0 the sector is a
 * SECTION HEADER: u32 tag 0x1200/0x1201; x/y derived per mode registers
 * (D_80059F24 + D_80059F28/2C bias for 0x1200, D_80059F30 + D_80059F34/38
 * for 0x1201 -- mode 0, the field-stream case, sums u16 pairs at +4/+8 and
 * +6/+A); u16 width at +0xC; u32 total-section count at +0x14 (latched into
 * D_80059F3C only when it is 0); u32 strip count at +0x18 into D_80059F50;
 * height table (u16 per strip) from +0x1C via D_80059F4C. The header's slot
 * stays CLAIMED (state 2) so the height table survives until section end.
 * Otherwise the sector is one STRIP: LoadImage((x,y,w,heights[i]), sector),
 * DrawSync, y += height. On section end the whole ring is reset; when
 * D_80059F3C reaches 0 the stream finishes (g_ArchiveCurFileSize = 0,
 * D_8004FDFC = 0) without bumping the sequence counter. */
#include "system/archive.h"
#include "psyq/libgpu.h"

extern s8* D_8004FE08;
extern s16 D_8004FE28;
extern s32 D_8004FE40;
extern s16 D_80059F24, D_80059F30;
extern u16 D_80059F28, D_80059F2C, D_80059F34, D_80059F38;
extern u16 D_80059F40, D_80059F44, D_80059F48;
extern s32 D_80059F3C;
extern u16* D_80059F4C;
extern s32 D_80059F50;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002BF38);
#else
void func_8002BF38(void) {
    s32 slotCount = D_8004FE40;
    u8* slot = (u8*)D_8004FE2C;
    s16 i = 0;
    u8* sector;

    if (slotCount > 0) {
        u16 seq = (u16)D_8004FE28;
        for (i = 0; i < slotCount; i++, slot += 8) {
            if (*(u16*)(slot + 0) == 1 && *(u16*)(slot + 2) == seq) {
                break;
            }
        }
    }
    if (i == slotCount) {
        return;
    }

    *(u16*)(slot + 0) = 2;
    sector = (u8*)D_8004FE08 + ((s32)i << 11);

    if (D_80059F50 == 0) {
        /* header sector */
        u32 tag = *(u32*)sector;
        u8* hdr = sector + 4;
        u8* p = hdr;

        if (tag - 0x1200 >= 2u) {
            return;
        }

        if (tag == 0x1200) {
            switch (D_80059F24) {
            case 1:
                D_80059F40 = D_80059F28 + *(u16*)(hdr + 4);
                D_80059F44 = D_80059F2C + *(u16*)(hdr + 6);
                break;
            case 2:
                D_80059F40 = D_80059F28 + *(u16*)(hdr + 0) + *(u16*)(hdr + 4);
                D_80059F44 = D_80059F2C + *(u16*)(hdr + 2) + *(u16*)(hdr + 6);
                break;
            default:
                D_80059F40 = *(u16*)(hdr + 0) + *(u16*)(hdr + 4);
                D_80059F44 = *(u16*)(hdr + 2) + *(u16*)(hdr + 6);
                break;
            }
        } else {
            switch (D_80059F30) {
            case 1:
                D_80059F40 = D_80059F34 + *(u16*)(hdr + 4);
                D_80059F44 = D_80059F38 + *(u16*)(hdr + 6);
                break;
            case 2:
                D_80059F40 = D_80059F34 + *(u16*)(hdr + 0) + *(u16*)(hdr + 4);
                D_80059F44 = D_80059F38 + *(u16*)(hdr + 2) + *(u16*)(hdr + 6);
                break;
            default:
                D_80059F40 = *(u16*)(hdr + 0) + *(u16*)(hdr + 4);
                D_80059F44 = *(u16*)(hdr + 2) + *(u16*)(hdr + 6);
                break;
            }
        }
        p += 8;
        D_80059F48 = *(u16*)p;
        p += 8;
        if (D_80059F3C == 0) {
            D_80059F3C = *(s32*)p;
        }
        p += 4;
        D_80059F50 = *(s32*)p;
        p += 4;
        D_80059F4C = (u16*)p;
        /* header slot stays claimed (state 2); sequence advances */
        D_8004FE28 = D_8004FE28 + 1;
        return;
    }

    /* strip sector */
    {
        RECT rect;
        u16 stripHeight = *D_80059F4C;

        rect.x = (short)D_80059F40;
        rect.y = (short)D_80059F44;
        rect.w = (short)D_80059F48;
        rect.h = (short)stripHeight;
        LoadImage(&rect, (u_long*)sector);
        DrawSync(0);

        D_80059F4C = D_80059F4C + 1;
        D_80059F50 = D_80059F50 - 1;
        D_80059F44 = D_80059F44 + stripHeight;

        if (D_80059F50 > 0) {
            *(u16*)(slot + 0) = 0;
            D_8004FE28 = D_8004FE28 + 1;
            return;
        }

        /* section complete: reset the whole ring */
        D_80059F3C = D_80059F3C - 1;
        D_80059F50 = 0;
        for (i = 0; i < D_8004FE40; i++) {
            u8* s2 = (u8*)D_8004FE2C + ((s32)i << 3);
            *(u16*)(s2 + 0) = 0;
            *(u16*)(s2 + 2) = 0;
        }
        if (D_80059F3C > 0) {
            *(u16*)(slot + 0) = 0;
            D_8004FE28 = D_8004FE28 + 1;
            return;
        }
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;
    }
}
#endif

extern void func_800379C8(char*, ...);
extern char D_80018944[];
extern char D_80018954[];
extern char D_80018964[];
extern char D_80018974[];
extern s32 D_8004FE10;
extern s16 temp2_D_80059F40 asm("D_80059F40");
extern s16 temp2_D_80059F44 asm("D_80059F44");
extern s16 temp2_D_80059F48 asm("D_80059F48");

void func_8002C310(void)
{
    func_800379C8(D_80018944, g_ArchiveTable, D_8004FE08, D_8004FE2C);
    func_800379C8(D_80018954, D_8004FE40, D_8004FE10, D_80059F3C,
                  D_80059F50);
    func_800379C8(D_80018964, temp2_D_80059F40, temp2_D_80059F44,
                  temp2_D_80059F48, D_80059F4C);
    func_800379C8(D_80018974, D_80059F4C[0], D_80059F4C[1],
                  D_80059F4C[2]);
}

extern u32 g_ArchiveDebugTable;

u32 func_8002C3D8(void) {
    return g_ArchiveDebugTable;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002C3E8);
#else
/* Relocates a model's internal offset fields to absolute addresses (adds the model
 * base). Runs once per model (guarded by flag bit 0x1 at +0x4). For each of the
 * `count` (at +0x0) sub-entries (0x38 stride from +0x2C): relocate the four offset
 * fields at entry-0x14/-0x10/-0xC/-0x8, and if the entry+0x0 offset is set, relocate
 * it and walk its sub-list (0xC stride) relocating each element's +0x4/+0x8. Stores
 * are `sw` (32-bit); write truncated u32 (host RAM below 4 GiB) so pointer fields
 * stay 4 bytes -- FieldLoad's model actors read these back via func_8002C8CC. */
int func_8002C3E8(u8* pModel) {
    s32 count = *(s32*)(pModel + 0x0);
    s32 flags = *(s32*)(pModel + 0x4);
    u8* pEntry;
    s32 i;

    if (flags & 0x1) {
        return count;
    }
    *(s32*)(pModel + 0x4) = flags | 0x1;
    if (count <= 0) {
        return count;
    }

    pEntry = pModel + 0x2C;
    for (i = 0; i < count; i++) {
        *(u32*)(pEntry - 0x14) = *(u32*)(pEntry - 0x14) + (u32)pModel;
        *(u32*)(pEntry - 0x10) = *(u32*)(pEntry - 0x10) + (u32)pModel;
        *(u32*)(pEntry - 0x0C) = *(u32*)(pEntry - 0x0C) + (u32)pModel;
        *(u32*)(pEntry - 0x08) = *(u32*)(pEntry - 0x08) + (u32)pModel;

        {
            u32 subOff = *(u32*)(pEntry + 0x0);
            if (subOff != 0) {
                u8* pSub = (u8*)(u32)(subOff + (u32)pModel);
                s32 subCount;
                *(u32*)(pEntry + 0x0) = (u32)pSub;
                subCount = *(s32*)(pSub + 0x0);
                if (subCount != -1) {
                    /* a1 = pSub + 4 + subCount*0xC, walked backwards by 0xC */
                    u8* a1 = (pSub + 0x4) + (((subCount << 1) + subCount) << 2);
                    do {
                        subCount -= 1;
                        *(u32*)(a1 + 0x4) = *(u32*)(a1 + 0x4) + (u32)pModel;
                        *(u32*)(a1 + 0x8) = *(u32*)(a1 + 0x8) + (u32)pModel;
                        a1 -= 0xC;
                    } while (subCount != -1);
                }
            }
        }
        pEntry += 0x38;
    }
    return count;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002C4BC);
#else
s32 func_8002C4BC(u8* pBlock) {
    u32 flags = *(u32*)(pBlock + 4);
    s32 count = *(s32*)(pBlock);
    s32 i;
    u8* pEntry;
    if (!(flags & 1)) return count;
    *(u32*)(pBlock + 4) = flags & ~1;
    pEntry = pBlock + 0x2C;
    for (i = 0; i < count; i++, pEntry += 0x38) {
        *(u32*)(pEntry - 0x14) -= (u32)pBlock;
        *(u32*)(pEntry - 0x10) -= (u32)pBlock;
        *(u32*)(pEntry - 0x0C) -= (u32)pBlock;
        *(u32*)(pEntry - 0x08) -= (u32)pBlock;
        {
            u32 tableAddress = *(u32*)pEntry;
            if (tableAddress != 0) {
                /* Retail uses the relocated table directly, then restores
                 * its offset after walking entries backwards through zero. */
                u8* pTable = (u8*)(uintptr_t)tableAddress;
                s32 remaining = *(s32*)pTable;
                if (remaining != -1) {
                    u8* pRecord = pTable + 4 + remaining * 12;
                    do {
                        remaining--;
                        *(u32*)(pRecord + 4) -= (u32)pBlock;
                        *(u32*)(pRecord + 8) -= (u32)pBlock;
                        pRecord -= 12;
                    } while (remaining != -1);
                }
                *(u32*)pEntry -= (u32)pBlock;
            }
        }
    }
    return count;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002C59C);
#else
void func_8002C59C(u8* pBlock) {
    u16 flags = *(u16*)(pBlock);
    s32 count;
    s32 i;
    u8* pTable;
    if (flags & 0x20) return;
    *(u16*)(pBlock) = flags | 0x20;
    *(u32*)(pBlock + 0x08) += (u32)pBlock;
    *(u32*)(pBlock + 0x0C) += (u32)pBlock;
    *(u32*)(pBlock + 0x10) += (u32)pBlock;
    *(u32*)(pBlock + 0x14) += (u32)pBlock;
    {
        u32 rel = *(u32*)(pBlock + 0x1C);
        if (rel == 0) return;
        pTable = pBlock + rel;
        *(u32*)(pBlock + 0x1C) = (u32)pTable;
    }
    count = *(s32*)(pTable);
    if (count == -1) return;
    pTable += 4;
    for (i = count; i >= 0; i--) {
        *(u32*)(pTable + i * 12 + 4) += (u32)pBlock;
        *(u32*)(pTable + i * 12 + 8) += (u32)pBlock;
    }
}
#endif

/* Finalizes a model control block: pins its backing buffer into the heap so it
 * won't move. Reads a flags word at +0x4; if bit 0x2 is already set the block is
 * already pinned and it returns 1. Otherwise it sets bit 0x2, computes the
 * distance from the block base (a0) to the buffer stored at +0x24, and hands
 * that to HeapInsertAlloc to register the region. */
s32 func_8002C644(u8* a0) {
    s32 result;
    s32 v1 = *(s32*)(a0 + 0x4);
    if ((v1 & 0x2) == 0) {
        s32 a1;

        *(s32*)(a0 + 0x4) = v1 | 0x2;
        a1 = *(s32*)(a0 + 0x24);
#ifdef XENO_PC_PORT
        /* PC-port hardening: this pin is only meaningful for heap-standalone
         * models, where a0 is a real heap block and +0x24 holds an end pointer.
         * FieldLoad package models reach here mid-blob with +0x24 = baked file
         * data (packed s16 pairs, not a pointer); on PSX the resulting wild
         * HeapInsertAlloc write lands harmlessly in mirrored RAM, on the host
         * it corrupts the heap block list / faults. Validate before pinning:
         * end must lie after base within a 2 MiB (PSX RAM) bound, and the
         * heap header HeapInsertAlloc trusts (pMem[-1].pNext) must itself be
         * plausible. No heap-end API exists in memory.c, so the 2 MiB bound is
         * a documented temporary guard. Skips keep the retail control flow
         * (flag 0x2 set above, return 0) minus the junk insert. */
        {
            u32 base = (u32)(uintptr_t)a0;
            u32 end = (u32)a1;
            u32 hdrNext = (u32)(uintptr_t)((HeapBlock*)a0)[-1].pNext;
            static s32 s_skippedPins;
            static s32 s_realPins;

            if (end <= base || (end - base) >= 0x200000 ||
                hdrNext <= base || (hdrNext - base) >= 0x200000) {
                s_skippedPins++;
                if (s_skippedPins <= 4) {
                    printf("[field-diag] func_8002C644: skip invalid pin base=0x%x end=0x%x hdrNext=0x%x (skips=%d)\n",
                           base, end, hdrNext, s_skippedPins);
                }
                return 0;
            }
            s_realPins++;
            printf("[field-diag] func_8002C644: real pin base=0x%x size=%u (pins=%d)\n",
                   base, end - base, s_realPins);
        }
#endif
        HeapInsertAlloc((HeapBlock*)a0, (u_int)(a1 - (s32)a0));
        result = 0;
    } else {
        result = 1;
    }
    return result;
}

s32 func_8002C68C(u8* pBlock) {
    u16 flags = *(u16*)(pBlock);
    if (flags & 0x40) {
        return 1;
    }
    *(u16*)(pBlock) = flags | 0x40;
    HeapInsertAlloc((void*)pBlock, *(u32*)(pBlock + 0x14) - (u32)pBlock);
    *(u32*)(pBlock + 0x14) = 0;
    return 0;
}

extern u8 D_80059598;
extern u8 D_80059599;
extern u8 D_8005959A;

void func_8002C6E0(u8 a0, u8 a1, u8 a2) {
    D_80059598 = a0;
    D_80059599 = a1;
    D_8005959A = a2;
}

extern void func_8002CCAC(void);
extern u8* D_80059424;
extern u32 D_80059538;
extern u32 D_80059528;
extern u32 D_8005952C;
extern u32 D_8005953C;
extern u32 D_80059498;
extern u32 D_800595C0;
#ifndef XENO_PC_PORT
extern u8 D_8004FE50[];
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002C700);
#else
typedef s32 (*ModelPrimProc)(u8* pCmd, s32 count);
extern void PcPort_LinkModelPrim(u32* ot, s32 index, void* packet,
                                  u32 tag_length);
/* Build-pass proc, called per packet-source record by func_8002C8CC as
 * fn(D_80059538, D_80059528, shade) — mirrors the retail dispatch; the ported
 * buildProcs only consume the first arg. */
typedef s32 (*ModelPrimBuildProc)(u32 pSrc, u32 pCmd, s32 shade);

typedef struct ModelPrimDesc {
    ModelPrimProc proc[6];
    ModelPrimBuildProc buildProc;   /* host pointer; PSX addr kept in comments */
    u32 cmdStride;
    u32 packetStride;
    u32 outputStride;
} ModelPrimDesc;

extern ModelPrimDesc D_8004FE50[];
extern s32 D_80050104;
extern s32 D_80050100;
extern s32 D_800500F8;
extern s32 D_800500FC;
extern u32 D_80059568;
extern s32 D_80059578;
extern s32 func_8003101C(const u8* bounds, s32 mode);

static u32 ModelPrimVertexIndex1(u32 word) {
    return (word >> 16) & 0xFFFF;
}

/* Retail screen cull (asm 8002E7CC-8002E818): keep a prim only when at least
 * one vertex has packed SXY <u D_800500FC (y within [0, screenH-1]; negative
 * or below-screen y fails the unsigned compare) AND at least one vertex has
 * x <u D_800500F8. An earlier port transcription inverted the first
 * comparison, culling exactly the on-screen prims. */
static int ModelPrimPackedOverlapsScreen(u32 xy0, u32 xy1, u32 xy2, u32 xy3) {
    u32 yMaxPacked = (u32)D_800500FC;
    u32 xMax = (u32)D_800500F8;

    if (!(xy0 < yMaxPacked || xy1 < yMaxPacked || xy2 < yMaxPacked || xy3 < yMaxPacked)) {
        return 0;
    }
    return (((xy0 & 0xFFFF) < xMax) || ((xy1 & 0xFFFF) < xMax) ||
            ((xy2 & 0xFFFF) < xMax) || ((xy3 & 0xFFFF) < xMax));
}

/* PSX GPU hardware rule (see game_overrides.c ModelPrimTriOversized):
 * polygons wider than 1023 or taller than 511 in projected screen space are
 * silently rejected by the rasterizer; PsyX does not implement this. */
/* Kept for reference; retail E688 has no oversize cull. */
static int __attribute__((unused)) ModelPrimPackedOversized(u32 xy0, u32 xy1, u32 xy2, u32 xy3) {
    s32 xs[4], ys[4], xmin, xmax, ymin, ymax, i;
    u32 v[4];
    v[0] = xy0; v[1] = xy1; v[2] = xy2; v[3] = xy3;
    for (i = 0; i < 4; i++) {
        xs[i] = (s16)(v[i] & 0xFFFF);
        ys[i] = (s16)(v[i] >> 16);
    }
    xmin = xmax = xs[0];
    ymin = ymax = ys[0];
    for (i = 1; i < 4; i++) {
        if (xs[i] < xmin) xmin = xs[i];
        if (xs[i] > xmax) xmax = xs[i];
        if (ys[i] < ymin) ymin = ys[i];
        if (ys[i] > ymax) ymax = ys[i];
    }
    return (xmax - xmin) > 1023 || (ymax - ymin) > 511;
}

s32 func_8002E688(u8* pCmd, s32 count) {
    const s32 packetStep = 0x28;
    const u32 tagLen = 0x09000000;
    u8* vertexBase = (u8*)(uintptr_t)D_8005953C;
    u8* out = D_80059424 - packetStep;
    u32* ot = (u32*)(uintptr_t)D_80059568;
    s32 emitted = D_80059578;

    while (count != 0) {
        u32 cmd = *(u32*)pCmd;
        SVECTOR* v0 = (SVECTOR*)(vertexBase + ((cmd & 0xFFFF) << 3));
        SVECTOR* v1 = (SVECTOR*)(vertexBase + (ModelPrimVertexIndex1(cmd) << 3));
        SVECTOR* v2 = (SVECTOR*)(vertexBase + (*(u16*)(pCmd + 0x04) << 3));
        SVECTOR* v3 = (SVECTOR*)(vertexBase + (*(u16*)(pCmd + 0x06) << 3));
        long xy0 = 0;
        long xy1 = 0;
        long xy2 = 0;
        long xy3 = 0;
        long otz = 0;
        long p = 0;
        long flag = 0;

        count--;
        pCmd += 8;
        out += packetStep;

        otz = RotTransPers4(v0, v1, v2, v3, &xy0, &xy1, &xy2, &xy3, &p, &flag);
        /* No FLAG rejection (inert-LZCR sweep): retail's apparent flag gates
         * at 8002E788/8002E7C0 are `mfc2 $t0, $31` -- MFC2 reads DATA reg 31
         * (LZCR, always 1..32), not the FLAG control reg (CFC2) -- so the
         * bltz never takes on hardware; overflowed divides saturate and the
         * quad draws.  (The former XENO_E688_IGNORE_FLAG diagnostic toggle
         * modeled a gate that does not exist; deleted.)  The zero/negative
         * depth check is kept. */
        if (otz <= 0) {
            continue;
        }
        /* asm 8002E7AC: blez MAC0 after NCLIP — reject backface/degenerate. */
        if (NormalClip(xy0, xy1, xy2) <= 0) {
            continue;
        }
        if (!ModelPrimPackedOverlapsScreen((u32)xy0, (u32)xy1, (u32)xy2, (u32)xy3)) {
            continue;
        }
        /* No oversize cull in retail 8002E688 (asm 8002E7CC-8002E894 is only the
         * screen-overlap + max-SZ OT path). A port-only 1023x511 reject would
         * delete extreme close-up FT4s (Map014 painting hold). */

        {
            /* Retail 0x8002E82C-0x8002E894 rejects any zero SZ, selects
             * max(SZ0,SZ1,SZ2,SZ3), and shifts it by D_80050100 + 2.
             * RotTransPers4's return is SZ3 >> 2 and is correct for the PsyQ
             * helper contract, but is not this walker's OT-depth source. */
#ifdef XENO_PC_PORT
            u16 sz0 = (u16)C2_SZ0;
            u16 sz1 = (u16)C2_SZ1;
            u16 sz2 = (u16)C2_SZ2;
            u16 sz3 = (u16)C2_SZ3;
            u16 maxSz;
            s32 otIndex;

            if (sz0 == 0 || sz1 == 0 || sz2 == 0 || sz3 == 0) {
                continue;
            }
            maxSz = sz0;
            if (sz1 > maxSz) maxSz = sz1;
            if (sz2 > maxSz) maxSz = sz2;
            if (sz3 > maxSz) maxSz = sz3;
            otIndex = (s32)maxSz >> (D_80050100 + 2);
#else
            s32 otIndex = (s32)otz >> D_80050100;
#endif
            PcPort_LinkModelPrim(ot, otIndex, out, tagLen);
            *(u32*)(out + 0x08) = (u32)xy0;
            *(u32*)(out + 0x10) = (u32)xy1;
            *(u32*)(out + 0x18) = (u32)xy2;
            *(u32*)(out + 0x20) = (u32)xy3;
            emitted++;
        }
    }

    D_80059578 = emitted;
    D_80059424 = out + packetStep;
    return 1;
}

s32 func_8002C700(u8* a0, u8* a1, u32* a2, s32 a3) {
    s32 groupCount;

    if (D_80050104 != 0 && func_8003101C(a0, D_80050104) != 0) {
        return 0;
    }

    D_80059424 = a1;
    D_80059568 = (u32)(uintptr_t)a2;
    D_800595C0 += *(u16*)(a0 + 0x4);
    D_80059528 = *(u32*)(a0 + 0x10);
    D_80059498 = *(u32*)(a0 + 0x18);
    D_8005952C = *(u32*)(a0 + 0x0C);
    D_8005953C = *(u32*)(a0 + 0x08);

    groupCount = *(u16*)(a0 + 0x06) - 1;
    if (groupCount != -1) {
        do {
            u8* pGroup = (u8*)(uintptr_t)D_80059528;
            u8 prim = pGroup[0];
            s32 count = *(s16*)(pGroup + 0x02);
            ModelPrimDesc* desc;
            ModelPrimProc proc;

            D_80059528 += 4;
            if (prim >= 17) {
                D_80059528 += (u32)count * 8u;
                groupCount--;
                continue;
            }
            desc = &D_8004FE50[prim];
            proc = (a3 < 6) ? desc->proc[a3] : NULL;
            if (proc == NULL) {
                static int s_missing_draw_logs;
                u32 stride = desc->cmdStride ? desc->cmdStride : 8u;
                if (s_missing_draw_logs < 8) {
                    fprintf(stderr, "[xeno-port] missing D_8004FE50 prim=%u variant=%d; skip group\n",
                            prim, a3);
                    s_missing_draw_logs++;
                }
                D_80059528 += (u32)count * stride;
                groupCount--;
                continue;
            }

            proc((u8*)(uintptr_t)D_80059528, count);
            D_80059528 += count * desc->cmdStride;
            groupCount--;
        } while (groupCount != -1);
    }

    return 1;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002C8CC);
#else
/* Builds the render command list for one model into the destination double
 * buffer half (a0=dst header, a1=src buffer, a2=mode).
 *
 * The mode arg selects a "shade/priority" code (s5) that gets packed into the
 * high halfword of the per-primitive callback's third argument; the exact code
 * is derived from `mode` combined with flag bits in *(u16*)dst (bits 0x1/0x2).
 * If the header lacks the 0x1 "packet allocated" flag it first HeapAllocs a
 * packet buffer sized by *(u32*)(dst+0x30) and stores it at dst+0x18.
 *
 * It then walks *(u16*)(dst+0x6) mesh groups; for each group it reads a run of
 * primitives from the parse cursor (D_80059528). Each primitive's first byte
 * indexes a 0x28-byte descriptor table at D_8004FE50; descriptor+0x18 is a
 * function pointer invoked as fn(D_80059538, D_80059528, s5<<16>>16). A nonzero
 * return advances the output/packet cursors by descriptor fields; a zero return
 * advances the packet cursor by 4. func_8002CCAC finalizes the pass. */
void func_8002C8CC(u8* a0, void* a1, s32 a2) {
    u8* s0 = a0;
    s32 s1 = a2;
    s32 s2;
    s32 s4;
    s32 s5 = 0;
    u16 v1_flags;

    D_80059424 = a1;

    if (((*(u16*)(s0 + 0x0) & 0x1) == 0) && (*(s32*)(s0 + 0x30) != 0) &&
        (s1 != 0)) {
        HeapSetCurrentContentType(0x26);
        /* The model header stores a PSX u32 pointer at +0x18.  A native
         * void* store is eight bytes on the port host and overwrites +0x1C. */
        *(u32*)(s0 + 0x18) = (u32)(uintptr_t)HeapAlloc(*(s32*)(s0 + 0x30), 0);
        *(u16*)(s0 + 0x0) = *(u16*)(s0 + 0x0) | 0x1;
    }

    D_80059538 = *(u32*)(s0 + 0x14);
    D_80059528 = *(u32*)(s0 + 0x10);
    D_8005952C = *(u32*)(s0 + 0xC);
    D_8005953C = *(u32*)(s0 + 0x8);
    D_80059498 = *(u32*)(s0 + 0x18);
    v1_flags = *(u16*)(s0 + 0x0);

    /* mode -> s5 shade code, mirroring the branch table in the asm */
    if (s1 == 1) {
        s5 = 1;
    } else if (s1 < 2) {
        if (s1 == 0) {
            s5 = 0;
        }
        /* s1 < 0: fall through, s5 left as-is */
    } else if (s1 == 2) {
        if (v1_flags & 0x2) {
            if (v1_flags & 0x1) {
                s5 = 4;
            } else {
                s5 = 1;
            }
        } else {
            if (v1_flags & 0x1) {
                s5 = 3;
                *(u16*)(s0 + 0x0) = v1_flags | 0x2;
            } else {
                s5 = 1;
            }
        }
    } else if (s1 == 3) {
        if (v1_flags & 0x1) {
            s5 = 3;
            *(u16*)(s0 + 0x0) = v1_flags | 0x2;
        }
        /* (v1_flags & 1) == 0: s5 left as-is */
    }

    s2 = (s32)*(u16*)(s0 + 0x6) - 1;
    D_800595C0 = D_800595C0 + *(u16*)(s0 + 0x4);

    s4 = -1;
#ifdef XENO_PC_PORT
    /* Port dispatch: the host ModelPrimDesc is 0x40 bytes (64-bit proc
     * pointers), so the retail byte math below (+prim*0x28, field reads at
     * +0x18/+0x1C/+0x20/+0x24) would misindex it. Use structural access and
     * the typed host buildProc pointer instead (same migration as
     * func_8002C700's render dispatch). */
    if (s2 != s4) {
        do {
            u8* pCur = (u8*)D_80059528;
            s32 s0_cnt = (s32)*(s16*)(pCur + 0x2) - 1;
            ModelPrimDesc* desc;
            ModelPrimBuildProc fn;

            D_80059528 = (u32)(pCur + 0x4);
            {
                u32 prim = *(u8*)(pCur + 0x0);
                s32 nprim;
                u32 stride;
                if (prim >= 17) {
                    nprim = (s32)*(s16*)(pCur + 0x2);
                    stride = 8u;
                    if (nprim > 0) {
                        D_80059528 += (u32)nprim * stride;
                    }
                    s2 = s2 - 1;
                    continue;
                }
                desc = &D_8004FE50[prim];
                if (desc->buildProc == NULL) {
                    /* Retail table has a full 15-row set; the port wires a
                     * sparse subset. Advance the command cursor by this
                     * group's prim count so a missing row does not desync
                     * later groups in the same header. */
                    static int s_missing_prim_logs;
                    nprim = (s32)*(s16*)(pCur + 0x2);
                    stride = desc->cmdStride ? desc->cmdStride : 8u;
                    if (s_missing_prim_logs < 8) {
                        fprintf(stderr,
                                "[xeno-port] missing D_8004FE50 buildProc "
                                "prim=%u; skip group\n",
                                prim);
                        s_missing_prim_logs++;
                    }
                    if (nprim > 0) {
                        D_80059528 += (u32)nprim * stride;
                    }
                    s2 = s2 - 1;
                    continue;
                }
            }
            fn = desc->buildProc;

            if (s0_cnt != s4) {
                do {
                    s32 ret = fn(D_80059538, D_80059528, (s5 << 16) >> 16);
                    if (ret != 0) {
                        D_80059528 = D_80059528 + desc->cmdStride;
                        D_80059424 = D_80059424 + desc->outputStride;
                        D_80059538 = D_80059538 + desc->packetStride;
                        s0_cnt = s0_cnt - 1;
                    } else {
                        D_80059538 = D_80059538 + 4;
                        /* s0 += 1; s0 -= 1  (net no change to counter) */
                    }
                } while (s0_cnt != s4);
            }
            s2 = s2 - 1;
        } while (s2 != s4);
    }
#else
    if (s2 != s4) {
        do {
            u8* pCur = (u8*)D_80059528;
            s32 s0_cnt = (s32)*(s16*)(pCur + 0x2) - 1;
            u8* s1_desc;
            u32 s3_fn;

            D_80059528 = (u32)(pCur + 0x4);
            {
                u32 prim = *(u8*)(pCur + 0x0);
                s1_desc = D_8004FE50 + (((prim << 2) + prim) << 3); /* *0x28 */
            }
            s3_fn = *(u32*)(s1_desc + 0x18);

            if (s0_cnt != s4) {
                do {
                    s32 (*fn)(u32, u32, s32) = (s32 (*)(u32, u32, s32))s3_fn;
                    s32 ret = fn(D_80059538, D_80059528, (s5 << 16) >> 16);
                    if (ret != 0) {
                        D_80059528 = D_80059528 + *(u32*)(s1_desc + 0x1C);
                        D_80059424 = D_80059424 + *(u32*)(s1_desc + 0x24);
                        D_80059538 = D_80059538 + *(u32*)(s1_desc + 0x20);
                        s0_cnt = s0_cnt - 1;
                    } else {
                        D_80059538 = D_80059538 + 4;
                        /* s0 += 1; s0 -= 1  (net no change to counter) */
                    }
                } while (s0_cnt != s4);
            }
            s2 = s2 - 1;
        } while (s2 != s4);
    }
#endif

    func_8002CCAC();
}
#endif

/* Allocates the per-model double buffer. modelData+0x34 is the model's byte
 * size; it allocates 2x that, hands out1 (first half) via out1 and out2 (second
 * half, base+size) via out2. Callers fill out1 then memcpy into out2. */
/* out1/out2 point at PSX 4-byte pointer slots in the caller's model control block
 * (misc3.c pModel+0x8/+0xC). The asm stores them with `sw` (32-bit); write u32
 * (truncated host pointer, lossless below 4 GiB) so we don't clobber the adjacent
 * slot the way an 8-byte `void*` store would. */
void func_8002CB54(u8* modelData, u32* out1, u32* out2) {
    void* buf;
    HeapSetCurrentContentType(0x25);
    buf = HeapAlloc(*(s32*)(modelData + 0x34) << 1, 0);
    *out1 = (u32)buf;
    *out2 = (u32)((u8*)buf + *(s32*)(modelData + 0x34));
}

// Frees the model's optional secondary buffer at modelData+0x18 iff the
// model's own flags word (+0x0) has bit 0x1 set, then clears that bit. The
// flag is self-describing: it is only set when +0x18 was actually populated
// with a real allocation, so this never frees an unowned/uninitialized field.
void func_8002CBBC(u8* modelData) {
    if (*(u16*)(modelData + 0x0) & 0x1) {
        /* The optional buffer is stored in the model's PSX u32 slot. */
        HeapFree((void*)(uintptr_t)*(u32*)(modelData + 0x18));
        *(u16*)(modelData + 0x0) = *(u16*)(modelData + 0x0) & 0xFFFE;
    }
}

extern s32 D_80059310;
extern s32 D_80050108;

void func_8002CC10(u16 x, u16 y) {
    D_80059310 = GetTPage(0, 0, x, y) & 0x1F;
    D_80050108 = 1;
}

extern s32 D_80059310;
extern s32 D_80050108;

void func_8002CC54(u16 a0) {
    D_80059310 = a0;
    D_80050108 = 2;
}

extern u_short GetClut(int x, int y);
extern s32 D_80059314;
extern s32 D_8005010C;

void func_8002CC74(u16 a0, u16 a1) {
    D_80059314 = GetClut(a0, a1) & 0xFFF0;
    D_8005010C = 0;
}

extern s32 D_8005010C;

void func_8002CCAC(void) {
    D_80050108 = 0;
    D_8005010C = 1;
}

/* Inline-state handler for code 0xC4: latch the record's texture page into
 * D_80059308 (the hi half OR'd into textured packet words at +0x14). Mode
 * D_80050108: 0 = raw latch, 1 = mask low bits and merge the D_80059310
 * override, 2 = full override from D_80059310. State-only; no cursor effects.
 * asm: func_8002CCC8.s. */
extern u16 D_80059308;
extern u16 D_8005930C;

void func_8002CCC8(u8* pSrc) {
    u16 v = *(u16*)pSrc;
    s32 mode = D_80050108;

    D_80059308 = v;
    if (mode == 1) {
        u16 masked = v & 0xFFE0;
        D_80059308 = masked;
        D_80059308 = masked | (u16)D_80059310;
    } else if (mode == 2) {
        D_80059308 = (u16)D_80059310;
    }
}

/* Inline-state handler for code 0xC8: latch the record's CLUT id into
 * D_8005930C (the hi half OR'd into textured packet words at +0x0C). Mode
 * D_8005010C == 0: keep only the low nibble and merge the D_80059314 CLUT
 * base (set by func_8002CC74); nonzero: raw latch. State-only. asm:
 * func_8002CD24.s. */
void func_8002CD24(u8* pSrc) {
    u16 v = *(u16*)pSrc;

    D_8005930C = v;
    if (D_8005010C == 0) {
        u16 lo = v & 0xF;
        D_8005930C = lo;
        D_8005930C = lo | (u16)D_80059314;
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002CD64);
#else
/* Gate shared by the textured model buildProcs (func_8002D984/func_8002D0E4):
 * packet-source records whose code byte (+0x3) is a 0xC0-class inline state
 * command dispatch to a state handler and emit no primitive (return 0);
 * anything else emits (return 1). The handlers receive the record pointer
 * (retail keeps pSrc in $a0 across the call) and latch tpage/CLUT state. */
s32 func_8002CD64(u8* pSrc) {
    u8 code = pSrc[0x3];

    if ((code & 0xF0) != 0xC0) {
        return 1;
    }
    if (code == 0xC4) {
        func_8002CCC8(pSrc);
        return 0;
    }
    if (code == 0xC8) {
        func_8002CD24(pSrc);
        return 0;
    }
    return 1;
}
#endif

/* buildProc for prim 0x00 (lit flat tri template, tag len 4). Byte-identical
 * retail clone of prim 0x08's builder func_8002CF58 below -- only the three
 * intra-function jump targets differ in the retail bytes; see that function's
 * comment for the four shade paths. asm: func_8002CDCC.s. */
extern void func_8002DB84(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* outNormal);
extern void NormalLightCol(void* a0, void* a1, void* a2);

#ifdef XENO_PC_PORT
/* Coexistence: the C transcription compiles ~59% fuzzy under mwcc (same as
 * the sibling builders), so the matching build keeps the retail bytes via
 * INCLUDE_ASM below and only the port compiles this body. */
s32 func_8002CDCC(u8* pSrc, u8* pCmd, s32 shade) {
    u8* p = D_80059424;
    SVECTOR tmpNormal; /* asm's sp+0x10 out-normal for the plain-lit path */

    p[0x3] = 0x4;
    if (shade & 0x1) {
        if (shade & 0x2) {
            u8* vb;
            SVECTOR *n0;
            SVECTOR *n1;
            SVECTOR *n2;

            *(u32*)(uintptr_t)D_80059498 = *(u32*)pSrc;
            vb = (u8*)(uintptr_t)D_8005953C;
            n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));
            D_80059498 += 4;
            func_8002DB84(n0, n1, n2, (SVECTOR*)(uintptr_t)D_80059498);
            NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
            D_80059498 += 8;
        } else {
            u8* vb = (u8*)(uintptr_t)D_8005953C;
            SVECTOR* n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            SVECTOR* n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            SVECTOR* n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));

            func_8002DB84(n0, n1, n2, &tmpNormal);
            NormalLightCol(&tmpNormal, pSrc, p + 0x4);
        }
        p[0x7] = pSrc[0x3];
    } else if (shade & 0x4) {
        D_80059498 += 4;
        NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
        D_80059498 += 8;
        p[0x7] = pSrc[0x3];
    } else {
        *(u32*)(p + 0x4) = *(u32*)pSrc;
    }
    return 1;
}
#else
/* retail 0x8002CDCC: prim builder 0x04 -- four shade paths */
s32 func_8002CDCC(u8* pSrc, u8* pCmd, s32 shade) {
    u8* p = D_80059424;
    SVECTOR tmpNormal;

    p[0x3] = 0x4;
    if (shade & 0x1) {
        if (shade & 0x2) {
            u8* vb;
            SVECTOR *n0;
            SVECTOR *n1;
            SVECTOR *n2;

            *(u32*)(uintptr_t)D_80059498 = *(u32*)pSrc;
            vb = (u8*)(uintptr_t)D_8005953C;
            n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));
            D_80059498 += 4;
            func_8002DB84(n0, n1, n2, (SVECTOR*)(uintptr_t)D_80059498);
            NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
            D_80059498 += 8;
        } else {
            u8* vb = (u8*)(uintptr_t)D_8005953C;
            SVECTOR* n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            SVECTOR* n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            SVECTOR* n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));

            func_8002DB84(n0, n1, n2, &tmpNormal);
            NormalLightCol(&tmpNormal, pSrc, p + 0x4);
        }
        p[0x7] = pSrc[0x3];
    } else if (shade & 0x4) {
        D_80059498 += 4;
        NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
        D_80059498 += 8;
        p[0x7] = pSrc[0x3];
    } else {
        *(u32*)(p + 0x4) = *(u32*)pSrc;
    }
    return 1;
}
#endif

extern u8* D_80059424;

s32 func_8002CF34(s32* a0) {
    u8* p = D_80059424;
    p[3] = 4;
    *(s32*)(p + 4) = *a0;
    return 1;
}

/* buildProc for prim 0x08 (lit flat tri template, tag len 4). Four paths by
 * shade mode: unlit (copy rgb|code word), lit (face normal via func_8002DB84 +
 * NormalLightCol into packet color), lit+stream (also store rgb and normals to
 * the D_80059498 work stream), and pre-lit stream (shade&4). The lit paths
 * compute the face normal via func_8002DB84 (out param: stack normal for the
 * plain-lit path, the D_80059498 stream slot for the lit+stream path, per the
 * asm's $a3) and light it with NormalLightCol/NCCS. asm: func_8002CF58.s. */
extern void func_8002DB84(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* outNormal);
extern void NormalLightCol(void* a0, void* a1, void* a2);

s32 func_8002CF58(u8* pSrc, u8* pCmd, s32 shade) {
    u8* p = D_80059424;
    SVECTOR tmpNormal; /* asm's sp+0x10 out-normal for the plain-lit path */

    p[0x3] = 0x4;
    if (shade & 0x1) {
        if (shade & 0x2) {
            u8* vb;
            SVECTOR *n0;
            SVECTOR *n1;
            SVECTOR *n2;

            *(u32*)(uintptr_t)D_80059498 = *(u32*)pSrc;
            vb = (u8*)(uintptr_t)D_8005953C;
            n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));
            D_80059498 += 4;
            func_8002DB84(n0, n1, n2, (SVECTOR*)(uintptr_t)D_80059498);
            NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
            D_80059498 += 8;
        } else {
            u8* vb = (u8*)(uintptr_t)D_8005953C;
            SVECTOR* n0 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x0) << 3));
            SVECTOR* n1 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x2) << 3));
            SVECTOR* n2 = (SVECTOR*)(vb + ((s32)*(s16*)(pCmd + 0x4) << 3));

            func_8002DB84(n0, n1, n2, &tmpNormal);
            NormalLightCol(&tmpNormal, pSrc, p + 0x4);
        }
        p[0x7] = pSrc[0x3];
    } else if (shade & 0x4) {
        D_80059498 += 4;
        NormalLightCol((void*)(uintptr_t)D_80059498, pSrc, p + 0x4);
        D_80059498 += 8;
        p[0x7] = pSrc[0x3];
    } else {
        *(u32*)(p + 0x4) = *(u32*)pSrc;
    }
    return 1;
}

s32 func_8002D0C0(s32* a0) {
    u8* p = D_80059424;
    p[3] = 5;
    *(s32*)(p + 4) = *a0;
    return 1;
}

/* buildProc for prim 0x0D (textured quad, POLY_FT4 template, tag len 9).
 * Reads one 0x0C-byte packet-source record (a0 = D_80059538 cursor) and writes
 * the static packet fields at the D_80059424 output cursor; the per-frame render
 * proc (func_8002E688) later patches the projected xy words in. asm:
 * func_8002D984.s sibling — p[3]=9; +0x4 = rgb|code word; +0xC = uv0 | clut<<16;
 * +0x14 = uv1 | tpage<<16; +0x1C = uv2; +0x24 = uv3. */
extern u16 D_80059308;  /* current model texture page (hi half of packet +0x14) */
extern u16 D_8005930C;  /* current model CLUT id      (hi half of packet +0x0C) */

s32 func_8002D0E4(u8* pSrc) {
    u8* p;

    if (func_8002CD64(pSrc) == 0) {
        return 0;
    }
    p = D_80059424;
    p[0x3] = 0x9;
    *(u32*)(p + 0x04) = *(u32*)(pSrc + 0x0);
    *(u32*)(p + 0x0C) = *(u16*)(pSrc + 0x4) | ((u32)D_8005930C << 16);
    *(u32*)(p + 0x14) = *(u16*)(pSrc + 0x6) | ((u32)D_80059308 << 16);
    *(u16*)(p + 0x1C) = *(u16*)(pSrc + 0x8);
    *(u16*)(p + 0x24) = *(u16*)(pSrc + 0xA);
    return 1;
}

#ifdef XENO_PC_PORT
/* Retail 0x8002D180 returns 1 and lights the 4th vertex from pColor (a0),
 * not the shade word C8CC passes as a2. The matching C body left a2 as a
 * pointer and dropped the return, which made the POLY_G4 builder unusable
 * as a D_8004FE50 buildProc. Vertices 0-2 are lit from pColor into +4/+0xC/
 * +0x14 (retail NormalColorCol3 arguments), vertex 3 into +0x1C. */
s32 func_8002D180(u8* pColor, s16* pIndices, s32 shade) {
    u8* pPoly = D_80059424;
    s16* normals = (s16*)D_8005952C;
    (void)shade;
    pPoly[3] = 8; /* POLY_G4 tag */
    NormalColorCol3(
        (SVECTOR*)&normals[pIndices[0] * 4],
        (SVECTOR*)&normals[pIndices[1] * 4],
        (SVECTOR*)&normals[pIndices[2] * 4],
        (CVECTOR*)pColor,
        (CVECTOR*)(pPoly + 4),
        (CVECTOR*)(pPoly + 0xC),
        (CVECTOR*)(pPoly + 0x14)
    );
    NormalLightCol((SVECTOR*)&normals[pIndices[3] * 4], pColor, pPoly + 0x1C);
    pPoly[7] = pColor[3];
    return 1;
}
#else

#ifndef XENO_PC_PORT
s32 func_8002D180(void *arg0, void *arg1) {
    void *temp_s0;

    temp_s0 = (*(void **)&D_80059424);
    *(s8 *)((s8*)(temp_s0) + 3) = 8;
    NormalColorCol3((*(s32*)&D_8005952C) + (*(s16 *)((s8*)(arg1) + 0) * 8), (*(s32*)&D_8005952C) + (*(s16 *)((s8*)(arg1) + 2) * 8), (*(s32*)&D_8005952C) + (*(s16 *)((s8*)(arg1) + 4) * 8), arg0, temp_s0 + 4, temp_s0 + 0xC, temp_s0 + 0x14);
    NormalLightCol((*(s32*)&D_8005952C) + (*(s16 *)((s8*)(arg1) + 6) * 8), arg0, temp_s0 + 0x1C);
    *(u8 *)((s8*)(temp_s0) + 7) = (u8) *(u8 *)((s8*)(arg0) + 3);
    return 1;
}
#endif

#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002D244);
#else
s32 func_8002D244(u8* pColor, s16* pIndices) {
    if (func_8002CD64(pColor) == 0) return 0;
    {
        u8* pPoly = D_80059424;
        s16* normals = (s16*)D_8005952C;
        pPoly[3] = 0xC; /* POLY_GT4 tag */
        NormalColor3(
            (SVECTOR*)&normals[pIndices[0] * 4],
            (SVECTOR*)&normals[pIndices[1] * 4],
            (SVECTOR*)&normals[pIndices[2] * 4],
            (CVECTOR*)(pPoly + 4),
            (CVECTOR*)(pPoly + 0x10),
            (CVECTOR*)(pPoly + 0x1C)
        );
        NormalColor((SVECTOR*)&normals[pIndices[3] * 4], (CVECTOR*)(pPoly + 0x28));
        *(u32*)(pPoly + 0x0C) = *(u16*)(pColor + 4) | ((u32)D_8005930C << 16);
        *(u32*)(pPoly + 0x18) = *(u16*)(pColor + 6) | ((u32)D_80059308 << 16);
        *(u16*)(pPoly + 0x24) = *(u16*)(pColor + 8);
        *(u16*)(pPoly + 0x30) = *(u16*)(pColor + 0xA);
        pPoly[7] = pColor[3];
    }
    return 1;
}
#endif

/* Lit POLY_G4 (tag 8): copies the base colour, lights three vertices with
 * NormalColorCol3 and the fourth with NormalLightCol, all from pColor.
 * Build-pass proc: fn(pColor, pIndices, shade); retail returns 1. */
s32 func_8002D354(u8* pColor, s16* pIndices) {
    u8* pPoly = D_80059424;

    pPoly[3] = 8;
    *(s32*)(pPoly + 4) = *(s32*)pColor;
    NormalColorCol3((SVECTOR*)(uintptr_t)(D_8005952C + pIndices[0] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[1] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[2] * 8), (CVECTOR*)pColor,
                    (CVECTOR*)(pPoly + 4), (CVECTOR*)(pPoly + 0xC), (CVECTOR*)(pPoly + 0x14));
    NormalLightCol((SVECTOR*)(uintptr_t)(D_8005952C + pIndices[3] * 8), pColor, pPoly + 0x1C);
    pPoly[7] = pColor[3];
    return 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002D420);
#else
s32 func_8002D420(u8* pColor, s16* pIndices) {
    if (func_8002CD64(pColor) == 0) return 0;
    {
        u8* pPoly = D_80059424;
        s16* normals = (s16*)D_8005952C;
        pPoly[3] = 0xC; /* POLY_GT4 tag */
        NormalColor3(
            (SVECTOR*)&normals[pIndices[0] * 4],
            (SVECTOR*)&normals[pIndices[1] * 4],
            (SVECTOR*)&normals[pIndices[2] * 4],
            (CVECTOR*)(pPoly + 4),
            (CVECTOR*)(pPoly + 0x10),
            (CVECTOR*)(pPoly + 0x1C)
        );
        NormalColor((SVECTOR*)&normals[pIndices[3] * 4], (CVECTOR*)(pPoly + 0x28));
        *(u32*)(pPoly + 0x0C) = *(u16*)(pColor + 4) | ((u32)D_8005930C << 16);
        *(u32*)(pPoly + 0x18) = *(u16*)(pColor + 6) | ((u32)D_80059308 << 16);
        *(u16*)(pPoly + 0x24) = *(u16*)(pColor + 8);
        *(u16*)(pPoly + 0x30) = *(u16*)(pColor + 0xA);
        pPoly[7] = pColor[3];
    }
    return 1;
}
#endif

/* Textured lit POLY_GT3 (tag 9): tpage/clut words and UVs from pColor, one
 * face normal lit with NormalColor. flags bit 0: compute the normal from the
 * three vertex normals (into the D_80059498 stream with bit 1, else a
 * scratch vector); bit 2 alone: reuse the stream's current normal. The
 * stream always advances by 8. Build-pass proc: fn(pColor, pIndices, flags). */
s32 func_8002D530(u8* pColor, s16* pIndices, s32 flags) {
    SVECTOR normal;
    u8* pPoly;

    if (func_8002CD64(pColor) == 0) {
        return 0;
    }
    pPoly = D_80059424;
    pPoly[3] = 9;
    *(u32*)(pPoly + 0x0C) = *(u16*)(pColor + 4) | ((u32)D_8005930C << 16);
    *(u32*)(pPoly + 0x14) = *(u16*)(pColor + 6) | ((u32)D_80059308 << 16);
    *(u16*)(pPoly + 0x1C) = *(u16*)(pColor + 8);
    *(u16*)(pPoly + 0x24) = *(u16*)(pColor + 0xA);
    if (flags & 1) {
        if (flags & 2) {
            func_8002DB84((SVECTOR*)(uintptr_t)(D_8005953C + pIndices[0] * 8), (SVECTOR*)(uintptr_t)(D_8005953C + pIndices[1] * 8), (SVECTOR*)(uintptr_t)(D_8005953C + pIndices[2] * 8), (SVECTOR*)(uintptr_t)D_80059498);
            NormalColor((SVECTOR*)(uintptr_t)D_80059498, (CVECTOR*)(pPoly + 4));
        } else {
            func_8002DB84((SVECTOR*)(uintptr_t)(D_8005953C + pIndices[0] * 8), (SVECTOR*)(uintptr_t)(D_8005953C + pIndices[1] * 8), (SVECTOR*)(uintptr_t)(D_8005953C + pIndices[2] * 8), &normal);
            NormalColor(&normal, (CVECTOR*)(pPoly + 4));
        }
    } else if (flags & 4) {
        NormalColor((SVECTOR*)(uintptr_t)D_80059498, (CVECTOR*)(pPoly + 4));
    }
    D_80059498 += 8;
    pPoly[7] = pColor[3];
    return 1;
}

extern u32 D_80059498;

/* Lit POLY_G3 (tag 6); with flags bit 1 it also appends the base colour
 * word to the D_80059498 stream. Build-pass proc: fn(pColor, pIndices,
 * flags) -- flags is the third argument; retail returns 1. */
s32 func_8002D6AC(u8* pColor, s16* pIndices, s32 flags) {
    u8* pPoly = D_80059424;

    pPoly[3] = 6;
    if (flags & 2) {
        *(s32*)(uintptr_t)D_80059498 = *(s32*)pColor;
        D_80059498 += 4;
    }
    NormalColorCol3((SVECTOR*)(uintptr_t)(D_8005952C + pIndices[0] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[1] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[2] * 8), (CVECTOR*)pColor,
                    (CVECTOR*)(pPoly + 4), (CVECTOR*)(pPoly + 0xC), (CVECTOR*)(pPoly + 0x14));
    pPoly[7] = pColor[3];
    return 1;
}

/* Lit POLY_G3 (tag 6). Build-pass proc: fn(pColor, pIndices, shade);
 * retail returns 1. */
s32 func_8002D77C(u8* pColor, s16* pIndices) {
    u8* pPoly = D_80059424;

    pPoly[3] = 6;
    NormalColorCol3((SVECTOR*)(uintptr_t)(D_8005952C + pIndices[0] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[1] * 8), (SVECTOR*)(uintptr_t)(D_8005952C + pIndices[2] * 8), (CVECTOR*)pColor,
                    (CVECTOR*)(pPoly + 4), (CVECTOR*)(pPoly + 0xC), (CVECTOR*)(pPoly + 0x14));
    pPoly[7] = pColor[3];
    return 1;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002D814);
#else
s32 func_8002D814(u8* pColor, s16* pIndices, s32 flags) {
    u8* pPoly;
    s16* normals;
    SVECTOR* n0;
    SVECTOR* n1;
    SVECTOR* n2;

    if (func_8002CD64(pColor) == 0) return 0;
    pPoly = D_80059424;
    normals = (s16*)D_8005953C;
    pPoly[3] = 7; /* POLY_GT3 tag */
    n0 = (SVECTOR*)&normals[pIndices[0] * 4];
    n1 = (SVECTOR*)&normals[pIndices[1] * 4];
    n2 = (SVECTOR*)&normals[pIndices[2] * 4];

    /* Vertex color tpage/clut merge */
    *(u32*)(pPoly + 0x0C) = *(u16*)(pColor + 4) | ((u32)D_8005930C << 16);
    *(u32*)(pPoly + 0x14) = *(u16*)(pColor + 6) | ((u32)D_80059308 << 16);
    *(u16*)(pPoly + 0x1C) = *(u16*)(pColor + 0);

    if (flags & 1) {
        if (flags & 2) {
            func_8002DB84(n0, n1, n2, (SVECTOR*)(uintptr_t)D_80059498);
#ifdef XENO_PC_PORT
            /* Retail shade 3 (C8CC mode 2 first pass) stores the face
             * normal at D_80059498; the matching C body never lit the
             * packet, so POLY_FT3 rgb stayed 0 and overlay meshes drew
             * black. Mirror func_8002CDCC: light from that normal. */
            NormalLightCol((void*)(uintptr_t)D_80059498, pColor, pPoly + 4);
#endif
        } else {
            SVECTOR tmpNormal;
            func_8002DB84(n0, n1, n2, &tmpNormal);
            NormalColor(&tmpNormal, (CVECTOR*)(pPoly + 4));
        }
    } else if (flags & 4) {
        NormalColor((SVECTOR*)(uintptr_t)D_80059498, (CVECTOR*)(pPoly + 4));
    }

    D_80059498 += 8;
    pPoly[7] = pColor[3];
#ifdef XENO_PC_PORT
    if ((pPoly[4] | pPoly[5] | pPoly[6]) == 0) {
        pPoly[4] = pPoly[5] = pPoly[6] = 0x80;
    }
#endif
    return 1;
}
#endif

/* buildProc for prim 0x05 (textured tri, POLY_FT3 template, tag len 7).
 * Reads one 0x08-byte packet-source record and writes the static packet fields;
 * the render proc (ModelPrimTriVariant0) later patches the xy words. asm:
 * p[3]=7; +0xC = uv0 | clut<<16; +0x14 = uv1 | tpage<<16; +0x1C = uv2;
 * p[0x7] = code byte (rgb bytes at +0x4..+0x6 are owned by the render pass). */
s32 func_8002D984(u8* pSrc) {
    u8* p;

    if (func_8002CD64(pSrc) == 0) {
        return 0;
    }
    p = D_80059424;
    p[0x3] = 0x7;
    *(u32*)(p + 0x0C) = *(u16*)(pSrc + 0x4) | ((u32)D_8005930C << 16);
    *(u32*)(p + 0x14) = *(u16*)(pSrc + 0x6) | ((u32)D_80059308 << 16);
    *(u16*)(p + 0x1C) = *(u16*)(pSrc + 0x0);
    p[0x7] = pSrc[0x3];
    return 1;
}

extern u16 D_80059308;
extern u16 D_8005930C;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002DA14);
#else
s32 func_8002DA14(u8* pColor, s16* pIndices) {
    if (func_8002CD64(pColor) == 0) return 0;
    {
        u8* pPoly = D_80059424;
        s16* normals = (s16*)D_8005952C;
        pPoly[3] = 9; /* POLY_F4 tag */
        NormalColor3(
            (SVECTOR*)&normals[pIndices[0] * 4],
            (SVECTOR*)&normals[pIndices[1] * 4],
            (SVECTOR*)&normals[pIndices[2] * 4],
            (CVECTOR*)(pPoly + 4),
            (CVECTOR*)(pPoly + 0x10),
            (CVECTOR*)(pPoly + 0x1C)
        );
        *(u32*)(pPoly + 0x0C) = *(u16*)(pColor + 4) | ((u32)D_8005930C << 16);
        *(u32*)(pPoly + 0x18) = *(u16*)(pColor + 6) | ((u32)D_80059308 << 16);
        *(u16*)(pPoly + 0x24) = *(u16*)(pColor);
        pPoly[7] = pColor[3];
    }
    return 1;
}
#endif

#ifndef XENO_PC_PORT
s32 func_8002DAFC(void) {
    void *poly;

    poly = D_80059424;
    SetPolyFT3(poly);
    SetShadeTex(poly, 1);
    *(s16 *)((u8 *)poly + 0x16) = (GetTPage(1, 0, 0x280, 0) & 0xFFE0) | D_80059310;
    *(s16 *)((u8 *)poly + 0xE) = (GetClut(0, 0x1E0) & 0xF) | D_80059314;
    return 1;
}
#else
void func_8002DAFC(void) {
    u8* pPoly = D_80059424;
    SetPolyFT3((POLY_FT3*)pPoly);
    SetShadeTex(pPoly, 1);
    {
        s32 tpage = GetTPage(1, 0, 0x280, 0);
        *(u16*)(pPoly + 0x16) = (tpage & 0xFFE0) | D_80059310;
    }
    {
        s32 clut = GetClut(0, 0x1E0);
        *(u16*)(pPoly + 0x0E) = (clut & 0xF) | D_80059314;
    }
}
#endif

/* Computes the normalized face normal of the triangle (v0,v1,v2) into
 * outNormal (SVECTOR, 4096-scale via VectorNormalS). asm: edge diffs ->
 * OuterProduct0 cross -> divide by sqrt(|dominant component|) as range
 * reduction -> VectorNormalS. The asm does a raw MIPS div (div-by-zero is
 * non-trapping garbage on PSX); the host must guard the degenerate-triangle
 * case explicitly, so len==0 skips the reduction (VectorNormalS of the zero
 * vector then yields a zero normal, matching the garbage-tolerant intent). */
/* SquareRoot0 comes from the shim psyq/libgte.h (PsyX: int SquareRoot0(int)). */
extern long VectorNormalS(VECTOR* in, SVECTOR* out);
extern void OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* out);
s32 func_8002DC9C(s32 x, s32 y, s32 z);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002DB84);
#else
void func_8002DB84(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* outNormal) {
    VECTOR d1;
    VECTOR d2;
    VECTOR cross;
    s32 dom;
    s32 len;

    d1.vx = v1->vx - v0->vx;
    d1.vy = v1->vy - v0->vy;
    d1.vz = v1->vz - v0->vz;
    d2.vx = v2->vx - v0->vx;
    d2.vy = v2->vy - v0->vy;
    d2.vz = v2->vz - v0->vz;

    OuterProduct0(&d1, &d2, &cross);

    dom = func_8002DC9C(cross.vx, cross.vy, cross.vz);
    if (dom < 0) {
        dom = -dom;
    }
    len = SquareRoot0(dom);
    if (len != 0) {
        cross.vx /= len;
        cross.vy /= len;
        cross.vz /= len;
    }
    VectorNormalS(&cross, outNormal);
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002DC9C);
#else
/* Returns the signed component with the largest absolute value (ties favor
 * x, then y). Used by func_8002DB84 as the range-reduction pivot before
 * normalizing a face normal. asm: abs-compare branch tree in func_8002DC9C.s
 * (the "return 1" edges in the tree are unreachable for real inputs). */
s32 func_8002DC9C(s32 x, s32 y, s32 z) {
    s32 ax = (x < 0) ? -x : x;
    s32 ay = (y < 0) ? -y : y;
    s32 az = (z < 0) ? -z : z;

    if (ax >= ay && ax >= az) {
        return x;
    }
    if (ay >= ax && ay >= az) {
        return y;
    }
    return z;
}
#endif

#ifdef XENO_PC_PORT
/* TIM resource loader: pList[0] = entry count + 1; pList[1..count] is an offset
 * table; each offset points at a TIM blob within pList. Walk it high->low,
 * OpenTIM/ReadTIM each, upload the CLUT (if present) then the pixels to VRAM via
 * LoadImage. This uploads the member-change menu's textures + font CLUT --
 * stubbed, the font renders with the wrong palette (colour bands, not glyphs). */
void func_8002DD20(u32* pList) {
    s32 count = (s32)pList[0] - 1;
    u32* pEntry;

    if (count == -1) {
        return;
    }
    pEntry = pList + count;
    do {
        u32 offset = (pEntry[1] >> 2) << 2;
        TIM_IMAGE tim;

        OpenTIM((u_long*)((u8*)pList + offset));
        ReadTIM(&tim);
        if (tim.caddr != NULL) {
            DrawSync(0);
            LoadImage(tim.crect, (u_long*)tim.caddr);
        }
        DrawSync(0);
        pEntry -= 1;
        LoadImage(tim.prect, (u_long*)tim.paddr);
        count -= 1;
    } while (count != -1);
}
#else
/* retail 0x8002DD20: upload the TIM images referenced by a strip table */
void func_8002DD20(u_long* table) {
    TIM_IMAGE tim;
    u_long* entry;
    s32 i;
    s32 end;

    i = table[0];
    if (--i == -1) return;
    end = -1;
    entry = (u_long*)((u32)i * 4 + (u32)table);

    while (1) {
        OpenTIM((u_long*)((u8*)table + ((entry[1] >> 2) << 2)));
        ReadTIM(&tim);
        if (tim.caddr != 0) {
            DrawSync(0);
            LoadImage(tim.crect, tim.caddr);
        }
        DrawSync(0);
        entry--;
        LoadImage(tim.prect, tim.paddr);
        if (--i == end) break;
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002DDE4);
#else
/* Transcribed from asm/slus_006.64/nonmatchings/system/temp2/func_8002DDE4.s
 * (0x8002DDE4-0x8002DFE0, 127 instructions). Uploads a sequence of images: the
 * header word at pImageData[0] is the count, records follow at +4 with a 0x1100
 * (texX/texY mode) or 0x1101 (ofsX/ofsY mode) opcode. For each record the RECT
 * x/y comes from the record's u/v fields — via the mode-1 offset pair, the
 * mode-2 offset pair plus the record sums, or the record sums alone when the
 * mode is neither — then the width/height words follow, and LoadImage uploads
 * the pixels; the cursor advances by w*h*2. Returns 0 normally, 0 when the count
 * is not positive, and **0x1101** for an unknown opcode (the value retail leaves
 * in v0 via its delay slot). */
s32 func_8002DDE4(u32* pImageData, s32 arg1, s32 texX, s32 texY, s32 modeB, u16 ofsX, u16 ofsY) {
    RECT rect;
    s16 modeA = (s16)arg1;
    u32 count = pImageData[0];
    u8* pSrc = (u8*)pImageData + (count * 4) + 4;
    s32 i = 0;
    u32 opcode;

    if ((s32)count <= 0) {
        return 0;
    }
    do {
        opcode = *(u32*)pSrc;
        pSrc += 4;
        if (opcode == 0x1100) {
            if (modeA == 1) {
                rect.x = (s16)(texX + *(u16*)(pSrc + 4));
                rect.y = (s16)(texY + *(u16*)(pSrc + 6));
            } else if (modeA == 2) {
                rect.x = (s16)(texX + *(u16*)(pSrc + 0) + *(u16*)(pSrc + 4));
                rect.y = (s16)(texY + *(u16*)(pSrc + 2) + *(u16*)(pSrc + 6));
            } else {
                rect.x = (s16)(*(u16*)(pSrc + 0) + *(u16*)(pSrc + 4));
                rect.y = (s16)(*(u16*)(pSrc + 2) + *(u16*)(pSrc + 6));
            }
        } else if (opcode == 0x1101) {
            if (modeB == 1) {
                rect.x = (s16)(ofsX + *(u16*)(pSrc + 4));
                rect.y = (s16)(ofsY + *(u16*)(pSrc + 6));
            } else if (modeB == 2) {
#ifdef TEMP2_DDE4_MUTANT_MODE2_NO_OFFSET
                rect.x = (s16)(*(u16*)(pSrc + 0) + *(u16*)(pSrc + 4));
#else
                rect.x = (s16)(ofsX + *(u16*)(pSrc + 0) + *(u16*)(pSrc + 4));
#endif
                rect.y = (s16)(ofsY + *(u16*)(pSrc + 2) + *(u16*)(pSrc + 6));
            } else {
                rect.x = (s16)(*(u16*)(pSrc + 0) + *(u16*)(pSrc + 4));
                rect.y = (s16)(*(u16*)(pSrc + 2) + *(u16*)(pSrc + 6));
            }
        } else {
            return 0x1101;
        }
        pSrc += 8;
        rect.w = (s16)*(u16*)pSrc;
        pSrc += 2;
        rect.h = (s16)*(u16*)pSrc;
        pSrc += 2;
        LoadImage(&rect, (u_long*)pSrc);
        pSrc += rect.w * rect.h * 2;
        i++;
    } while (i < (s32)count);
    return 0;
}
#endif

extern u8 D_8006FAF0[];

void* func_8002DFE0(void) {
    return D_8006FAF0;
}

extern s32 D_800500FC;
extern s32 D_800500F8;

void func_8002DFF0(s32 a0, s32 a1) {
    D_800500FC = (a1 - 1) << 16;
    D_800500F8 = a0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002E010);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002E448);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002E64C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002E8B4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002EAB8);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002ED20);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002EEF8);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002F0E4);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002F2E0);
#else
/* Retail 8002F2E0..8002F4B0, with the shared tail at 8002E1F4.
 * Lit FT3 variant: one normal per triangle, NCS updates the first RGB word.
 * Keep the pipeline's lookahead and final RTPT, including count == 0; those
 * leave observable GTE state. Rejected triangles can still write XY words.
 * Native OT address translation is confined to the shared link adapter. */
s32 func_8002F2E0(u8* pCmd, s32 count) {
    u8* vertices = (u8*)(uintptr_t)D_8005953C;
    u8* normals = (u8*)(uintptr_t)D_80059498;
    uintptr_t packet = (uintptr_t)D_80059424 - 0x20u;
    u32* ot = (u32*)(uintptr_t)D_80059568;
    u32 emitted = (u32)D_80059578;
    u32 yBound = (u32)D_800500FC;
    u32 xBound = (u32)D_800500F8;
    u32 shift = (u32)D_80050100 & 31u;
    u32 remaining = (u32)count;
    u32 cmd = *(u32*)pCmd;
    u8* nextV0 = vertices + ((cmd & 0xffffu) << 3);
    u8* nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
    u8* nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);

    MTC2(*(u32*)nextV1, 2);
    MTC2(*(u32*)(nextV1 + 4), 3);
    MTC2(*(u32*)nextV2, 4);
    MTC2(*(u32*)(nextV2 + 4), 5);
    for (;;) {
        u32 xy0, xy1, xy2, averageZ;
        u8* out;

        MTC2(*(u32*)nextV0, 0);
        MTC2(*(u32*)(nextV0 + 4), 1);
        gte_rtpt();
        if (remaining == 0) break;
        remaining--;
        pCmd += 8;
        packet += 0x20u;
        cmd = *(u32*)pCmd;
        nextV0 = vertices + ((cmd & 0xffffu) << 3);
        normals += 8;
        nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
        nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);
        MTC2(*(u32*)nextV1, 2);
        MTC2(*(u32*)(nextV1 + 4), 3);
        MTC2(*(u32*)nextV2, 4);
        MTC2(*(u32*)(nextV2 + 4), 5);

        /* MFC2 $31 reads LZCR, not the FLAG control register. */
        if ((s32)MFC2(31) < 0) continue;
        xy0 = MFC2(12);
        xy1 = MFC2(13);
        xy2 = MFC2(14);
        gte_nclip();
        if (!(xy0 < yBound || xy1 < yBound || xy2 < yBound)) continue;
        if (!((xy0 & 0xffffu) < xBound ||
              (xy1 & 0xffffu) < xBound ||
              (xy2 & 0xffffu) < xBound)) continue;

        out = (u8*)packet;
        *(u32*)(out + 8) = xy0;
        *(u32*)(out + 0x10) = xy1; /* delay slot, even for backfaces */
        if ((s32)MFC2(24) <= 0) continue;
        gte_avsz3();
        *(u32*)(out + 0x18) = xy2;
        /* Retail masks the packet pointer to its 24-bit DMA identity here.
         * Host pointers remain host pointers until the link adapter below. */
        averageZ = MFC2(7);
        emitted++;
        MTC2(*(u32*)(normals - 8), 0); /* OTZ-zero branch delay slot */
        if (averageZ == 0) continue;
        MTC2(*(u32*)(normals - 4), 1);
        gte_ncs();
        *(u32*)(out + 4) = (*(u32*)(out + 4) & 0xff000000u) |
                          (MFC2(22) & 0x00ffffffu);
        PcPort_LinkModelPrim(ot, (s32)averageZ >> shift, out, 0x07000000u);
    }
    D_80059498 = (u32)(uintptr_t)normals;
    D_80059578 = (s32)emitted;
    D_80059424 = (u8*)(packet + 0x20u);
    return (s32)yBound;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002F4B4);
#else
/* Retail 8002F4B4..8002F6B4 and shared tail 8002E1F4: lit GT3.
 * Scratchpad holds indexed normal addresses, not a per-face normal cursor.
 * Preserve partial XY writes on rejection and the terminal RTPT. */
s32 func_8002F4B4(u8* pCmd, s32 count) {
    u32 vertices = D_8005953C;
    u32 normalDelta = D_8005952C - vertices;
    uintptr_t packet = (uintptr_t)D_80059424 - 0x28u;
    u32* ot = (u32*)(uintptr_t)D_80059568;
    u32* scratch = (u32*)g_PsxScratchpad; /* retail 0x1F800000 */
    u32 emitted = (u32)D_80059578;
    u32 yBound = (u32)D_800500FC;
    u32 xBound = (u32)D_800500F8;
    u32 shift = (u32)D_80050100 & 31u;
    u32 remaining = (u32)count;
    u32 cmd = *(u32*)pCmd;
    u32 v0 = vertices + ((cmd & 0xffffu) << 3);
    u32 v1 = vertices + ((cmd >> 13) & 0xfff8u);
    u32 v2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);
    for (;;) {
        u32 xy0, xy1, xy2, averageZ;
        u8* out;
        u8* n0;
        u8* n1;
        u8* n2;
        MTC2(*(u32*)(uintptr_t)v0, 0);
        MTC2(*(u32*)(uintptr_t)(v0 + 4), 1);
        MTC2(*(u32*)(uintptr_t)v1, 2);
        MTC2(*(u32*)(uintptr_t)(v1 + 4), 3);
        MTC2(*(u32*)(uintptr_t)v2, 4);
        MTC2(*(u32*)(uintptr_t)(v2 + 4), 5);
        gte_rtpt();
        if (remaining == 0) break;
        scratch[0] = v0 + normalDelta;
        scratch[1] = v1 + normalDelta;
        scratch[2] = v2 + normalDelta;
        remaining--;
        pCmd += 8;
        packet += 0x28u;
        cmd = *(u32*)pCmd;
        v0 = vertices + ((cmd & 0xffffu) << 3);
        v1 = vertices + ((cmd >> 13) & 0xfff8u);
        v2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);
        if ((s32)MFC2(31) < 0) continue;
        xy0 = MFC2(12);
        xy1 = MFC2(13);
        xy2 = MFC2(14);
        gte_nclip();
        if (!(xy0 < yBound || xy1 < yBound || xy2 < yBound)) continue;
        if (!((xy0 & 0xffffu) < xBound || (xy1 & 0xffffu) < xBound ||
              (xy2 & 0xffffu) < xBound)) continue;
        out = (u8*)packet;
        *(u32*)(out + 8) = xy0;
        *(u32*)(out + 0x14) = xy1; /* backface branch delay slot */
        if ((s32)MFC2(24) <= 0) continue;
        gte_avsz3();
        *(u32*)(out + 0x20) = xy2;
        averageZ = MFC2(7);
        if (averageZ == 0) continue;
        emitted++;
        n0 = (u8*)(uintptr_t)scratch[0];
        n1 = (u8*)(uintptr_t)scratch[1];
        n2 = (u8*)(uintptr_t)scratch[2];
        MTC2(*(u32*)n0, 0);
        MTC2(*(u32*)(n0 + 4), 1);
        MTC2(*(u32*)n1, 2);
        MTC2(*(u32*)(n1 + 4), 3);
        MTC2(*(u32*)n2, 4);
        MTC2(*(u32*)(n2 + 4), 5);
        gte_nct();
        *(u32*)(out + 4) = (*(u32*)(out + 4) & 0xff000000u) |
                          (MFC2(20) & 0x00ffffffu);
        *(u32*)(out + 0x10) = MFC2(21);
        *(u32*)(out + 0x1c) = MFC2(22);
        PcPort_LinkModelPrim(ot, (s32)averageZ >> shift, out, 0x09000000u);
    }
    D_80059578 = (s32)emitted;
    D_80059424 = (u8*)(packet + 0x28u);
    return (s32)yBound;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002F6B4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002F8D0);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002FAE8);
#else
/* Retail 8002FAE8..8002FCFC and shared tail 8002E1F4: lit FT4.
 * Preserve lookahead, terminal RTPT, rejection-side GTE operations and
 * per-face normal consumption. Only OT pointer translation is host-specific. */
s32 func_8002FAE8(u8* pCmd, s32 count) {
    u8* vertices = (u8*)(uintptr_t)D_8005953C;
    u8* normals = (u8*)(uintptr_t)D_80059498;
    uintptr_t packet = (uintptr_t)D_80059424 - 0x28u;
    u32* ot = (u32*)(uintptr_t)D_80059568;
    u32 emitted = (u32)D_80059578;
    u32 yBound = (u32)D_800500FC;
    u32 xBound = (u32)D_800500F8;
    u32 shift = (u32)D_80050100 & 31u;
    u32 remaining = (u32)count;
    u32 cmd = *(u32*)pCmd;
    u8* nextV0 = vertices + ((cmd & 0xffffu) << 3);
    u8* nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
    u8* nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);

    MTC2(*(u32*)nextV1, 2);
    MTC2(*(u32*)(nextV1 + 4), 3);
    MTC2(*(u32*)nextV2, 4);
    MTC2(*(u32*)(nextV2 + 4), 5);
    for (;;) {
        u32 xy0, xy1, xy2, xy3, averageZ;
        s32 lzcr;
        u8* fourth;
        u8* out;
        MTC2(*(u32*)nextV0, 0);
        MTC2(*(u32*)(nextV0 + 4), 1);
        gte_rtpt();
        if (remaining == 0) break;
        remaining--;
        pCmd += 8;
        packet += 0x28u;
        cmd = *(u32*)pCmd;
        /* Unlike the initial preload, retail masks lookahead V0 to 13 bits. */
        nextV0 = vertices + ((cmd << 3) & 0xfff8u);
        nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
        nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);
        MTC2(*(u32*)nextV1, 2);
        MTC2(*(u32*)(nextV1 + 4), 3);
        MTC2(*(u32*)nextV2, 4);
        MTC2(*(u32*)(nextV2 + 4), 5);
        normals += 8;
        lzcr = (s32)MFC2(31);
        gte_nclip();
        if (lzcr < 0) continue;
        fourth = vertices + ((u32)*(u16*)(pCmd - 2) << 3);
        xy0 = MFC2(12);
        if ((s32)MFC2(24) <= 0) continue;
        xy1 = MFC2(13);
        xy2 = MFC2(14);
        MTC2(*(u32*)fourth, 0);
        MTC2(*(u32*)(fourth + 4), 1);
        gte_rtps();
        xy3 = MFC2(14);
        lzcr = (s32)MFC2(31);
        gte_avsz4(); /* executes even on the LZCR rejection branch */
        if (lzcr < 0) continue;
        if (!(xy0 < yBound || xy1 < yBound || xy2 < yBound || xy3 < yBound)) continue;
        if (!((xy0 & 0xffffu) < xBound || (xy1 & 0xffffu) < xBound ||
              (xy2 & 0xffffu) < xBound || (xy3 & 0xffffu) < xBound)) continue;
        averageZ = MFC2(7);
        emitted++;
        if (averageZ == 0) continue;
        MTC2(*(u32*)(normals - 8), 0);
        MTC2(*(u32*)(normals - 4), 1);
        gte_ncs();
        out = (u8*)packet;
        *(u32*)(out + 8) = xy0;
        *(u32*)(out + 0x10) = xy1;
        *(u32*)(out + 0x18) = xy2;
        *(u32*)(out + 0x20) = xy3;
        *(u32*)(out + 4) = (*(u32*)(out + 4) & 0xff000000u) |
                          (MFC2(22) & 0x00ffffffu);
        PcPort_LinkModelPrim(ot, (s32)averageZ >> shift, out, 0x09000000u);
    }
    D_80059498 = (u32)(uintptr_t)normals;
    D_80059578 = (s32)emitted;
    D_80059424 = (u8*)(packet + 0x28u);
    return (s32)yBound;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002FCFC);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002FF0C);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003014C);
#else
s32 func_8003014C(s32* pData) {
    s32 cur = pData[2];
    s32 target = pData[1];
    s32 step = pData[3];
    if (cur > target) {
        cur -= step;
        if (cur < target) cur = target;
        pData[2] = cur;
    } else if (cur < target) {
        cur += step;
        if (cur > target) cur = target;
        pData[2] = cur;
    }
    return pData[2];
}
#endif

/* Closest C so far (3 words off, only where the -1 loop constant is
 * scheduled): `volatile u8 unused[1];` for retail's 8-byte frame,
 * `i = count - 1;` before `if (count != 0)`, a separate
 * `s16* pIndex = (s16*)(i * 2 + (uintptr_t)pIndices);`, offsets added as
 * `idx + (uintptr_t)pSrc/pDst`, and `do { ... } while (--i != -1);`. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800301C8);
#else
void func_800301C8(u8* pDst, u8* pSrc, s32 count, s16* pIndices) {
    s32 i;
    if (count == 0) return;
    pIndices += count - 1;
    for (i = count - 1; i >= 0; i--) {
        s32 idx = pIndices[0] * 8;
        u8* s = pSrc + idx;
        u8* d = pDst + idx;
        *(u16*)(d + 0) = *(u16*)(s + 0);
        *(u16*)(d + 2) = *(u16*)(s + 2);
        *(u16*)(d + 4) = *(u16*)(s + 4);
        pIndices--;
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030228);
#else
void func_80030228(u8* pVertices, s16* pDeltas, s32 count, s32 scale) {
    s32 i;
    if (scale == 0 || count == 0) return;
    for (i = 0; i < count; i++) {
        s32 idx = pDeltas[i * 4 + 1] * 8;
        u16* pVert = (u16*)(pVertices + idx);
        s32 d;
        d = (pDeltas[i * 4 + 0] * scale) >> 12;
        pVert[0] += d;
        d = (pDeltas[i * 4 + 3] * scale) >> 12;
        pVert[1] += d;
        d = (pDeltas[i * 4 + 2] * scale) >> 12;
        pVert[2] += d;
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800302D4);
#else
void func_800302D4(u8* pNormals, s16* pDeltas, s32 count, s32 scale) {
    s32 i;
    if (scale == 0 || count == 0) return;
    for (i = count - 1; i >= 0; i--) {
        s32 idx = pDeltas[i * 4 + 1] * 8;
        SVECTOR* pNorm = (SVECTOR*)(pNormals + idx);
        s32 d;
        d = (pDeltas[i * 4 + 0] * scale) >> 12;
        pNorm->vx += d;
        d = (pDeltas[i * 4 + 3] * scale) >> 12;
        pNorm->vy += d;
        d = (pDeltas[i * 4 + 2] * scale) >> 12;
        pNorm->vz += d;
        VectorNormalSS(pNorm, pNorm);
    }
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800303C8);
#else
/* Allocates and populates the "animation/joint" work block for a model that has
 * the 0x2000 (skeletal) status bit. a0=modelData, a1=heap flags (0).
 *
 * Reads a descriptor pointer at modelData+0x1C; if null, returns 0. Otherwise
 * allocs a control block sized (desc[0]<<5)|0x14 and back-links modelData into
 * it (+0x0), copies two source pointers (+0x4,+0x8) and the count word (+0xC).
 * A vertex table of (*(u16*)(modelData+2)) 8-byte entries is allocated at
 * modelData+0x8 and its first three u16 fields are copied from the block's +0x4
 * array; if status bit 0x10 is set the same is done for a normal table at
 * modelData+0xC from the block's +0x8 array. Finally it initializes desc[0]
 * per-item 0x20-byte slots (starting at block+0x14) with the func_8003014C
 * callback and three zeroed words. Returns the control block. */
void* func_800303C8(u8* a0, s32 a1) {
    u8* s0 = a0;
    s32 s4 = a1;
    u8* s2 = (u8*)*(s32*)(s0 + 0x1C);
    u8* s1;
    u8* s3;
    s32 a5;

    if (s2 == NULL) {
        return NULL;
    }

    HeapSetCurrentContentType(0x2B);
    s1 = HeapAlloc((*(s32*)(s2 + 0x0) << 5) | 0x14, s4);
    *(s32*)(s1 + 0x0) = (s32)s0;
    *(s32*)(s1 + 0x4) = *(s32*)(s0 + 0x8);
    s3 = s1 + 0x14;
    *(s32*)(s1 + 0x10) = (s32)s3;
    *(s32*)(s1 + 0x8) = *(s32*)(s0 + 0xC);
    *(s32*)(s1 + 0xC) = *(s32*)(s2 + 0x0);

    HeapSetCurrentContentType(0x2C);
    *(s32*)(s0 + 0x8) = (s32)HeapAlloc(*(u16*)(s0 + 0x2) << 3, s4);
    {
        s32 i = (s32)*(u16*)(s0 + 0x2) - 1;
        for (; i != -1; i--) {
            s32 off = i << 3;
            u8* src = (u8*)*(s32*)(s1 + 0x4) + off;
            u8* dst = (u8*)*(s32*)(s0 + 0x8) + off;
            *(u16*)(dst + 0x0) = *(u16*)(src + 0x0);
            *(u16*)(dst + 0x2) = *(u16*)(src + 0x2);
            *(u16*)(dst + 0x4) = *(u16*)(src + 0x4);
        }
    }

    if (*(u16*)(s0 + 0x0) & 0x10) {
        HeapSetCurrentContentType(0x2D);
        *(s32*)(s0 + 0xC) = (s32)HeapAlloc(*(u16*)(s0 + 0x2) << 3, s4);
        {
            s32 i = (s32)*(u16*)(s0 + 0x2) - 1;
            for (; i != -1; i--) {
                s32 off = i << 3;
                u8* src = (u8*)*(s32*)(s1 + 0x8) + off;
                u8* dst = (u8*)*(s32*)(s0 + 0xC) + off;
                *(u16*)(dst + 0x0) = *(u16*)(src + 0x0);
                *(u16*)(dst + 0x2) = *(u16*)(src + 0x2);
                *(u16*)(dst + 0x4) = *(u16*)(src + 0x4);
            }
        }
    }

    if (*(s32*)(s1 + 0xC) > 0) {
        for (a5 = 0; a5 < *(s32*)(s1 + 0xC); a5++) {
            *(void**)(s3 + 0x0) = (void*)func_8003014C;
            *(s32*)(s3 + 0x4) = 0;
            *(s32*)(s3 + 0x8) = 0;
            *(s32*)(s3 + 0xC) = 0;
            s3 += 0x20;
        }
    }

    return s1;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800305D8);
#else
void func_800305D8(u8* pAnimData) {
    u8* pModel;
    u8* pDeltas;
    u32 count;
    u8* pChannels;
    u8* pJoint;
    u32 i;
    s32 scale;
    u32 hasNormals;

    if (pAnimData == NULL) return;
    pModel = *(u8**)(pAnimData + 0x00);
    pDeltas = *(u8**)(pAnimData + 0x04);
    count = *(u32*)(pAnimData + 0x0C);
    pChannels = *(u8**)(pAnimData + 0x10);
    pJoint = *(u8**)(pModel + 0x1C) + 4;
    hasNormals = *(u16*)(pModel) & 0x10;

    func_800301C8(
        *(u8**)(pModel + 0x08),
        pJoint,
        count,
        *(s16**)((u32)(pJoint + count * 12))
    );

    for (i = 0; i < count; i++) {
        s32 (*cb)(u8*) = *(s32 (**)(u8*))(pChannels);
        scale = cb(pChannels);
        func_80030228(
            *(u8**)(pModel + 0x08),
            *(s16**)(pJoint + 4),
            *(s32*)(pJoint),
            scale
        );
        if (hasNormals) {
            func_800302D4(
                *(u8**)(pModel + 0x0C),
                *(s16**)(pJoint + 8),
                *(s32*)(pJoint),
                scale
            );
        }
        pChannels += 0x20;
        pJoint += 0xC;
    }
}
#endif

// Hands a model's shared anim-work buffers (pAnimInfo->+0x0, a pointer to a
// per-model-type shared work block) back before freeing pAnimInfo itself:
// frees the work block's own +0x8 (and, if its flags bit 0x10 is set, +0xC),
// then restashes pAnimInfo's +0x4/+0x8 into the work block's +0x8/+0xC so the
// next actor sharing that model type picks them up, then frees pAnimInfo.
void func_800306D0(u8* pAnimInfo) {
    u8* pWork;

    if (pAnimInfo == NULL) {
        return;
    }

    pWork = (u8*)(uintptr_t)*(u32*)(pAnimInfo + 0x0);
    HeapFree((void*)(uintptr_t)*(u32*)(pWork + 0x8));

    if (*(u16*)(pWork + 0x0) & 0x10) {
        HeapFree((void*)(uintptr_t)*(u32*)(pWork + 0xC));
    }

    *(u32*)(pWork + 0x8) = *(u32*)(pAnimInfo + 0x4);
    *(u32*)(pWork + 0xC) = *(u32*)(pAnimInfo + 0x8);
    HeapFree(pAnimInfo);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030750);
#endif

extern s16 D_800308D0[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030988);
#else
void func_80030988(u16 u, u16 v, s16 texX, s16 texY) {
    u16 uVal = u << 6;
    u16 vVal = v << 6;
    D_800308D0[0x18] = (D_800308D0[0x18] & 0xF83F) | uVal;
    D_800308D0[0x2E] = (D_800308D0[0x2E] & 0xF83F) | uVal;
    D_800308D0[0x3E] = (D_800308D0[0x3E] & 0xF83F) | uVal;
    D_800308D0[0x1E] = (D_800308D0[0x1E] & 0xF83F) | vVal;
    D_800308D0[0x34] = (D_800308D0[0x34] & 0xF83F) | vVal;
    D_800308D0[0x44] = (D_800308D0[0x44] & 0xF83F) | vVal;
    D_800308D0[0x1A] = texX;
    D_800308D0[0x30] = texX;
    D_800308D0[0x40] = texX;
    D_800308D0[0x20] = texY;
    D_800308D0[0x36] = texY;
    D_800308D0[0x46] = texY;
}
#endif

MATRIX D_80059F64;
MATRIX D_80059F84;

#ifdef XENO_PC_PORT
static s16 func_80030A18(s64 value) {
    value >>= 12;
    if (value > 0x7FFF) {
        return 0x7FFF;
    }
    if (value < -0x8000) {
        return -0x8000;
    }
    return value;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030A30);
#else
void func_80030A30(s32 lightId, void* pLight) {
    VECTOR lightDirection;
    u16 id = lightId;
    s16* pLightMatrix = (s16*)&D_80059F64;
    s16* pColorMatrix = (s16*)&D_80059F84;

    lightDirection.vx = -*(s32*)((u8*)pLight + 0x0);
    lightDirection.vy = -*(s32*)((u8*)pLight + 0x4);
    lightDirection.vz = -*(s32*)((u8*)pLight + 0x8);
    VectorNormalS(&lightDirection, (SVECTOR*)(pLightMatrix + (id * 3)));

    pColorMatrix[id] = *(u16*)((u8*)pLight + 0xC);
    pColorMatrix[id + 3] = *(u16*)((u8*)pLight + 0xE);
    pColorMatrix[id + 6] = *(u16*)((u8*)pLight + 0x10);
    SetColorMatrix(&D_80059F84);
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030B14);
#else
void func_80030B14(MATRIX* pMatrix) {
    MATRIX matrix;
    s32 row;
    s32 col;

    for (row = 0; row < 3; row++) {
        for (col = 0; col < 3; col++) {
            matrix.m[row][col] = func_80030A18(
                (s64)D_80059F64.m[row][0] * pMatrix->m[0][col] +
                (s64)D_80059F64.m[row][1] * pMatrix->m[1][col] +
                (s64)D_80059F64.m[row][2] * pMatrix->m[2][col]);
        }
    }
    SetLightMatrix(&matrix);
}
#endif

void func_80030C40(u16 a, u16 b, u16 c) {
    gte_SetBackColor((u32)a >> 4, (u32)b >> 4, (u32)c >> 4);
}

void func_80030C78(u32 a, u32 b, u32 c) {
    gte_SetBackColor(a, b, c);
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80030C98);
#endif

s32 func_80030EE8(void) {
    u32 sz0;
    u32 sz1;
    u32 sz2;
    u32 sxy;
    /* nop;nop;RTPT -- aspsx assembled inline_c.h gte_rtpt()'s placeholder
     * word 0x000000bf as the real encoding 0x4A280030; emit it directly. */
    __asm__ volatile("nop\n\tnop\n\t.word 0x4a280030" ::: "memory");
    gte_stsz3(&sz0, &sz1, &sz2);
    gte_stsxy0(&sxy);
    /* Check each vertex's screen coords */
    if ((u16)(sz0 + 1) >= 2) {
        if (sxy < (u32)D_800500FC && (u16)sxy < (u32)D_800500F8) return 1;
    }
    gte_stsxy1(&sxy);
    if ((u16)(sz1 + 1) >= 2) {
        if (sxy < (u32)D_800500FC && (u16)sxy < (u32)D_800500F8) return 1;
    }
    gte_stsxy2(&sxy);
    if ((u16)(sz2 + 1) >= 2) {
        if (sxy < (u32)D_800500FC && (u16)sxy < (u32)D_800500F8) return 1;
    }
    return 0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003101C);
#endif

/* GPU primitive link functions: set next pointer and tag with type code */
#define GPU_LINK_FUNC(name, tag) \
    void name(u32* pPrim, u32 nextAddr) { \
        u32 old = *pPrim; \
        *pPrim = nextAddr & 0xFFFFFF; \
        *(u32*)(uintptr_t)nextAddr = old | tag; \
    }

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800315A0);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800315C4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800315E8);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003160C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031630);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031654);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031678);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003169C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800316C0);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800316E4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031708);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003172C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031750);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031774);
#else
GPU_LINK_FUNC(func_800315A0, 0x04000000)
GPU_LINK_FUNC(func_800315C4, 0x07000000)
GPU_LINK_FUNC(func_800315E8, 0x06000000)
GPU_LINK_FUNC(func_8003160C, 0x09000000)
GPU_LINK_FUNC(func_80031630, 0x05000000)
GPU_LINK_FUNC(func_80031654, 0x08000000)
GPU_LINK_FUNC(func_80031678, 0x0B000000)
GPU_LINK_FUNC(func_8003169C, 0x0A000000)
GPU_LINK_FUNC(func_800316C0, 0x0D000000)
GPU_LINK_FUNC(func_800316E4, 0x0C000000)
GPU_LINK_FUNC(func_80031708, 0x0E000000)

GPU_LINK_FUNC(func_8003172C, 0x07000000)
GPU_LINK_FUNC(func_80031750, 0x06000000)
GPU_LINK_FUNC(func_80031774, 0x09000000)
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031798);
#else
void func_80031798(void* ot, void* prim) {
#ifdef XENO_PC_PORT
    *(u32*)prim = (*(u32*)prim & 0x00FFFFFFu) | 0x04000000u;
    PcPort_AddPrimDomainAware(ot, prim);
#else
    u32 primAddr = (u32)(uintptr_t)prim & 0x00FFFFFF;
    u32 old = *(u32*)ot;

    *(u32*)ot = primAddr;
    *(u32*)(uintptr_t)primAddr = old | 0x04000000;
#endif
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800317BC);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800317E0);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031804);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031828);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8003184C);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80031870);
#else
GPU_LINK_FUNC(func_800317BC, 0x03000000)
GPU_LINK_FUNC(func_800317E0, 0x03000000)
GPU_LINK_FUNC(func_80031804, 0x03000000)

GPU_LINK_FUNC(func_80031828, 0x02000000)
GPU_LINK_FUNC(func_8003184C, 0x02000000)
GPU_LINK_FUNC(func_80031870, 0x02000000)
#endif
