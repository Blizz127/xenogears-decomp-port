#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39DC;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C48E8;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2CA9;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C94;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C98;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C96;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2CAA;
#endif
extern void func_800941A4(void);

/* func_80085CCC.s: snapshot D_800D39DC, clear D_800C48E8, store a0/a1/a2
 * through the D_800D2Cxx byte/halfword windows, write back the 0x2DC
 * byte minus one (addiu never traps: low byte wraps mod 256), then call
 * func_800941A4 (jal delay is nop: residue args flow through). Retail
 * leaves residue in v0, ignored by every caller. */
#if BATTLE_SUB(1)
void func_80085CCC(u8 a0, u16 a1, u16 a2) {
    u8* p = D_800C3EAC;
    u16 v = D_800D39DC;

    D_800C48E8 = 0;
    D_800D2CA9 = a0;
    {
        u8 b = *(p + 0x2DCu);
        D_800D2C94 = a1;
        D_800D2C98 = a2;
        D_800D2C96 = v;
        D_800D2CAA = (u8)(b + 0xFFu);
    }
    func_800941A4();
}
#endif /* BATTLE_SUB(1) */


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EA4;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCB34;
#endif
extern u32 func_80076A10(u32 a0, u32 a1, u32 a2, u32 a3);

/* func_80085D34.s: zero D_800D2D28[0x7B], then three rounds calling
 * func_80076A10(a0, D_800D2D28[0x7B]*80 + 0x9C8 + (u32)D_800C3EA4,
 * a2, 0xD0) with (a0, a2) = (C3EAC[0x2D4]+0xF, 0x2A),
 * (0x19, 0x32), (C3EAC[0x2D5]+0xF, 0x3A); after each call the return
 * value is added back into D_800D2D28[0x7B] (read-after-call: the
 * old byte is reloaded after the jal, then addu+sb wraps mod 256).
 * Finally D_800D2D28[0xA4] = D_800CCB34. Takes no args (every arg
 * register is set internally); retail v0 residue is ignored. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80085D34(void) {
    u32 r;

    D_800D2D28[0x7B] = 0;
    r = func_80076A10((u32)*(D_800C3EAC + 0x2D4u) + 15u,
                      (u32)D_800D2D28[0x7B] * 80u + 0x9C8u +
                          (u32)D_800C3EA4,
                      0x2Au, 0xD0u);
    D_800D2D28[0x7B] = (u8)(D_800D2D28[0x7B] + r);
    r = func_80076A10(0x19u,
                      (u32)D_800D2D28[0x7B] * 80u + 0x9C8u +
                          (u32)D_800C3EA4,
                      0x32u, 0xD0u);
    D_800D2D28[0x7B] = (u8)(D_800D2D28[0x7B] + r);
    r = func_80076A10((u32)*(D_800C3EAC + 0x2D5u) + 15u,
                      (u32)D_800D2D28[0x7B] * 80u + 0x9C8u +
                          (u32)D_800C3EA4,
                      0x3Au, 0xD0u);
    D_800D2D28[0x7B] = (u8)(D_800D2D28[0x7B] + r);
    D_800D2D28[0xA4] = D_800CCB34;
}
#endif /* XENO_PC_PORT */


/* func_80085E78.s */
#if BATTLE_SUB(2)
void func_80085E78(void) {
    s32 i;

    for (i = 0; i < 7; i++) {
        *(u8*)(i + 0x2CC + (s32)(unsigned int)D_800C3EAC) = 0xFF;
    }
    *(u8*)(0x2D6 + (s32)(unsigned int)D_800C3EAC) = 0;
}
#endif /* BATTLE_SUB(2) */
