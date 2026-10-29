#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT
extern POLY_G3* D_800C3E24__p asm("D_800C3E24");
extern u8 D_800CCB34__b asm("D_800CCB34");
extern u8* D_800D2D28;
void bzero(void*, s32);

/* func_80077698.s, byte-exact under gcc 2.6.3 (BattleGcc263; Default 2.7.2
 * differs by the SetPolyG3 argument's operand order).  Build four direction
 * arrows (right, down, left, up around (0xA0, 0x70)) in the 0xEC-byte
 * D_800C3E24 block: clear it, then for arrow i and half j SetPolyG3
 * triangle [i * 2 + j], give it arrow i's vertices and the
 * 0xFF/0x40/0x40 red ramp; then word +0xE0 = 0xFF, +0xE5 = 1, +0xE4 = the
 * current buffer (D_800CCB34) and D_800D2D28[0xC6] = 1.  Codegen: every case
 * indexes [i * 2 + j], which gives cases 1-3 their own induction variables
 * and recomputes case 0 and the shared offset as retail does; the POLY_G3*
 * view gives retail's offset-first adds; the +0xE0 word store goes through a
 * cast, which keeps it ahead of the global reloads. */
void func_80077698(void) {
    s32 i;
    s32 j;

    bzero(D_800C3E24__p, 0xEC);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            SetPolyG3(&D_800C3E24__p[i * 2 + j]);
            switch (i) {
            case 0:
                D_800C3E24__p[i * 2 + j].x0 = 0xC0;
                D_800C3E24__p[i * 2 + j].y0 = 0x70;
                D_800C3E24__p[i * 2 + j].x1 = 0xB0;
                D_800C3E24__p[i * 2 + j].y1 = 0x78;
                D_800C3E24__p[i * 2 + j].x2 = 0xB0;
                D_800C3E24__p[i * 2 + j].y2 = 0x68;
                break;
            case 1:
                D_800C3E24__p[i * 2 + j].x0 = 0xA0;
                D_800C3E24__p[i * 2 + j].y0 = 0x90;
                D_800C3E24__p[i * 2 + j].x1 = 0x98;
                D_800C3E24__p[i * 2 + j].y1 = 0x80;
                D_800C3E24__p[i * 2 + j].x2 = 0xA8;
                D_800C3E24__p[i * 2 + j].y2 = 0x80;
                break;
            case 2:
                D_800C3E24__p[i * 2 + j].x0 = 0x80;
                D_800C3E24__p[i * 2 + j].y0 = 0x70;
                D_800C3E24__p[i * 2 + j].x1 = 0x90;
                D_800C3E24__p[i * 2 + j].y1 = 0x68;
                D_800C3E24__p[i * 2 + j].x2 = 0x90;
                D_800C3E24__p[i * 2 + j].y2 = 0x78;
                break;
            case 3:
                D_800C3E24__p[i * 2 + j].x0 = 0xA0;
                D_800C3E24__p[i * 2 + j].y0 = 0x50;
                D_800C3E24__p[i * 2 + j].x1 = 0x98;
                D_800C3E24__p[i * 2 + j].y1 = 0x60;
                D_800C3E24__p[i * 2 + j].x2 = 0xA8;
                D_800C3E24__p[i * 2 + j].y2 = 0x60;
                break;
            }
            D_800C3E24__p[i * 2 + j].r0 = 0xFF;
            D_800C3E24__p[i * 2 + j].g0 = 0;
            D_800C3E24__p[i * 2 + j].b0 = 0;
            D_800C3E24__p[i * 2 + j].r1 = 0x40;
            D_800C3E24__p[i * 2 + j].g1 = 0;
            D_800C3E24__p[i * 2 + j].b1 = 0;
            D_800C3E24__p[i * 2 + j].r2 = 0x40;
            D_800C3E24__p[i * 2 + j].g2 = 0;
            D_800C3E24__p[i * 2 + j].b2 = 0;
        }
    }
    *(s32*)((u8*)D_800C3E24__p + 0xE0) = 0xFF;
    ((u8*)D_800C3E24__p)[0xE5] = 1;
    ((u8*)D_800C3E24__p)[0xE4] = D_800CCB34__b;
    D_800D2D28[0xC6] = 1;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_80077980.s */
void func_80077980(void) {
    D_800D2D28[0xC6] = 0;
}
