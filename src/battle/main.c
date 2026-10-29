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
/* Per-party-slot pair copied from the game state's slot table. */
typedef struct {
    u16 a;
    u16 b;
} M0Pair;
typedef struct {
    u8 pad0[0x30];
    u16 unk30;
    u8 pad32[0xA2];
    u16 unkD4;
    u8 padD6[0x9A];
} M0Actor; /* 0x170: party actor record at D_800CCD34 */
typedef struct {
    u16 a;
    u16 b;
    u8 pad4[0x1C];
} M0Slot; /* 0x20 */
typedef struct {
    u8 pad0[0x16C0];
    M0Slot slot[1];
} M0Game; /* g_GameState view: struct indexing keeps symbol+reg addressing */

extern u8* D_800C3EA4;
extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern s8 D_8005959C;
extern u8 D_800C3E29;
extern u8 D_800C3E28;
extern s8 D_800D366C;
extern s32 D_800C3E54;
extern s32 D_80062528;
extern u8 D_8005947C;
extern u8 D_80059508;
extern u8 D_80059180;
extern u8 D_800594F8;
extern u8 D_8006F9DC[];
extern u8 D_800658DC[];
extern u8 D_8005954C;
extern s32 D_800C3DEC;
extern u8 D_800C3E4C;
extern u8 D_800C3D48;
extern u8 D_80062648[];
extern s32 D_8005949C;
extern s32 D_800D3364;
extern s32 D_800C3EB0__w asm("D_800C3EB0");
extern u8 D_800D3298;
extern u16 D_800D39DC;
extern u8 D_800CCC58;
extern void* D_800595D0;
extern void* D_80059480;
extern void* D_800594AC;
extern u8 D_800D2FC4;
extern u8 D_800D2D24[];
extern M0Pair D_800C3E0C[];
extern M0Game g_GameState__g asm("g_GameState");
/* Unsized (non-small): cse keeps its address in a register where it is used twice. */
extern u8 D_800C48EA[];
extern u8 D_800C3D5C;
extern u8 D_800594D0;
typedef struct {
    u8 pad0[0x394];
    s16 unk394;
    u8 pad396[0x1E];
    u16 unk3B4;
    u8 pad3B6[0x44A];
    u8 unk800;
} M0Ctx;
/* Struct view for the +0x394/+0x800 stores: in-struct stores let the D_800C3D44
 * load be scheduled above them, as retail. */
extern M0Ctx* D_800D3278__c asm("D_800D3278");
extern u8* D_800D3278;
extern u8 D_800C3D44;
extern M0Actor D_800CCD34[];
extern u8 D_800D2D50;
extern void* D_800D2D3C;
extern void* D_800D2F60;
extern u8 D_8006F9DD;

extern void* func_8008ABB8(s32 size, s32 flags);
extern void bzero(void* p, s32 n);
extern void func_8003A89C(s32 a, s32 b, s32 c);
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern void ArchiveReadFileToBuffer(u32 a0, void* buf, u32 a2, u32 a3);
extern void ArchiveCdDataSync(s32 mode);
extern u_int HeapFree(void* pMem);
extern void func_801E0A34(void);
extern void* memmove(void* dst, void* src, s32 n);
extern void func_800B8098(u32 mode);
extern void func_8007252C(void);
extern void func_800B81BC(s32 arg0);
extern void func_800B39C0(s32 ticks, s32 kind, u8 r, u8 g, u8 b);
extern s32 func_800397FC(u8* p, s32 a, s32 b);
extern void func_80077990(void);
extern void func_800BC404(u32 v);
extern void func_8007819C(void);
extern void func_8003A094(void* p);
extern void func_8003852C(void* p);
extern void func_80070E2C(void);
extern void func_80070EB0(s32 v);
extern void func_8009892C(void);
extern void func_800723E0(void);
extern void func_800C0F70(void);
extern void func_800B8D7C(void);
extern void func_800B853C(s32 mode);
extern void func_80070EDC(void);
extern void func_801E252C(void);

/* Battle entry: allocate the battle work areas, load the battle archive
 * entries, set up the party, run the battle frame loop (func_800716D8) until
 * D_800C48EA or D_800D2FC4 ends it, classify the outcome (result 0..3 and
 * D_800594D0), copy the party's status flags back for result 0/3, then load
 * and run the result screen (func_800B853C).  As in retail, `result` is left
 * unset when D_800C48EA has 0x80 without 0x40, D_800C3D48 is set and
 * D_800C3D5C is clear: it is then the low byte the caller left in s2 (undefined
 * in C). */
void func_80070F40(void) {
    u8* buf;
    u8* buf2;
    s32 i;
    u8 result;
    s32 mode;
    u8 saved;
    u8* p;
    s32 v;

    mode = 1;
    D_800C3EA4 = func_8008ABB8(0xA2B4, 0);
    D_800D2D28 = func_8008ABB8(0x10C, 0);
    D_800C3EAC = func_8008ABB8(0x2F8, 0);
    bzero(D_800C3EA4, 0xA2B4);
    bzero(D_800D2D28, 0x10C);
    bzero(D_800C3EAC, 0x2F8);
    D_8005959C = 0;
    D_800C3E29 = 0xFF;
    D_800C3E28 = 0xFF;
    D_800D366C = 0;
    D_800C3E54 = D_80062528;
    if (D_8005947C != 0) {
        D_80059508 = D_8005947C - 1;
        if (D_80059180 != 0) {
            D_80059180 = 0;
            func_8003A89C(D_80062528, 0x7F, 0x3C);
        }
        if (D_800594F8 == 0) {
            D_8005947C = 0;
        }
    }
    if (D_800594F8 != 0) {
        ArchiveSetIndex(0x10, 2);
        buf = func_8008ABB8(4, 1);
        buf2 = func_8008ABB8((s32)buf - 0x801E0000, 1);
        ArchiveReadFileToBuffer(1, (void*)0x801E0000, 0, 0x80);
        ArchiveCdDataSync(0);
        HeapFree(buf);
        HeapFree(buf2);
        if (D_8005947C == 0) {
            func_801E0A34();
        } else {
            D_80059508 = D_8005947C - 1;
            D_8005947C = 0;
        }
    }
    if (*D_8005917C != -1) {
        ArchiveSetIndex(0x10, 2);
        ArchiveReadFileToBuffer(6, (void*)0x80280000, 0, 0x80);
        ArchiveCdDataSync(0);
    }
    memmove(D_8006F9DC, D_800658DC + D_80059508 * 0x20, 0x20);
    func_800B8098(D_8005954C);
    func_8007252C();
    D_800C3E4C = 2;
    func_800B81BC(D_800C3DEC);
    func_800B39C0(0, 2, 0xFF, 0xFF, 0xFF);
    if (D_800C3D48 == 0) {
        func_800B39C0(0x14, 2, 0, 0, 0);
    }
    if (D_800594F8 != 0) {
        D_800C3E54 = func_800397FC(D_80062648, 0x7F, 0);
    }
    D_800C3EB0__w = D_800D3364 = D_8005949C;
    func_80077990();
    D_800D3298 = 1;
    func_800BC404(D_800D39DC);
    func_800716D8();
    func_8007819C();
    while (D_800CCC58 == 0) {
        func_800716D8();
    }
    HeapFree(D_800595D0);
    if (D_8005954C != 4) {
        func_8003A094(D_800595D0);
    }
    func_8003852C(D_800595D0);
    HeapFree(D_80059480);
    HeapFree(D_800594AC);
    func_80070E2C();
    func_80070EB0(1);
    if (D_800D2FC4 == 0) {
        D_800C3E4C = 1;
    }
    func_8009892C();
    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] != 0x7F) {
            D_800C3E0C[i].a = g_GameState__g.slot[D_800D2D24[i]].a;
            D_800C3E0C[i].b = g_GameState__g.slot[D_800D2D24[i]].b;
        }
    }
    while (D_800C48EA[0] == 0 && D_800D2FC4 == 0) {
        if (D_800CCC58 != 0) {
            func_800723E0();
        }
        func_800716D8();
    }
    if (!(D_800C48EA[0] & 0xC0)) {
        result = 0;
    } else if (D_800C48EA[0] & 0x40) {
        result = 1;
    } else if (D_800C3D48 == 0) {
        result = 2;
    } else if (D_800C3D5C != 0) {
        result = 3;
        D_800594D0 = 3;
        D_800C48EA[0] = 1;
    }
    switch (result) {
    case 0:
        D_800594D0 = 0;
        mode = 0;
    case 3:
        if (D_800C3D48 != 0) {
            D_800D3278__c->unk394 = 0xFF;
            if (D_800C3D44 != 0 || D_800D2FC4 == 0) {
                D_800D3278__c->unk800 = 0;
                p = D_800D3278;
                saved = D_800C48EA[0];
                D_800C48EA[0] = 0;
                for (i = 0; i < 3; i++) {
                    /* Volatile: loop.c then leaves p as the biv and keeps the
                     * +0x3B4 store offset, and the second store does not
                     * re-read the first. */
                    v = D_800CCD34[i].unk30 & 0x8000;
                    *(volatile u16*)(p + 0x3B4) = v;
                    *(volatile u16*)(p + 0x3B4) = v | (D_800CCD34[i].unkD4 & 0x8000);
                    p += 2;
                }
                func_800C0F70();
                func_80070EB0(1);
                D_800C48EA[0] = saved;
            }
        }
        break;
    case 1:
        D_800594D0 = 2;
        mode = 2;
        break;
    case 2:
        D_800594D0 = 1;
        mode = 1;
        break;
    }
    func_800B8D7C();
    if (D_800D2D50 != 0) {
        mode = 1;
    }
    ArchiveSetIndex(0x10, 0);
    D_800D2D3C = func_8008ABB8(4, 1);
    D_800D2F60 = func_8008ABB8((s32)D_800D2D3C - 0x801DE000, 1);
    ArchiveReadFileToBuffer(4, (void*)0x801DE000, 0, 0x80);
    func_800B853C(mode);
    while (D_800CCC58 != 0) {
        func_800716D8();
    }
    if (D_800C3D48 == 0 && !(D_800C48EA[0] & 0x40) && !(D_8006F9DD & 8)) {
        func_800B39C0(0x40, 2, 0x40, 0x40, 0x40);
    }
    func_80070EDC();
    func_801E252C();
    HeapFree(D_800D2D3C);
    HeapFree(D_800D2F60);
}

#ifndef XENO_PC_PORT
s32 func_800716D8(void) {
    if (*D_8005917C != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}
#endif

#endif /* !XENO_PC_PORT; func_8007171C below is also a port host body */

/* func_8007171C's data.  The matching build names the retail symbols (the
 * records through asm-label row views); the port reads the same guest RAM
 * through typed PSX_ADDR views, at the widths func_8007171C.s accesses:
 * lbu D_800D3298, D_800D2DCC, D_800D2DE4 and the D_800CCE44 rows; lhu/sh the
 * D_800CCD64/68/6C/6E rows (0x170-byte records), D_800D2E06 and D_800D2E1C.
 * The generated guest-RAM alias of D_800D2E06 is a byte view and D_800D3298
 * and D_800D2E1C have none, so the body reads its own __v view names; in the
 * matching build each is the retail symbol itself, token for token. */
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
extern u8 D_800D2DE4[];
extern u16 D_800CCD64__rows[][184] asm("D_800CCD64");
extern u16 D_800CCD68__rows[][184] asm("D_800CCD68");
extern u16 D_800CCD6C__rows[][184] asm("D_800CCD6C");
extern u16 D_800CCD6E__rows[][184] asm("D_800CCD6E");
extern u8 D_800CCE44__rows[][368] asm("D_800CCE44");
extern u16 D_800D2E06[];
extern u16 D_800D2E1C[];
#define D_800D3298__v D_800D3298
#define D_800D2DCC__v D_800D2DCC
#define D_800D2DE4__v D_800D2DE4
#define D_800D2E06__v D_800D2E06
#define D_800D2E1C__v D_800D2E1C
#else
#define D_800D3298__v (*(u8*)PSX_ADDR(0x800D3298u))
#define D_800D2DCC__v ((u8*)PSX_ADDR(0x800D2DCCu))
#define D_800D2DE4__v ((u8*)PSX_ADDR(0x800D2DE4u))
#define D_800CCD64__rows ((u16(*)[184])PSX_ADDR(0x800CCD64u))
#define D_800CCD68__rows ((u16(*)[184])PSX_ADDR(0x800CCD68u))
#define D_800CCD6C__rows ((u16(*)[184])PSX_ADDR(0x800CCD6Cu))
#define D_800CCD6E__rows ((u16(*)[184])PSX_ADDR(0x800CCD6Eu))
#define D_800CCE44__rows ((u8(*)[368])PSX_ADDR(0x800CCE44u))
#define D_800D2E06__v ((u16*)PSX_ADDR(0x800D2E06u))
#define D_800D2E1C__v ((u16*)PSX_ADDR(0x800D2E1Cu))
#endif

/* func_8007171C.s, byte-exact: per-frame actor wait countdowns
 * (func_800718BC below reloads an actor's D_800D2E06 count and clears its
 * D_800D2DE4 flag).  While D_800D3298 is set, for each of the 11 actors
 * i that is present (D_800D2DCC) and whose D_800D2DE4 flag is clear:
 * step = 2 if either D_800CCD6C/D_800CCD6E halfword of its 0x170-byte
 * record has bit 15, else 1.  With D_800CCD64 bit 12 the actor only
 * ticks every other frame (D_800D2E1C toggles).  With bit 13 the
 * D_800CCE44 byte counts down by step instead and, at 0, bit 13 is
 * cleared; otherwise, unless bit 7 or the D_800CCD68 bit 12 is set, the
 * D_800D2E06 count goes down by step and at <= 0 is zeroed and the
 * actor's D_800D2DE4 flag set.
 * Codegen: the records go through asm-label row views (retail keeps one
 * 0x170 offset with the symbol in each address); both countdowns test
 * the stored value.  Matches under gcc 2.6.x and 2.7.2. */
void func_8007171C(void) {
    s32 i;
    u8 step;
    u16 h;

    if (D_800D3298__v != 0) {
        for (i = 0; i < 11; i++) {
            if (D_800D2DCC__v[i] != 0 && D_800D2DE4__v[i] == 0) {
                step = 1;
                if ((D_800CCD6C__rows[i][0] | D_800CCD6E__rows[i][0]) & 0x8000) {
                    step = 2;
                }
                if (D_800CCD64__rows[i][0] & 0x1000) {
                    if ((D_800D2E1C__v[i] ^= 1) != 0) {
                        continue;
                    }
                }
                h = D_800CCD64__rows[i][0];
                if (h & 0x2000) {
                    if ((D_800CCE44__rows[i][0] -= step) <= 0) {
                        D_800CCE44__rows[i][0] = 0;
                        D_800CCD64__rows[i][0] &= ~0x2000;
                    }
                } else if (!(h & 0x80)) {
                    if (!(D_800CCD68__rows[i][0] & 0x1000)) {
                        D_800D2E06__v[i] -= step;
                        if ((s16)D_800D2E06__v[i] <= 0) {
                            D_800D2DE4__v[i] = 1;
                            D_800D2E06__v[i] = 0;
                        }
                    }
                }
            }
        }
    }
}

#ifndef XENO_PC_PORT
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
/* func_80071964.s, byte-exact: render the pending battle string
 * (GetStringEntry(D_800D39F0, D_800D2CAF)) into the D_800D39C0 strip,
 * upload it, then run three func_800716D8 passes.  Skipped while
 * D_800D2CAF is clear or the D_800D2C94/D_800C48E8 masks intersect.
 * GetStringEntry takes (table, index) (slus system.c defines
 * void* GetStringEntry(void*, s32)).  The tested D_800D2CAF byte is the
 * index argument, which is why retail loads it into $a1 at entry: CSE
 * reuses the loaded value and the copy preference puts it in a1.  The
 * one-argument declaration above is shared with the port arm, so this
 * body calls it through a two-argument asm-label alias. */
extern u32 D_800D39C0__a[] asm("D_800D39C0");
extern u32 GetStringEntryAt(u32 table, s32 index) asm("GetStringEntry");

void func_80071964(void) {
    if (D_800D2CAF != 0 && (D_800D2C94 & D_800C48E8) == 0) {
        u32* strip = D_800D39C0__a;
        s32 i = 3;

        D_800D39C6 = SystemRenderStringEntry(GetStringEntryAt(D_800D39F0, D_800D2CAF), *strip, 0x39, 1);
        LoadImage((u8*)strip - 8, *strip);
        do {
            func_800716D8();
            i--;
        } while (i != 0);
    }
}
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
