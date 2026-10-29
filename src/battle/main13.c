#include "common.h"


#ifndef XENO_PC_PORT
extern void func_80079934(u8** pp);

/* func_80079948.s: advance the script cursor *pp 4 bytes at a time
 * (func_80079934) past records whose lead byte is >= 0x80, then on to the
 * next record whose lead byte is in [0x80, 0xF0).
 * The second test is written in unsigned arithmetic, which keeps both of
 * retail's `addiu -0x80` and unsigned `sltiu 0x70`: the `x < 0x80 ||
 * x >= 0xF0` and (u8)(x - 0x80) spellings fold to `addiu 0x80`, and an int
 * (x - 0x80) & 0xFF compares signed (`slti`). */
void func_80079948(u8** pp) {
    while (**pp >= 0x80) {
        func_80079934(pp);
    }
    while (((**pp - 0x80U) & 0xFF) >= 0x70) {
        func_80079934(pp);
    }
}

extern u8 D_800D3400[];
extern u8 D_800C402F[];
extern u8 D_800D2E5C[];
extern u8 func_8007F8C0(u8** pp, u8 e);
extern u8 func_8007EF6C(u8** pp, u8 e, u8 state);

/* func_800799C8.s, byte-exact: func_80079AB0's AI-script loop for enemy
 * `actor` without its guard and result (e = actor - 3): the script
 * pointer comes from its 64-byte D_800D3400 record, the 0x100-byte
 * D_800D2E5C buffer is cleared, all 32 command rows get D_800C402F =
 * 0xFF (retail counts the offset down), and opcodes run until 0xFD or
 * 0xFF (>= 0x80 to func_8007F8C0, then func_80079948 when it returns 0;
 * others to func_8007EF6C, which returns the next state).  `flag` is
 * func_80071B94's second argument (D_800CCD68 & 0x2000); retail never
 * reads it.
 * Codegen: func_80079AB0's `ended` flag (opcode 0x62) is kept but never
 * read.  Its compare and store are dead and vanish after loop.c, but
 * they keep loop.c from hoisting 0xFD/0xFF into saved registers, as in
 * retail; the clear loop is a do/while over the pointer.  Matches under
 * gcc 2.7.2 (main13's preset) and 2.6.x. */
void func_800799C8(u8 actor, s32 flag) {
    u8 e;
    u8 ended;
    u8 state;
    u8* pc;
    u8* p;
    u8* end;
    s32 k;

    ended = 0;
    state = 0;
    e = actor - 3;
    pc = *(u8**)(D_800D3400 + (e << 6));
    p = D_800D2E5C;
    end = p + 0x100;
    do {
        *p++ = 0;
    } while (p < end);
    for (k = 0x8B8; k >= 0; k -= 0x48) {
        D_800C402F[k] = 0xFF;
    }
    while (*pc != 0xFD && *pc != 0xFF) {
        if (*pc >= 0x80) {
            if (func_8007F8C0(&pc, e) == 0) {
                func_80079948(&pc);
            }
        } else {
            if (*pc == 0x62) {
                ended = 1;
            }
            state = func_8007EF6C(&pc, e, state);
        }
    }
}

extern u16 D_800CCD64__h[][184] asm("D_800CCD64");
extern u16 D_800CCD1C__h[][184] asm("D_800CCD1C");
extern u8 D_800C3D18[];
extern u8 D_800D3408[];
extern void func_800793F0(u8 actor);

/* func_80079AB0.s, byte-exact: run enemy `actor`'s AI script (e = actor
 * - 3).  Unless its D_800CCD64 halfword (0x170-byte records) has bit
 * 0x8000 without bit 0x800 of its D_800CCD1C halfword: D_800D2E5C[0] = 0;
 * when the enemy's D_800C3D18 byte is set, the script pointer comes from
 * its 64-byte D_800D3408 record, the 0x100-byte D_800D2E5C buffer is
 * cleared, and opcodes run until 0xFD or 0xFF: opcodes >= 0x80 go to
 * func_8007F8C0 (and func_80079948 skips on when it returns 0), others
 * to func_8007EF6C (which returns the next state; opcode 0x62 marks the
 * result).  A set D_800D2E5C[0] then calls func_800793F0(actor).
 * Returns 1 when opcode 0x62 ran.
 * Codegen: the guard wraps the body (`!A || B`), which keeps e & 0xFF in
 * one register across it; the clear loop is a pointer loop with an end
 * pointer taken from it before it moves (retail reuses the `la` register
 * as the pointer); opcodes >= 0x80 are tested first, as in retail's block
 * order.  Matches under gcc 2.7.2 (main13's preset) and 2.6.x. */
u8 func_80079AB0(u8 actor) {
    u8 e;
    u8 ended;
    u8 state;
    u8* pc;
    u8* p;
    u8* end;

    ended = 0;
    e = actor - 3;
    state = 0;
    if (!(D_800CCD64__h[e + 3][0] & 0x8000) || (D_800CCD1C__h[e + 3][0] & 0x800)) {
        D_800D2E5C[0] = 0;
        if (D_800C3D18[e * 4] != 0) {
            pc = *(u8**)(D_800D3408 + (e << 6));
            p = D_800D2E5C;
            end = p + 0x100;
            *p = 0;
            while (++p < end) {
                *p = 0;
            }
            while (*pc != 0xFD && *pc != 0xFF) {
                if (*pc >= 0x80) {
                    if (func_8007F8C0(&pc, e) == 0) {
                        func_80079948(&pc);
                    }
                } else {
                    if (*pc == 0x62) {
                        ended = 1;
                    }
                    state = func_8007EF6C(&pc, e, state);
                }
            }
        }
        if (D_800D2E5C[0] != 0) {
            func_800793F0(e + 3);
        }
    }
    return ended;
}
extern u8* D_800C3EAC;
extern u8 D_800C3D18__rows[][4] asm("D_800C3D18");
extern u8 D_800D340C[];
extern void func_80079778(u8 a);
extern void func_80070EB0(s32 arg0);

/* func_80079C24.s, byte-exact: run the AI scripts of the enemies whose
 * D_800C3D18 record bytes 2 and 1 are both set (e = 0..7, actor e + 3),
 * as func_80079AB0 does for one: D_800C3EAC +0x2D3 = actor (restored at
 * the end), the script pointer comes from the enemy's 64-byte D_800D340C
 * record, and unless its D_800CCD64 halfword (0x170-byte records) has
 * bit 0x8000 without bit 0x800 of its D_800CCD1C halfword: all 32
 * command rows get D_800C402F = 0xFF, the 0x100-byte D_800D2E5C buffer
 * is cleared, opcodes run until 0xFD or 0xFF (>= 0x80 to func_8007F8C0,
 * then func_80079948 when it returns 0; others to func_8007EF6C, whose
 * state carries over to the next enemy), then +0x2DB = 0,
 * func_80079778(actor) and func_80070EB0(0).
 * Codegen: the two flag halfwords are read through one byte offset with
 * u16 casts.  That keeps the D_800CCD64 load a plain (non-struct) MEM, so
 * gcc 2.x's struct/scalar alias rule does not let it pass the store of
 * the address-taken script pointer (retail loads and stores pc first,
 * with a nop); D_800CCD64__h[e + 3][0] passes it and drops that nop.
 * The flag bytes go through the D_800C3D18 asm-label row view (loop.c
 * then reduces e * 4 into retail's own register).  Matches under gcc
 * 2.7.2 (main13's preset) and 2.6.x. */
void func_80079C24(void) {
    u8 saved;
    u8 state;
    s32 e;
    s32 k;
    s32 off;
    u8* pc;
    u8* p;
    u8* end;

    state = 0;
    saved = D_800C3EAC[0x2D3];
    for (e = 0; e < 8; e++) {
        if (D_800C3D18__rows[e][2] != 0 && D_800C3D18__rows[e][1] != 0) {
            D_800C3EAC[0x2D3] = e + 3;
            pc = *(u8**)(D_800D340C + (e << 6));
            p = D_800D2E5C;
            off = (e + 3) * 0x170;
            if (!(*(u16*)((u8*)D_800CCD64__h + off) & 0x8000)
                || (*(u16*)((u8*)D_800CCD1C__h + off) & 0x800)) {
                for (k = 0x8B8; k >= 0; k -= 0x48) {
                    D_800C402F[k] = 0xFF;
                }
                end = p + 0x100;
                *p = 0;
                while (++p < end) {
                    *p = 0;
                }
                while (*pc != 0xFD && *pc != 0xFF) {
                    if (*pc >= 0x80) {
                        if (func_8007F8C0(&pc, e) == 0) {
                            func_80079948(&pc);
                        }
                    } else {
                        state = func_8007EF6C(&pc, e, state);
                    }
                }
                D_800C3EAC[0x2DB] = 0;
                func_80079778(e + 3);
                func_80070EB0(0);
            }
        }
    }
    D_800C3EAC[0x2D3] = saved;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3725[];
#endif
extern u32 func_80089C9C(u32 a, u32 b);


/* func_80079E18.s */
void func_80079E18(u8 index) {
    D_800D2D28[0xB4] = 1;
    *(u8*)(D_800D3725 + (((u32)index * 3) << 5)) = 1;
}
/* func_80079E4C.s */
void func_80079E4C(u8 index) {
    D_800D2D28[0xB4] = 0;
    *(u8*)(D_800D3725 + (((u32)index * 3) << 5)) = 0;
}
/* func_80079E7C.s */
u32 func_80079E7C(u16 v) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        if ((func_80089C9C(v & 0xFFFF, i & 0xFF) & 0xFFFF) != 0) {
            break;
        }
    }
    return i & 0xFF;
}
