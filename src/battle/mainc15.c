#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif

#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D3410[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_80085454(s32);
extern void func_80085618(s32);
extern void func_8008AA40(u8 v);
#ifndef XENO_PC_PORT
extern u32 D_800C3EA4;
#endif
extern void* func_8008ABB8(s32 size, s32 flag);
extern void* bzero(unsigned char* p, int size);
extern void func_80077074(void);


/* func_8007A8B4.s */
void func_8007A8B4(u8** ppBoard, u8* pDst) {
    s32 i;

    for (i = 0; i < 8; i++) {
        u8* p = *ppBoard;

        pDst[(p[2] << 3) + i] = pDst[(p[1] << 3) + i];
    }
}
/* func_8007A900.s */
void func_8007A900(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[1]] = p[2];
}
