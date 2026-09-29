/* Native shop overlay .data, loaded at startup from the user's
 * disc/shop_menu.bin [CF50,D800) (retail_data.h; no bytes committed).
 * Keep aliases in one writable image: retail tables and scratch globals share
 * this address layout. Values correspond to asm/shop_menu/data/CF50.data.s.
 * No guest pointers occur in this region. Unlisted trailing bytes are zero.
 */
unsigned char xeno_shop_data[0x8B0] __attribute__((aligned(8)));

__asm__(
    ".globl D_801D1F50\n.set D_801D1F50, xeno_shop_data + 0x0\n"
    ".globl D_801D1F54\n.set D_801D1F54, xeno_shop_data + 0x4\n"
    ".globl D_801D1F6C\n.set D_801D1F6C, xeno_shop_data + 0x1c\n"
    ".globl D_801D1F70\n.set D_801D1F70, xeno_shop_data + 0x20\n"
    ".globl D_801D1FCC\n.set D_801D1FCC, xeno_shop_data + 0x7c\n"
    ".globl D_801D1FD0\n.set D_801D1FD0, xeno_shop_data + 0x80\n"
    ".globl D_801D1FD4\n.set D_801D1FD4, xeno_shop_data + 0x84\n"
    ".globl D_801D1FD8\n.set D_801D1FD8, xeno_shop_data + 0x88\n"
    ".globl D_801D1FE8\n.set D_801D1FE8, xeno_shop_data + 0x98\n"
    ".globl D_801D1FF8\n.set D_801D1FF8, xeno_shop_data + 0xa8\n"
    ".globl D_801D2008\n.set D_801D2008, xeno_shop_data + 0xb8\n"
    ".globl D_801D2018\n.set D_801D2018, xeno_shop_data + 0xc8\n"
    ".globl D_801D201C\n.set D_801D201C, xeno_shop_data + 0xcc\n"
    ".globl D_801D2094\n.set D_801D2094, xeno_shop_data + 0x144\n"
    ".globl D_801D2114\n.set D_801D2114, xeno_shop_data + 0x1c4\n"
    ".globl D_801D2194\n.set D_801D2194, xeno_shop_data + 0x244\n"
    ".globl D_801D21B0\n.set D_801D21B0, xeno_shop_data + 0x260\n"
    ".globl D_801D21CC\n.set D_801D21CC, xeno_shop_data + 0x27c\n"
    ".globl D_801D21F0\n.set D_801D21F0, xeno_shop_data + 0x2a0\n"
    ".globl D_801D2210\n.set D_801D2210, xeno_shop_data + 0x2c0\n"
    ".globl D_801D2214\n.set D_801D2214, xeno_shop_data + 0x2c4\n"
    ".globl D_801D2218\n.set D_801D2218, xeno_shop_data + 0x2c8\n"
    ".globl D_801D2228\n.set D_801D2228, xeno_shop_data + 0x2d8\n"
    ".globl D_801D2230\n.set D_801D2230, xeno_shop_data + 0x2e0\n"
    ".globl D_801D2240\n.set D_801D2240, xeno_shop_data + 0x2f0\n"
    ".globl D_801D2248\n.set D_801D2248, xeno_shop_data + 0x2f8\n"
    ".globl D_801D224C\n.set D_801D224C, xeno_shop_data + 0x2fc\n"
    ".globl D_801D2250\n.set D_801D2250, xeno_shop_data + 0x300\n"
    ".globl D_801D2254\n.set D_801D2254, xeno_shop_data + 0x304\n"
    ".globl D_801D2258\n.set D_801D2258, xeno_shop_data + 0x308\n"
    ".globl D_801D225C\n.set D_801D225C, xeno_shop_data + 0x30c\n"
    ".globl D_801D2260\n.set D_801D2260, xeno_shop_data + 0x310\n"
);

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(xeno_shop_data) == 0x8B0, "xeno_shop_data size");

XENO_RETAIL_DATA_BEGIN(shop_menu)
    XENO_RD(xeno_shop_data, XENO_RD_SHOP_MENU, XENO_RD_MENU_OFF(0x801D1F50u), 0x8B0),
XENO_RETAIL_DATA_END(shop_menu)
