#include "common.h"


#ifndef XENO_PC_PORT
void func_800BE790();
void func_8028022C();
extern s32 *D_8005917C;
s32 func_800716D8(void);
#endif

#ifndef XENO_PC_PORT
#else
/* The port builds this TU with -DXENO_PC_PORT -DSKIP_ASM (build_port.sh
 * excludes src/battle/main.c from the shipped executable and the animation
 * leaf test compiles it directly), so every C body here is port-side only.
 * File-scope __asm__ blocks are emitted before compiled bodies, so leaving a
 * C body in the matching build moves it out of its retail slot and shifts
 * every downstream address. */
__attribute__((weak))
void func_8009795C(void) {
}
#endif
#ifndef XENO_PC_PORT
#else
__attribute__((weak))
__attribute__((weak))
void func_8009F5B0(void) {
}
#endif
#ifndef XENO_PC_PORT
#else
/* Retail 800B168C..800B16A4: record address, modulo 32-bit arithmetic. */
__attribute__((weak))
u32 func_800B168C(u32 base, u32 index) {
    return base + index * 28u + 12u;
}
#endif
#ifndef XENO_PC_PORT
#else
/* Retail 800B16A4..800B16F0: byte 0 sizes the output, byte 1 advances
 * the input record. They are independent, unsigned lengths in words.
 * Retail walks with a rising index and compares it against the count
 * (`addiu a1,a1,1` / `bne a1,a3`), which is what the loop shape below
 * has to reproduce.  Not yet byte-exact: retail initialises the accumulator
 * with `addu a2,a1,zero`, while the pinned compiler folds the same source
 * shape to `move a2,zero` (both gcc-2.7.2-psx and gcc-2.6.0-psx). */
__attribute__((weak))
u32 func_800B16A4(const u8* data) {
    u32 i = 0;
    u32 total = 0;
    const u8* record = data + *(const u32*)(data + 0x10);
    u32 count = *(const u32*)(data + 0x14);

    while (i != count) {
        total += ((u32)record[0] + 1u) * 4u;
        record += ((u32)record[1] + 1u) * 4u;
        i++;
    }
    return total;
}
#endif
#ifndef XENO_PC_PORT
#else
__attribute__((weak))
void func_800B3348(void) {
}

__attribute__((weak))
void func_800B3350(void) {
}
#endif
#ifndef XENO_PC_PORT
#else
__attribute__((weak))
__attribute__((weak))
void func_800B89F4(void) {
}
#endif
#ifndef XENO_PC_PORT
#else
__attribute__((weak))
__attribute__((weak))
void func_800BAF40(void) {
}
#endif
#ifndef XENO_PC_PORT
#else
__attribute__((weak))
__attribute__((weak))
void func_800BDD34(void) {
}
#endif


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main", func_80070F40);

#ifndef XENO_PC_PORT
s32 func_800716D8(void) {
    if (*D_8005917C != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main", func_8007171C);
extern u8* D_800C3EAC;
extern u8 D_800D2DE4[];
extern u16 D_800D2E06[];
extern u16 func_80098AF8(u8 idx, u32 a1);

/* clear the current actor's D_800D2DE4 flag unless it is 0xFF, refresh its
 * D_800D2E06 halfword from func_80098AF8 and mirror it 11 halfwords back. */
void func_800718BC(void) {
    u8 idx = D_800C3EAC[0x2D3];

    if (D_800D2DE4[idx] != 0xFF) {
        D_800D2DE4[idx] = 0;
    }
    D_800D2E06[D_800C3EAC[0x2D3]] = func_80098AF8(D_800C3EAC[0x2D3], 0);
    (D_800D2E06 - 11)[D_800C3EAC[0x2D3]] = D_800D2E06[D_800C3EAC[0x2D3]];
}

#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2CAF;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C94;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C48E8;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D39F0;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D39C0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D39C6;
#endif
extern u32 GetStringEntry(u32 id);
extern u8 SystemRenderStringEntry(u32 entry, u32 buf, u32 a2, u32 a3);
extern void LoadImage(void* pRect, u32 buf);
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_80071964.s: render the pending battle string into the D_800D39C0 strip
 * and upload it, then run three func_800716D8 passes.  Skipped while
 * D_800D2CAF is clear or the D_800D2C94/D_800C48E8 masks intersect.  Retail
 * materialises the strip address once and reads the buffer through it. */
void func_80071964(void) {
    if (D_800D2CAF != 0 && (D_800D2C94 & D_800C48E8) == 0) {
        u32* strip = &D_800D39C0;
        s32 i = 3;

        D_800D39C6 = SystemRenderStringEntry(GetStringEntry(D_800D39F0),
                                             *strip, 0x39, 1);
        LoadImage((u8*)strip - 8, *strip);
        do {
            func_800716D8();
            i--;
        } while (i != 0);
    }
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main", func_80071964);
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u8 g_GameState[];
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
extern s32 func_800716D8(void); /* retail returns 0 */
#ifndef XENO_PC_PORT
extern u8 D_800D3725[];
#endif


/* func_80071A08.s */
void func_80071A08(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        *(u8*)(i + 0x2EB + (s32)(unsigned int)D_800C3EAC) = 0;
    }
}
/* func_80071A38.s */
void func_80071A38(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        (D_800D2D28 + i)[0x7C] = (D_800C3EAC + i)[0x2EB];
    }
    func_800716D8();
}
/* func_80071A8C.s */
void func_80071A8C(void) {
    s32 i;

    for (i = 0x2A0; i >= 0; i -= 0x60) {
        D_800D3725[i] = 0;
    }
    D_800D2D28[0xB5] = 0;
    D_800D2D28[0xB4] = 0;
    func_800716D8();
}
