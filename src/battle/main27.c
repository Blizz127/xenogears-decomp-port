#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800CCD64[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EB7[];
#endif


extern u32 func_8007A6C8(u8 a0, u8 a1);
extern u8 func_8001BD40(u8 a0, u8 a1);
/* D_800D3420 / func_80089C08 are also declared below; repeated here so
 * func_8007D344 (transcribed above them) sees them. */
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u32 func_80089C08(u8 idx);

/* func_8007D344.s: clear the D_800D3420 slot ((index & 0xFF) << 6 +
 * p[1]*2, p = *ppBoard), then for s1 in 3..10 call func_8007A6C8(s1,
 * p[2]); when that is nonzero and D_800C3EB7[0x54 + k*0x1C] & 0x80,
 * collect s1 into a stack list. If any collected, store
 * func_80089C08(list[func_8001BD40(0, count-1)]) back to the slot.
 * (The dead `slti` in the collect path is a compiler artifact: its value
 * is discarded and recomputed for the loop test.) */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D344(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 base = ((u32)index & 0xFFu) << 6;
    u8 buf[8];
    u32 s1 = 3;
    u32 s4 = 0;
    u32 s3 = 0x54;
    u32 n = 0;

    *(u16*)(D_800D3420 + base + (u32)p[1] * 2) = 0;
    for (;;) {
        u8* q = *ppBoard;
        u8 s0 = (u8)s1;
        u32 r = func_8007A6C8((u8)s1, q[2]);
        s1++;
        if (((r & 0xFFu) != 0) && ((D_800C3EB7[s3] & 0x80u) != 0)) {
            buf[n++] = s0;
            s4++;
        }
        if (s1 >= 11) {
            break;
        }
        s3 += 0x1C;
    }
    if (s4 != 0) {
        u8 b = buf[func_8001BD40(0, (u8)(s4 - 1))];
        u8* w = *ppBoard;
        *(u16*)(D_800D3420 + base + (u32)w[1] * 2) = (u16)func_80089C08(b);
    }
}
#endif /* XENO_PC_PORT */
/* func_8007D478.s: clear the D_800D3420 slot ((index & 0xFF) << 6 +
 * p[1]*2), then loop calling func_8001BD40(0, 2) for a row r: if flag[r]
 * is clear, read D_800CCD64[r*368]: store func_80089C08(r) to the slot
 * and return when bit15 is set with bits 0x4002 clear, else set flag[r].
 * When all three flags are set, return with the slot still zero.
 * The flags are a three-byte stack array indexed by r, which is why retail
 * reads off the end of the frame for the unreachable r > 2 case
 * (func_8001BD40(0, 2) returns min + x % 3, so r is always 0..2). */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D478(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 base = ((u32)index & 0xFFu) << 6;
    u8 flags[3];

    flags[0] = 0;
    flags[1] = 0;
    flags[2] = 0;
    *(u16*)(D_800D3420 + base + (u32)p[1] * 2) = 0;
    for (;;) {
        u32 r = func_8001BD40(0, 2);
        u8 a0 = (u8)(r & 0xFFu);

        if (flags[a0] == 0) {
            u32 off = (((u32)a0 << 1) + a0) << 3;
            u16 h;
            off = (off - a0) << 4;
            h = *(u16*)(D_800CCD64 + off);
            if ((h & 0x8000u) != 0) {
                if ((h & 0x4002u) == 0) {
                    u8* w = *ppBoard;
                    *(u16*)(D_800D3420 + base + (u32)w[1] * 2) =
                        (u16)func_80089C08(a0);
                    return;
                }
            }
            flags[a0] = 1;
        }
        if ((flags[0] & flags[1] & flags[2]) != 0) {
            return;
        }
    }
}
#endif /* XENO_PC_PORT */

/* func_8007D5B0.s: count i in 0..2 with (D_800CCD64[i * 0x170] & 0xC000) == 0,
 * store the count to D_800D3430[((index & 0xFF) << 6) + p[1]] (p = *ppBoard). */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D5B0(u8** ppBoard, u8 index) {
    s32 count = 0;
    u32 off = 0;
    s32 i;
    u8* p = *ppBoard;

    for (i = 0; i < 3; i++) {
        if ((*(u16*)(D_800CCD64 + off) & 0xC000) == 0)
            count++;
        off += 0x170;
    }
    (D_800D3430 + (((u32)index & 0xFF) << 6))[p[1]] = (u8)count;
}
#endif /* XENO_PC_PORT */
/* func_8007D610.s: count i in 3..10 with D_800D2DCC[i] != 0,
 * (D_800CCD64[0x450 + (i - 3) * 0x170] & 0xC000) == 0 and
 * D_800C3EB7[0x54 + (i - 3) * 0x1C] == 0; store the count to
 * D_800D3430[((index & 0xFF) << 6) + p[1]] (p = *ppBoard).
 * Retail loads p only after the loop and materialises the table base as a
 * pointer value rather than using a %lo store offset. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8007D610(u8** ppBoard, u8 index) {
    s32 count = 0;
    s32 i = 3;
    u32 off = 0x450;
    u32 off2 = 0x54;
    u8* p;
    for (; i < 0xB; i++) {
        if (D_800D2DCC[i] != 0 && (*(u16*)(D_800CCD64 + off) & 0xC000) == 0 && D_800C3EB7[off2] == 0)
            count++;
        off += 0x170;
        off2 += 0x1C;
    }
    p = *ppBoard;
    (D_800D3430 + (((u32)index & 0xFF) << 6))[p[1]] = (u8)count;
}
#endif /* XENO_PC_PORT */

#ifndef XENO_PC_PORT
extern u8 D_800D32A1[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDEC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);


/* func_8007D6A8.s */
void func_8007D6A8(u8** ppBoard, u8 index) {
    u32 best = 0xFFFFFFFF;
    s32 bestIndex = 0;
    s32 i = 0;
    u32 off2 = 0;
    u32 off1 = 0;

    for (; i < 3; i++) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            if (D_800D32A1[off1] != 0) {
                u32 v = *(u32*)(D_800CCDEC + off2);

                if (best >= v) {
                    best = v;
                    bestIndex = i;
                }
            }
        }
        off2 += 0x170;
        off1 += 8;
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007D7B4.s */
void func_8007D7B4(u8** ppBoard, u8 index) {
    u32 best = 0xFFFFFFFF;
    s32 bestIndex = 0;
    s32 i = 3;
    u32 off2 = 0x450;
    u32 off1 = 0x18;

    for (; i < 0xB; i++) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            if (D_800D32A1[off1] != 0) {
                u32 v = *(u16*)(D_800CCD34 + off2);

                if (best >= v) {
                    best = v;
                    bestIndex = i;
                }
            }
        }
        off2 += 0x170;
        off1 += 8;
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
