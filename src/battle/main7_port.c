/* Host port body of func_80077698 (the Attack-ring arrow builder).  The
 * byte-exact C stays in main7.c under #ifndef XENO_PC_PORT; this TU is the
 * only owner an adopted host build links. */
#include "common.h"
#ifdef XENO_PC_PORT
#include <stddef.h>
#include "battle_host_guest.h"

/* bzero is the retail SDK's 0x8003F8E8; the port binds the libc one. */
extern void bzero(void* p, size_t n);
/* PsyCross SetPolyG3: tag length (+3) = 6, code (+7) = 0x30. */
extern void SetPolyG3(void* p);

#define G3_BLOCK() BG_U32(0x800C3E24u) /* D_800C3E24: the 0xEC-byte block */
#define G3_SIZE 0xECu
#define G3_STRIDE 0x1Cu /* sizeof(POLY_G3) */

/* func_80077698.s (main7.c), byte-exact: build four direction arrows (right,
 * down, left, up around (0xA0, 0x70)) in the D_800C3E24 block: clear it, then
 * for arrow i and half j SetPolyG3 triangle [i * 2 + j], give it arrow i's
 * vertices (x0/y0 at +8/+0xA, x1/y1 at +0x10/+0x12, x2/y2 at +0x18/+0x1A) and
 * the 0xFF/0x40/0x40 red ramp (r0 +4, r1 +0xC, r2 +0x14; g/b zero); then word
 * +0xE0 = 0xFF, +0xE5 = 1, +0xE4 = the current buffer (D_800CCB34) and
 * D_800D2D28[0xC6] = 1.  The block pointer is re-read before each group of
 * stores, as retail does. */
void func_80077698(void) {
    s32 i;
    s32 j;
    u32 p;

    bzero(BG_PTR(G3_BLOCK()), G3_SIZE);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            u32 off = (u32)(i * 2 + j) * G3_STRIDE;

            SetPolyG3(BG_PTR(off + G3_BLOCK()));
            p = off + G3_BLOCK();
            switch (i) {
            case 0:
                BG_U16(p + 0x8) = 0xC0;
                BG_U16(p + 0xA) = 0x70;
                BG_U16(p + 0x12) = 0x78;
                BG_U16(p + 0x10) = 0xB0;
                BG_U16(p + 0x18) = 0xB0;
                BG_U16(p + 0x1A) = 0x68;
                break;
            case 1:
                BG_U16(p + 0x8) = 0xA0;
                BG_U16(p + 0x10) = 0x98;
                BG_U16(p + 0xA) = 0x90;
                BG_U16(p + 0x12) = 0x80;
                BG_U16(p + 0x18) = 0xA8;
                BG_U16(p + 0x1A) = 0x80;
                break;
            case 2:
                BG_U16(p + 0xA) = 0x70;
                BG_U16(p + 0x12) = 0x68;
                BG_U16(p + 0x8) = 0x80;
                BG_U16(p + 0x10) = 0x90;
                BG_U16(p + 0x18) = 0x90;
                BG_U16(p + 0x1A) = 0x78;
                break;
            case 3:
                BG_U16(p + 0x8) = 0xA0;
                BG_U16(p + 0xA) = 0x50;
                BG_U16(p + 0x10) = 0x98;
                BG_U16(p + 0x12) = 0x60;
                BG_U16(p + 0x18) = 0xA8;
                BG_U16(p + 0x1A) = 0x60;
                break;
            }
            BG_U8(off + G3_BLOCK() + 0x4) = 0xFF;
            BG_U8(off + G3_BLOCK() + 0x5) = 0;
            BG_U8(off + G3_BLOCK() + 0x6) = 0;
            BG_U8(off + G3_BLOCK() + 0xC) = 0x40;
            BG_U8(off + G3_BLOCK() + 0xD) = 0;
            BG_U8(off + G3_BLOCK() + 0xE) = 0;
            BG_U8(off + G3_BLOCK() + 0x14) = 0x40;
            BG_U8(off + G3_BLOCK() + 0x15) = 0;
            BG_U8(off + G3_BLOCK() + 0x16) = 0;
        }
    }
    BG_U32(G3_BLOCK() + 0xE0) = 0xFF;
    BG_U8(G3_BLOCK() + 0xE5) = 1;
    BG_U8(G3_BLOCK() + 0xE4) = BG_U8(0x800CCB34u);
    BG_U8(BG_U32(0x800D2D28u) + 0xC6) = 1;
}
#endif
