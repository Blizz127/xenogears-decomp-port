#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800CCE34[];
#endif




#ifndef XENO_PC_PORT
extern u8 D_800CCE3D[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE3B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE39[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE3C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE3A[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE38[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE3E[];
#endif


/* func_8007BB2C.s */
void func_8007BB2C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16 v = (u16)(p[1] | (p[2] << 8));
    u32 slot = index + 3;

    *(u16*)(D_800CCE3E + slot * 368) = v;
}
/* func_8007BB70.s */
void func_8007BB70(u8** ppBoard, u8 index) {
    u32 slot = index + 3;
    u32 off = slot * 368;

    D_800CCE3D[off] = (*ppBoard)[1];
    D_800CCE3B[off] = (*ppBoard)[2];
    D_800CCE39[off] = (*ppBoard)[3];
}
/* func_8007BBD8.s */
void func_8007BBD8(u8** ppBoard, u8 index) {
    u32 slot = index + 3;
    u32 off = slot * 368;

    D_800CCE3C[off] = (*ppBoard)[1];
    D_800CCE3A[off] = (*ppBoard)[2];
    D_800CCE38[off] = (*ppBoard)[3];
}
/* func_8007BC40.s */
void func_8007BC40(u8** ppBoard, u8* dst, u8 index) {
    u8* p = *ppBoard;
    u32 off = ((u32)index & 0xFF) << 3;

    dst[off + p[1]] = p[2];
    p = *ppBoard;
    dst += off + p[1];
    dst[1] = p[3];
}
