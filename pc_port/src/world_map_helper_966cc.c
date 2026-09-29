/*
 * World-map helpers 0x800966CC and 0x8009699C — file I/O and CD operations.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_966cc.h"

/* Retail integer-handle ABI, implemented by pc_file_io.c. */
#include "psyq/pc.h"

/* PsyQ CD functions (from PsyCross) */

#define D_8009BE48  0x8009BE48u
#define D_8009CCB0  0x8009CCB0u
#define D_8009CCA8  0x8009CCA8u
#define D_8009CCA0  0x8009CCA0u

static u32 f_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void f_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 f_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

void wm_800966CC(u32 file_table)
{
    s32 retry;
    u32 table = file_table;

    /* Clear status globals */
    f_sw(D_8009BE48, 0);
    f_sw(D_8009CCB0, 0);
    f_sw(D_8009CCA8, 0);
    f_sw(D_8009CCA0, 0);

    /* Iterate over file table entries */
    while (f_lw(table) != 0) {
        u32 filename = f_lw(table);
        u32 offset = f_lw(table + 8);
        u32 buffer = f_lw(table + 4);
        u32 size = f_lw(table + 12);
        u32 fd;

        /* Open file with retries */
        for (retry = 0; retry < 8; retry++) {
            fd = PCopen((char*)PSX_ADDR(filename), 0, 0);
            if ((s32)fd != -1) break;
        }
        if ((s32)fd == -1) {
            table += 0x10;
            continue;
        }

        /* Seek to offset */
        PClseek(fd, (int)offset, 0);

        /* Read data with retries */
        for (retry = 0; retry < 8; retry++) {
            int result = PCread(fd, (char*)PSX_ADDR(buffer), (int)size);
            if (result != 0) break;
        }

        /* Close file with retries */
        for (retry = 0; retry < 8; retry++) {
            int result = PCclose(fd);
            if (result == 0) break;
        }

        table += 0x10;
    }
}

/*
 * 0x8009699C — CD record-list loader (W34C11).
 *
 * Retail starts an asynchronous CdlSetloc/CdlReadS stream and finishes it in
 * the sync callback 0x80096A6C and the ready callback 0x80096C0C; the port
 * performs the same record contract synchronously on PsyCross's CD layer
 * (CdControl(CdlSetloc) seeks the image, CdReadSync reads whole sectors).
 *
 * Record list (a0): 12-byte records {sector, size, buf}, next at +0xC
 * (0x80096D50-60), terminated by sector == 0 (0x80096D9C).
 * Per record (0x800969D8-0x80096A30): sectors = (size + 0x7FF) >> 11.
 * Per sector (0x80096C6C-0x80096CFC): if the remaining size < 0x800, copy
 * size/4 words into buf and the rest into the buffer pointed to by
 * 0x8009D7D4 (0x80096CB0);
 * otherwise copy 0x200 words, buf += 0x800, size -= 0x800.
 * State globals (retail names by PC): 0x8009CD44 state (0x800969BC),
 * 0x8009D3BC list cursor (0x800969CC/D4), 0x8009D7F4 current sector
 * (0x80096A10), 0x8009D614 start sector (0x80096A18), 0x8009D56C sector
 * count (0x80096A20), 0x8009CEB8 remaining size (0x80096A28), 0x8009C590
 * buffer cursor (0x80096A30), 0x8009BE48/CCB0/CCA8/CCA0 cleared
 * (0x800969E4-0x80096A00). Completion (pre-check 0x80096958-0x80096988):
 * D788[BCB8] = 0, BCB8 = (BCB8 + 1) & 0xF, state = 0.
 */
#define WM_CD_STATE      0x8009CD44u
#define WM_CD_LIST       0x8009D3BCu
#define WM_CD_CUR_SECTOR 0x8009D7F4u
#define WM_CD_START      0x8009D614u
#define WM_CD_COUNT      0x8009D56Cu
#define WM_CD_SIZE       0x8009CEB8u
#define WM_CD_BUF        0x8009C590u
#define WM_CD_LOC        0x8009CEBCu
#define WM_CD_SPILL_PTR  0x8009D7D4u   /* 0x80096CB0 lw: pointer to the spill area */
#define WM_CD_D788       0x8009D788u
#define WM_CD_BCB8       0x8009BCB8u

extern int CdControl(u8 com, u8* param, u8* result);
extern int CdRead(int sectors, u32* buf, int mode);
extern int CdReadSync(int mode, u8* result);
extern void* CdIntToPos(int i, void* p);

static u32 s_cd_sector_tmp[0x200];

/* One sector into the record's buffer per the retail partial rule. */
static void wm_cd_copy_sector(u32 size_left)
{
    u32 buf = f_lw(WM_CD_BUF);
    if (size_left < 0x800u) {                     /* 0x80096C78 slti */
        u32 words = size_left >> 2;               /* 0x80096C9C sra 2 (+3 if negative) */
#if defined(WM_9699C_MUTANT_M3)                   /* partial sector copies 0x800 */
        words = 0x200u;
#endif
        u32 spill = f_lw(WM_CD_SPILL_PTR);
        memcpy(PSX_ADDR(buf), s_cd_sector_tmp, words * 4u);
        if (spill != 0u)                          /* 0x80096CA0-C8: CdGetSector(lw D7D4, rest) */
            memcpy(PSX_ADDR(spill), (const u8*)s_cd_sector_tmp + words * 4u,
                   (0x200u - words) * 4u);
    } else {
        memcpy(PSX_ADDR(buf), s_cd_sector_tmp, 0x800u); /* 0x80096CDC */
        f_sw(WM_CD_SIZE, size_left - 0x800u);     /* 0x80096CEC-F8 */
    }
}

void wm_8009699C(u32 list)
{
    u32 rec = list;

    f_sw(WM_CD_STATE, 1u);
    f_sw(D_8009BE48, 0u); f_sw(D_8009CCB0, 0u); f_sw(D_8009CCA8, 0u); f_sw(D_8009CCA0, 0u);

    while (f_lw(rec) != 0u) {                     /* 0x80096D9C beqz sector */
        u32 sector = f_lw(rec);
        u32 size = f_lw(rec + 4u);
        u32 count;
        u8 loc[8];
        u8 result[8];

#if defined(WM_9699C_MUTANT_M4)                   /* missing round-up */
        count = size >> 11;
#else
        count = (size + 0x7FFu) >> 11;            /* 0x80096A04-08 */
#endif
        f_sw(WM_CD_LIST, rec + 0xCu);
        f_sw(WM_CD_CUR_SECTOR, sector); f_sw(WM_CD_START, sector);
        f_sw(WM_CD_COUNT, count); f_sw(WM_CD_SIZE, size); f_sw(WM_CD_BUF, f_lw(rec + 8u));

        CdIntToPos((int)sector, loc);             /* 0x80096A34 */
        memcpy(PSX_ADDR(WM_CD_LOC), loc, 4u);
        (void)CdControl(2u /* CdlSetloc */, loc, result);
        while (count != 0u) {
            (void)CdRead(1, s_cd_sector_tmp, 0x80 /* CdlModeSpeed */);
            (void)CdReadSync(0, result);
            wm_cd_copy_sector(f_lw(WM_CD_SIZE));
            if (f_lw(WM_CD_SIZE) >= 0x800u || count > 1u) {
                f_sw(WM_CD_BUF, f_lw(WM_CD_BUF) + 0x800u);  /* 0x80096D30 */
            }
            f_sw(WM_CD_START, f_lw(WM_CD_START) + 1u);      /* 0x80096D2C */
            f_sw(WM_CD_CUR_SECTOR, f_lw(WM_CD_CUR_SECTOR) + 1u); /* 0x80096E18 */
            count--;                                          /* 0x80096D08 */
            f_sw(WM_CD_COUNT, count);
        }
        rec += 0xCu;
    }

#if !defined(WM_9699C_MUTANT_M5)                  /* completion omitted */
    {
        u32 idx = f_lw(WM_CD_BCB8);                /* 0x8009695C */
        f_sw(WM_CD_D788 + idx * 4u, 0u);           /* 0x8009697C */
        f_sw(WM_CD_BCB8, (idx + 1u) & 0xFu);       /* 0x8009696C-84 */
    }
#endif
    f_sw(WM_CD_STATE, 0u);                        /* 0x80096964 */
}
