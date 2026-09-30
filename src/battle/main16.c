#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3410[];
#endif
/* func_8007A92C.s: store ((p[3] << 8) | p[2]) halfword to D_800D3420[index] row slot p[1] */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007A92C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16 v = (u16)((p[3] << 8) | p[2]);

    ((u16*)(D_800D3420 + ((u32)index << 6)))[p[1]] = v;
}
#endif /* XENO_PC_PORT */
#endif /* XENO_PC_PORT */
/* func_8007A968.s: store (((p[3] << 8) | p[2]) << 4) word to D_800D3410[index] row slot p[1] */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007A968(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 v = (u32)(((p[3] << 8) | p[2]) << 4);

    ((u32*)(D_800D3410 + ((u32)index << 6)))[p[1]] = v;
}
#endif /* XENO_PC_PORT */
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
extern u32 D_800D2D40;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D48;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3BCC[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


/* func_8007A9A8.s */
void func_8007A9A8(u8** ppBoard) {
    u8* p = *ppBoard;

    D_8005A3A0[p[1]] = p[2];
}
