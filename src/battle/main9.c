#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u8 D_800C4022[];
extern u8 D_800C3FFE[];
extern u8 D_800D2E5D[];
extern u8 D_800D2E5E[];
extern u8 D_800D2E60[];
extern u8 D_800D2E62[];
extern u8 D_800D32A1[];
extern u16 D_800D2C94[];
extern u8 D_800C402F__rows[][0x48] asm("D_800C402F");
extern u16 D_800C3FFE__rows[][0x24] asm("D_800C3FFE");
extern void func_8007893C(u8 a, u8 b);
extern void func_80085CCC(u8 a0, u16 a1, u16 a2);
extern void func_80085C88(u8 index);
extern u32 func_800877E0(u8 a, u8 b);
extern void func_80087EDC(u8 index, u8 target);
extern void func_800881B8(u8 index, u8 target);
extern u32 func_80089C08(u8 idx);
extern void func_800BC404(u32 v);
extern void func_800785D4(u8 a, u8 b);

/* func_80078998.s, byte-exact: queue entry `idx` (8-byte D_800D2E5D..
 * entries) for actor `actor`: func_8007893C(idx, actor); D_800C3EAC
 * +0x2DC = the entry's D_800D2E5D byte + 1; func_80085CCC(actor, the
 * D_800D2E62 halfword, the D_800D2E5E byte); func_80085C88(row); the
 * D_800C3EAC +0x2DA command row gets D_800C402F = the D_800D2E5E byte and
 * D_800C3FFE = D_800D2C94; func_800785D4(actor, idx).  If the entry's
 * D_800D2E62 halfword differs from D_800D2C94, the first row
 * i < D_800C3EAC +0x2DA (read after func_800785D4) whose D_800C402F byte
 * is 0xFD also gets D_800C3FFE = D_800D2C94.
 * Codegen: the row search goes through asm-label row views indexed by
 * the row number; retail keeps one 0x48 offset with the symbol in each
 * address and the D_800C3EAC pointer hoisted out of the loop.
 * D_800D2C94 is declared as an array (retail keeps its address in a
 * register across the call).  Matches under gcc 2.7.2 and 2.6.x. */
void func_80078998(u8 actor, u8 idx) {
    s32 i;

    func_8007893C(idx, actor);
    D_800C3EAC[0x2DC] = D_800D2E5D[idx << 3] + 1;
    func_80085CCC(actor, *(u16*)(D_800D2E62 + (idx << 3)), D_800D2E5E[idx << 3]);
    func_80085C88(D_800C3EAC[0x2DA]);
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = D_800D2E5E[idx << 3];
    ((u16*)D_800C3FFE)[D_800C3EAC[0x2DA] * 0x24] = D_800D2C94[0];
    func_800785D4(actor, idx);
    if (*(u16*)(D_800D2E62 + (idx << 3)) != D_800D2C94[0]) {
        for (i = 0; i < D_800C3EAC[0x2DA]; i++) {
            if (D_800C402F__rows[i][0] == 0xFD) {
                D_800C3FFE__rows[i][0] = D_800D2C94[0];
                break;
            }
        }
    }
}


/* func_80078B34.s, byte-exact: queue command 0xFD for actor `actor` and
 * entry `idx`.  The D_800C3EAC +0x2DA command row gets D_800C402F = 0xFD,
 * a zero D_800C4022 halfword and, in D_800C3FFE, the D_800D2E62 halfword
 * of entry idx (8-byte entries); then func_800877E0(actor, col).  Unless
 * the entry's D_800D2E60 byte is 1, func_80087EDC(actor, col) runs when
 * the D_800D32A1 byte of the actor's 8-byte row is clear, else
 * func_800881B8(actor, col).  Finally func_800BC404(entry halfword |
 * func_80089C08(actor)), passed as a u16, and func_800785D4(actor, idx).
 * First draft; it matches under gcc 2.7.2 (main9's preset) and 2.6.x. */
void func_80078B34(u8 actor, u8 idx, u8 col) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFD;
    ((u16*)D_800C4022)[D_800C3EAC[0x2DA] * 0x24] = 0;
    ((u16*)D_800C3FFE)[D_800C3EAC[0x2DA] * 0x24] = *(u16*)(D_800D2E62 + (idx << 3));
    func_800877E0(actor, col);
    if (D_800D2E60[idx << 3] != 1) {
        if (D_800D32A1[actor << 3] == 0) {
            func_80087EDC(actor, col);
        } else {
            func_800881B8(actor, col);
        }
    }
    func_800BC404((u16)(*(u16*)(D_800D2E62 + (idx << 3)) | func_80089C08(actor)));
    func_800785D4(actor, idx);
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
extern void func_800785D4(u8 a, u8 b);


/* func_80078C9C.s */
void func_80078C9C(u8 a, u8 b) {
    u32 off = (u32)*(u8*)(D_800C3EAC + 0x2DA) * 72;

    *(u8*)(D_800C402F + off) = 0xFC;
    func_800785D4(a & 0xFF, b & 0xFF);
}
/* func_80078CEC.s */
void func_80078CEC(u8 a, u8 b) {
    u32 off = (u32)*(u8*)(D_800C3EAC + 0x2DA) * 72;

    *(u8*)(D_800C402F + off) = *(u8*)(D_800D2E60 + ((u32)b << 3));
    func_800785D4(a & 0xFF, b & 0xFF);
}
/* func_80078D48.s (the index arrives in a1: retail ignores a0) */
void func_80078D48(u32 unused, u8 index) {
    D_800D39E0 = *(u16*)(D_800D2E62 + ((u32)index << 3));
}
