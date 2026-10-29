#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;

/* func_8009B104.s, byte-exact: seed character idx's HP words in its
 * 0x170-byte D_800C34B0 record (+0x104 current, +0x108 max) from the
 * record at a1 (+0x4C/+0x4E halfwords, x50).  When the max exceeds 99999
 * both are capped at 99999.  Retail writes that capped max through the
 * record's +0xA4 block (+0x64), so `stats` is its own pointer, set up
 * front; the cap is an if/else (a ternary compares the other way round). */
void func_8009B104(u32 idx, u8* a1) {
    u8* rec = D_800C34B0 + (idx & 0xFF) * 0x170;
    u8* stats = rec + 0xA4;
    s32 cap = 99999;
    s32 hp;

    if (cap < *(u16*)(a1 + 0x4E) * 50) {
        hp = *(u16*)(a1 + 0x4C) * 50;
        if (cap < hp) {
            *(u32*)(rec + 0x104) = cap;
        } else {
            *(u32*)(rec + 0x104) = hp;
        }
        *(u32*)(stats + 0x64) = 99999;
    } else {
        *(u32*)(rec + 0x104) = *(u16*)(a1 + 0x4C) * 50;
        *(u32*)(rec + 0x108) = *(u16*)(a1 + 0x4E) * 50;
    }
}
extern u8 g_GameState[];

/* func_8009B1E4.s, byte-exact: reset the g_GameState battle tables: ids
 * 1..47 into +0x2026[i] with 0x63 at +0x1F90[i], then the fixed
 * per-character mask fields.  The statements go in record order (the
 * 0x20-byte records from +0x16C0; within each, the +0x1A halfword before
 * the +0x17 byte); the scheduler reorders the stores into retail's final
 * order, which the port arm below lists.  Matches under gcc 2.7.2 and
 * 2.6.3. */
void func_8009B1E4(void) {
    u8 i;

    for (i = 1; i < 0x30; i++) {
        g_GameState[0x2026 + i] = i;
        g_GameState[0x1F90 + i] = 0x63;
    }
    *(u16*)(g_GameState + 0x16C0) = 0xFFF8;
    *(u16*)(g_GameState + 0x16C2) = 0xFF00;
    *(u16*)(g_GameState + 0x16C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16C6) = 0xFE00;
    *(u16*)(g_GameState + 0x16DA) = 0xF000;
    *(u8*)(g_GameState + 0x16D7) = 0x7;
    *(u16*)(g_GameState + 0x16E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x16E2) = 0xFFF0;
    *(u16*)(g_GameState + 0x16E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16E6) = 0xFFF0;
    *(u16*)(g_GameState + 0x16FA) = 0xC000;
    *(u8*)(g_GameState + 0x16F7) = 0x7;
    *(u16*)(g_GameState + 0x1700) = 0xFFE0;
    *(u16*)(g_GameState + 0x1702) = 0xFFE0;
    *(u16*)(g_GameState + 0x1704) = 0xFFFF;
    *(u16*)(g_GameState + 0x1706) = 0xFF00;
    *(u16*)(g_GameState + 0x171A) = 0x8000;
    *(u8*)(g_GameState + 0x1717) = 0x7;
    *(u16*)(g_GameState + 0x1720) = 0xFFE0;
    *(u16*)(g_GameState + 0x1722) = 0xFFC0;
    *(u16*)(g_GameState + 0x1724) = 0xFFFF;
    *(u16*)(g_GameState + 0x1726) = 0xFF00;
    *(u16*)(g_GameState + 0x173A) = 0xF000;
    *(u8*)(g_GameState + 0x1737) = 0x7;
    *(u16*)(g_GameState + 0x1740) = 0xFFC0;
    *(u16*)(g_GameState + 0x1742) = 0xFFC0;
    *(u16*)(g_GameState + 0x1744) = 0xFFFF;
    *(u16*)(g_GameState + 0x1746) = 0xFC00;
    *(u16*)(g_GameState + 0x175A) = 0xE000;
    *(u8*)(g_GameState + 0x1757) = 0x7;
    *(u16*)(g_GameState + 0x1760) = 0xFFC0;
    *(u16*)(g_GameState + 0x1762) = 0xF000;
    *(u16*)(g_GameState + 0x1764) = 0xFFFF;
    *(u16*)(g_GameState + 0x1766) = 0xF000;
    *(u16*)(g_GameState + 0x177A) = 0x8000;
    *(u8*)(g_GameState + 0x1777) = 0x7;
    *(u16*)(g_GameState + 0x1780) = 0xFFC0;
    *(u16*)(g_GameState + 0x1782) = 0xFF00;
    *(u16*)(g_GameState + 0x1784) = 0xFFFF;
    *(u16*)(g_GameState + 0x1786) = 0xFF00;
    *(u16*)(g_GameState + 0x179A) = 0x8000;
    *(u8*)(g_GameState + 0x1797) = 0x7;
    *(u16*)(g_GameState + 0x17A0) = 0x0;
    *(u16*)(g_GameState + 0x17A2) = 0xFF00;
    *(u16*)(g_GameState + 0x17A4) = 0x0;
    *(u16*)(g_GameState + 0x17A6) = 0xFF00;
    *(u16*)(g_GameState + 0x17BA) = 0x0;
    *(u8*)(g_GameState + 0x17B7) = 0x7;
    *(u16*)(g_GameState + 0x17C0) = 0x0;
    *(u16*)(g_GameState + 0x17C2) = 0xF800;
    *(u16*)(g_GameState + 0x17C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17C6) = 0x0;
    *(u16*)(g_GameState + 0x17DA) = 0xE000;
    *(u8*)(g_GameState + 0x17D7) = 0x7;
    *(u16*)(g_GameState + 0x17E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E2) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17E6) = 0xFFE0;
    *(u16*)(g_GameState + 0x17FA) = 0x8000;
    *(u8*)(g_GameState + 0x17F7) = 0x7;
    *(u16*)(g_GameState + 0x1800) = 0xFFC0;
    *(u16*)(g_GameState + 0x1802) = 0xFF00;
    *(u16*)(g_GameState + 0x1804) = 0xFFFF;
    *(u16*)(g_GameState + 0x1806) = 0xFF00;
    *(u16*)(g_GameState + 0x181A) = 0xC000;
    *(u8*)(g_GameState + 0x1817) = 0x7;
}
extern u8 D_800CCE42__rows[][0x170] asm("D_800CCE42");
extern u8* D_800C3E00;

/* func_8009B46C.s, byte-exact: scale the value at p by the party's low
 * health.  For slots 0..2 in state 0 (or 3), n counts current < max / 2
 * and current < max / 4.  The fields are the u32 +0x104/+0x108 pair (stats
 * +0x60/+0x64 in state 3) when the slot's D_800CCE42 row has bit 7, else
 * the u16 +0x4C/+0x4E pair.  Then *p += n * (*p / 2), and with a 10% chance
 * (60% under flag 0x200 at D_800C3E00 +0x32) *p = mult * *p >> 1, where mult
 * is 3, or 4 under flag 0x400.
 * Codegen notes:
 * - Each flag arm keeps its own mx/cur locals and its own copy of the two
 *   tests (post-reload cross-jumping folds the copies back).  The extra
 *   pre-reload insns lengthen p's live range, giving retail's chance/mult/p
 *   = s0/s1/s2.
 * - The parameter is used directly as p (no local copy declared after
 *   `n = 0`), so n is not born while a copy still holds a0, and takes a0
 *   as in retail. */
void func_8009B46C(u16* p) {
    u8 n = 0;
    u8 i;
    u8* rec;
    u8* stats;
    u32 t;
    s32 chance;
    s32 mult;

    for (i = 0; i < 3; i++) {
        t = i * 0x170;
        rec = D_800C34B0 + t;
        stats = rec + 0xA4;
        if (rec[0x56] == 0) {
            if (D_800CCE42__rows[i][0] & 0x80) {
                u32 mx = *(u32*)(rec + 0x108);
                u32 cur = *(u32*)(rec + 0x104);

                if (cur < mx / 2) {
                    n++;
                }
                if (cur < mx / 4) {
                    n++;
                }
            } else {
                u32 mx = *(u16*)(rec + 0x4E);
                u32 cur = *(u16*)(rec + 0x4C);

                if (cur < mx / 2) {
                    n++;
                }
                if (cur < mx / 4) {
                    n++;
                }
            }
        }
        if (rec[0x56] == 3) {
            if (D_800CCE42__rows[i][0] & 0x80) {
                u32 mx = *(u32*)(stats + 0x64);
                u32 cur = *(u32*)(stats + 0x60);

                if (cur < mx / 2) {
                    n++;
                }
                if (cur < mx / 4) {
                    n++;
                }
            } else {
                u32 mx = *(u16*)(rec + 0x4E);
                u32 cur = *(u16*)(rec + 0x4C);

                if (cur < mx / 2) {
                    n++;
                }
                if (cur < mx / 4) {
                    n++;
                }
            }
        }
    }
    if (n != 0) {
        *p += n * (*p / 2);
    }
    chance = 10;
    mult = 3;
    if (*(u16*)(D_800C3E00 + 0x32) & 0x400) {
        mult = 4;
    }
    if (*(u16*)(D_800C3E00 + 0x32) & 0x200) {
        chance = 0x3C;
    }
    if (rand() % 100 < chance) {
        *p = (mult * *p) >> 1;
    }
}

/* func_8009B684.s, byte-exact: map (menu, mask bit) to the D_800C34B0
 * +0x5FC7 command code for menus 0, 2, 5, 7 and 9, the twin of main64's
 * func_8009E868; unknown pairs store nothing.  Each case stores its code
 * itself, in retail's block order.  The menu switch is a jump table
 * (jtbl_80070478, placed by the main58 .rodata split in
 * config/battle.yaml).  Matches under gcc 2.6.x and 2.7.2; the port arm
 * is below. */
void func_8009B684(u8 menu, u16 mask) {
    switch (menu) {
    case 0:
        switch (mask) {
        case 0x2000:
            D_800C34B0[0x5FC7] = 0x1;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x2;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0x3;
            break;
        case 0x400:
            D_800C34B0[0x5FC7] = 0x4;
            break;
        case 0x200:
            D_800C34B0[0x5FC7] = 0x5;
            break;
        case 0x1:
            D_800C34B0[0x5FC7] = 0x8;
            break;
        }
        break;
    case 2:
        switch (mask) {
        case 0x2000:
            D_800C34B0[0x5FC7] = 0x9;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0xA;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0xB;
            break;
        case 0x400:
            D_800C34B0[0x5FC7] = 0xC;
            break;
        case 0x1:
            D_800C34B0[0x5FC7] = 0xD;
            break;
        case 0x20:
            D_800C34B0[0x5FC7] = 0x7;
            break;
        }
        break;
    case 5:
        switch (mask) {
        case 0x8000:
            D_800C34B0[0x5FC7] = 0xE;
            break;
        case 0x4000:
            D_800C34B0[0x5FC7] = 0xF;
            break;
        case 0x2000:
            D_800C34B0[0x5FC7] = 0x10;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x11;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0x12;
            break;
        case 0x1800:
            D_800C34B0[0x5FC7] = 0x13;
            break;
        }
        break;
    case 7:
        switch (mask) {
        case 0x8000:
            D_800C34B0[0x5FC7] = 0x15;
            break;
        case 0x4000:
            D_800C34B0[0x5FC7] = 0x16;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x18;
            break;
        case 0x8:
        case 0x2:
            D_800C34B0[0x5FC7] = 0x19;
            break;
        case 0x1:
        case 0x4:
            D_800C34B0[0x5FC7] = 0x1A;
            break;
        }
        break;
    case 9:
        switch (mask) {
        case 0x8000:
            D_800C34B0[0x5FC7] = 0x1B;
            break;
        case 0x4000:
            D_800C34B0[0x5FC7] = 0x1C;
            break;
        case 0x2000:
            D_800C34B0[0x5FC7] = 0x1D;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x1E;
            break;
        case 0x400:
            D_800C34B0[0x5FC7] = 0x1F;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0x20;
            break;
        case 0x100:
            D_800C34B0[0x5FC7] = 0x21;
            break;
        case 0x200:
            D_800C34B0[0x5FC7] = 0x22;
            break;
        }
        break;
    }
}
/* func_8009BAC4.s, byte-exact: pick an enemy action for actor idx into
 * out[0] (kind) and out[1] (argument).
 * - Kind 4 at 10% while the +0x7A bit 8 is clear and *a2 == 0.
 * - Kind 2 at 25% while +0x7A bit 5 is clear, with a random slot r < n
 *   allowed by the type's mask word at g_GameState+0x16C2[type * 32].
 * - 0/0 for type 8.
 * - Otherwise kind 1, with argument 0/1/2 weighted 48/32/20.
 * As in retail, n and mask stay unset for a type >= 11.  Codegen
 * (gcc 2.6.3):
 * - A narrow r (u8; u16 matches too) gets its own register, copied from
 *   the remainder as in retail (addu a1, v1, zero); an int r is one insn
 *   short.
 * - The last test is written `>= 0x3C` with out[1] = 1 first, so the
 *   out[1] = 0 arm cross-jumps into type 8's tail as in retail.
 * - The case source order is retail's block order.
 * The switch table jtbl_800704A0 is now main58's .rodata, after
 * func_8009B684's (config/battle.yaml: [0x988, .rodata, main58] covers
 * both). */
extern u8 D_800CCCE8[];
extern u8 D_800CCE42[];
extern u8 g_GameState[];

void func_8009BAC4(u8 idx, u8* out, s16* a2) {
    u32 off = idx * 0x170;
    u8* rec = off + D_800CCCE8;
    u8 n;
    u16 mask;
    u8 r;

    if (!(D_800CCE42[off] & 0x80)) {
        if (rand() % 100 < 10 && !(*(u16*)(rec + 0x7A) & 0x100) && *a2 == 0) {
            out[0] = 4;
            return;
        }
        switch (rec[0x56]) {
        case 3:
            mask = 0xC3C0;
            n = 0xA;
            break;
        case 4:
            mask = 0xDF80;
            n = 0xA;
            break;
        case 5:
            mask = 0x1000;
            n = 4;
            break;
        case 7:
            mask = 0xE000;
            n = 3;
            break;
        case 0:
        case 8:
            mask = 0xC000;
            n = 2;
            break;
        case 2:
        case 9:
            mask = 0xBFE0;
            n = 0xB;
            break;
        case 1:
        case 6:
        case 10:
            mask = 0xF000;
            n = 4;
            break;
        }
        if (rand() % 100 < 0x19 && !(*(u16*)(rec + 0x7A) & 0x20)) {
            out[0] = 2;
            r = rand() % n;
            if (*(u16*)(g_GameState + 0x16C2 + rec[0x56] * 32) & ((0x8000 >> r) & mask)) {
                out[1] = r;
                return;
            }
        }
        if (rec[0x56] == 8) {
            out[0] = 0;
            out[1] = 0;
            return;
        }
    }
    out[0] = 1;
    if (rand() % 100 < 0x50) {
        if (rand() % 100 >= 0x3C) {
            out[1] = 1;
        } else {
            out[1] = 0;
        }
    } else {
        out[1] = 2;
    }
}
#include "battle/c34b0.h"
extern u8* D_800C34B0;
extern u8* D_800C3DFC;
extern u8 D_800C3E50;
extern u8* D_800D2DC8;

/* Slot D_800C3E50: state byte +0x5FA0 = 2 (struct view: pointer loaded
 * first), then word +0x5F6C = D_800C3DFC[0x11] * D_800D2DC8 word +0x64 / 20
 * (pointer arithmetic: index loaded first).  Same family as main62's
 * func_8009E278 / func_8009E2EC. */
void func_8009BD94(void) {
    u32 prod;

    ((BattleC34B0*)D_800C34B0)->state[D_800C3E50] = 2;
    prod = D_800C3DFC[0x11] * *(u32*)(D_800D2DC8 + 0x64);
    ((u32*)(D_800C34B0 + 0x5F6C))[D_800C3E50] = prod / 20;
}
extern u8 D_800CCCE8[];
extern u8 D_800CCE42__rows[][0x170] asm("D_800CCE42");
extern u8 g_GameState[];
extern u8 D_800D2D24[];

/* func_8009BE0C.s, byte-exact WITH A CONSTRUCT: copy the live battle stats
 * of the first three actors (0x170-byte D_800CCCE8 records) back into their
 * g_GameState records (0xA4 stride from +0x26C: the character by +0x56, the
 * Gear by +0xA0, which follows the 11 character records), skipping slots
 * whose D_800D2D24 byte is 0x7F.  Character 7 with D_800CCE42 bit 7 first
 * takes HP = (+0xA4 block +0x60 + 1) / 50, at least 1.  HP +0x4C and MP
 * +0x50 are copied and clamped to +0x4E / +0x52, the seven +0x90 halfwords
 * and +0x3A copied, HP set to 1 when +0x7C has 0xC000; for Gear ids 0..6
 * and 8..16 the block's +0x60 / +0x38 are copied and clamped to +0x64 /
 * +0x3A, and +0x60 becomes +0x64 / 10 with block +0x7C bit 0x8000.  The
 * port arm is the XENO_PC_PORT body below.
 * Construct (counted separately; cause unknown): retail has an empty
 * 8-byte stack frame (no saves, no stack accesses).  The unused 8-byte
 * local `unused` reproduces it; without it the function is exactly 2
 * instructions short (572 vs 580 bytes) and otherwise identical.
 * Codegen otherwise natural: `off + (D_800CCCE8 + 0xA4)` as in
 * func_8009C050, the Gear base hoisted as g_GameState + 0x26C + 0x70C, the
 * clamps tested as `cur > max`.  Matches under gcc 2.6.x. */
void func_8009BE0C(void) {
    u8 i;
    u32 off;
    s32 unused[2];
    u8 j;
    u8* rec;
    u8* stats;
    u8* gs;
    u8* gm;

    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] != 0x7F) {
            off = i * 0x170;
            rec = off + D_800CCCE8;
            stats = off + (D_800CCCE8 + 0xA4);
            gs = g_GameState + 0x26C + rec[0x56] * 0xA4;
            gm = g_GameState + 0x26C + 0x70C + rec[0xA0] * 0xA4;
            if (rec[0x56] == 7 && (D_800CCE42__rows[i][0] & 0x80)) {
                *(u16*)(rec + 0x4C) = (*(u32*)(stats + 0x60) + 1) / 50;
                if (*(u16*)(rec + 0x4C) == 0) {
                    *(u16*)(rec + 0x4C) = 1;
                }
            }
            *(u16*)(gs + 0x4C) = *(u16*)(rec + 0x4C);
            *(u16*)(gs + 0x50) = *(u16*)(rec + 0x50);
            if (*(u16*)(gs + 0x4C) > *(u16*)(gs + 0x4E)) {
                *(u16*)(gs + 0x4C) = *(u16*)(gs + 0x4E);
            }
            if (*(u16*)(gs + 0x50) > *(u16*)(gs + 0x52)) {
                *(u16*)(gs + 0x50) = *(u16*)(gs + 0x52);
            }
            for (j = 0; j < 7; j++) {
                *(u16*)(gs + 0x90 + j * 2) = *(u16*)(rec + 0x90 + j * 2);
            }
            *(u16*)(gs + 0x3A) = *(u16*)(rec + 0x3A);
            if (*(u16*)(rec + 0x7C) & 0xC000) {
                *(u16*)(gs + 0x4C) = 1;
            }
            switch (rec[0xA0]) {
            case 0 ... 6:
            case 8 ... 16:
                *(u32*)(gm + 0x60) = *(u32*)(stats + 0x60);
                *(u16*)(gm + 0x38) = *(u16*)(stats + 0x38);
                if (*(u32*)(gm + 0x60) > *(u32*)(gm + 0x64)) {
                    *(u32*)(gm + 0x60) = *(u32*)(gm + 0x64);
                }
                if (*(u16*)(gm + 0x38) > *(u16*)(gm + 0x3A)) {
                    *(u16*)(gm + 0x38) = *(u16*)(gm + 0x3A);
                }
                if (*(u16*)(stats + 0x7C) & 0x8000) {
                    *(u32*)(gm + 0x60) = *(u32*)(gm + 0x64) / 10;
                }
                break;
            }
        }
    }
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCCE8[];
#endif
#ifdef XENO_PC_PORT
/* Port body; the retail build compiles the byte-exact C in the #else branch.
 */
/* func_8009C050.s: status flags for actor idx (368-byte stride over
 * D_800CCCE8): bit0 = +0x120 & 0x400, bit1 = +0x104 < (+0x108 >> 3) with
 * +0x36 bit0 clear, bit2 = +0x148 == 4. */
u8 func_8009C050(u8 idx) {
    u32 off = idx * 368;
    u8* base = D_800CCCE8;
    u8* a = off + (base + 0xA4);
    u8* b = off + base;
    u8* c = off + (base + 0x148);
    u32 t = (*(u16*)(a + 0x7C) & 0x400) != 0;
    u32 r = t;

    if (*(u32*)(a + 0x60) < (*(u32*)(a + 0x64) >> 3) && !(*(u16*)(b + 0x36) & 1)) {
        r = t | 2;
    }
    if (c[0] == 4) {
        r |= 4;
    }
    return r;
}
#else
/* func_8009C050.s, byte-exact: the plain-base pointer b comes first so CSE
 * derives the +0xA4 and +0x148 addresses from the base register; the 0x400
 * test goes through a masked temp (andi + sltu), and r is u8 so the two
 * flags keep separate registers (t's type makes no difference). */
u8 func_8009C050(u8 idx) {
    u32 off = idx * 368;
    u8* b = off + D_800CCCE8;
    u8* a = off + (D_800CCCE8 + 0xA4);
    u8* c = off + (D_800CCCE8 + 0x148);
    u32 m;
    u8 t;
    u8 r;

    m = *(u16*)(a + 0x7C) & 0x400;
    t = m != 0;
    r = t;
    if (*(u32*)(a + 0x60) < (*(u32*)(a + 0x64) >> 3) && !(*(u16*)(b + 0x36) & 1)) {
        r = t | 2;
    }
    if (c[0] == 4) {
        r |= 4;
    }
    return r;
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE30[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE4A[];
#endif
extern u8 g_GameState[];


/* func_8009C0E0.s */
void func_8009C0E0(u8 index) {
    u32 off = (u32)index * 368;
    u16* p = (u16*)(g_GameState + 0x16DA);

    D_800CCE30[off] = 4;
    D_800CCE4A[off] = 3;
    *p |= 0x4000;
}


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE42[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
#ifdef XENO_PC_PORT
extern int rand(void);

/* func_8009B104.s: seed actor idx's HP words (+0x104 current, +0x108 max)
 * from the record at a1 (+0x4C/+0x4E halfwords, x50), clamping to 99999 when
 * the max overflows it. */
void func_8009B104(u32 idx, u8* a1) {
    u8* a0 = D_800C34B0 + (idx & 0xFF) * 0x170;
    s32 cap = 0x1869F;
    s32 max = *(u16*)(a1 + 0x4E) * 50;

    if (cap < max) {
        s32 cur = *(u16*)(a1 + 0x4C) * 50;

        *(u32*)(a0 + 0x104) = (cap < cur) ? cap : cur;
        *(u32*)(a0 + 0x108) = 0x1869F;
    } else {
        *(u32*)(a0 + 0x104) = *(u16*)(a1 + 0x4C) * 50;
        *(u32*)(a0 + 0x108) = *(u16*)(a1 + 0x4E) * 50;
    }
}

/* func_8009B1E4.s: reset the g_GameState battle tables: ids 1..47 into
 * +0x2026[i] with 0x63 at +0x1F90[i], then the fixed per-character mask
 * words (retail store order). */
void func_8009B1E4(void) {
    u32 i;

    for (i = 1; i < 0x30; i++) {
        g_GameState[0x2026 + i] = i;
        g_GameState[0x1F90 + i] = 0x63;
    }
    *(u16*)(g_GameState + 0x16C0) = 0xFFF8;
    *(u16*)(g_GameState + 0x16C6) = 0xFE00;
    *(u16*)(g_GameState + 0x16E2) = 0xFFF0;
    *(u16*)(g_GameState + 0x16E6) = 0xFFF0;
    *(u16*)(g_GameState + 0x1746) = 0xFC00;
    *(u16*)(g_GameState + 0x16C2) = 0xFF00;
    *(u16*)(g_GameState + 0x16C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16DA) = 0xF000;
    *(u8*)(g_GameState + 0x16D7) = 0x7;
    *(u16*)(g_GameState + 0x16E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x16E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16FA) = 0xC000;
    *(u8*)(g_GameState + 0x16F7) = 0x7;
    *(u16*)(g_GameState + 0x1700) = 0xFFE0;
    *(u16*)(g_GameState + 0x1702) = 0xFFE0;
    *(u16*)(g_GameState + 0x1704) = 0xFFFF;
    *(u16*)(g_GameState + 0x1706) = 0xFF00;
    *(u16*)(g_GameState + 0x171A) = 0x8000;
    *(u8*)(g_GameState + 0x1717) = 0x7;
    *(u16*)(g_GameState + 0x1720) = 0xFFE0;
    *(u16*)(g_GameState + 0x1722) = 0xFFC0;
    *(u16*)(g_GameState + 0x1724) = 0xFFFF;
    *(u16*)(g_GameState + 0x1726) = 0xFF00;
    *(u16*)(g_GameState + 0x173A) = 0xF000;
    *(u8*)(g_GameState + 0x1737) = 0x7;
    *(u16*)(g_GameState + 0x1740) = 0xFFC0;
    *(u16*)(g_GameState + 0x1742) = 0xFFC0;
    *(u16*)(g_GameState + 0x1744) = 0xFFFF;
    *(u16*)(g_GameState + 0x175A) = 0xE000;
    *(u8*)(g_GameState + 0x1757) = 0x7;
    *(u16*)(g_GameState + 0x1760) = 0xFFC0;
    *(u16*)(g_GameState + 0x1762) = 0xF000;
    *(u16*)(g_GameState + 0x1764) = 0xFFFF;
    *(u16*)(g_GameState + 0x1766) = 0xF000;
    *(u16*)(g_GameState + 0x177A) = 0x8000;
    *(u8*)(g_GameState + 0x1777) = 0x7;
    *(u16*)(g_GameState + 0x1780) = 0xFFC0;
    *(u16*)(g_GameState + 0x1782) = 0xFF00;
    *(u16*)(g_GameState + 0x1784) = 0xFFFF;
    *(u16*)(g_GameState + 0x1786) = 0xFF00;
    *(u16*)(g_GameState + 0x179A) = 0x8000;
    *(u8*)(g_GameState + 0x1797) = 0x7;
    *(u16*)(g_GameState + 0x17A0) = 0x0;
    *(u16*)(g_GameState + 0x17A2) = 0xFF00;
    *(u16*)(g_GameState + 0x17A4) = 0x0;
    *(u16*)(g_GameState + 0x17A6) = 0xFF00;
    *(u16*)(g_GameState + 0x17BA) = 0x0;
    *(u8*)(g_GameState + 0x17B7) = 0x7;
    *(u16*)(g_GameState + 0x17C0) = 0x0;
    *(u16*)(g_GameState + 0x17C2) = 0xF800;
    *(u16*)(g_GameState + 0x17C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17C6) = 0x0;
    *(u16*)(g_GameState + 0x17DA) = 0xE000;
    *(u8*)(g_GameState + 0x17D7) = 0x7;
    *(u16*)(g_GameState + 0x17E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E2) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17E6) = 0xFFE0;
    *(u16*)(g_GameState + 0x17FA) = 0x8000;
    *(u8*)(g_GameState + 0x17F7) = 0x7;
    *(u16*)(g_GameState + 0x1800) = 0xFFC0;
    *(u16*)(g_GameState + 0x1802) = 0xFF00;
    *(u16*)(g_GameState + 0x1804) = 0xFFFF;
    *(u16*)(g_GameState + 0x1806) = 0xFF00;
    *(u16*)(g_GameState + 0x181A) = 0xC000;
    *(u8*)(g_GameState + 0x1817) = 0x7;
}

/* HP-fraction score for one actor: +1 below half, +1 more below a quarter.
 * A +0x162 bit7 actor uses the 32-bit words at +0x104/+0x108. */
static u32 bat3_low_hp_score(u8* a2, u32 off) {
    u32 cur;
    u32 max;
    u32 n = 0;

    if (D_800CCE42[off] & 0x80) {
        max = *(u32*)(a2 + 0x108);
        cur = *(u32*)(a2 + 0x104);
    } else {
        max = *(u16*)(a2 + 0x4E);
        cur = *(u16*)(a2 + 0x4C);
    }
    if (cur < (max >> 1)) n++;
    if (cur < (max >> 2)) n++;
    return n;
}

/* func_8009B46C.s: scale the halfword at s2 up by the low-HP score of the
 * type-0 and type-3 actors among the first three, then on a rand() roll
 * (10%, or 60% with D_800C3E00+0x32 bit9) multiply it by 3/2 (4/2 with bit10). */
void func_8009B46C(u16* s2) {
    u32 n = 0;
    u32 i;
    u32 f;
    s32 mul;
    s32 pct;

    for (i = 0; i < 3; i++) {
        u32 off = i * 0x170;
        u8* a2 = D_800C34B0 + off;

        if (a2[0x56] == 0) {
            n += bat3_low_hp_score(a2, off);
        }
        if (a2[0x56] == 3) {
            n += bat3_low_hp_score(a2, off);
        }
    }
    if ((n & 0xFF) != 0) {
        *s2 = *s2 + (n & 0xFF) * (*s2 >> 1);
    }
    f = *(u16*)(D_800C3E00 + 0x32);
    mul = (f & 0x400) ? 4 : 3;
    pct = (f & 0x200) ? 0x3C : 0xA;
    if ((s32)(rand() % 100) < pct) {
        *s2 = (mul * *s2) >> 1;
    }
}

/* func_8009B684.s: map (menu a0, bit a1) to the D_800C34B0+0x5FC7 command
 * code (jtbl_80070478 over menus 0/2/5/7/9); unknown pairs store nothing. */
void func_8009B684(u8 a0, u16 a1) {
    u32 code = 0;

    switch (a0) {
    case 0:
        switch (a1) {
        case 0x2000: code = 0x01; break;
        case 0x1000: code = 0x02; break;
        case 0x0800: code = 0x03; break;
        case 0x0400: code = 0x04; break;
        case 0x0200: code = 0x05; break;
        case 0x0001: code = 0x08; break;
        }
        break;
    case 2:
        switch (a1) {
        case 0x2000: code = 0x09; break;
        case 0x1000: code = 0x0A; break;
        case 0x0800: code = 0x0B; break;
        case 0x0400: code = 0x0C; break;
        case 0x0001: code = 0x0D; break;
        case 0x0020: code = 0x07; break;
        }
        break;
    case 5:
        switch (a1) {
        case 0x8000: code = 0x0E; break;
        case 0x4000: code = 0x0F; break;
        case 0x2000: code = 0x10; break;
        case 0x1000: code = 0x11; break;
        case 0x0800: code = 0x12; break;
        case 0x1800: code = 0x13; break;
        }
        break;
    case 7:
        switch (a1) {
        case 0x8000: code = 0x15; break;
        case 0x4000: code = 0x16; break;
        case 0x1000: code = 0x18; break;
        case 0x0008:
        case 0x0002: code = 0x19; break;
        case 0x0001:
        case 0x0004: code = 0x1A; break;
        }
        break;
    case 9:
        switch (a1) {
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

/* func_8009BAC4.s: pick an enemy action for actor idx into out[0] (kind) and
 * out[1] (argument).  Kinds: 4 (10% when +0x7A bit8 clear and *a2 == 0), 2
 * with a random slot allowed by the type's mask word at
 * g_GameState+0x16C2[type*32] (25% when +0x7A bit5 clear), 0/0 for type 8,
 * else 1 with 0/1/2 weighted 48/32/20.  Retail leaves $s0/$s1 unset for a
 * type >= 11; the host uses 0 there, and a zero modulus follows the R3000
 * rule (remainder = dividend). */
void func_8009BAC4(u8 idx, u8* out, s16* a2) {
    u32 off = idx * 0x170;
    u8* s3 = D_800CCCE8 + off;
    u32 s0 = 0;
    u32 s1 = 0;

    if (!(D_800CCE42[off] & 0x80)) {
        if ((s32)(rand() % 100) < 0xA && !(*(u16*)(s3 + 0x7A) & 0x100) && *a2 == 0) {
            out[0] = 4;
            return;
        }
        switch (s3[0x56]) {
        case 0: case 8: s1 = 0xC000; s0 = 2; break;
        case 1: case 6: case 10: s1 = 0xF000; s0 = 4; break;
        case 2: case 9: s1 = 0xBFE0; s0 = 0xB; break;
        case 3: s1 = 0xC3C0; s0 = 0xA; break;
        case 4: s1 = 0xDF80; s0 = 0xA; break;
        case 5: s1 = 0x1000; s0 = 4; break;
        case 7: s1 = 0xE000; s0 = 3; break;
        }
        if ((s32)(rand() % 100) < 0x19 && !(*(u16*)(s3 + 0x7A) & 0x20)) {
            s32 v;
            s32 d = s0 & 0xFF;
            s32 r;

            out[0] = 2;
            v = rand();
            r = (d == 0) ? v : v % d;
            if (*(u16*)(g_GameState + 0x16C2 + s3[0x56] * 32) &
                ((0x8000 >> (r & 31)) & (s1 & 0xFFFF))) {
                out[1] = r;
                return;
            }
        }
        if (s3[0x56] == 8) {
            out[0] = 0;
            out[1] = 0;
            return;
        }
    }
    out[0] = 1;
    if ((s32)(rand() % 100) < 0x50) {
        if ((s32)(rand() % 100) < 0x3C) {
            out[1] = 0;
        } else {
            out[1] = 1;
        }
    } else {
        out[1] = 2;
    }
}

/* func_8009BD94.s: mark the current actor (D_800C3E50) with state 2 and
 * store D_800C3DFC+0x11 x D_800D2DC8+0x64 / 20 as its +0x5F6C word. */
void func_8009BD94(void) {
    u32 prod;

    D_800C34B0[D_800C3E50 + 0x5FA0] = 2;
    prod = D_800C3DFC[0x11] * *(u32*)(D_800D2DC8 + 0x64);
    *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = prod / 20;
}

/* func_8009BE0C.s: copy the live battle stats of the first three actors back
 * into their g_GameState character records (0xA4 stride from +0x26C, indexed
 * by +0x56; the Gear record by +0xA0 from +0x978), clamping to the maxima.
 * Slots whose D_800D2D24 byte is 0x7F are skipped. */
void func_8009BE0C(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        u32 off;
        u8* a2;
        u8* a1;
        u8* t0;
        u8* t1;
        u32 j;
        u32 g;

        if (D_800D2D24[i] == 0x7F) {
            continue;
        }
        off = i * 0x170;
        a2 = D_800CCCE8 + off;
        a1 = g_GameState + 0x26C + a2[0x56] * 0xA4;
        t0 = g_GameState + 0x26C + 0x70C + a2[0xA0] * 0xA4;
        t1 = a2 + 0xA4;
        if (a2[0x56] == 7 && (D_800CCE42[off] & 0x80)) {
            *(u16*)(a2 + 0x4C) = (*(u32*)(t1 + 0x60) + 1) / 50;
            if (*(u16*)(a2 + 0x4C) == 0) {
                *(u16*)(a2 + 0x4C) = 1;
            }
        }
        *(u16*)(a1 + 0x4C) = *(u16*)(a2 + 0x4C);
        {
            u16 mp = *(u16*)(a2 + 0x50);

            if (*(u16*)(a1 + 0x4E) < *(u16*)(a1 + 0x4C)) {
                *(u16*)(a1 + 0x50) = mp;
                *(u16*)(a1 + 0x4C) = *(u16*)(a1 + 0x4E);
            } else {
                *(u16*)(a1 + 0x50) = mp;
            }
        }
        if (*(u16*)(a1 + 0x52) < *(u16*)(a1 + 0x50)) {
            *(u16*)(a1 + 0x50) = *(u16*)(a1 + 0x52);
        }
        for (j = 0; j < 7; j++) {
            *(u16*)(a1 + 0x90 + j * 2) = *(u16*)(a2 + 0x90 + j * 2);
        }
        *(u16*)(a1 + 0x3A) = *(u16*)(a2 + 0x3A);
        if (*(u16*)(a2 + 0x7C) & 0xC000) {
            *(u16*)(a1 + 0x4C) = 1;
        }
        g = a2[0xA0];
        if (g < 7 || (g >= 8 && g < 0x11)) {
            u32 cap = *(u32*)(t0 + 0x64);
            u16 hp = *(u16*)(t1 + 0x38);

            *(u32*)(t0 + 0x60) = *(u32*)(t1 + 0x60);
            *(u16*)(t0 + 0x38) = hp;
            if (cap < *(u32*)(t0 + 0x60)) {
                *(u32*)(t0 + 0x60) = cap;
            }
            if (*(u16*)(t0 + 0x3A) < *(u16*)(t0 + 0x38)) {
                *(u16*)(t0 + 0x38) = *(u16*)(t0 + 0x3A);
            }
            if (*(u16*)(t1 + 0x7C) & 0x8000) {
                *(u32*)(t0 + 0x60) = *(u32*)(t0 + 0x64) / 10;
            }
        }
    }
}
#endif /* XENO_PC_PORT */
