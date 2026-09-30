#ifndef _XENO_GRAPHICS_H
#define _XENO_GRAPHICS_H

typedef struct {
    /* 0x0  */ u_short x;
    /* 0x2  */ u_short y;
    /* 0x4  */ u_short modulus;
    /* 0x6  */ u_short width;
    /* 0x8  */ u_short height;
    /* 0xA  */ u_short numLines;
    /* 0xC  */ u_short destX;
    /* 0xE  */ u_short destY;
#ifdef XENO_PC_PORT
    /* Guest overlay: both slots are 32-bit (retail lw at +0x10/+0x14).
     * Host pointers must not live here — LP64 would shift unk14 to +0x18
     * and GfxLineScrollFree then HeapFree's the next word (0x800e97dc in
     * the intro-battle stop RAM). */
    /* 0x10 */ u32 pData;
    /* 0x14 */ u32 unk14;
#else
    /* 0x10 */ s8* pData;
    /* 0x14 */ u_short* unk14;
#endif
} LineScroll;

#endif