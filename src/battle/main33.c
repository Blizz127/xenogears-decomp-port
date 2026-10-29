#include "common.h"
#include "battle_guest_abi.h"


#ifndef XENO_PC_PORT
void bzero(void *, s32);
s32 func_8008ABB8(s32, s32);
extern u32 D_800D2DB4;
void func_8007FEC4(void);
#else
/* The port's allocator wrapper returns a host pointer (src/battle/main44.c). */
void bzero(void *, s32);
void *func_8008ABB8(s32, s32);
#endif

#ifndef XENO_PC_PORT
s32 ArchiveDecodeAlignedSize(s32);
void ArchiveReadFileToBuffer(s32, s32, s32, s32);
void func_8008AB94();
s32 func_8008ABB8(s32, s32);
void func_8008AC50();
extern s32 D_800C3DE8;
extern u8* D_800D2D28;
void func_8007FE3C(void);
#endif

#ifndef XENO_PC_PORT

#ifndef XENO_PC_PORT
void func_8007FE3C(void) {
    s32 temp_v0;

    if (*(u8 *)((s8*)((*(void **)&D_800D2D28)) + 0x96) == 0) {
        func_8008AC50();
        func_8008AB94();
        temp_v0 = func_8008ABB8(ArchiveDecodeAlignedSize(3), 0);
        D_800C3DE8 = temp_v0;
        ArchiveReadFileToBuffer(3, temp_v0, 0, 0x80);
        func_8008AC50();
        *(u8 *)((s8*)((*(void **)&D_800D2D28)) + 0x96) = 1U;
    }
}
#endif


#endif /* !XENO_PC_PORT */
/* Native under the port (battle guest ABI): the slot at 0x800D2DB4 receives
 * the GUEST address of the block, as retail stores it, and the two halfwords
 * are written back through that guest word. */
void func_8007FEC4(void) {
    void *temp_v0;

    temp_v0 = func_8008ABB8(0x5DA4, 0);
    GSTORE(D_800D2DB4, temp_v0);
    bzero(temp_v0, 0x5DA4);
    *(s16 *)((s8*)(GGET(D_800D2DB4)) + 0x5D9C) = 0xA0;
    *(s16 *)((s8*)(GGET(D_800D2DB4)) + 0x5D9E) = 0x64;
}
#ifndef XENO_PC_PORT

extern u8* D_800C3EAC;
extern u8 D_800D2C0C[];
extern u8 D_800D2D24[];
extern void func_8009A2D4(u8 idx);
extern void func_80089AF8(u8 idx);
extern void func_800898F0(u8 index);
extern s32 func_800716D8(void);

/* func_8007FF14.s, byte-exact: func_8009A2D4(actor), then write the low
 * byte of each of the three D_800D2C0C halfwords (slots k = 2..4, the
 * byte at (k - 2) * 2) as up to three digit glyphs (0x83 + digit) into
 * bytes k * 4 + 8 .. of the actor's 64-byte D_800C3EAC row: the slot is
 * first filled with 0xFF and leading zeros are skipped (the last digit is
 * always written).  Then func_8007FEC4(), and unless D_800D2D24[actor] is
 * 7, func_80089AF8(actor) and func_800898F0(actor), each followed by
 * func_800716D8(), and D_800D2D28 +0xAD and +0xC7 = 1.  Matches under gcc
 * 2.6.x (first draft). */
void func_8007FF14(u8 actor) {
    s32 k;
    s32 j;
    u8 v;
    u8 d;

    func_8009A2D4(actor);
    for (k = 2; k < 5; k++) {
        v = D_800D2C0C[(k - 2) * 2];
        (D_800C3EAC + (actor << 6))[k * 4 + 8] = 0xFF;
        (D_800C3EAC + (actor << 6))[k * 4 + 9] = 0xFF;
        (D_800C3EAC + (actor << 6))[k * 4 + 10] = 0xFF;
        j = 0;
        d = v / 100;
        if (d != 0) {
            v -= d * 100;
            j = 1;
            (D_800C3EAC + (actor << 6))[k * 4 + 8] = d + 0x83;
        }
        d = v / 10;
        if (d != 0 || j != 0) {
            v -= d * 10;
            (D_800C3EAC + (actor << 6))[k * 4 + j + 8] = d + 0x83;
            j++;
        }
        (D_800C3EAC + (actor << 6))[k * 4 + j + 8] = v + 0x83;
    }
    func_8007FEC4();
    if (D_800D2D24[actor] != 7) {
        func_80089AF8(actor);
        func_800716D8();
        func_800898F0(actor);
        func_800716D8();
        D_800D2D28[0xAD] = 1;
        D_800D2D28[0xC7] = 1;
    }
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32A1[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2DB4;
#endif
#ifndef XENO_PC_PORT
extern void HeapFree(u32 p);
#else
extern void HeapFree(void* p); /* the port allocator takes the host block */
#endif


/* func_800800E8.s */
void func_800800E8(u8 index) {
    D_800D2D28[0xAD] = 0;
    D_800D2D28[0xC7] = 0;
    D_800D2D28[0xA8] = 0;
    if (D_800D32A1[((u32)index & 0xFF) << 3] == 2) {
        D_800D32A1[((u32)index & 0xFF) << 3] = 1;
    }
    HeapFree(D_800D2DB4);
}
