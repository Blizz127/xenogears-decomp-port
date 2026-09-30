#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
extern u8 g_GameState[];

/* 1 when entry idx's +0x5B47 byte (20-byte stride off D_800C34B0) is one of
 * the three Gear slot ids (+0x98D/+0x995/+0x99D) of the current Gear party
 * record (g_GameState[0x59C] selects the 0xA4-stride record). */
s32 func_8009E53C(u8 idx) {
    u8 id = D_800C34B0[idx * 20 + 0x5B47];
    s32 o = g_GameState[0x59C] * 0xA4;

    if (g_GameState[0x98D + o] == id || g_GameState[0x995 + o] == id) {
        return 1;
    }
    return g_GameState[0x99D + o] == id;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009E5C8);
INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009E788);
INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009E868);
typedef struct {
    u8** items;
    u32 count;
} ItemList;

extern void HeapChangeCurrentUser(u32 user, u32 a1);
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 func_8002C3E8(u8* data);

/* build a pointer list over the 0x38-byte records at data + 0x10. */
ItemList* func_8009EBA8(u8* data, ItemList* list) {
    u32 n;
    u32 i;

    HeapChangeCurrentUser(4, 0);
    n = func_8002C3E8(data);
    list->items = HeapAlloc(n * 4, 0);
    list->count = n;
    if (list->items != NULL) {
        for (i = 0; i < n; i++) {
            list->items[i] = data + 0x10 + i * 0x38;
        }
    }
    return list;
}

INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009EC4C);
INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009EF3C);
INCLUDE_ASM("asm/battle/nonmatchings/main64", func_8009F1C4);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_8009F5B0.s */
void func_8009F5B0(void) {
}


/* ---- Port bodies (fo/bat3). ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D6C;
#endif
#ifdef XENO_PC_PORT
/* func_8009E5C8.s: the Gear counterpart of func_8009A854: copy entry a0's
 * bytes (+0xE/+0x10/+0x11, 20-byte stride at D_800C34B0+0x5750) into the
 * Gear slot whose id matches its +0xF byte (slot 0, 1 or 3; the last match
 * wins, none keeps a1), record a0, mirror +0xC into g_GameState+0x2286[a0],
 * then patch every type-4 actor among the first three.  Retail stores +0x11
 * twice. */
void func_8009E5C8(u32 a0, u32 a1) {
    u32 idx = a0 & 0xFF;
    u8* t0 = D_800C34B0 + idx * 20 + 0x5750;
    u32 a2 = g_GameState[0x59C] * 0xA4;
    u32 id = t0[0xF];
    u32 s;
    u32 o;
    u32 i;

    if (g_GameState[0x98D + a2] == id) a1 = 0;
    if (g_GameState[0x995 + a2] == id) a1 = 1;
    if (g_GameState[0x99D + a2] == id) a1 = 3;
    s = a1 & 0xFF;
    o = s * 8;
    g_GameState[0x98A + o + a2] = t0[0xE];
    g_GameState[0x98C + o + a2] = t0[0x11];
    g_GameState[0x98B + o + a2] = t0[0x10];
    g_GameState[0x98C + o + a2] = t0[0x11];
    g_GameState[0x59C + 0x3E0 + a2 + s] = a0;
    g_GameState[0x2286 + idx] = t0[0xC];
    for (i = 0; i < 3; i++) {
        u32 a3 = i * 0x170;

        if (D_800C34B0[a3 + 0x56] == 4) {
            u32 x = o + a3;

            D_800C34B0[x + 0xB6] = t0[0xE];
            D_800C34B0[x + 0xB8] = t0[0x11];
            D_800C34B0[x + 0xB7] = t0[0x10];
            D_800C34B0[x + 0xB8] = t0[0x11];
            D_800C34B0[a3 + s + 0xA8] = a0;
        }
    }
}

static void bat3_dec_gs22b6(u32 id) {
    u8 v = g_GameState[0x22B6 + id];

    if (v != 0) {
        g_GameState[0x22B6 + id] = v - 1;
    }
}

/* func_8009E788.s: by the D_800C34B0+0x5FC2 mode (jtbl_800704CC), tick down
 * the g_GameState+0x22B6 counters of D_800D2D6C's +4 id (modes 0, 15), its
 * +7 id (modes 2, 17) or both (modes 3-14). */
void func_8009E788(void) {
    u32 mode = D_800C34B0[0x5FC2];

    if (mode >= 0x12) {
        return;
    }
    switch (mode) {
    case 0:
    case 15:
        bat3_dec_gs22b6(D_800D2D6C[4]);
        break;
    case 1:
    case 16:
        break;
    case 2:
    case 17:
        bat3_dec_gs22b6(D_800D2D6C[7]);
        break;
    default:
        bat3_dec_gs22b6(D_800D2D6C[4]);
        bat3_dec_gs22b6(D_800D2D6C[7]);
        break;
    }
}

/* func_8009E868.s: map (menu kind, bit mask) to the D_800C34B0+0x5FC7 command
 * code for kinds 0, 1 and 3; unknown pairs store nothing. */
void func_8009E868(u8 kind, u16 mask) {
    u32 code = 0;

    switch (kind) {
    case 0:
        switch (mask) {
        case 0x400: code = 0x24; break;
        case 0x200: code = 0x25; break;
        case 0x100: code = 0x26; break;
        case 0x080: code = 0x27; break;
        case 0x040: code = 0x28; break;
        case 0x020: code = 0x29; break;
        case 0x010: code = 0x2A; break;
        case 0x004: code = 0x2B; break;
        }
        break;
    case 1:
        switch (mask) {
        case 0x0020: code = 0x16; break;
        case 0x0002:
        case 0x0008: code = 0x19; break;
        case 0x0001:
        case 0x0004: code = 0x1A; break;
        case 0x0040: code = 0x15; break;
        case 0x0400: code = 0x2F; break;
        case 0x0800: code = 0x2E; break;
        case 0x1000: code = 0x2D; break;
        }
        break;
    case 3:
        switch (mask) {
        case 0x8000: code = 0x1B; break;
        case 0x4000: code = 0x1C; break;
        case 0x2000: code = 0x1D; break;
        case 0x1000: code = 0x1E; break;
        case 0x0400: code = 0x1F; break;
        case 0x0800: code = 0x20; break;
        case 0x0100: code = 0x21; break;
        case 0x0200: code = 0x22; break;
        }
        break;
    }
    if (code != 0) {
        D_800C34B0[0x5FC7] = code;
    }
}
#endif /* XENO_PC_PORT */
