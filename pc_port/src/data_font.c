/*
 * data_font.c - migrated game data for the Xenogears PC port.
 *
 * D_80050240: the LZSS-compressed system font, baked into the retail executable
 * at PSX 0x80050240. FontLoadFont() falls back to it (font.c) and decompresses
 * it (LZSSHeapDecompress) to 0x40A bytes (font header + 4bpp glyph bitmap). The
 * bytes are loaded at startup from the user's disc/SLUS_006.64 (retail_data.h),
 * never committed. The ELF symbol records size 852, but the real compressed
 * stream is 854 bytes; the full stream is loaded. Defining the symbol removes it
 * from the zeroed data-stub set, so the KernelMenu (and all in-game text) gets
 * real glyphs.
 */

unsigned char D_80050240[854];

/*
 * g_SystemPaletteData: the 32-colour system-font CLUT, baked into the retail
 * executable at PSX 0x80050190 (.sdata, asm/slus_006.64/data/3F290.sdata.s).
 * SystemTransferPaletteToVRAM (system.c) LoadImage's this 32x1 palette into VRAM
 * and records it as g_SystemPalette1/2 (GetClut). The field dialog text rows
 * sample it (func_80034888 sets rowPrim clut = g_SystemPalette1). Without the
 * real bytes the port's zeroed data-stub uploaded an all-black palette, so
 * every glyph rendered transparent/black (invisible dialog text). Bytes verbatim
 * from the .sdata dump (64 bytes = 32 RGB555 entries). Defining the symbol
 * removes it from the zeroed data-stub set.
 */
unsigned char g_SystemPaletteData[64];

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(D_80050240) == 0x356, "D_80050240 size");
_Static_assert(sizeof(g_SystemPaletteData) == 0x40, "g_SystemPaletteData size");

XENO_RETAIL_DATA_BEGIN(font)
    XENO_RD(D_80050240, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050240u), 0x356),
    XENO_RD(g_SystemPaletteData, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050190u), 0x40),
XENO_RETAIL_DATA_END(font)
