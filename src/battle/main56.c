#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
extern u8* D_800C3DFC;

/* func_8009AB38.s, byte-exact: when actor idx's ready flag is set (+0x124
 * bit 13 for a +0x15A bit 7 actor, +0x88 bit 10 otherwise), mark its
 * command slots with 1: the 0x690-stride table at +0x2228 for a +0x15A
 * bit 7 actor, else the 0x5F0-stride table at +0x1058 (the port arm's
 * bat3_actor_flag and bat3_stamp_slots, inline).  Codegen: each record
 * pointer adds the offset to D_800C34B0 loaded into a local first (`base`,
 * and a block-local `base2` for the second +0x15A test through p2); gcc
 * 2.6.x then adds the base first, as retail does, where
 * `D_800C34B0 + idx * 0x170` comes out offset first.  Each table pointer
 * is built in place, offset first (`q = (u8*)(idx * 0x690 + 0x2228);
 * q += (u32)D_800C34B0;`); the two tables use separate pointers (one
 * shared pointer swaps $v0/$v1 in the else block). */
void func_8009AB38(u8 idx) {
    u8* base = D_800C34B0;
    u8* p = base + idx * 0x170;
    u32 flag;
    u8* q;
    u8* r;
    u8* p2;

    if (p[0x15A] & 0x80) {
        flag = *(u16*)(p + 0x124) & 0x2000;
    } else {
        flag = *(u16*)(p + 0x88) & 0x400;
    }
    if (flag != 0) {
        u8* base2 = D_800C34B0;

        p2 = base2 + idx * 0x170;
        if (p2[0x15A] & 0x80) {
            q = (u8*)(idx * 0x690 + 0x2228);
            q += (u32)D_800C34B0;
            *(u16*)(q + 0x348) = 1;
            *(u16*)(q + 0x398) = 1;
            *(u16*)(q + 0x3C0) = 1;
            *(u16*)(q + 0x3E8) = 1;
            *(u16*)(q + 0x410) = 1;
            *(u16*)(q + 0x438) = 1;
            *(u16*)(q + 0x460) = 1;
        } else {
            r = (u8*)(idx * 0x5F0 + 0x1058);
            r += (u32)D_800C34B0;
            *(u16*)(r + 0x370) = 1;
            *(u16*)(r + 0x3C0) = 1;
            *(u16*)(r + 0x3E8) = 1;
            *(u16*)(r + 0x410) = 1;
            *(u16*)(r + 0x438) = 1;
            *(u16*)(r + 0x460) = 1;
            *(u16*)(r + 0x488) = 1;
            *(u16*)(r + 0x4B0) = 1;
            *(u16*)(r + 0x4D8) = 1;
            *(u16*)(r + 0x500) = 1;
        }
    }
}
/* func_8009AC48.s, byte-exact: unless `check` is set and D_800C3DFC+0xA
 * bit 8 is clear, consume actor idx's ready flag: stamp its command slots
 * with 0x2000 as func_8009AB38 does, then clear the flag bit through p2.
 * Same form as func_8009AB38. */
void func_8009AC48(u8 idx, u8 check) {
    u8* base;
    u8* p;
    u32 flag;
    u8* q;
    u8* r;
    u8* p2;

    if (check != 0 && !(*(u16*)(D_800C3DFC + 0xA) & 0x100)) {
        return;
    }
    base = D_800C34B0;
    p = base + idx * 0x170;
    if (p[0x15A] & 0x80) {
        flag = *(u16*)(p + 0x124) & 0x2000;
    } else {
        flag = *(u16*)(p + 0x88) & 0x400;
    }
    if (flag != 0) {
        u8* base2 = D_800C34B0;

        p2 = base2 + idx * 0x170;
        if (p2[0x15A] & 0x80) {
            q = (u8*)(idx * 0x690 + 0x2228);
            q += (u32)D_800C34B0;
            *(u16*)(q + 0x348) = 0x2000;
            *(u16*)(q + 0x398) = 0x2000;
            *(u16*)(q + 0x3C0) = 0x2000;
            *(u16*)(q + 0x3E8) = 0x2000;
            *(u16*)(q + 0x410) = 0x2000;
            *(u16*)(q + 0x438) = 0x2000;
            *(u16*)(q + 0x460) = 0x2000;
            *(u16*)(p2 + 0x124) &= 0xDFFF;
        } else {
            r = (u8*)(idx * 0x5F0 + 0x1058);
            r += (u32)D_800C34B0;
            *(u16*)(r + 0x370) = 0x2000;
            *(u16*)(r + 0x3C0) = 0x2000;
            *(u16*)(r + 0x3E8) = 0x2000;
            *(u16*)(r + 0x410) = 0x2000;
            *(u16*)(r + 0x438) = 0x2000;
            *(u16*)(r + 0x460) = 0x2000;
            *(u16*)(r + 0x488) = 0x2000;
            *(u16*)(r + 0x4B0) = 0x2000;
            *(u16*)(r + 0x4D8) = 0x2000;
            *(u16*)(r + 0x500) = 0x2000;
            *(u16*)(p2 + 0x88) &= 0xFBFF;
        }
    }
}
/* func_8009ADA0.s, byte-exact: return 0 for a +0x7C bit 15 actor;
 * otherwise fill out[0..2]: out[0] = +0x4E / 20 when +0x7C bit 11,
 * out[1] = +0x52 / 20 when +0x80 bit 9 or +0x120 bit 9, out[2] = 0, then
 * +0xDE / 50 when +0x120 bit 7, plus another +0xDE / 50 when +0x124 bit 15;
 * return 1 if any of these applied.  `out` walks (retail's addiu), and the
 * record pointer adds the offset first.  gcc 2.6.x code: 2.7.2 puts each
 * magic-multiply quotient through a scratch register (mfhi $a3). */
u32 func_8009ADA0(u8 idx, u32* out) {
    u8* p = (u8*)(idx * 0x170 + (u32)D_800C34B0);
    u16 f = *(u16*)(p + 0x7C);
    u32 r = 0;

    if (f & 0x8000) {
        return 0;
    }
    if (f & 0x800) {
        r = 1;
        *out = (u16)(*(u16*)(p + 0x4E) / 20);
    }
    out++;
    if (*(u16*)(p + 0x80) & 0x200) {
        r = 1;
        *out = (u16)(*(u16*)(p + 0x52) / 20);
    }
    if (*(u16*)(p + 0x120) & 0x200) {
        r = 1;
        *out = (u16)(*(u16*)(p + 0x52) / 20);
    }
    out++;
    *out = 0;
    if (*(u16*)(p + 0x120) & 0x80) {
        r = 1;
        *out = (u16)(*(u16*)(p + 0xDE) / 50);
    }
    if (*(u16*)(p + 0x124) & 0x8000) {
        r = 1;
        *out = (u16)(*(u16*)(p + 0xDE) / 50) + *out;
    }
    return r;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD68[];
#endif
extern void func_8009B104(u32 a0, u8* a1);


/* func_8009AEFC.s */
void func_8009AEFC(u8 index) {
    u32 off;
    u8* p;
    u32 v;
    u32 t;

    D_800C34B0[0x5FC2] = 0;
    off = ((u32)index & 0xFF) * 368;
    p = (u8*)(off + (u32)(unsigned int)D_800C34B0);
    v = *(u16*)(p + 0x80);
    t = p[0x56];
    *(u16*)(p + 0x7C) = 0;
    *(u16*)(p + 0x84) = 0;
    *(u16*)(p + 0x88) = 0;
    *(u16*)(p + 0x8C) = 0;
    *(u16*)(p + 0x80) = (u16)(v & 0x2000);
    if (t == 7) {
        func_8009B104((u32)index & 0xFF, p);
    }
    if (p[0x56] == 3) {
        u32 i;

        for (i = 3; (u32)(i & 0xFF) < 0xB; i++) {
            u32 o2 = (i & 0xFF) * 368;

            *(u16*)((u8*)D_800CCD68 + o2) &= 0xFFDF;
        }
    }
}


/* ---- Port bodies (fo/bat3).  The retail build compiles the byte-exact C
 * above for func_8009AB38, func_8009AC48 and func_8009ADA0.  The bodies
 * below are kept for the port and the differential harnesses. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifdef XENO_PC_PORT
/* Shared by func_8009AB38 / func_8009AC48: actor idx's "ready" flag, which
 * lives in +0x124 bit13 for a +0x15A bit7 actor and +0x88 bit10 otherwise. */
static u32 bat3_actor_flag(u8* p) {
    if (p[0x15A] & 0x80) {
        return *(u16*)(p + 0x124) & 0x2000;
    }
    return *(u16*)(p + 0x88) & 0x400;
}

/* Stamp value into the command-slot halfwords of actor idx: the 0x690-stride
 * table at +0x2228 for a +0x15A bit7 actor, else the 0x5F0-stride table at
 * +0x1058.  Offsets are the retail store list. */
static void bat3_stamp_slots(u8 idx, u8* base, u16 value) {
    if (base[idx * 0x170 + 0x15A] & 0x80) {
        u8* q = base + idx * 0x690 + 0x2228;

        *(u16*)(q + 0x348) = value;
        *(u16*)(q + 0x398) = value;
        *(u16*)(q + 0x3C0) = value;
        *(u16*)(q + 0x3E8) = value;
        *(u16*)(q + 0x410) = value;
        *(u16*)(q + 0x438) = value;
        *(u16*)(q + 0x460) = value;
    } else {
        u8* q = base + idx * 0x5F0 + 0x1058;

        *(u16*)(q + 0x370) = value;
        *(u16*)(q + 0x3C0) = value;
        *(u16*)(q + 0x3E8) = value;
        *(u16*)(q + 0x410) = value;
        *(u16*)(q + 0x438) = value;
        *(u16*)(q + 0x460) = value;
        *(u16*)(q + 0x488) = value;
        *(u16*)(q + 0x4B0) = value;
        *(u16*)(q + 0x4D8) = value;
        *(u16*)(q + 0x500) = value;
    }
}

/* func_8009AB38.s: when actor idx's ready flag is set, mark its command
 * slots with 1. */
void func_8009AB38(u8 idx) {
    if (bat3_actor_flag(D_800C34B0 + idx * 0x170) != 0) {
        bat3_stamp_slots(idx, D_800C34B0, 1);
    }
}

/* func_8009AC48.s: unless `check` is set and D_800C3DFC+0xA bit8 is clear,
 * consume actor idx's ready flag: mark its command slots with 0x2000 and
 * clear the flag bit. */
void func_8009AC48(u8 idx, u8 check) {
    u8* p;

    if (check != 0 && !(*(u16*)(D_800C3DFC + 0xA) & 0x100)) {
        return;
    }
    if (bat3_actor_flag(D_800C34B0 + idx * 0x170) == 0) {
        return;
    }
    p = D_800C34B0 + idx * 0x170;
    bat3_stamp_slots(idx, D_800C34B0, 0x2000);
    if (p[0x15A] & 0x80) {
        *(u16*)(p + 0x124) &= 0xDFFF;
    } else {
        *(u16*)(p + 0x88) &= 0xFBFF;
    }
}

/* func_8009ADA0.s: fill out[0..2] with actor idx's damage-over-time style
 * amounts (HP/20, MP/20, and /50 terms) and return 1 if any applied;
 * return 0 immediately for a +0x7C bit15 actor. */
u32 func_8009ADA0(u8 idx, u32* out) {
    u8* p = D_800C34B0 + idx * 0x170;
    u32 f = *(u16*)(p + 0x7C);
    u32 r = 0;

    if (f & 0x8000) {
        return 0;
    }
    if (f & 0x800) {
        r = 1;
        out[0] = (*(u16*)(p + 0x4E) / 20) & 0xFFFF;
    }
    if (*(u16*)(p + 0x80) & 0x200) {
        r = 1;
        out[1] = (*(u16*)(p + 0x52) / 20) & 0xFFFF;
    }
    if (*(u16*)(p + 0x120) & 0x200) {
        r = 1;
        out[1] = (*(u16*)(p + 0x52) / 20) & 0xFFFF;
    }
    out[2] = 0;
    if (*(u16*)(p + 0x120) & 0x80) {
        r = 1;
        out[2] = (*(u16*)(p + 0xDE) / 50) & 0xFFFF;
    }
    if (*(u16*)(p + 0x124) & 0x8000) {
        r = 1;
        out[2] = ((*(u16*)(p + 0xDE) / 50) & 0xFFFF) + out[2];
    }
    return r;
}
#endif /* XENO_PC_PORT */
