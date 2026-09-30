/* battle mainasm_133C: the retail-asm run at file offset 0x133C, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif

#ifndef XENO_PC_PORT
void ArchiveReadFileToBuffer(s32, s32, s32, s32);
void func_8008AB4C();
s32 func_8008ABB8(s32, s32);
void func_8008AC50();
void func_801E5160();
extern u8 D_800C3D48;
extern s32 D_800D3284;
extern s32 D_800D328C;
void func_801E879C(s32);
void func_8003A89C(s32, s32, s32);
s32 func_801E563C();
extern s32 D_800C3E54;
extern u8 D_800C48EA;
void func_80070E2C(void);
void func_80070EB0(s32 arg0);
void func_80070EDC(void);
#endif

#ifndef XENO_PC_PORT

#ifndef XENO_PC_PORT
void func_80070E2C(void) {
    s32 temp_v0;

    if (D_800C3D48 != 0) {
        func_8008AB4C();
        temp_v0 = func_8008ABB8(4, 1);
        D_800D3284 = temp_v0;
        D_800D328C = func_8008ABB8(temp_v0 + 0x7FE1B000, 1);
        ArchiveReadFileToBuffer(1, 0x801E5000, 0, 0x80);
        func_8008AC50();
        func_801E5160();
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80070EB0(s32 arg0) {
    if (D_800C3D48 != 0) {
        func_801E879C(arg0 & 0xFF);
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80070EDC(void) {
    s32 var_v1;

    var_v1 = 0;
    if (D_800C3D48 != 0) {
        var_v1 = func_801E563C();
    }
    if (!(var_v1 & 0xFF) && (D_800C48EA == 0x81)) {
        func_8003A89C(D_800C3E54, 0, 0xF0);
    }
}
#endif

#endif
