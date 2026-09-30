#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main60", func_8009CBC4);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
extern u32 func_8009DBFC(u32 a0);


/* func_8009D354.s */
void func_8009D354(void) {
    if ((func_8009DBFC(0) << 24) == 0) {
        u8* p = D_800C34B0;

        *(u8*)(p + D_800C3E50 + 0x5FA0) = 6;
    }
}


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above; these are compiled only
 * for the port and the differential harness. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E34;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D6C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E04;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DC4;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C88[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C54[];
#endif
#ifdef XENO_PC_PORT
extern int rand(void);
extern u32 func_8009D3A0(void);
extern u16 func_8009D948(void);
extern u16 func_8009DA04(void);
extern u32 func_8009DB54(u32 a0);
extern void func_80096494(u16* atk, u16* def, u8* hit);

/* func_8009CBC4.s: the physical-damage handler (entry 0 of the D_800C34DC
 * action table).  Rolls hit/miss (func_8009D3A0), attack (func_8009D948)
 * and defence (func_8009DA04), lets func_80096494 adjust the three, applies
 * the attacker/target buff and debuff bits, then computes the damage into
 * the slot's +0x5F6C word (0..9999) and its +0x5FA0 outcome byte. */
void func_8009CBC4(void) {
    u32 s1 = D_800C3DFC[0x11];
    s32 s0;
    u16 atk;
    u16 def;
    u8 hit;
    u32 m1;
    u32 m2;

    hit = (u8)func_8009D3A0();
    atk = func_8009D948();
    def = func_8009DA04();
    func_80096494(&atk, &def, &hit);
    {
        u8* a = D_800D2D6C;

        if ((*(u16*)(a + 0x80) | *(u16*)(a + 0x82)) & 0x8) {
            atk = atk + atk / 5;
        }
        if ((*(u16*)(a + 0x80) | *(u16*)(a + 0x82)) & 0x2) {
            atk = atk + atk / 10;
        }
        if ((*(u16*)(a + 0x80) | *(u16*)(a + 0x82)) & 0x4) {
            atk = atk - atk / 5;
        }
        if ((*(u16*)(a + 0x80) | *(u16*)(a + 0x82)) & 0x1) {
            atk = atk - atk / 10;
        }
    }
    {
        u8* d = D_800D2DC8;

        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x4) {
            def = def + def / 5;
        }
        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x1) {
            def = def + def / 10;
        }
        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x8) {
            def = def - def / 5;
        }
        if ((*(u16*)(d + 0x80) | *(u16*)(d + 0x82)) & 0x2) {
            def = def - def / 10;
        }
    }
    if (D_800C3DFC[0x22] & 0x10) {
        u8* t = D_800C3E34;

        if (!(*(u16*)(t + 0x82) & 0x40)) {
            *(u16*)(t + 0x80) |= 0x40;
        }
        t = D_800D2D6C;
        if ((*(u16*)(t + 0x84) | *(u16*)(t + 0x86)) & 0x4000) {
            D_800D2C88[D_800C3E04] = 3;
            *(u32*)(D_800D2C54 + D_800C3E04 * 4) =
                ((*(u16*)(D_800C3E00 + 0x52) / 10) & 0xFFFF) * 2;
        }
        t = D_800D2D6C;
        if ((*(u16*)(t + 0x84) | *(u16*)(t + 0x86)) & 0x1000) {
            D_800D2C88[D_800C3E04] = 2;
            *(u32*)(D_800D2C54 + D_800C3E04 * 4) =
                (*(u32*)(D_800D2D6C + 0x64) / 10) * 2;
        }
    }
    if (D_800C3DFC[0x22] & 0x20) {
        u8* t = D_800C3E34;

        if (!(*(u16*)(t + 0x82) & 0x80)) {
            *(u16*)(t + 0x80) |= 0x80;
        }
    }
    {
        u8* t = D_800C3E34;

        if (*(u16*)(t + 0x80) & 0x40) {
            def = def - (def >> 2);
            *(u16*)(t + 0x80) &= 0xFFBF;
        }
    }
    {
        u8* t = D_800C3E00;

        if (*(u16*)(t + 0x80) & 0x80) {
            atk = atk - (atk >> 2);
            *(u16*)(t + 0x80) &= 0xFF7F;
        }
    }
    {
        u32 f = *(u16*)(D_800C3DFC + 0xA);

        if (f & 0x400) {
            s1 = 0x14;
        }
        if (f & 0x100) {
            m1 = 5;
            m2 = 4;
        } else {
            m1 = 4;
            m2 = 3;
        }
    }
    if (def != 0) {
        s0 = (s32)(m1 * atk - m2 * def);
    } else {
        s0 = (s32)(m1 * atk);
    }
    /* lbu, so the retail bltz on this byte is never taken */
    if (D_800C3DFC[0x1A] < 2) {
        s0 = (s32)(s1 * (u32)s0) / 20;
    }
    if (s0 <= 0) {
        s0 = 0;
    } else if (s0 < 0xF) {
        s0 = s0 + (s32)rand() % 3;
    } else {
        /* divisor s0 / 15 + 2 >= 3, so never zero */
        s0 = s0 + (s32)rand() % (s0 / 15 + 2);
    }
    if (*(u16*)(D_800C3DFC + 0x8) != 0) {
        s0 = (s32)func_8009DB54((u32)s0);
    }
    switch ((s8)hit) {
    case 1:
        (D_800C34B0 + D_800C3E50)[0x5FA0] = 0;
        break;
    case 2: {
        u32 v;

        (D_800C34B0 + D_800C3E50)[0x5FA0] = 5;
        v = D_800D2DC8[0x9C];
        if (v >= 0xA) {
            v = 9;
        }
        if (s0 != 0) {
            s0 = (s32)((u32)s0 * (0xA - v)) / 20;
        }
        break;
    }
    case 3:
        s0 = 0;
        (D_800C34B0 + D_800C3E50)[0x5FA0] = 4;
        break;
    case 4:
        (D_800C34B0 + D_800C3E50)[0x5FA0] = 2;
        break;
    }
    if (D_800D2DC4 != 0 && (*(u16*)(D_800C3DFC + 0xA) & 0x100) && s0 != 0) {
        s0 = s0 / 3;
    }
    if (s0 >= 0x2710) {
        s0 = 0x270F;
    }
    if (s0 < 0) {
        s0 = 0;
    }
    *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = (u32)s0;
}
#endif /* XENO_PC_PORT */
