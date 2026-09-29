/*
 * data_published_logo.c - migrated game data for the Xenogears PC port.
 *
 * g_PublishedByLogoCompressed: the LZSS-compressed "Published by Square
 * Electronic Arts L.L.C." splash image, baked into the retail executable at
 * PSX 0x8004eabc. Decompresses (LZSSHeapDecompress) to 0x1841 bytes: a 16-colour
 * CLUT at +0x14 and a 256x48 4bpp image at +0x40, drawn by GameShowSplashScreen.
 *
 * The bytes are loaded at startup from the user's disc/SLUS_006.64 (file offset
 * 0x800 + 0x3eabc; retail_data.h), never committed. The ELF symbol records size
 * 1540, but the real compressed stream is 2046 bytes (NOTYPE size is imprecise);
 * the full stream is loaded. Defining the symbol removes it from the
 * auto-generated zeroed data-stub set.
 */

unsigned char g_PublishedByLogoCompressed[2046];

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(g_PublishedByLogoCompressed) == 0x7FE, "g_PublishedByLogoCompressed size");

XENO_RETAIL_DATA_BEGIN(published_logo)
    XENO_RD(g_PublishedByLogoCompressed, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004EABCu), 0x7FE),
XENO_RETAIL_DATA_END(published_logo)
