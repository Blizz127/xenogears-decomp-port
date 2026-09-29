/*
 * data_kernel_menu.c - migrated game data for the Xenogears PC port.
 *
 * The KernelMenu's on-screen strings live in the retail executable's .rodata as
 * data symbols (PSX 0x800182xx). In the port those are zeroed data stubs, so the
 * menu drew its cursor but no text. The strings are loaded at startup from the
 * user's disc/SLUS_006.64 (retail_data.h); defining the symbols removes them
 * from the zeroed data-stub set so FontPrintf has real text to render.
 *
 * (This is the per-symbol form of data migration; the scalable approach is to
 * bulk-load the whole .main section into g_PsxRam and alias the data symbols.)
 */

char D_800182B0[15];
char D_800182C0[38];
char D_800182E8[7];
char D_800182F0[7];
char D_800182F8[68];

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(D_800182B0) == 0xF, "D_800182B0 size");
_Static_assert(sizeof(D_800182C0) == 0x26, "D_800182C0 size");
_Static_assert(sizeof(D_800182E8) == 0x7, "D_800182E8 size");
_Static_assert(sizeof(D_800182F0) == 0x7, "D_800182F0 size");
_Static_assert(sizeof(D_800182F8) == 0x44, "D_800182F8 size");

XENO_RETAIL_DATA_BEGIN(kernel_menu)
    XENO_RD(D_800182B0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800182B0u), 0xF),
    XENO_RD(D_800182C0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800182C0u), 0x26),
    XENO_RD(D_800182E8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800182E8u), 0x7),
    XENO_RD(D_800182F0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800182F0u), 0x7),
    XENO_RD(D_800182F8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800182F8u), 0x44),
XENO_RETAIL_DATA_END(kernel_menu)
