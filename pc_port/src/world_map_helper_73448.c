/*
 * World-map helper 0x80073448 (fresh-entry placement).
 * W34C9 transcription from disc/world_map.bin; retail PCs cited inline.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73448.h"
#include "world_map_helper_8dff4.h"

#define WM_FLAGS_EE68      0x8006EE68u   /* 0x8007344C-58 lhu; 0x80073478 sh */
#define WM_VALUE_EE66      0x8006EE66u   /* 0x80073480 lhu */
#define WM_STATE_C584      0x8009C584u   /* 0x80073488 sw */
#define WM_TABLE_D3F4      0x8009D3F4u   /* 0x800734CC lw */
#define WM_POS_X           0x8009C5ACu   /* 0x800734A8 / 0x8007350C */
#define WM_POS_Y           0x8009C5B0u   /* 0x8007349C / 0x80073514 */
#define WM_POS_Z           0x8009C5B4u   /* 0x800734BC / 0x8007351C */

static u16 p48_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 p48_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u32 p48_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void p48_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void p48_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

#if defined(WM_73448_MUTANT_M1)
#define WM_ROW_STRIDE 6u                 /* wrong row stride */
#else
#define WM_ROW_STRIDE 8u                 /* 0x800734F4 addiu a2,a2,8 */
#endif

void wm_80073448(s32 world_index)
{
    u16 flags = p48_lhu(WM_FLAGS_EE68);

    if ((flags & 0x2000u) != 0u) {        /* 0x80073460-64 */
#if !defined(WM_73448_MUTANT_M5)
        p48_sh(WM_FLAGS_EE68, (u16)(flags & 0xDFFFu)); /* 0x80073468, stored in the jal delay slot 0x80073478 */
#endif
        wm_8008DFF4(WM_POS_X);            /* 0x80073474 */
        p48_sw(WM_STATE_C584, p48_lhu(WM_VALUE_EE66)); /* 0x80073480-88 */
        return;
    }

    {
        u32 row = p48_lw(WM_TABLE_D3F4);  /* 0x800734CC */
        for (;;) {
            s32 id = (s32)p48_lh(row + 2u);   /* 0x800734D4 lh */
            if (id == -1)                    /* 0x800734DC */
                break;
#if defined(WM_73448_MUTANT_M4)          /* unsigned id compare */
            if ((s32)p48_lhu(row + 2u) == world_index) {
#else
            if (id == world_index) {         /* 0x800734EC (sign-extended lhu) */
#endif
                p48_sw(WM_POS_Y, 0u);        /* 0x8007349C */
#if defined(WM_73448_MUTANT_M2)          /* missing << 12 */
                p48_sw(WM_POS_X, (u32)(s32)p48_lh(row));
                p48_sw(WM_POS_Z, (u32)(s32)p48_lh(row + 4u));
#elif defined(WM_73448_MUTANT_M3)        /* x/z swapped */
                p48_sw(WM_POS_X, (u32)(s32)p48_lh(row + 4u) << 12);
                p48_sw(WM_POS_Z, (u32)(s32)p48_lh(row) << 12);
#else
                p48_sw(WM_POS_X, (u32)(s32)p48_lh(row) << 12);       /* 0x80073494-A8 sll 12 */
                p48_sw(WM_POS_Z, (u32)(s32)p48_lh(row + 4u) << 12);  /* 0x800734AC-BC */
#endif
                return;
            }
            row += WM_ROW_STRIDE;
        }
        /* 0x80073508-1C: no row for this index -> origin. */
        p48_sw(WM_POS_X, 0u);
        p48_sw(WM_POS_Y, 0u);
        p48_sw(WM_POS_Z, 0u);
    }
}
